#include "md3_button.h"
#include "md3_theme.h"
#include "md3_icon.h"

#include <QPainter>
#include <QEnterEvent>
#include <QMouseEvent>

namespace {
// 控件外观常量（像素），取自 MD3 按钮规范
constexpr int kHeight = 40;
constexpr int kPaddingX = 24;
constexpr qreal kIconGap = 8;     // 图标与文本间距
constexpr qreal kHoverAlpha = 8.0;   // hover 状态层透明度
constexpr qreal kPressAlpha = 12.0;  // 按下状态层透明度
constexpr qreal kFocusAlpha = 10.0;  // 聚焦状态层透明度
}

Md3Button::Md3Button(const QString &text, QWidget *parent)
    : QAbstractButton(parent)
{
    setText(text);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    // Fixed 水平策略：按钮保持紧凑宽度，不被布局横向拉伸
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // MD3 Label Large 排版：14px / Medium 字重
    QFont btnFont = font();
    btnFont.setPointSizeF(14.0);
    btnFont.setWeight(QFont::Medium);
    setFont(btnFont);

    // hover 状态层透明度动画，150ms 符合 MD3 短动效时长
    stateAnim_.setDuration(150);
    stateAnim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&stateAnim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        stateAlpha_ = v.toReal();
        update();
    });
}

// 设置按钮规格并立即重绘
void Md3Button::setStyle(Style style)
{
    if (style_ == style) {
        return;
    }
    style_ = style;
    update();
}

// 切换主题并重绘
void Md3Button::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

// 设置前置图标（默认开启显示）
void Md3Button::setIcon(md3::Glyph glyph)
{
    glyph_ = glyph;
    iconVisible_ = true;
    updateGeometry();
    update();
}

void Md3Button::setIconVisible(bool visible)
{
    iconVisible_ = visible;
    updateGeometry();
    update();
}

QSize Md3Button::sizeHint() const
{
    // 宽度 = 图标（含间距） + 文本宽 + 两侧内边距，高度固定 40px
    const qreal extra = iconVisible_ ? 18.0 + kIconGap : 0.0;
    return QSize(static_cast<int>(fontMetrics().horizontalAdvance(text()) + extra + 2 * kPaddingX),
                 kHeight);
}

QSize Md3Button::minimumSizeHint() const
{
    return sizeHint();
}

// 目标状态层透明度：hover / 聚焦 / 按下取最大值
qreal Md3Button::targetStateAlpha() const
{
    qreal alpha = 0.0;
    if (hovered_ || isDown()) {
        alpha = hovered_ ? kHoverAlpha : kPressAlpha;
    }
    if (focused_) {
        alpha = qMax(alpha, kFocusAlpha);
    }
    if (isDown()) {
        alpha = kPressAlpha;
    }
    return alpha;
}

// 背景色：Filled 用 primary，FilledTonal 用 secondary-container，其余透明
QColor Md3Button::backgroundColor() const
{
    switch (style_) {
    case Style::Filled:
        return theme_.primary;
    case Style::FilledTonal:
        return theme_.secondaryContainer;
    case Style::Outlined:
    case Style::Text:
        return Qt::transparent;
    }
    return Qt::transparent;
}

// 文字色：Filled 用 on-primary，FilledTonal 用 on-secondary-container，其余 primary
QColor Md3Button::contentColor() const
{
    switch (style_) {
    case Style::Filled:
        return theme_.onPrimary;
    case Style::FilledTonal:
        return theme_.onSecondaryContainer;
    case Style::Outlined:
    case Style::Text:
        return theme_.primary;
    }
    return theme_.primary;
}

// 状态层基色：Filled 用 on-primary，FilledTonal 用 on-secondary-container，
// Outlined 用 on-surface，Text 用 primary
QColor Md3Button::stateLayerColor() const
{
    switch (style_) {
    case Style::Filled:
        return theme_.onPrimary;
    case Style::FilledTonal:
        return theme_.onSecondaryContainer;
    case Style::Outlined:
        return theme_.onSurface;
    case Style::Text:
        return theme_.primary;
    }
    return theme_.primary;
}

// 绘制：背景 → 状态层 → 描边边框 → 文字，全部按主题角色着色
void Md3Button::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const QRectF r = rect().adjusted(1, 1, -1, -1);
    const bool disabled = !isEnabled();
    const QColor bg = disabled ? theme_.disabledContainer() : backgroundColor();

    // 背景（实心规格才有底色，Outlined / Text 透明）
    if (bg.alpha() > 0) {
        p.setBrush(bg);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, kHeight / 2.0, kHeight / 2.0);
    }

    // 状态层覆盖。
    // 透明背景按钮（Outlined / Text）直接叠加带 alpha 的 tint，否则 blend 会把
    // 透明黑的 RGB 分量混入，hover 时产生近黑覆盖层
    if (!disabled && stateAlpha_ > 0.0) {
        QColor layer;
        if (bg.alpha() == 0) {
            layer = stateLayerColor();
            layer.setAlphaF(stateAlpha_ / 100.0);
        } else {
            layer = Md3Theme::blend(bg, stateLayerColor(), stateAlpha_ / 100.0);
        }
        p.setBrush(layer);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, kHeight / 2.0, kHeight / 2.0);
    }

    // Outlined 规格画 1px 轮廓边框（禁用态用 outline-variant）
    if (style_ == Style::Outlined) {
        QPen pen(disabled ? theme_.outlineVariant : theme_.outline, 1.0);
        p.setBrush(Qt::NoBrush);
        p.setPen(pen);
        p.drawRoundedRect(r, kHeight / 2.0, kHeight / 2.0);
    }

    // 文字（禁用态用 38% on-surface）。有图标时留出左侧图标 + 间距
    p.setPen(disabled ? theme_.disabledContent() : contentColor());
    p.setFont(font());
    if (iconVisible_) {
        const qreal textWidth = fontMetrics().horizontalAdvance(text());
        const qreal total = 18.0 + kIconGap + textWidth;
        const qreal startX = (width() - total) / 2.0;
        md3::paintGlyph(p, glyph_, startX + 9.0, height() / 2.0,
                        disabled ? theme_.disabledContent() : contentColor(), 2.0);
        p.drawText(QRectF(startX + 18.0 + kIconGap, 0, textWidth + kPaddingX, height()),
                   Qt::AlignVCenter, text());
    } else {
        p.drawText(rect(), Qt::AlignCenter, text());
    }
}

void Md3Button::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    hovered_ = true;
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}

void Md3Button::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hovered_ = false;
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}

// 按下时状态层立即抬升到 press 档（isDown() 由基类更新，随后驱动动画）
void Md3Button::mousePressEvent(QMouseEvent *event)
{
    QAbstractButton::mousePressEvent(event);
    if (isDown()) {
        stateAnim_.stop();
        stateAnim_.setStartValue(stateAlpha_);
        stateAnim_.setEndValue(targetStateAlpha());
        stateAnim_.start();
    }
}

// 松开后回落到 hover / 无状态档
void Md3Button::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}

void Md3Button::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    focused_ = true;
    update();
}

void Md3Button::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    focused_ = false;
    update();
}
