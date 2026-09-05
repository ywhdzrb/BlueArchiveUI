#include "ba_slider.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <cmath>

#include "ba_icon.h"
#include "ba_style.h"

BaSlider::BaSlider(QWidget *parent)
    : QAbstractSlider(parent)
    , thumbAnim_(120, ba::Animator::outCubic,
                 // 每帧更新 thumb 动画位置并重绘
                 [this](qreal t) {
                     thumbAni_ = t;
                     update();
                 })
{
    setOrientation(Qt::Horizontal);
    setRange(0, 100);
    setValue(0);
    setSingleStep(1);
    setPageStep(10);
    setFixedHeight(28);
    setCursor(Qt::PointingHandCursor);
}

QSize BaSlider::sizeHint() const
{
    return QSize(320, 28);
}

void BaSlider::setVolumeIcons(bool on)
{
    volumeIcons_ = on;
    update();
}

void BaSlider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform); // 图片缩放平滑（防锯齿）

    const int cy = height() / 2;
    const qreal trackH = 6;
    const qreal trackX0 = thumbStart();
    const qreal trackX1 = width() - 9;
    const qreal trackY = cy - trackH / 2.0;

    // 轨道底：细长浅灰白圆头条（真机 Options 音量轨：整段同色，无填充段）
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#D6DADD"));
    p.drawRoundedRect(QRectF(trackX0, trackY, trackX1 - trackX0, trackH), trackH / 2, trackH / 2);

    // 左端灰帽：轨道起点的小灰球（官方去噪示风格）
    const qreal capD = 13;
    p.setBrush(QColor("#BFC6CB"));
    p.drawEllipse(QPointF(trackX0, cy), capD / 2, capD / 2);

    // thumb：亮蓝大圆 + 白描边 + 顶部浅蓝径向渐变（3D 立体球感）
    const qreal tt = thumbCenter();
    const qreal thumbD = 22;
    QRadialGradient tg(QPointF(tt, cy - 4), thumbD * 0.9);
    tg.setColorAt(0, QColor("#5BC4F5"));
    tg.setColorAt(0.55, QColor("#33AEF0"));
    tg.setColorAt(1, QColor("#1F9AE6"));
    p.setPen(QPen(QColor(255, 255, 255, 230), 2));
    p.setBrush(tg);
    p.drawEllipse(QPointF(tt, cy), thumbD / 2, thumbD / 2);

    // 右喇叭：紧贴轨道右端内部（17x18 完整落于控件内，避免被右缘裁剪）
    if (volumeIcons_) {
        p.drawPixmap(QRectF(0, cy - 8, 14, 16).topLeft(), ba::pixmap(ba::Glyph::SpeakerMini, QSize(14, 16)));
        p.drawPixmap(QPointF(trackX1 + 4, cy - 9), ba::pixmap(ba::Glyph::Speaker, QSize(17, 18)));
    }
}

// thumb 可移动域起点（与轨道绘制域一致：有喇叭时避开左喇叭；无喇叭时为 9）
qreal BaSlider::thumbStart() const
{
    return 9 + (volumeIcons_ ? 40 : 0);
}

qreal BaSlider::thumbSpan() const
{
    return (width() - 9) - thumbStart();
}

qreal BaSlider::thumbCenter() const
{
    if (maximum() <= minimum())
        return thumbStart();
    // 位置始终按当前值映射（拖动值实时变化即跟手；按下平滑飞行由 thumbAnim_ 仅视觉过渡）
    const qreal ratio = qBound(qreal(0), qreal(value() - minimum()) / (maximum() - minimum()), qreal(1));
    return thumbStart() + ratio * thumbSpan();
}

void BaSlider::applyValueFromPos(int x)
{
    const qreal ratio = qBound(qreal(0), (qreal(x) - thumbStart()) / thumbSpan(), qreal(1));
    setValue(minimum() + int(ratio * (maximum() - minimum())));
}

void BaSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = true;
        setSliderDown(true);
        // thumb 从当前位置平滑飞向点击处（仅视觉过渡，值立即跳到点击位）
        const qreal ratio = qBound(qreal(0), (qreal(event->pos().x()) - thumbStart()) / thumbSpan(), qreal(1));
        const qreal target = thumbStart() + ratio * thumbSpan();
        thumbAnim_.start(thumbCenter(), target);
        applyValueFromPos(event->pos().x());
    }
    QAbstractSlider::mousePressEvent(event);
}

void BaSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging_) {
        // 拖动：接管位置（停止按下飞行动画，值随鼠标走 → thumb 跟手）
        thumbAnim_.stop();
        applyValueFromPos(event->pos().x());
    }
    QAbstractSlider::mouseMoveEvent(event);
}

void BaSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = false;
        setSliderDown(false);
    }
    QAbstractSlider::mouseReleaseEvent(event);
}

void BaSlider::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    applyValueFromPos(event->pos().x());
}

void BaSlider::enterEvent(QEnterEvent *event)
{
    QAbstractSlider::enterEvent(event);
    hovered_ = true;
    update();
}

void BaSlider::leaveEvent(QEvent *event)
{
    QAbstractSlider::leaveEvent(event);
    hovered_ = false;
    update();
}

void BaSlider::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:  setValue(value() - singleStep()); break;
    case Qt::Key_Right: setValue(value() + singleStep()); break;
    case Qt::Key_Home:  setValue(minimum()); break;
    case Qt::Key_End:   setValue(maximum()); break;
    default:            QAbstractSlider::keyPressEvent(event); return;
    }
}
