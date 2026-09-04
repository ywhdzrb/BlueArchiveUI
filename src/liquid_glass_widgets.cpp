#include "liquid_glass_widgets.h"
#include "liquid_glass.h"
#include "md3_icon.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QShowEvent>
#include <QResizeEvent>
#include <QFontMetrics>
#include <QLineEdit>
#include <QHideEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>
#include <QDateTime>
#include <QTimer>
#include <optional>

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
    // 磨砂素材：恒定 20px 模糊（面板版 0.10 → 20px 的默认档）
    frosted_ = frostedBackdrop(backdrop_, 20);
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
    // 磨砂采样图换用模糊快照（frosted_）；k/edgeK/glowMul 已弃用只是兼容传参
    return renderGlassPlateCPU(frosted_.isNull() ? backdrop_ : frosted_,
                               pr, pr.size(), corner, k, edgeK, dark_, glowMul);
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
    // 复位时长：拖动分支可能把 anim_ 临时改短到 100ms，后续程序设值必须回到标准时长
    anim_.setDuration(200);
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
    // 与滑块版一致：入参 min > max 时交换，保证后序 qBound 与比例计算正确
    if (min > max) {
        qSwap(min, max);
    }
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
        md3::paintGlyph(p, items_.at(i).glyph, itemCx, iconCy, iconColor, kGlyphStroke);

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
    if (items_.isEmpty()) {
        return;
    }
    const qreal itemW = width() / qreal(items_.size());
    const int idx = int(event->pos().x() / itemW);
    if (idx >= 0 && idx < items_.size()) {
        setCurrentIndex(idx);
    }
}

void LiquidGlassNavigationBar::mouseMoveEvent(QMouseEvent *event)
{
    if (items_.isEmpty()) {
        return;
    }
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

// ---------------------------------------------------------------------------
// 玻璃弹出菜单
// ---------------------------------------------------------------------------

namespace {
constexpr int kLgMenuRadius = 4;         // 与 Md3MenuPopup 一致
constexpr int kLgMenuItemHeight = 48;
constexpr int kLgMenuPadding = 8;
}

LiquidGlassMenuPopup::LiquidGlassMenuPopup(LiquidGlassDropdown *owner)
    : QWidget(nullptr, Qt::Popup)
    , owner_(owner)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);

    // 层级阴影（blur 16 / 偏移 2px，MD3 Elevation Level 2 等效）
    auto *effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(16.0);
    effect->setOffset(0, 2);
    effect->setColor(QColor(0, 0, 0, 90));
    setGraphicsEffect(effect);
}

void LiquidGlassMenuPopup::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

void LiquidGlassMenuPopup::setItems(const QStringList &items)
{
    items_ = items;
    update();
}

void LiquidGlassMenuPopup::setSelectedIndex(int index)
{
    selectedIndex_ = index;
    update();
}

// 宽度对齐宿主控件（至少 200px），高度由选项数量决定
QSize LiquidGlassMenuPopup::popupSize() const
{
    const int w = qMax(200, owner_ ? owner_->width() : 200);
    const int h = kLgMenuPadding * 2 + items_.size() * kLgMenuItemHeight;
    return QSize(w, h);
}

int LiquidGlassMenuPopup::itemAt(const QPoint &pos) const
{
    if (pos.x() < 0 || pos.x() >= width()) {
        return -1;
    }
    const int y = pos.y() - kLgMenuPadding;
    if (y < 0) {
        return -1;
    }
    const int idx = y / kLgMenuItemHeight;
    return idx < items_.size() ? idx : -1;
}

