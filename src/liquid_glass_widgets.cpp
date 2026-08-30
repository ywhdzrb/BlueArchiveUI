#include "liquid_glass_widgets.h"
#include "liquid_glass.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QShowEvent>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QDateTime>
#include <QTimer>
#include <QFile>
#include <QTextStream>

// ---------------------------------------------------------------------------
// 基类：主题状态 + 快照抓取 + 玻璃板渲染
// ---------------------------------------------------------------------------

// 抓帧节流（毫秒）：show/resize 会连续触发，限制 300ms 内最多抓一次
constexpr qint64 kBackdropThrottleMs = 300;

LiquidGlassThemeKeeper::LiquidGlassThemeKeeper(QWidget *parent)
    : QAbstractButton(parent)
{
    // 标记玻璃控件：抓帧时互相隐藏（素材中不包含任何玻璃帧）
    setProperty("lgGlass", true);
}

// 抓取窗口快照并缓存，按实例节流（防止 show/resize 风暴），首帧无素材必抓。
// 注意：快照是**全窗口**视角（所有玻璃控件与遮罩已隐藏），所有玻璃控件
// 共享同一张背景，各自渲染自己窗口矩形区域。
void LiquidGlassThemeKeeper::grabBackdrop(bool force)
{
    if (grabInProgress_) {
        return;
    }
    grabInProgress_ = true;   // 防重入：hide/show 会同步触发 resize→再次抓帧
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    // 仅当「已有可用素材且距上次抓帧 < 节流窗口」才跳过：
    // 无素材（首帧）或强制刷新（页面/主题切换）时必抓
    if (!force && !backdrop_.isNull() && now - lastGrabbedMs_ < kBackdropThrottleMs) {
        grabInProgress_ = false;
        return;
    }
    lastGrabbedMs_ = now;

    QWidget *w = window();
    if (!w || w->width() <= 0 || w->height() <= 0) {
        grabInProgress_ = false;
        return;
    }
    backdrop_ = grabGlassBackdrop(this, w);
    backdropWinSize_ = w->size();   // 记录快照时的窗口几何，用于失效检测
    grabInProgress_ = false;
    update();
}

// show/resize 后的异步抓帧：布局 pass 尚未落定时直接抓会抓到旧几何
//（玻璃板内折射出控件自身残影 = 用户可见的「看到自己」）。
// 用 zero-timer 延后到事件循环下一个空档，等布局稳定再抓。
void LiquidGlassThemeKeeper::backdropDeferred()
{
    QTimer::singleShot(0, this, [this]() {
        if (!isVisible()) {
            return;
        }
        grabBackdrop();
        update();
    });
}

// 本控件在顶层窗口坐标系中的矩形（与抓来的快照同坐标系）
QRect LiquidGlassThemeKeeper::mappedPanelRect() const
{
    const QWidget *w = window();
    if (!w) {
        return QRect();
    }
    return QRect(mapTo(const_cast<QWidget *>(w), QPoint(0, 0)), size());
}

// 渲染本控件的玻璃板：整体为一块倒角玻璃（主边缘折射 + tint + 透光）。
// 圆角=控件外观圆角（外部形状由各自 paintEvent 再裁），滑块 thumbs 等
// 细元素由子类画在玻璃板上。viewRect 为**控件内部坐标**矩形（默认为
// 整个控件）：轨道类的取景矩形要使边缘倒角带占自身足够比例，
// 函数内部再换算到窗口坐标供折射采样。
QImage LiquidGlassThemeKeeper::renderGlassPlate(qreal corner, qreal k, qreal edgeK,
                                                qreal glowMul, const QRect &viewRect)
{
    QRect pr;
    QWidget *w = window();
    if (viewRect.isNull()) {
        pr = mappedPanelRect();
    } else {
        if (!w) {
            return QImage();
        }
        const QPoint win = mapTo(w, viewRect.topLeft());
        pr = QRect(win, viewRect.size());
    }
    if (pr.isEmpty()) {
        return QImage();
    }
    // 快照未就绪、取景区不被覆盖、或**窗口几何已变**（快照所反映的布局
    // 是旧几何：控件位置/文字与当前不符——「背景错位」症状）时调度异步
    // 重抓（paint 循环内严禁同步抓帧: hide/show + render 顶层窗口在绘画
    // 期间会递归 repaint 卡死），本轮先回退占位。
    const bool stale = w && backdropWinSize_.isValid() && (w->size() != backdropWinSize_);
    if (backdrop_.isNull() || !backdrop_.rect().contains(pr) || stale) {
        backdropDeferred();
    }
    if (backdrop_.isNull() || !backdrop_.rect().contains(pr) || stale) {
        return QImage();
    }
    return renderGlassPlateCPU(backdrop_, pr, pr.size(), corner, k, edgeK, dark_, glowMul);
}

