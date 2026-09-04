#include "md3_slider.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QFocusEvent>
#include <QFont>

namespace {
// MD3 2024 版滑块外观常量：轨道 16px 高胶囊，thumb 为 44px 高圆角竖条
constexpr qreal kTrackHeight = 16.0;       // 轨道高
constexpr qreal kTrackRadius = 8.0;        // 轨道外圆角 = 半高
constexpr qreal kInnerRadius = 2.0;        // gap 分段处内圆角
constexpr qreal kThumbWidth = 4.0;         // 平时 thumb 宽度
constexpr qreal kThumbWidthActive = 2.0;   // press/focus 时 thumb 宽度
constexpr qreal kThumbHeight = 44.0;       // thumb 高度
constexpr qreal kTrackGap = 6.0;           // active 段与 thumb 间的 gap
constexpr qreal kStopIndicatorRadius = 2.0;// 右端 stop 指示圆点半径
constexpr int kWidgetHeight = 48;
// 轨道两端内缩量，thumb 竖条在端点时完整可见
constexpr qreal kTrackPad = 10.0;
}

Md3Slider::Md3Slider(QWidget *parent)
    : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setFixedHeight(kWidgetHeight);

    // thumb 平滑移动动画：程序设值 / 点击轨道时从当前位置滑动到目标
    anim_.setDuration(200);
    anim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        displayValue_ = v.toReal();
        update();
    });
}

void Md3Slider::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

// 设置取值范围并校正当前值
void Md3Slider::setRange(int min, int max)
{
    if (min > max) {
        qSwap(min, max);
    }
    min_ = min;
    max_ = max;
    setValue(value_);
}

// 设置当前值，越界时钳制并发出信号。
// 若正处于拖动中则由 setValueFromPos 直接同步 displayValue_，这里不做动画。
void Md3Slider::setValue(int value)
{
    value = qBound(min_, value, max_);
    if (value == value_) {
        return;
    }
    const qreal from = anim_.state() == QAbstractAnimation::Running ? displayValue_ : qreal(value_);
    value_ = value;
    anim_.stop();
    // 复位时长：拖动分支可能把 anim_ 临时改短到 100ms，后续程序设值必须回到标准时长
    anim_.setDuration(200);
    anim_.setStartValue(from);
    anim_.setEndValue(qreal(value_));
    anim_.start();
    emit valueChanged(value_);
}

QSize Md3Slider::sizeHint() const
{
    return QSize(200, kWidgetHeight);
}

// 根据鼠标 x 坐标计算新值，滑块中心与值一一对应。
// 拖动中（pressed_）实时同步 displayValue_，保证跟随鼠标无延迟。
void Md3Slider::setValueFromPos(int x)
{
    const qreal trackLeft = kTrackPad;
    const qreal trackRight = width() - kTrackPad;
    const qreal ratio = (x - trackLeft) / (trackRight - trackLeft);
    const qreal clamped = qBound(0.0, ratio, 1.0);
    const int target = qRound(min_ + clamped * (max_ - min_));
    if (pressed_) {
        if (target == value_) {
            return;
        }
        value_ = target;
        // 拖动中也做平滑过渡（短动画），thumb 平滑跟随鼠标而不是生硬跳动
        anim_.stop();
        anim_.setStartValue(displayValue_);
        anim_.setEndValue(qreal(target));
        anim_.setDuration(100);
        anim_.start();
        emit valueChanged(target);
    } else {
        setValue(target);
    }
}

// 绘制轨道、分段填充、stop indicator 与竖条 thumb
void Md3Slider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const qreal cy = height() / 2.0;
    const qreal trackLeft = kTrackPad;
    const qreal trackRight = width() - kTrackPad;
    const qreal trackWidth = trackRight - trackLeft;
    // 用插值显示值计算 thumb 位置，动画期间平滑移动
    const qreal ratio = (max_ > min_) ? (displayValue_ - min_) / qreal(max_ - min_) : 0.0;
    const qreal thumbX = trackLeft + ratio * trackWidth;
    // 仅拖动（按下）时 thumb 变细，松开立即恢复粗
    const bool thumbThin = pressed_;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 1) 整条 inactive 轨道：secondary-container 胶囊
    const QRectF trackRect(trackLeft, cy - kTrackHeight / 2.0, trackWidth, kTrackHeight);
    p.setPen(Qt::NoPen);
    p.setBrush(theme_.secondaryContainer);
    p.drawRoundedRect(trackRect, kTrackRadius, kTrackRadius);

    // 2) active 段：primary，从轨道左端到 thumb 中心左侧留 gap，
    //    左端外圆角 8、右端内圆角 2（arcTo 角度约定：0°=右 90°=上 180°=左 270°=下）
    const qreal activeEnd = thumbX - kTrackGap;
    if (activeEnd > trackLeft) {
        const qreal top = cy - kTrackHeight / 2.0;
        const QRectF ar(trackLeft, top, activeEnd - trackLeft, kTrackHeight);
        QPainterPath activePath;
        activePath.moveTo(ar.left(), top + kTrackRadius);
        activePath.arcTo(QRectF(ar.left(), top, 2 * kTrackRadius, 2 * kTrackRadius), 180, -90);
        activePath.lineTo(ar.right() - kInnerRadius, top);
        activePath.arcTo(QRectF(ar.right() - 2 * kInnerRadius, top,
                                2 * kInnerRadius, 2 * kInnerRadius), 90, -90);
        activePath.lineTo(ar.right(), top + kTrackHeight);
        activePath.arcTo(QRectF(ar.right() - 2 * kInnerRadius, top + kTrackHeight - 2 * kInnerRadius,
                                2 * kInnerRadius, 2 * kInnerRadius), 0, -90);
        activePath.lineTo(ar.left() + kTrackRadius, top + kTrackHeight);
        activePath.arcTo(QRectF(ar.left(), top + kTrackHeight - 2 * kTrackRadius,
                                2 * kTrackRadius, 2 * kTrackRadius), 270, -90);
        activePath.closeSubpath();
        p.setBrush(theme_.primary);
        p.drawPath(activePath);
    }

    // 3) stop indicator：轨道右端内侧 8px 的 primary 圆点，thumb 未到最右端时显示
    const qreal stopX = trackRight - kTrackHeight / 2.0;
    if (thumbX < stopX) {
        p.setBrush(theme_.primary);
        p.drawEllipse(QPointF(stopX, cy), kStopIndicatorRadius, kStopIndicatorRadius);
    }

    // 4) thumb：primary 圆角竖条，press/focus 时变细
    const qreal thumbW = thumbThin ? kThumbWidthActive : kThumbWidth;
    const QRectF thumbRect(thumbX - thumbW / 2.0, cy - kThumbHeight / 2.0, thumbW, kThumbHeight);
    p.setBrush(theme_.primary);
    p.drawRoundedRect(thumbRect, thumbW / 2.0, thumbW / 2.0);
}

// 按下时先平滑滑动到点击位置，再进入拖动状态
void Md3Slider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !isEnabled()) {
        QWidget::mousePressEvent(event);
        return;
    }
    setValueFromPos(event->position().x());
    pressed_ = true;
}

// 拖动过程中持续取值
void Md3Slider::mouseMoveEvent(QMouseEvent *event)
{
    if (pressed_) {
        setValueFromPos(event->position().x());
    }
}

void Md3Slider::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        pressed_ = false;
        update();
    }
}