void LiquidGlassMenuPopup::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 菜单为独立窗口，抓不到宿主背景快照：用半透磨砂表面色模拟玻璃体。
    // 明暗由主题背景亮度推断（亮色表面 94%、暗色 86% 不透明度）
    const QRectF r = rect().adjusted(1, 1, -1, -1);
    QColor menuBg = theme_.surface;
    menuBg.setAlphaF(theme_.background.lightness() < 128 ? 0.86 : 0.94);
    p.setBrush(menuBg);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(r, kLgMenuRadius, kLgMenuRadius);

    for (int i = 0; i < items_.size(); ++i) {
        const QRectF itemRect(0, kLgMenuPadding + i * kLgMenuItemHeight,
                              width(), kLgMenuItemHeight);

        // hover 状态层：on-surface 8% 叠加
        if (i == hoverIndex_) {
            QColor layer = Md3Theme::blend(theme_.surface, theme_.onSurface, 0.08);
            p.setBrush(layer);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(itemRect.adjusted(1, 1, -1, -1), kLgMenuRadius, kLgMenuRadius);
        }

        // 选项文字：选中项 primary 加粗，其余 on-surface
        const bool selected = (i == selectedIndex_);
        QFont f = p.font();
        f.setPointSizeF(16.0);
        f.setWeight(selected ? QFont::Medium : QFont::Normal);
        p.setFont(f);
        p.setPen(selected ? theme_.primary : theme_.onSurface);
        const qreal textX = kLgMenuPadding + 16;
        const QString elided = QFontMetrics(f).elidedText(items_.at(i), Qt::ElideRight,
                                                          int(width() - textX - kLgMenuPadding));
        p.drawText(QRectF(textX, itemRect.y(), width() - textX - kLgMenuPadding, kLgMenuItemHeight),
                   Qt::AlignVCenter | Qt::AlignLeft, elided);

        // 选中标记：对勾折线
        if (selected) {
            const qreal cx = kLgMenuPadding + 8;
            const qreal cy = itemRect.center().y();
            QPen checkPen(theme_.primary, 2.0);
            checkPen.setCapStyle(Qt::RoundCap);
            checkPen.setJoinStyle(Qt::RoundJoin);
            p.setPen(checkPen);
            QPainterPath check;
            check.moveTo(cx - 3, cy);
            check.lineTo(cx, cy + 3);
            check.lineTo(cx + 4, cy - 3);
            p.drawPath(check);
        }
    }
}

void LiquidGlassMenuPopup::mouseMoveEvent(QMouseEvent *event)
{
    const int idx = itemAt(event->pos());
    if (idx != hoverIndex_) {
        hoverIndex_ = idx;
        update();
    }
}

void LiquidGlassMenuPopup::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
}

void LiquidGlassMenuPopup::mouseReleaseEvent(QMouseEvent *event)
{
    const int idx = itemAt(event->pos());
    if (idx >= 0) {
        emit itemClicked(idx);
    }
}

void LiquidGlassMenuPopup::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hoverIndex_ = -1;
    update();
}

void LiquidGlassMenuPopup::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    emit closed();
}

// ---------------------------------------------------------------------------
// 玻璃输入框
// ---------------------------------------------------------------------------

namespace {
// 与 md3_text_field 一致的几何常量
constexpr int kFieldHeight = 56;
constexpr qreal kFieldRadiusTop = 4.0;
constexpr qreal kFieldIconSize = 18.0;
constexpr int kFieldIconPad = 12;
constexpr int kFieldTextPad = 12;      // 编辑框左/右默认内边距
constexpr int kFieldIconGap = 8;       // 图标与文字区的间隔
}

LiquidGlassTextField::LiquidGlassTextField(const QString &placeholder, QWidget *parent)
    : LiquidGlassThemeKeeper(parent)
{
    setFixedHeight(kFieldHeight);
    setCursor(Qt::IBeamCursor);

    // 内部编辑框：透明背景无边框，玻璃底色由父控件绘制
    editor_ = new QLineEdit(this);
    editor_->setPlaceholderText(placeholder);
    editor_->setFrame(false);
    editor_->setAttribute(Qt::WA_MacShowFocusRect, false);
    editor_->setStyleSheet("QLineEdit { background: transparent; border: none; }");

    QFont inputFont = editor_->font();
    inputFont.setPointSizeF(16.0);
    editor_->setFont(inputFont);

    // 聚焦指示线过渡动画，150ms 符合 MD3 短动效时长
    anim_.setDuration(150);
    anim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        focusProgress_ = v.toReal();
        update();
    });
    connect(editor_, &QLineEdit::textChanged, this, [this](const QString &) { update(); });
    connect(editor_, &QLineEdit::selectionChanged, this, [this]() { update(); });
}

void LiquidGlassTextField::setTheme(const Md3Theme &theme)
{
    LiquidGlassThemeKeeper::setTheme(theme);
    QPalette pal = editor_->palette();
    pal.setColor(QPalette::Text, theme_.onSurface);
    pal.setColor(QPalette::PlaceholderText, theme_.onSurfaceVariant);
    editor_->setPalette(pal);
    update();
}