// ---------------------------------------------------------------------------
// 玻璃开关
// ---------------------------------------------------------------------------

namespace {
// 与 md3_switch 一致的外观常量
constexpr int kTrackWidth = 52;
constexpr int kTrackHeight = 32;
constexpr qreal kThumbRadiusOff = 8.0;
constexpr qreal kThumbRadiusOn = 12.0;
constexpr qreal kCenterXOff = 16.0;
constexpr qreal kCenterXOn = 36.0;
}

LiquidGlassSwitch::LiquidGlassSwitch(QWidget *parent)
    : LiquidGlassThemeKeeper(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setCheckable(true);
    setFixedSize(kTrackWidth, kTrackHeight);

    anim_.setDuration(200);
    anim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        progress_ = v.toReal();
        update();
    });
    connect(this, &QAbstractButton::toggled, this, &LiquidGlassSwitch::animateTo);
}

QSize LiquidGlassSwitch::sizeHint() const
{
    return QSize(kTrackWidth, kTrackHeight);
}

void LiquidGlassSwitch::showEvent(QShowEvent *event)
{
    LiquidGlassThemeKeeper::showEvent(event);
    backdropDeferred();
}

// 绘制：玻璃轨道（52x32 胶囊）→ 选中色渐变 → thumb 玻璃球
void LiquidGlassSwitch::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const qreal t = progress_;

    // 玻璃底板：先按主题色 tint 出一块玻璃材质
    QImage glass = renderGlassPlate(kTrackHeight / 2.0, 0.10, 0.18, 0.6);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r(rect());

    if (!glass.isNull()) {
        // 玻璃材质作为底板，圆角裁剪到 32 胶囊
        QPainterPath clip;
        clip.addRoundedRect(r, kTrackHeight / 2.0, kTrackHeight / 2.0);
        p.setClipPath(clip);
        p.drawImage(QPoint(0, 0), glass);
        p.setClipping(false);

        // 选中态 overlay：primary 色玻璃罩，随 progress 淡入。
        // 强度取较高值：暗色背景上 38% 罩层几乎不可见，观感退化成
        // 普通半透明条，55% 才能读出玻璃里透的主题色
        if (t > 0.001) {
            QColor primary = theme_.primary;
            primary.setAlphaF(0.55 * t);
            p.setBrush(primary);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(r, kTrackHeight / 2.0, kTrackHeight / 2.0);
        }
    } else {
        // 材质没就绪时的兜底：纯色轨道（与 md3 一致）
        const QColor trackBg = Md3Theme::blend(theme_.surfaceContainerHighest, theme_.primary, t);
        p.setBrush(trackBg);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, kTrackHeight / 2.0, kTrackHeight / 2.0);
    }

    // thumb：半透明白玻璃球（玻璃观感：白面 + 弱描边）
    const qreal radius = kThumbRadiusOff + (kThumbRadiusOn - kThumbRadiusOff) * t;
    const qreal cx = kCenterXOff + (kCenterXOn - kCenterXOff) * t;
    const QColor thumbCol = dark_ ? QColor(235, 240, 250, 235) : QColor(255, 255, 255, 240);
    p.setBrush(thumbCol);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(cx, kTrackHeight / 2.0), radius, radius);
    // 顶部 1/3 高光：白 alpha 递增强化玻璃球感
    QLinearGradient sheen(0, 0, 0, kTrackHeight);
    sheen.setColorAt(0.0, QColor(255, 255, 255, 130));
    sheen.setColorAt(0.5, QColor(255, 255, 255, 0));
    p.setClipRect(QRectF(cx - radius, 0, radius * 2, radius));
    QPainterPath thumbClip;
    thumbClip.addEllipse(QPointF(cx, kTrackHeight / 2.0), radius, radius);
    p.setClipPath(thumbClip);
    p.fillRect(QRectF(cx - radius, 0, radius * 2, radius * 0.6), sheen);
    p.setClipping(false);
}

