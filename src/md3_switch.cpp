#include "md3_switch.h"
#include "md3_theme.h"

#include <QPainter>
#include <QMouseEvent>

namespace {
// MD3 规范尺寸：track 52x32，圆角 16；关闭态 thumb 直径 16，选中态 24
constexpr int kTrackWidth = 52;
constexpr int kTrackHeight = 32;
constexpr qreal kThumbRadiusOff = 8.0;   // 未选中 thumb 半径
constexpr qreal kThumbRadiusOn = 12.0;   // 选中 thumb 半径
constexpr qreal kCenterXOff = 16.0;      // 未选中 thumb 中心 x（与左侧圆角圆心重合）
constexpr qreal kCenterXOn = 36.0;       // 选中 thumb 中心 x（与右侧圆角圆心重合）
}

Md3Switch::Md3Switch(QWidget *parent)
    : QAbstractButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setCheckable(true);
    setFixedSize(kTrackWidth, kTrackHeight);

    // 选中态切换动画，200ms OutCubic 近似 MD3 开关过渡
    anim_.setDuration(200);
    anim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        progress_ = v.toReal();
        update();
    });

    // 选中状态变化时驱动动画，覆盖程序与点击两种入口
    connect(this, &QAbstractButton::toggled, this, &Md3Switch::animateTo);
}

// 切换主题并重绘
void Md3Switch::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

QSize Md3Switch::sizeHint() const
{
    return QSize(kTrackWidth, kTrackHeight);
}

// 线性插值辅助，t 取值 0~1
static qreal lerp(qreal a, qreal b, qreal t)
{
    return a + (b - a) * t;
}

// 绘制 track 与 thumb，颜色与几何随 progress_ 平滑过渡
void Md3Switch::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const qreal t = progress_;
    const bool disabled = !isEnabled();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // track：背景在 surface-container-highest 与 primary 之间过渡，
    // 边框在 outline 与 primary 之间过渡（选中后边框消失）
    const QRectF track(0, 0, kTrackWidth, kTrackHeight);
    const QColor trackBg = Md3Theme::blend(theme_.surfaceContainerHighest, theme_.primary, t);
    p.setBrush(trackBg);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(track, kTrackHeight / 2.0, kTrackHeight / 2.0);

    if (t < 0.999) {
        const QColor border = Md3Theme::blend(theme_.outline, theme_.primary, t);
        QPen pen(border, 2.0);
        p.setBrush(Qt::NoBrush);
        p.setPen(pen);
        // 边框内嵌描边：矩形向内缩 1px 使 2px 描边完全落在 track 内部，
        // 避免居中描边在四角圆弧处向外凸出
        const qreal inset = 1.0;
        p.drawRoundedRect(QRectF(inset, inset,
                                 kTrackWidth - 2.0 * inset, kTrackHeight - 2.0 * inset),
                          kTrackHeight / 2.0 - inset, kTrackHeight / 2.0 - inset);
    }

    // thumb：半径与位置随 progress 过渡，颜色从 outline 过渡到 on-primary
    const qreal radius = lerp(kThumbRadiusOff, kThumbRadiusOn, t);
    const qreal cx = lerp(kCenterXOff, kCenterXOn, t);
    const QColor thumbColor = disabled
        ? theme_.disabledContainer()
        : Md3Theme::blend(theme_.outline, theme_.onPrimary, t);

    p.setBrush(thumbColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(cx, kTrackHeight / 2.0), radius, radius);
}

// 点击切换选中态并播放动画
void Md3Switch::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !isEnabled()) {
        QAbstractButton::mousePressEvent(event);
        return;
    }
    setChecked(!isChecked());
}

// 将 progress_ 动画驱动到目标状态
void Md3Switch::animateTo(bool checked)
{
    anim_.stop();
    anim_.setStartValue(progress_);
    anim_.setEndValue(checked ? 1.0 : 0.0);
    anim_.start();
}
