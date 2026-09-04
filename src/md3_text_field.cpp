#include "md3_text_field.h"
#include "md3_theme.h"
#include "md3_icon.h"

#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <optional>

namespace {
constexpr int kHeight = 56;
constexpr qreal kRadiusTop = 4.0;      // 顶部圆角（底部直角留给指示线）
constexpr qreal kIconSize = 18.0;      // 前后缀图标尺寸
constexpr int kIconPad = 12;           // 图标到边缘间距
constexpr int kTextLeft = 12;          // 编辑框默认左内边距
constexpr int kTextRight = 12;         // 编辑框默认右内边距
constexpr int kHelperHeight = 16;      // helper 文本区高度
}

Md3TextField::Md3TextField(const QString &placeholder, QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(kHeight);
    setCursor(Qt::IBeamCursor);

    // 内部编辑框：透明背景无边框，MD3 填充底色由父控件绘制
    editor_ = new QLineEdit(this);
    editor_->setPlaceholderText(placeholder);
    editor_->setFrame(false);
    editor_->setAttribute(Qt::WA_MacShowFocusRect, false);
    editor_->setStyleSheet("QLineEdit { background: transparent; border: none; }");

    // MD3 Body Large 排版：16px / Regular 字重
    QFont inputFont = editor_->font();
    inputFont.setPointSizeF(16.0);
    editor_->setFont(inputFont);

    // 聚焦过渡动画，150ms 符合 MD3 短动效时长
    anim_.setDuration(150);
    anim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        focusProgress_ = v.toReal();
        update();
    });

    connect(editor_, &QLineEdit::textChanged, this, [this](const QString &) { update(); });
}

// 切换主题：同步编辑框文字与占位符配色
void Md3TextField::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    QPalette pal = editor_->palette();
    pal.setColor(QPalette::Text, theme_.onSurface);
    pal.setColor(QPalette::PlaceholderText, theme_.onSurfaceVariant);
    editor_->setPalette(pal);
    update();
}

QString Md3TextField::text() const
{
    return editor_->text();
}

void Md3TextField::setText(const QString &text)
{
    editor_->setText(text);
}

void Md3TextField::setPlaceholderText(const QString &placeholder)
{
    editor_->setPlaceholderText(placeholder);
}

void Md3TextField::setLeadingIcon(std::optional<md3::Glyph> glyph)
{
    leadingGlyph_ = glyph;
    leadingVisible_ = glyph.has_value();
    resizeEvent(nullptr);
    update();
}

void Md3TextField::setTrailingIcon(std::optional<md3::Glyph> glyph)
{
    trailingGlyph_ = glyph;
    trailingVisible_ = glyph.has_value();
    resizeEvent(nullptr);
    update();
}

void Md3TextField::setLeadingIconVisible(bool visible)
{
    leadingVisible_ = visible;
    resizeEvent(nullptr);
    update();
}

void Md3TextField::setTrailingIconVisible(bool visible)
{
    trailingVisible_ = visible;
    resizeEvent(nullptr);
    update();
}

void Md3TextField::setError(const QString &errorText)
{
    errorText_ = errorText;
    updateAreaHeight();
    update();
}

void Md3TextField::setHelperText(const QString &helperText)
{
    helperText_ = helperText;
    updateAreaHeight();
    update();
}

// helper / 错误文本激活时控件整体加高（保留主体 56px 不变）
void Md3TextField::updateAreaHeight()
{
    const bool hasArea = !errorText_.isEmpty() || !helperText_.isEmpty();
    setFixedHeight(kHeight + (hasArea ? kHelperHeight : 0));
    resizeEvent(nullptr);
}

void Md3TextField::setTrailingIconClicked(std::function<void()> callback)
{
    trailingCallback_ = std::move(callback);
}

QSize Md3TextField::sizeHint() const
{
    return QSize(240, kHeight);
}

// 顶部填充区矩形：主体恒为 kHeight 高，其余 16px 留给 helper / 错误文本
QRectF Md3TextField::textFieldRect() const
{
    return QRectF(0, 0, width(), kHeight);
}

// 尺寸变化时同步内部编辑框几何：
// 左/右按前后缀图标是否显示增加内边距，底边留给指示线 8px。
// 若要加入 helper 文字，编辑框保持固定高度，helper 独占最底部一段。
void Md3TextField::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    const qreal left = leadingVisible_ ? (kTextLeft + kIconSize + 8) : kTextLeft;
    const qreal right = trailingVisible_ ? (kTextRight + kIconSize + 8) : kTextRight;
    const qreal h = kHeight;
    editor_->setGeometry(static_cast<int>(left), 4,
                         static_cast<int>(width() - left - right),
                         static_cast<int>(h - 8));
}