void LiquidGlassSwitch::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !isEnabled()) {
        QWidget::mousePressEvent(event);
        return;
    }
    setChecked(!isChecked());
}

void LiquidGlassSwitch::animateTo(bool checked)
{
    anim_.stop();
    anim_.setStartValue(progress_);
    anim_.setEndValue(checked ? 1.0 : 0.0);
    anim_.start();
}

// ---------------------------------------------------------------------------
// 玻璃滑块
// ---------------------------------------------------------------------------

namespace {
// 与 md3_slider 一致的几何常量（前缀区别于开关段的 kTrackWidth 组）
constexpr qreal kSliderTrackHeight = 16.0;
constexpr qreal kSliderTrackRadius = 8.0;
constexpr qreal kSliderInnerRadius = 2.0;
constexpr qreal kSliderThumbWidth = 4.0;
constexpr qreal kSliderThumbWidthActive = 2.0;
constexpr qreal kSliderThumbHeight = 44.0;
constexpr qreal kSliderTrackGap = 6.0;
constexpr qreal kSliderStopIndicatorRadius = 2.0;
constexpr int kSliderWidgetHeight = 48;
constexpr qreal kSliderTrackPad = 10.0;
}

LiquidGlassSlider::LiquidGlassSlider(QWidget *parent)
    : LiquidGlassThemeKeeper(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setFixedHeight(kSliderWidgetHeight);

    anim_.setDuration(200);
    anim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        displayValue_ = v.toReal();
        update();
    });
}

void LiquidGlassSlider::setRange(int min, int max)
{
    if (min > max) {
        qSwap(min, max);
    }
    min_ = min;
    max_ = max;
    setValue(value_);
}

void LiquidGlassSlider::setValue(int value)
{
    value = qBound(min_, value, max_);
    if (value == value_) {
        return;
    }
    const qreal from = anim_.state() == QAbstractAnimation::Running ? displayValue_ : qreal(value_);
    value_ = value;
    anim_.stop();
    anim_.setStartValue(from);
    anim_.setEndValue(qreal(value_));
    anim_.start();
    emit valueChanged(value_);
}

QSize LiquidGlassSlider::sizeHint() const
{
    return QSize(200, kSliderWidgetHeight);
}

void LiquidGlassSlider::showEvent(QShowEvent *event)
{
    LiquidGlassThemeKeeper::showEvent(event);
    backdropDeferred();
}

void LiquidGlassSlider::resizeEvent(QResizeEvent *event)
{
    LiquidGlassThemeKeeper::resizeEvent(event);
    backdropDeferred();
}