QString LiquidGlassTextField::text() const
{
    return editor_->text();
}

void LiquidGlassTextField::setText(const QString &text)
{
    editor_->setText(text);
}

void LiquidGlassTextField::setPlaceholderText(const QString &placeholder)
{
    editor_->setPlaceholderText(placeholder);
}

void LiquidGlassTextField::setLeadingIcon(std::optional<md3::Glyph> glyph)
{
    leadingGlyph_ = glyph;
    resizeEvent(nullptr);
    update();
}

void LiquidGlassTextField::setTrailingIcon(std::optional<md3::Glyph> glyph)
{
    trailingGlyph_ = glyph;
    resizeEvent(nullptr);
    update();
}

void LiquidGlassTextField::setTrailingIconClicked(std::function<void()> callback)
{
    trailingCallback_ = std::move(callback);
}

QSize LiquidGlassTextField::sizeHint() const
{
    return QSize(240, kFieldHeight);
}

// 顶部填充区矩形：与控件同尺寸（玻璃输入框不加 helper 区，保持 56 高）
QRectF LiquidGlassTextField::textFieldRect() const
{
    return QRectF(0, 0, width(), kFieldHeight);
}

void LiquidGlassTextField::showEvent(QShowEvent *event)
{
    LiquidGlassThemeKeeper::showEvent(event);
    backdropDeferred();
}

void LiquidGlassTextField::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    // 左/右按前后缀图标是否显示增加内边距，底边留给指示线 8px
    const qreal left = leadingGlyph_.has_value() ? (kFieldTextPad + kFieldIconSize + kFieldIconGap)
                                                 : kFieldTextPad;
    const qreal right = trailingGlyph_.has_value() ? (kFieldTextPad + kFieldIconSize + kFieldIconGap)
                                                   : kFieldTextPad;
    editor_->setGeometry(int(left), 4, int(width() - left - right), kFieldHeight - 8);
}

void LiquidGlassTextField::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 玻璃底板：仅顶部圆角。素材未就绪时回退表面色（md3 同款观感）
    const QRectF fr = textFieldRect();
    QImage glass = renderGlassPlate(kFieldRadiusTop, 0.10, 0.18, 0.7);
    QPainterPath shell;
    shell.moveTo(0, kFieldRadiusTop);
    shell.arcTo(QRectF(0, 0, 2 * kFieldRadiusTop, 2 * kFieldRadiusTop), 180, -90);
    shell.lineTo(fr.width() - 2 * kFieldRadiusTop, 0);
    shell.arcTo(QRectF(fr.width() - 2 * kFieldRadiusTop, 0, 2 * kFieldRadiusTop, 2 * kFieldRadiusTop), 90, -90);
    shell.lineTo(fr.width(), fr.height());
    shell.lineTo(0, fr.height());
    shell.closeSubpath();

    if (!glass.isNull()) {
        p.setClipPath(shell);
        p.drawImage(fr.toRect(), glass, fr.toRect());
        p.setClipping(false);
    } else {
        p.fillPath(shell, theme_.surfaceContainerHighest);
    }

    // 前后缀图标：18px 线性图标，on-surface-variant
    const qreal cy = fr.height() / 2.0;
    if (leadingGlyph_.has_value()) {
        md3::paintGlyph(p, *leadingGlyph_, kFieldIconPad + kFieldIconSize / 2.0, cy,
                        theme_.onSurfaceVariant, 2.0);
    }
    if (trailingGlyph_.has_value()) {
        md3::paintGlyph(p, *trailingGlyph_, width() - kFieldIconPad - kFieldIconSize / 2.0, cy,
                        theme_.onSurfaceVariant, 2.0);
    }

    // 底部指示线：未聚焦 1px outline-variant，聚焦 2px primary，渐变过渡
    const QColor lineColor = Md3Theme::blend(theme_.outlineVariant, theme_.primary, focusProgress_);
    p.setPen(QPen(lineColor, 1.0 + focusProgress_));
    p.drawLine(QPointF(0, fr.height() - 0.5), QPointF(fr.width(), fr.height() - 0.5));
}