// 绘制：填充背景 → 指示线（聚焦/错误态渐变）→ 前后缀图标 → helper / 错误文本
void Md3TextField::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF fr = textFieldRect();
    const bool hasError = !errorText_.isEmpty();

    // 填充背景，仅顶部圆角。
    // Qt 的 arcTo 角度约定：0°=右, 90°=上, 180°=左, 270°=下；sweep 正值 = 角度值增大。
    // 左上角从 180°(左) 扫到 90°(上)，右上角从 90°(上) 扫到 0°(右)，均为角度减小，故 sweep 用 -90。
    QPainterPath path;
    path.moveTo(0, kRadiusTop);
    path.arcTo(QRectF(0, 0, 2 * kRadiusTop, 2 * kRadiusTop), 180, -90);
    path.lineTo(fr.width() - 2 * kRadiusTop, 0);
    path.arcTo(QRectF(fr.width() - 2 * kRadiusTop, 0, 2 * kRadiusTop, 2 * kRadiusTop), 90, -90);
    path.lineTo(fr.width(), fr.height());
    path.lineTo(0, fr.height());
    path.closeSubpath();

    // 错误态背景：surfaceContainerHighest 上叠加 low error 色调（MD3 filled error 底色约 8%）
    QColor fillBase = theme_.surfaceContainerHighest;
    if (hasError) {
        fillBase = Md3Theme::blend(theme_.surfaceContainerHighest, theme_.error, 0.08);
    }
    p.fillPath(path, fillBase);

    // 底部指示线：未聚焦 1px outline-variant，聚焦 2px primary，颜色渐变过渡。
    // 错误态指示线恒为 error 色（不跟随聚焦态，保持红显眼语义）
    const QColor lineColor = hasError
        ? theme_.error
        : Md3Theme::blend(theme_.outlineVariant, theme_.primary, focusProgress_);
    const qreal lineWidth = hasError ? 2.0 : (1.0 + focusProgress_);
    p.setPen(QPen(lineColor, lineWidth));
    const qreal y = fr.height() - 0.5;
    p.drawLine(QPointF(0, y), QPointF(fr.width(), y));

    // 前后缀图标：leading 左 12px 处，trailing 右 12px 处（错误态 trailing 转 error 色）
    const qreal cy = fr.height() / 2.0;
    const QColor iconColor = hasError ? theme_.error : theme_.onSurfaceVariant;
    if (leadingVisible_ && leadingGlyph_.has_value()) {
        md3::paintGlyph(p, *leadingGlyph_, kIconPad + kIconSize / 2.0, cy, iconColor, 2.0);
    }
    if (trailingVisible_ && trailingGlyph_.has_value()) {
        md3::paintGlyph(p, *trailingGlyph_, width() - kIconPad - kIconSize / 2.0, cy, iconColor, 2.0);
    }

    // helper / 错误文本：输出在指示线下方。
    // helperColor：错误态用 error 色；正常态用 on-surface-variant
    if (hasError) {
        QFont f = font();
        f.setPointSizeF(12.0);
        p.setPen(theme_.error);
        p.setFont(f);
        p.drawText(QRectF(0, fr.height() + 1, width(), kHelperHeight),
                   Qt::AlignLeft | Qt::AlignVCenter, errorText_);
    } else if (!helperText_.isEmpty()) {
        QFont f = font();
        f.setPointSizeF(12.0);
        p.setPen(theme_.onSurfaceVariant);
        p.setFont(f);
        p.drawText(QRectF(0, fr.height() + 1, width(), kHelperHeight),
                   Qt::AlignLeft | Qt::AlignVCenter, helperText_);
    }
}

// 点击输入框空白区域时聚焦内部编辑框；点击 trailing 图标触发回调
void Md3TextField::mousePressEvent(QMouseEvent *event)
{
    if (trailingVisible_ && trailingGlyph_.has_value()) {
        // 命中区域：以图标中心为圆心的约 24px 邻域（x/y 双向判定，避免全高横带误触）
        const qreal cx = width() - kIconPad - kIconSize / 2.0;
        const qreal cy = kHeight / 2.0;
        if (qAbs(event->position().x() - cx) < 12.0
            && qAbs(event->position().y() - cy) < 12.0) {
            if (trailingCallback_) {
                trailingCallback_();
            }
            emit trailingIconClicked();
            return;
        }
    }
    Q_UNUSED(event)
    editor_->setFocus();
}

void Md3TextField::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    focused_ = true;
    anim_.stop();
    anim_.setStartValue(focusProgress_);
    anim_.setEndValue(1.0);
    anim_.start();
}

void Md3TextField::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    focused_ = false;
    anim_.stop();
    anim_.setStartValue(focusProgress_);
    anim_.setEndValue(0.0);
    anim_.start();
}