void LiquidGlassSlider::setValueFromPos(int x)
{
    const qreal trackLeft = kSliderTrackPad;
    const qreal trackRight = width() - kSliderTrackPad;
    const qreal ratio = (x - trackLeft) / (trackRight - trackLeft);
    const qreal clamped = qBound(0.0, ratio, 1.0);
    const int target = qRound(min_ + clamped * (max_ - min_));
    if (pressed_) {
        if (target == value_) {
            return;
        }
        value_ = target;
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

// 绘制：全矩形玻璃底 → 轨道胶囊（玻璃材质区域）→ active 段主题色 →
// stop indicator → 玻璃 thumb 竖条
void LiquidGlassSlider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const qreal cy = height() / 2.0;
    const qreal trackLeft = kSliderTrackPad;
    const qreal trackRight = width() - kSliderTrackPad;
    const qreal trackWidth = trackRight - trackLeft;
    const qreal ratio = (max_ > min_) ? (displayValue_ - min_) / qreal(max_ - min_) : 0.0;
    const qreal thumbX = trackLeft + ratio * trackWidth;
    const bool thumbThin = pressed_;

    // 玻璃轨道板：取景用轨道自身矩形（控件内部坐标，函数内映射窗口），
    // 倒角折射带相对 16px 高的轨道才有足够占比；若按控件整高 48px 取景，
    // 边缘带只占一半高度，中央折射几乎为零
    const QRectF trackRect(trackLeft, cy - kSliderTrackHeight / 2.0, trackWidth, kSliderTrackHeight);
    QImage trackGlass = renderGlassPlate(kSliderTrackHeight / 2.0, 0.10, 0.18, 0.7,
                                         trackRect.toRect());

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (!trackGlass.isNull()) {
        // 玻璃轨道：只显示轨道胶囊区域，其余部分裁剪掉
        QPainterPath clip;
        clip.addRoundedRect(trackRect, kSliderTrackRadius, kSliderTrackRadius);
        p.setClipPath(clip);
        p.drawImage(trackRect, trackGlass, trackRect);
        // 玻璃主体提亮：暗色背景上玻璃呈现近黑，加一层白玻璃罩
        // 恢复透明玻璃的通透感（与 active 段主题色形成明度对比）
        p.fillRect(trackRect, QColor(255, 255, 255, dark_ ? 26 : 40));
        p.setClipping(false);
    } else {
        p.setBrush(theme_.secondaryContainer);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(trackRect, kSliderTrackRadius, kSliderTrackRadius);
    }

    // active 段：primary 色玻璃罩（半透明，保留材质底）
    const qreal activeEnd = thumbX - kSliderTrackGap;
    if (activeEnd > trackLeft) {
        const qreal top = cy - kSliderTrackHeight / 2.0;
        const QRectF ar(trackLeft, top, activeEnd - trackLeft, kSliderTrackHeight);
        QPainterPath activePath;
        activePath.moveTo(ar.left(), top + kSliderTrackRadius);
        activePath.arcTo(QRectF(ar.left(), top, 2 * kSliderTrackRadius, 2 * kSliderTrackRadius), 180, -90);
        activePath.lineTo(ar.right() - kSliderInnerRadius, top);
        activePath.arcTo(QRectF(ar.right() - 2 * kSliderInnerRadius, top,
                                2 * kSliderInnerRadius, 2 * kSliderInnerRadius), 90, -90);
        activePath.lineTo(ar.right(), top + kSliderTrackHeight);
        activePath.arcTo(QRectF(ar.right() - 2 * kSliderInnerRadius, top + kSliderTrackHeight - 2 * kSliderInnerRadius,
                                2 * kSliderInnerRadius, 2 * kSliderInnerRadius), 0, -90);
        activePath.lineTo(ar.left() + kSliderTrackRadius, top + kSliderTrackHeight);
        activePath.arcTo(QRectF(ar.left(), top + kSliderTrackHeight - 2 * kSliderTrackRadius,
                                2 * kSliderTrackRadius, 2 * kSliderTrackRadius), 270, -90);
        activePath.closeSubpath();
        QColor primary = theme_.primary;
        primary.setAlphaF(0.62);
        p.setBrush(primary);
        p.setPen(Qt::NoPen);
        p.drawPath(activePath);
    }

    // stop indicator
    const qreal stopX = trackRight - kSliderTrackHeight / 2.0;
    if (thumbX < stopX) {
        p.setBrush(theme_.primary);
        p.drawEllipse(QPointF(stopX, cy), kSliderStopIndicatorRadius, kSliderStopIndicatorRadius);
    }

    // thumb：玻璃竖条（半透明白 + 右缘高光），press 变细
    const qreal thumbW = thumbThin ? kSliderThumbWidthActive : kSliderThumbWidth;
    const QRectF thumbRect(thumbX - thumbW / 2.0, cy - kSliderThumbHeight / 2.0, thumbW, kSliderThumbHeight);
    const QColor thumbBg = dark_ ? QColor(225, 232, 245, 215) : QColor(255, 255, 255, 225);
    p.setBrush(thumbBg);
    p.setPen(dark_ ? QPen(QColor(255, 255, 255, 70), 1.0) : QPen(QColor(255, 255, 255, 130), 1.0));
    p.drawRoundedRect(thumbRect, thumbW / 2.0, thumbW / 2.0);
}

void LiquidGlassSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !isEnabled()) {
        QWidget::mousePressEvent(event);
        return;
    }
    setValueFromPos(event->position().x());
    pressed_ = true;
}

void LiquidGlassSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (pressed_) {
        setValueFromPos(event->position().x());
    }
}

void LiquidGlassSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        pressed_ = false;
        update();
    }
}

// ---------------------------------------------------------------------------
// 玻璃进度条
// ---------------------------------------------------------------------------

namespace {
constexpr qreal kBarHeight = 4.0;       // 与 md3 一致
constexpr qreal kBarRadius = 2.0;
constexpr qreal kIndicatorWidth = 0.25;
}

LiquidGlassProgressBar::LiquidGlassProgressBar(QWidget *parent)
    : LiquidGlassThemeKeeper(parent)
{
    setFixedHeight(kBarHeight);
    anim_.setDuration(1600);
    anim_.setStartValue(0.0);
    anim_.setEndValue(1.0);
    anim_.setLoopCount(-1);
    anim_.setEasingCurve(QEasingCurve::Linear);
    connect(&anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        slide_ = v.toReal();
        update();
    });
}

void LiquidGlassProgressBar::setRange(int min, int max)
{
    min_ = min;
    max_ = max;
    value_ = qBound(min_, value_, max_);
    update();
}

void LiquidGlassProgressBar::setValue(int value)
{
    value_ = qBound(min_, value, max_);
    update();
}

void LiquidGlassProgressBar::setIndeterminate(bool indeterminate)
{
    if (indeterminate_ == indeterminate) {
        return;
    }
    indeterminate_ = indeterminate;
    if (indeterminate_) {
        anim_.start();
    } else {
        anim_.stop();
        slide_ = 0.0;
    }
    update();
}

QSize LiquidGlassProgressBar::sizeHint() const
{
    return QSize(200, qRound(kBarHeight));
}

void LiquidGlassProgressBar::showEvent(QShowEvent *event)
{
    LiquidGlassThemeKeeper::showEvent(event);
    backdropDeferred();
}

void LiquidGlassProgressBar::resizeEvent(QResizeEvent *event)
{
    LiquidGlassThemeKeeper::resizeEvent(event);
    backdropDeferred();
}

void LiquidGlassProgressBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF bar(0, 0, width(), kBarHeight);
    // 取景用 bar 自身矩形（控件内部坐标）：4px 高的条按自身取景才能让
    // 边缘倒角带盖过整条；按整控件取景折射带宽不足 1px
    QImage glass = renderGlassPlate(kBarRadius, 0.10, 0.18, 0.4, bar.toRect());
    if (!glass.isNull()) {
        QPainterPath clip;
        clip.addRoundedRect(bar, kBarRadius, kBarRadius);
        p.setClipPath(clip);
        p.drawImage(bar, glass, bar);
        p.setClipping(false);
    } else {
        p.setBrush(theme_.surfaceContainerHighest);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(bar, kBarRadius, kBarRadius);
    }

    p.setPen(Qt::NoPen);
    p.setBrush(theme_.primary);
    if (indeterminate_) {
        const qreal indW = width() * kIndicatorWidth;
        const qreal x = -indW + slide_ * (width() + indW);
        p.drawRoundedRect(QRectF(x, 0, indW, kBarHeight), kBarRadius, kBarRadius);
    } else {
        const qreal ratio = max_ > min_ ? qreal(value_ - min_) / (max_ - min_) : 0.0;
        if (ratio > 0.0) {
            p.drawRoundedRect(QRectF(0, 0, width() * ratio, kBarHeight), kBarRadius, kBarRadius);
        }
    }
}

// ---------------------------------------------------------------------------
// 玻璃卡片
// ---------------------------------------------------------------------------

namespace {
constexpr int kRadius = 12;         // 与 md3_card 一致
constexpr int kPadding = 16;
constexpr int kMinHeight = 96;
}

LiquidGlassCard::LiquidGlassCard(QWidget *parent)
    : LiquidGlassThemeKeeper(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    hoverAnim_.setDuration(150);
    hoverAnim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&hoverAnim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        hoverAlpha_ = v.toReal();
        update();
    });

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(kPadding, kPadding, kPadding, kPadding);
    lay->setSpacing(8);
}

void LiquidGlassCard::setContent(QWidget *content)
{
    qobject_cast<QVBoxLayout *>(layout())->addWidget(content);
}

QSize LiquidGlassCard::sizeHint() const
{
    return QSize(200, kMinHeight);
}

