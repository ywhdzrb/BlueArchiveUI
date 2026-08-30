#include "md3_text_field.h"
#include "md3_theme.h"

#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

namespace {
constexpr int kHeight = 56;
constexpr qreal kRadiusTop = 4.0;      // 顶部圆角（底部直角留给指示线）
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

QSize Md3TextField::sizeHint() const
{
    return QSize(240, kHeight);
}

// 尺寸变化时同步内部编辑框几何（左右 12px 内边距，底部留 8px 给指示线）
void Md3TextField::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    editor_->setGeometry(12, 4, width() - 24, height() - 8);
}

// 绘制背景与底部指示线，指示线颜色 / 宽度随聚焦进度过渡
void Md3TextField::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 填充背景，仅顶部圆角。
    // Qt 的 arcTo 角度约定：0°=右, 90°=上, 180°=左, 270°=下；sweep 正值 = 角度值增大。
    // 左上角从 180°(左) 扫到 90°(上)，右上角从 90°(上) 扫到 0°(右)，均为角度减小，故 sweep 用 -90。
    QPainterPath path;
    path.moveTo(0, kRadiusTop);
    path.arcTo(QRectF(0, 0, 2 * kRadiusTop, 2 * kRadiusTop), 180, -90);
    path.lineTo(width() - 2 * kRadiusTop, 0);
    path.arcTo(QRectF(width() - 2 * kRadiusTop, 0, 2 * kRadiusTop, 2 * kRadiusTop), 90, -90);
    path.lineTo(width(), height());
    path.lineTo(0, height());
    path.closeSubpath();
    p.fillPath(path, theme_.surfaceContainerHighest);

    // 底部指示线：未聚焦 1px outline-variant，聚焦 2px primary，颜色渐变过渡
    const QColor lineColor = Md3Theme::blend(theme_.outlineVariant, theme_.primary, focusProgress_);
    const qreal lineWidth = 1.0 + focusProgress_;
    p.setPen(QPen(lineColor, lineWidth));
    const qreal y = height() - 0.5;
    p.drawLine(QPointF(0, y), QPointF(width(), y));
}

// 点击输入框空白区域时聚焦内部编辑框
void Md3TextField::mousePressEvent(QMouseEvent *event)
{
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