void LiquidGlassTextField::mousePressEvent(QMouseEvent *event)
{
    if (trailingGlyph_.has_value()) {
        // 命中区域：以图标中心为圆心的约 24px 邻域（x/y 双向判定，避免全高横带误触）
        const qreal cx = width() - kFieldIconPad - kFieldIconSize / 2.0;
        const qreal cy = kFieldHeight / 2.0;
        if (qAbs(event->position().x() - cx) < 12.0
            && qAbs(event->position().y() - cy) < 12.0) {
            if (trailingCallback_) {
                trailingCallback_();
            }
            emit trailingIconClicked();
            return;
        }
    }
    editor_->setFocus();
}

void LiquidGlassTextField::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    anim_.stop();
    anim_.setStartValue(focusProgress_);
    anim_.setEndValue(1.0);
    anim_.start();
}

void LiquidGlassTextField::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    anim_.stop();
    anim_.setStartValue(focusProgress_);
    anim_.setEndValue(0.0);
    anim_.start();
}

// ---------------------------------------------------------------------------
// 玻璃浮动按钮
// ---------------------------------------------------------------------------

LiquidGlassFab::LiquidGlassFab(md3::Glyph glyph, QWidget *parent)
    : LiquidGlassThemeKeeper(parent)
    , glyph_(glyph)
{
    setCursor(Qt::PointingHandCursor);
    setFixedSize(56, 56);

    // 状态层动画：150ms OutCubic
    stateAnim_.setDuration(150);
    stateAnim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&stateAnim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        stateAlpha_ = v.toReal();
        update();
    });
}

QSize LiquidGlassFab::sizeHint() const
{
    return size_ == Size::Small ? QSize(40, 40) : QSize(56, 56);
}

void LiquidGlassFab::setGlyph(md3::Glyph glyph)
{
    glyph_ = glyph;
    update();
}

void LiquidGlassFab::setSize(Size size)
{
    size_ = size;
    setFixedSize(sizeHint());
    update();
}

void LiquidGlassFab::setTonal(bool tonal)
{
    tonal_ = tonal;
    update();
}

void LiquidGlassFab::showEvent(QShowEvent *event)
{
    LiquidGlassThemeKeeper::showEvent(event);
    backdropDeferred();
}

void LiquidGlassFab::resizeEvent(QResizeEvent *event)
{
    LiquidGlassThemeKeeper::resizeEvent(event);
    backdropDeferred();
}

void LiquidGlassFab::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    hovered_ = true;
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(hkHoverAlpha);
    stateAnim_.start();
}

void LiquidGlassFab::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hovered_ = false;
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(0.0);
    stateAnim_.start();
}

// 按下时状态层抬升（QAbstractButton 基类更新 isDown() 后驱动动画）
void LiquidGlassFab::mousePressEvent(QMouseEvent *event)
{
    QAbstractButton::mousePressEvent(event);
    if (isDown()) {
        stateAnim_.stop();
        stateAnim_.setStartValue(stateAlpha_);
        stateAnim_.setEndValue(hkHoverAlpha);
        stateAnim_.start();
    }
}

// 松开回落到 hover 档
void LiquidGlassFab::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(hovered_ ? hkHoverAlpha : 0.0);
    stateAnim_.start();
}

// 绘制：圆形玻璃板 → 主题色 tint（primary / secondary-container）→
// 顶部高光 → 状态层（hover/press 白色叠加）→ 图标
void LiquidGlassFab::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = rect();
    const qreal dia = qMin(r.width(), r.height());
    const qreal radius = dia / 2.0;
    const QPointF c = r.center();

    // 圆形玻璃板：以控件外接方为取景框，折射窗口背景
    QImage glass = renderGlassPlate(radius, 0.10, 0.18, 1.0);
    QPainterPath shell;
    shell.addEllipse(c, radius - 1, radius - 1);
    if (!glass.isNull()) {
        p.setClipPath(shell);
        p.drawImage(r.toRect(), glass, r.toRect());
        p.setClipping(false);
    } else {
        p.fillPath(shell, theme_.surfaceContainerHighest);
    }

    // 主题色 tint：玻璃着彩色罩，主调 primary / tonal 用 secondary-container。
    // 强度取 0.45：暗背景上足以读出玻璃内透出的主题色
    QColor tint = tonal_ ? theme_.secondaryContainer : theme_.primary;
    tint.setAlphaF(0.45);
    p.fillPath(shell, tint);

    // 顶部高光带：白色渐变强化玻璃球感（1/3 高度）
    if (!glass.isNull()) {
        QLinearGradient sheen(c.x(), 0, c.x(), dia * 0.5);
        sheen.setColorAt(0.0, QColor(255, 255, 255, 70));
        sheen.setColorAt(1.0, QColor(255, 255, 255, 0));
        p.setPen(Qt::NoPen);
        p.setBrush(sheen);
        p.drawEllipse(c, radius - 1, radius - 1);
    }

    // hover / 按压状态层：白色 12% 叠加
    if (stateAlpha_ > 0.0) {
        QColor layer = QColor(255, 255, 255, int(2.55 * stateAlpha_));
        p.fillPath(shell, layer);
    }

    // 图标：tonal 用 on-secondary-container，其余 on-primary
    const QColor iconColor = tonal_ ? theme_.onSecondaryContainer : theme_.onPrimary;
    md3::paintGlyph(p, glyph_, c.x(), c.y(), iconColor, 2.0);
}