void LiquidGlassCard::showEvent(QShowEvent *event)
{
    LiquidGlassThemeKeeper::showEvent(event);
    backdropDeferred();
}

void LiquidGlassCard::resizeEvent(QResizeEvent *event)
{
    LiquidGlassThemeKeeper::resizeEvent(event);
    backdropDeferred();
}

void LiquidGlassCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = rect().adjusted(1, 1, -1, -1);
    QImage glass = renderGlassPlate(kRadius, 0.10, 0.18, 1.0);
    if (!glass.isNull()) {
        QPainterPath clip;
        clip.addRoundedRect(r, kRadius, kRadius);
        p.setClipPath(clip);
        p.drawImage(r, glass, r);
        p.setClipping(false);
    } else {
        p.setBrush(theme_.surface);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, kRadius, kRadius);
    }

    // hover 状态层：on-surface 8% 叠加（与 md3_card 一致）
    if (hoverAlpha_ > 0.0) {
        QColor layer = Md3Theme::blend(theme_.surface, theme_.onSurface,
                                       0.08 * hoverAlpha_);
        p.setBrush(layer);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, kRadius, kRadius);
    }
}

void LiquidGlassCard::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    hovered_ = true;
    hoverAnim_.stop();
    hoverAnim_.setStartValue(hoverAlpha_);
    hoverAnim_.setEndValue(1.0);
    hoverAnim_.start();
}

void LiquidGlassCard::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hovered_ = false;
    hoverAnim_.stop();
    hoverAnim_.setStartValue(hoverAlpha_);
    hoverAnim_.setEndValue(0.0);
    hoverAnim_.start();
}

// ---------------------------------------------------------------------------
// 玻璃导航栏
// ---------------------------------------------------------------------------

namespace {
// iOS 26 液态玻璃导航栏规格：悬浮胶囊条 + 紧凑 tab（icon + label 同屏）
constexpr int kNavHeight = 64;          // 导航栏玻璃条高度
constexpr qreal kIndicatorW = 56.0;     // 选中玻璃泡胶囊尺寸
constexpr qreal kIndicatorH = 34.0;
constexpr qreal kBarCorner = 22.0;      // 玻璃条圆角（苹果悬浮条质感）
constexpr qreal kGlyphStroke = 2.0;     // 图标描边宽度
constexpr qreal kIconOffsetTop = 14.0;  // 图标中心顶部留白（居中+紧凑）
}

LiquidGlassNavigationBar::LiquidGlassNavigationBar(QWidget *parent)
    : LiquidGlassThemeKeeper(parent)
{
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(kNavHeight);

    // 选中玻璃泡滑动动画：200ms OutCubic，当前 tab 切换时从旧位滑到新位
    bubbleAnim_.setDuration(200);
    bubbleAnim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&bubbleAnim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        floatIndex_ = v.toReal();
        update();
    });
}

int LiquidGlassNavigationBar::addItem(const QString &label, Glyph glyph)
{
    items_.append({label, glyph});
    if (currentIndex_ < 0) {
        currentIndex_ = 0;
    }
    update();
    return items_.size() - 1;
}

void LiquidGlassNavigationBar::clearItems()
{
    items_.clear();
    currentIndex_ = -1;
    hoverIndex_ = -1;
    update();
}

void LiquidGlassNavigationBar::setCurrentIndex(int index)
{
    if (index < 0 || index >= items_.size() || index == currentIndex_) {
        return;
    }
    const qreal from = (currentIndex_ < 0 || floatIndex_ < 0) ? qreal(index) : floatIndex_;
    currentIndex_ = index;
    bubbleAnim_.stop();
    bubbleAnim_.setStartValue(from);
    bubbleAnim_.setEndValue(qreal(index));
    bubbleAnim_.start();
    update();
    emit currentIndexChanged(index);
}

QSize LiquidGlassNavigationBar::sizeHint() const
{
    // 4-6 项：每项 68 宽（紧凑），最小整条 300
    return QSize(qMax(300, items_.size() * 68), kNavHeight);
}

void LiquidGlassNavigationBar::showEvent(QShowEvent *event)
{
    LiquidGlassThemeKeeper::showEvent(event);
    backdropDeferred();
}

