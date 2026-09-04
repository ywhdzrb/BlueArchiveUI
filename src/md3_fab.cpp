#include "md3_fab.h"

#include <QPainter>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>

namespace {

// 各规格像素尺寸
constexpr int kRegularSize = 56;
constexpr int kSmallSize = 40;
constexpr int kLargeSize = 96;

// Large 布局：图标 24x24 + 间距 + 文字，水平排列
constexpr int kLargePad = 24;       // Large 左右内边距
constexpr int kLargeGap = 8;        // 图标与文字间距
constexpr int kRadiusRegular = 16;  // MD3 圆角基准（规格=半径）

} // namespace

Md3Fab::Md3Fab(md3::Glyph glyph, QWidget *parent)
    : QAbstractButton(parent)
    , glyph_(glyph)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAutoFillBackground(false);

    // 图标尺寸对应 24x24 基准，按钮内图标区域按基准绘制
    // 阴影 Level 3：让 FAB 悬浮于内容之上
    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(24.0);
    shadow->setOffset(0, 6.0);
    shadow->setColor(QColor(0, 0, 0, 60));
    setGraphicsEffect(shadow);

    // Large 规格用 Headline Small 字体（24px）
    QFont f = font();
    f.setPointSizeF(14.0);
    f.setWeight(QFont::Medium);
    setFont(f);

    stateAnim_.setDuration(150);
    stateAnim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&stateAnim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        stateAlpha_ = v.toReal();
        update();
    });
}

void Md3Fab::setGlyph(md3::Glyph glyph)
{
    if (glyph_ == glyph) {
        return;
    }
    glyph_ = glyph;
    update();
}

void Md3Fab::setSize(Size size)
{
    if (size_ == size) {
        return;
    }
    size_ = size;
    setText(text_);   // 重新计算最小尺寸
    updateGeometry();
    update();
}

void Md3Fab::setTonal(bool tonal)
{
    if (tonal_ == tonal) {
        return;
    }
    tonal_ = tonal;
    update();
}

void Md3Fab::setText(const QString &text)
{
    text_ = text;
    updateGeometry();
    update();
}

void Md3Fab::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

QSize Md3Fab::sizeHint() const
{
    switch (size_) {
    case Size::Regular:
        return QSize(kRegularSize, kRegularSize);
    case Size::Small:
        return QSize(kSmallSize, kSmallSize);
    case Size::Large:
        // 扩展 FAB：图标 + 间距 + 文字，四周留白，宽度随文字拉伸
        return QSize(fontMetrics().horizontalAdvance(text_) + 2 * kLargePad + 24 + kLargeGap,
                     kLargeSize);
    }
    return QSize(kRegularSize, kRegularSize);
}

QSize Md3Fab::minimumSizeHint() const
{
    return sizeHint();
}

// 绘制：圆形底（按规格半径）→ 状态层 → 图标（Large 加文字）
void Md3Fab::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const bool disabled = !isEnabled();
    const QRectF r = rect();
    const int size = r.width();
    const qreal radius = qMin(r.width(), r.height()) / 2.0;   // 完整半圆角

    // 底色：tonal 用 secondary-container，否则 primary
    QColor bg = tonal_ ? theme_.secondaryContainer : theme_.primary;
    if (disabled) {
        bg = theme_.disabledContainer();
    }
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(r.adjusted(1, 1, -1, -1), radius, radius);

    // 状态层（hover / 按下，blend 叠加）
    if (!disabled && stateAlpha_ > 0.0) {
        const QColor layer = Md3Theme::blend(bg, theme_.onPrimary, stateAlpha_ / 100.0);
        p.setBrush(layer);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r.adjusted(1, 1, -1, -1), radius, radius);
    }

    // 图标 + 文字颜色
    const QColor content = disabled ? theme_.disabledContent() : (tonal_ ? theme_.onSecondaryContainer : theme_.onPrimary);

    if (size_ == Size::Large) {
        // Large：图标（左侧）+ 文字（右侧）整体居中
        const qreal textWidth = fontMetrics().horizontalAdvance(text_);
        const qreal total = 24 + kLargeGap + textWidth;
        const qreal startX = (r.width() - total) / 2.0;
        md3::paintGlyph(p, glyph_, startX + 12.0, r.center().y(), content, 2.0);
        p.setPen(content);
        p.setFont(font());
        // 文字区域：从图标右侧到右边距
        p.drawText(QRectF(startX + 24 + kLargeGap, r.top(), textWidth, r.height()),
                   Qt::AlignVCenter, text_);
    } else {
        // Regular / Small：图标居中
        const qreal cx = r.center().x();
        const qreal cy = r.center().y();
        md3::paintGlyph(p, glyph_, cx, cy, content, 2.0);
    }
}

void Md3Fab::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    hovered_ = true;
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}

void Md3Fab::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hovered_ = false;
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}

// 按下时驱动状态层抬升（isDown() 生效）
void Md3Fab::mousePressEvent(QMouseEvent *event)
{
    QAbstractButton::mousePressEvent(event);
    if (isDown()) {
        stateAnim_.stop();
        stateAnim_.setStartValue(stateAlpha_);
        stateAnim_.setEndValue(targetStateAlpha());
        stateAnim_.start();
    }
}

// 松开带回落到 hover 档
void Md3Fab::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}