// ---------------------------------------------------------------------------
// 玻璃下拉选择框
// ---------------------------------------------------------------------------

namespace {
constexpr int kDropHeight = 56;             // 与 md3_dropdown 一致
constexpr qreal kDropRadiusTop = 4.0;
constexpr int kDropTextPad = 16;            // 文字左内边距
constexpr int kDropArrowPad = 16;           // 箭头中心距右缘
}

LiquidGlassDropdown::LiquidGlassDropdown(const QStringList &items, const QString &placeholder,
                                         QWidget *parent)
    : LiquidGlassThemeKeeper(parent)
    , items_(items)
    , placeholder_(placeholder)
{
    setFixedHeight(kDropHeight);
    setCursor(Qt::PointingHandCursor);

    QFont f = font();
    f.setPointSizeF(16.0);
    setFont(f);

    anim_.setDuration(150);
    anim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        activeProgress_ = v.toReal();
        update();
    });

    popup_ = new LiquidGlassMenuPopup(this);
    connect(popup_, &LiquidGlassMenuPopup::itemClicked, this, &LiquidGlassDropdown::onItemClicked);
    connect(popup_, &LiquidGlassMenuPopup::closed, this, &LiquidGlassDropdown::onPopupClosed);
}

// popup_（Qt::Popup 顶层窗口）构造时忽略父指针，必须显式释放
LiquidGlassDropdown::~LiquidGlassDropdown()
{
    delete popup_;
}

void LiquidGlassDropdown::setItems(const QStringList &items)
{
    items_ = items;
    if (currentIndex_ >= items_.size()) {
        currentIndex_ = -1;
        emit currentIndexChanged(currentIndex_);
    }
    update();
}

void LiquidGlassDropdown::setPlaceholderText(const QString &placeholder)
{
    placeholder_ = placeholder;
    update();
}

void LiquidGlassDropdown::setCurrentIndex(int index)
{
    if (index == currentIndex_) {
        return;
    }
    currentIndex_ = index;
    update();
    emit currentIndexChanged(index);
}

QString LiquidGlassDropdown::currentText() const
{
    if (currentIndex_ >= 0 && currentIndex_ < items_.size()) {
        return items_.at(currentIndex_);
    }
    return QString();
}

QSize LiquidGlassDropdown::sizeHint() const
{
    return QSize(200, kDropHeight);
}

void LiquidGlassDropdown::showEvent(QShowEvent *event)
{
    LiquidGlassThemeKeeper::showEvent(event);
    backdropDeferred();
}

void LiquidGlassDropdown::resizeEvent(QResizeEvent *event)
{
    LiquidGlassThemeKeeper::resizeEvent(event);
    backdropDeferred();
}