// 绘制：玻璃板（折射窗口背景）→ 每项（选中胶囊 / 悬停状态 → 图标 → 标签）
void LiquidGlassNavigationBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = rect().adjusted(1, 1, -1, -1);

    // 玻璃底板：折射来源为窗口背景快照（与其它玻璃控件共享）
    QImage glass = renderGlassPlate(kBarCorner, 0.10, 0.18, 1.0);
    if (!glass.isNull()) {
        QPainterPath clip;
        clip.addRoundedRect(r, kBarCorner, kBarCorner);
        p.setClipPath(clip);
        p.drawImage(r, glass, r);
        p.setClipping(false);
    } else {
        p.setBrush(theme_.surfaceContainerLow);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, kBarCorner, kBarCorner);
    }

    if (items_.isEmpty()) {
        return;
    }

    // 玻璃泡连续索引：动画运行中由 anim 驱动，未动画时=当前 index
    if (floatIndex_ < 0.0) {
        floatIndex_ = qreal(currentIndex_);
    }
    const qreal bubbleIndex = qBound(0.0, floatIndex_, qreal(items_.size() - 1));

    // 导航项均分：每项 68 宽
    const qreal itemW = width() / qreal(items_.size());

    // 选中玻璃泡**先画**（底层），图标/标签再叠在其上——
    // 泡为不透明胶囊，后画会盖住选中项图标与文字
    const qreal bubbleCx = itemW * (bubbleIndex + 0.5);
    const QRectF bubble(bubbleCx - kIndicatorW / 2.0,
                        kIconOffsetTop,
                        kIndicatorW, kIndicatorH);
    paintSelectedBubble(p, bubble);

    for (int i = 0; i < items_.size(); ++i) {
        const qreal itemCx = itemW * (i + 0.5);
        const qreal iconCy = kIconOffsetTop + kIndicatorH / 2.0;
        const bool selected = (i == currentIndex_);
        const QRectF indicator(itemCx - kIndicatorW / 2.0, iconCy - kIndicatorH / 2.0,
                               kIndicatorW, kIndicatorH);

        // 悬停 / 轻点区域：非选中项显示 on-surface 6% 状态层
        if (!selected && i == hoverIndex_) {
            QColor layer = Md3Theme::blend(theme_.surface, theme_.onSurface, 0.06);
            p.setBrush(layer);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(indicator, kIndicatorH / 2.0, kIndicatorH / 2.0);
        }

        // 图标：选中 primary（玻璃泡内），否则 on-surface-variant
        const QColor iconColor = selected ? theme_.primary : theme_.onSurfaceVariant;
        paintGlyph(p, items_.at(i).glyph, itemCx, iconCy, iconColor);

        // 标签：选中 primary Medium，否则 on-surface-variant；label 11px 紧凑
        QFont f = p.font();
        f.setPointSizeF(11.0);
        f.setWeight(selected ? QFont::Medium : QFont::Normal);
        p.setFont(f);
        p.setPen(selected ? theme_.primary : theme_.onSurfaceVariant);
        const QString elided = QFontMetrics(f).elidedText(items_.at(i).label, Qt::ElideRight,
                                                          itemW - 8);
        p.drawText(QRectF(itemCx - itemW / 2.0, iconCy + kIndicatorH / 2.0, itemW,
                          kNavHeight - kIndicatorH - kIconOffsetTop),
                   Qt::AlignHCenter | Qt::AlignVCenter, elided);
    }
}

// 选中指示器：MD3 风格纯色胶囊 —— secondaryContainer 实色填充（与 md3 侧栏
// primary-container 胶囊语义一致），无高光/反光/暗缘等玻璃层次。
// 泡保留滑动位置插值（floatIndex_），仅外观回归 md3。
void LiquidGlassNavigationBar::paintSelectedBubble(QPainter &p, const QRectF &bubble)
{
    const qreal corner = bubble.height() / 2.0;
    QColor indicator = theme_.secondaryContainer;
    if (dark_) {
        indicator = Md3Theme::blend(indicator, theme_.primaryContainer, 0.25);
    }
    p.setBrush(indicator);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(bubble, corner, corner);
}

