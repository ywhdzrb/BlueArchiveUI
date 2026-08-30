#include "md3_button.h"
#include "md3_theme.h"

#include <QPainter>
#include <QEnterEvent>

namespace {
// 控件外观常量（像素），取自 MD3 按钮规范
constexpr int kHeight = 40;
constexpr int kPaddingX = 24;
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

QSize Md3Button::sizeHint() const
{
    // 宽度 = 文本宽 + 两侧内边距，高度固定 40px
    return QSize(fontMetrics().horizontalAdvance(text()) + 2 * kPaddingX, kHeight);
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

    // 文字（禁用态用 38% on-surface）
    p.setPen(disabled ? theme_.disabledContent() : contentColor());
    p.setFont(font());
    p.drawText(rect(), Qt::AlignCenter, text());
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
