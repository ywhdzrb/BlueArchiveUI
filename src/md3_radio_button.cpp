#include "md3_radio_button.h"

#include <QPainter>
#include <QEnterEvent>
#include <QMouseEvent>

namespace {
constexpr qreal kTextGap = 12.0;    // 圆环与文本间距
constexpr qreal kStroke = 2.0;      // 描边宽
constexpr qreal kDotRadius = 4.5;   // 中心圆点半径
}

Md3RadioButton::Md3RadioButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setAutoExclusive(true);   // 同一父级自动互斥，便于整组单选
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

void Md3RadioButton::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

void Md3RadioButton::setText(const QString &text)
{
    text_ = text;
    updateGeometry();
    update();
}

void Md3RadioButton::setLabelColor(const QColor &color)
{
    labelColor_ = color;
    update();
}

QSize Md3RadioButton::sizeHint() const
{
    const qreal textWidth = text_.isEmpty() ? 0.0 : fontMetrics().horizontalAdvance(text_) + kTextGap;
    return QSize(static_cast<int>(kSize + textWidth), 32);
}

QSize Md3RadioButton::minimumSizeHint() const
{
    return sizeHint();
}

// 绘制：外环 → 状态层 → 中心圆点 → 右侧标签
void Md3RadioButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const bool disabled = !isEnabled();
    const bool checked = isChecked();

    const QRectF ring(0, (height() - kSize) / 2.0, kSize, kSize);
    const QPointF center = ring.center();

    // 一：外环。选中态用 primary（或 error 非默认），未选中 2px outline
    const QColor ringColor = checked ? theme_.primary : theme_.outline;
    QPen pen(disabled ? theme_.outline : ringColor, kStroke);
    p.setBrush(Qt::NoBrush);
    p.setPen(pen);
    p.drawEllipse(ring);

    // 二：状态层（hover / 按下）盖在环氧带上
    if (!disabled && stateAlpha_ > 0.0) {
        const QColor layer = checked ? Md3Theme::blend(theme_.surface, theme_.primary, stateAlpha_ / 100.0)
                                     : Md3Theme::blend(theme_.surface, theme_.onSurface, stateAlpha_ / 100.0);
        p.setPen(Qt::NoPen);
        p.setBrush(layer);
        p.drawEllipse(ring);
        // 重画环氧带，让状态层只盖内部才更接近 MD3 视觉
        p.setBrush(Qt::NoBrush);
        p.setPen(pen);
        p.drawEllipse(ring);
    }

    // 三：选中时中心实心圆点
    if (checked) {
        const QColor dot = disabled ? theme_.disabledContent() : theme_.primary;
        p.setPen(Qt::NoPen);
        p.setBrush(dot);
        p.drawEllipse(center, kDotRadius, kDotRadius);
    }

    // 四：右侧标签文字
    if (!text_.isEmpty()) {
        QColor c = labelColor_.isValid() ? labelColor_ : theme_.onSurface;
        if (disabled) {
            c = theme_.disabledContent();
        }
        p.setPen(c);
        p.drawText(QRectF(kSize + kTextGap, 0, rect().right() - kSize - kTextGap, height()),
                   Qt::AlignLeft | Qt::AlignVCenter, text_);
    }
}

void Md3RadioButton::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    hovered_ = true;
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}

void Md3RadioButton::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hovered_ = false;
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}

// 按下时状态层抬升
void Md3RadioButton::mousePressEvent(QMouseEvent *event)
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
void Md3RadioButton::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}