void LiquidGlassNavigationBar::mousePressEvent(QMouseEvent *event)
{
    const qreal itemW = width() / qreal(items_.size());
    const int idx = int(event->pos().x() / itemW);
    if (idx >= 0 && idx < items_.size()) {
        setCurrentIndex(idx);
    }
}

void LiquidGlassNavigationBar::mouseMoveEvent(QMouseEvent *event)
{
    const qreal itemW = width() / qreal(items_.size());
    const int idx = int(event->pos().x() / itemW);
    const int target = (idx >= 0 && idx < items_.size()) ? idx : -1;
    if (target != hoverIndex_) {
        hoverIndex_ = target;
        update();
    }
}

void LiquidGlassNavigationBar::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hoverIndex_ = -1;
    update();
}

// 绘制 24x24 基准的线性图标，居中于 (cx, cy)。与 md3_side_bar::paintGlyph
// 同源几何：角度约定 0°=右、90°=上、180°=左、270°=下。
void LiquidGlassNavigationBar::paintGlyph(QPainter &p, Glyph glyph, qreal cx, qreal cy,
                                          const QColor &color) const
{
    QPen pen(color, kGlyphStroke);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);

    p.save();
    p.translate(cx - 12.0, cy - 12.0);

    switch (glyph) {
    case Glyph::Home: {
        // 屋顶 + 墙体 + 门
        p.setPen(pen);
        QPainterPath path;
        path.moveTo(2, 11);
        path.lineTo(12, 3);
        path.lineTo(22, 11);
        p.drawPath(path);

        path = QPainterPath();
        path.moveTo(6, 10);
        path.lineTo(6, 20);
        path.lineTo(18, 20);
        path.lineTo(18, 10);
        p.drawPath(path);

        path = QPainterPath();
        path.moveTo(10, 20);
        path.lineTo(10, 15);
        path.lineTo(14, 15);
        path.lineTo(14, 20);
        p.drawPath(path);
        break;
    }
    case Glyph::Search: {
        // 放大镜：圆 + 手柄
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(10, 10), 5, 5);
        QPainterPath handle;
        handle.moveTo(13.5, 13.5);
        handle.lineTo(20, 20);
        p.drawPath(handle);
        break;
    }
    case Glyph::Star: {
        // 五角星：内 / 外半径交替的十边形
        QPainterPath path;
        const qreal outer = 8.5;
        const qreal inner = 3.5;
        for (int k = 0; k < 10; ++k) {
            const qreal r = (k % 2 == 0) ? outer : inner;
            const qreal angle = qDegreesToRadians(-90.0 + k * 36.0);
            const QPointF pt(12.0 + r * qCos(angle), 12.0 + r * qSin(angle));
            if (k == 0) {
                path.moveTo(pt);
            } else {
                path.lineTo(pt);
            }
        }
        path.closeSubpath();
        p.setPen(pen);
        p.drawPath(path);
        break;
    }
    case Glyph::Person: {
        // 头部实心圆 + 肩部半圆弧
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(12, 8.5), 3.5, 3.5);

        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        QPainterPath shoulders;
        shoulders.moveTo(4, 21);
        shoulders.arcTo(QRectF(4, 14, 16, 14), 180, 180);
        p.drawPath(shoulders);
        break;
    }
    case Glyph::Palette: {
        // 调色板：圆盘 + 孔
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(12, 12), 9, 9);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(7, 7), 1.5, 1.5);
        p.drawEllipse(QPointF(17, 7), 1.5, 1.5);
        p.drawEllipse(QPointF(12, 18), 1.5, 1.5);
        p.drawEllipse(QPointF(5, 13), 1.5, 1.5);
        p.drawEllipse(QPointF(19, 13), 1.5, 1.5);
        break;
    }
    case Glyph::Drop: {
        // 水滴：顶部尖角 + 四段贝塞尔下摆成圆底
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        QPainterPath drop;
        drop.moveTo(12, 4);
        drop.cubicTo(16.4, 8.4, 19, 11.8, 19, 15);
        drop.cubicTo(19, 18.9, 15.9, 21, 12, 21);
        drop.cubicTo(8.1, 21, 5, 18.9, 5, 15);
        drop.cubicTo(5, 11.8, 7.6, 8.4, 12, 4);
        p.drawPath(drop);
        break;
    }
    }

    p.restore();
}