// 绘制：玻璃底板（顶部圆角）→ 状态层 → 文本 → 箭头 → 底部指示线
void LiquidGlassDropdown::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QImage glass = renderGlassPlate(kDropRadiusTop, 0.10, 0.18, 0.7);
    QPainterPath shell;
    shell.moveTo(0, kDropRadiusTop);
    shell.arcTo(QRectF(0, 0, 2 * kDropRadiusTop, 2 * kDropRadiusTop), 180, -90);
    shell.lineTo(width() - 2 * kDropRadiusTop, 0);
    shell.arcTo(QRectF(width() - 2 * kDropRadiusTop, 0, 2 * kDropRadiusTop, 2 * kDropRadiusTop), 90, -90);
    shell.lineTo(width(), height());
    shell.lineTo(0, height());
    shell.closeSubpath();

    if (!glass.isNull()) {
        p.setClipPath(shell);
        p.drawImage(rect(), glass, rect());
        p.setClipping(false);
    } else {
        p.fillPath(shell, theme_.surfaceContainerHighest);
    }

    // hover / 展开状态层：on-surface 10%，透明度随过渡进度
    if (activeProgress_ > 0.0) {
        QColor layer = Md3Theme::blend(theme_.surfaceContainerHighest, theme_.onSurface,
                                       0.10 * activeProgress_);
        p.fillPath(shell, layer);
    }

    // 文本：已选中显示选项内容，否则显示占位提示
    const bool hasValue = currentIndex_ >= 0 && currentIndex_ < items_.size();
    p.setFont(font());
    p.setPen(hasValue ? theme_.onSurface : theme_.onSurfaceVariant);
    const QString text = hasValue ? items_.at(currentIndex_) : placeholder_;
    const qreal textRight = width() - 2 * kDropArrowPad - 8;
    const QString elided = QFontMetrics(font()).elidedText(text, Qt::ElideRight,
                                                           qMax(0, int(textRight - kDropTextPad)));
    p.drawText(QRectF(kDropTextPad, 0, textRight - kDropTextPad, height()),
               Qt::AlignVCenter | Qt::AlignLeft, elided);

    // 下拉箭头：V 形折线，激活时过渡为 primary
    if (!items_.isEmpty()) {
        const QColor arrowColor = Md3Theme::blend(theme_.onSurfaceVariant, theme_.primary,
                                                  activeProgress_);
        QPen pen(arrowColor, 2.0);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        const qreal cx = width() - kDropArrowPad - 5;
        const qreal cy = height() / 2.0;
        QPainterPath arrow;
        arrow.moveTo(cx - 4, cy - 2);
        arrow.lineTo(cx, cy + 2);
        arrow.lineTo(cx + 4, cy - 2);
        p.drawPath(arrow);
    }

    // 底部指示线：未激活 1px outline-variant，激活 2px primary，渐变过渡
    const QColor lineColor = Md3Theme::blend(theme_.outlineVariant, theme_.primary, activeProgress_);
    p.setPen(QPen(lineColor, 1.0 + activeProgress_));
    p.drawLine(QPointF(0, height() - 0.5), QPointF(width(), height() - 0.5));
}

void LiquidGlassDropdown::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    if (items_.isEmpty()) {
        return;
    }
    if (menuOpen_) {
        popup_->hide();
        return;
    }
    showPopup();
}

void LiquidGlassDropdown::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    hovered_ = true;
    animateActive(hovered_ || menuOpen_);
}

void LiquidGlassDropdown::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hovered_ = false;
    animateActive(hovered_ || menuOpen_);
}

void LiquidGlassDropdown::animateActive(bool active)
{
    anim_.stop();
    anim_.setStartValue(activeProgress_);
    anim_.setEndValue(active ? 1.0 : 0.0);
    anim_.start();
}

// 在控件正下方弹出菜单，空间不足时向上弹出
void LiquidGlassDropdown::showPopup()
{
    popup_->setTheme(theme_);
    popup_->setItems(items_);
    popup_->setSelectedIndex(currentIndex_);
    const QSize sz = popup_->popupSize();

    const QPoint anchor = mapToGlobal(QPoint(0, height()));
    QPoint pos = anchor;
    if (const QScreen *screen = QGuiApplication::screenAt(anchor)) {
        if (anchor.y() + sz.height() > screen->availableGeometry().bottom()) {
            pos = mapToGlobal(QPoint(0, -sz.height()));
        }
    }

    popup_->resize(sz);
    popup_->move(pos);
    menuOpen_ = true;
    animateActive(true);
    popup_->show();
    popup_->raise();
    popup_->setFocus();
}

void LiquidGlassDropdown::onItemClicked(int index)
{
    setCurrentIndex(index);
    popup_->hide();
}

void LiquidGlassDropdown::onPopupClosed()
{
    menuOpen_ = false;
    animateActive(hovered_ || menuOpen_);
}
