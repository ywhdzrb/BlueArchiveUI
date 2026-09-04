#include "md3_check_box.h"
#include "md3_icon.h"

#include <QPainter>
#include <QEnterEvent>
#include <QMouseEvent>

namespace {
// 复选与文本标签订档
constexpr qreal kTextGap = 12.0;    // 勾选框与文本间距
constexpr qreal kBoxRadius = 4.0;   // 勾选框圆角（MD3 常规是 2px 圆角，视觉 4px 更接近稿）
constexpr qreal kStroke = 2.0;      // 描边宽
}

Md3CheckBox::Md3CheckBox(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    stateAnim_.setDuration(150);
    stateAnim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&stateAnim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        stateAlpha_ = v.toReal();
        update();
    });
}

void Md3CheckBox::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

void Md3CheckBox::setPartially(bool partially)
{
    partially_ = partially;
    update();
}

void Md3CheckBox::setText(const QString &text)
{
    text_ = text;
    updateGeometry();
    update();
}

QSize Md3CheckBox::sizeHint() const
{
    const qreal textWidth = text_.isEmpty() ? 0.0 : fontMetrics().horizontalAdvance(text_) + kTextGap;
    return QSize(static_cast<int>(kSize + textWidth), 32);
}

QSize Md3CheckBox::minimumSizeHint() const
{
    return sizeHint();
}

// 绘制：勾选框（选中 / 部分态着色）→ 状态层 → 对勾或圆点 → 右侧标签
void Md3CheckBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const bool disabled = !isEnabled();
    const bool checked = isChecked();

    const QRectF box(0, (height() - kSize) / 2.0, kSize, kSize);

    // 一：选中 / 部分态填充底色（禁用用 disabledContainer）
    if (checked || partially_) {
        const QColor bg = disabled ? theme_.disabledContainer() : theme_.primary;
        p.setBrush(bg);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(box, kBoxRadius, kBoxRadius);
    } else {
        // 未选中：透明底 + 2px outline 描边
        p.setBrush(Qt::NoBrush);
        QPen pen(disabled ? theme_.disabledContent() : theme_.outline, kStroke);
        p.setPen(pen);
        p.drawRoundedRect(box, kBoxRadius, kBoxRadius);
    }

    // 二：状态层（hover / 按下）叠加在勾选框之上
    if (!disabled && stateAlpha_ > 0.0) {
        QColor layer = checked || partially_ ? Md3Theme::blend(theme_.primary, theme_.onPrimary, stateAlpha_ / 100.0)
                                             : Md3Theme::blend(theme_.surface, theme_.onSurface, stateAlpha_ / 100.0);
        p.setBrush(layer);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(box, kBoxRadius, kBoxRadius);
    }

    // 三：对勾（选中）或圆点（部分态）
    const QColor glyphColor = disabled ? theme_.disabledContent() : theme_.onPrimary;
    if (checked) {
        md3::paintGlyph(p, md3::Glyph::Check, box.center().x(), box.center().y(), glyphColor, 2.2);
    } else if (partially_) {
        p.setPen(Qt::NoPen);
        p.setBrush(glyphColor);
        p.drawEllipse(box.center(), 2.6, 2.6);
    }

    // 四：右侧标签文字（on-surface）
    if (!text_.isEmpty()) {
        p.setPen(disabled ? theme_.disabledContent() : theme_.onSurface);
        p.drawText(QRectF(kSize + kTextGap, 0, rect().right() - kSize - kTextGap, height()),
                   Qt::AlignLeft | Qt::AlignVCenter, text_);
    }
}

void Md3CheckBox::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    hovered_ = true;
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}

void Md3CheckBox::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hovered_ = false;
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}

// 按下时状态层抬升
void Md3CheckBox::mousePressEvent(QMouseEvent *event)
{
    QAbstractButton::mousePressEvent(event);
    if (isDown()) {
        stateAnim_.stop();
        stateAnim_.setStartValue(stateAlpha_);
        stateAnim_.setEndValue(targetStateAlpha());
        stateAnim_.start();
    }
}

// 松开回落到 hover 档
void Md3CheckBox::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}
