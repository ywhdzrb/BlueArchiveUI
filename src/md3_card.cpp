#include "md3_card.h"
#include "md3_theme.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QEnterEvent>

namespace {
constexpr int kRadius = 12;
constexpr int kPadding = 16;
constexpr int kMinHeight = 96;
}

Md3Card::Md3Card(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    // hover 状态层淡入淡出动画（150ms OutCubic，与按钮一致）
    hoverAnim_.setDuration(150);
    hoverAnim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&hoverAnim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        hoverAlpha_ = v.toReal();
        update();
    });

    // 内部内容区，四周 16px 内边距符合 MD3 卡片规范
    contentLayout_ = new QVBoxLayout(this);
    contentLayout_->setContentsMargins(kPadding, kPadding, kPadding, kPadding);
    contentLayout_->setSpacing(8);

    // Elevated 规格默认挂上层级阴影（Level 1 等效）
    shadowEffect();
}

// 设置卡片规格并刷新阴影
void Md3Card::setStyle(Style style)
{
    if (style_ == style) {
        return;
    }
    style_ = style;
    shadowEffect()->setEnabled(style_ == Style::Elevated);
    update();
}

// 切换主题并重绘
void Md3Card::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

// 挂载内容控件到卡片内容区
void Md3Card::setContent(QWidget *content)
{
    contentLayout_->addWidget(content);
}

QSize Md3Card::sizeHint() const
{
    return QSize(200, kMinHeight);
}

// 背景色：Elevated 用表面色，Filled 用 surface-container-highest，Outlined 用表面色
QColor Md3Card::backgroundColor() const
{
    switch (style_) {
    case Style::Elevated:
    case Style::Outlined:
        return theme_.surface;
    case Style::Filled:
        return theme_.surfaceContainerHighest;
    }
    return theme_.surface;
}

// Elevated 卡片阴影，懒加载创建（blur 16 / 垂直偏移 2px 近似 Level 1）
QGraphicsDropShadowEffect *Md3Card::shadowEffect() const
{
    auto *effect = qobject_cast<QGraphicsDropShadowEffect *>(this->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsDropShadowEffect(const_cast<Md3Card *>(this));
        effect->setBlurRadius(16.0);
        effect->setOffset(0, 2);
        effect->setColor(QColor(0, 0, 0, 90));
        const_cast<Md3Card *>(this)->setGraphicsEffect(effect);
    }
    return effect;
}

// 绘制：圆角背景 → 描边 → hover 状态层
void Md3Card::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = rect().adjusted(1, 1, -1, -1);
    const QColor bg = backgroundColor();

    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(r, kRadius, kRadius);

    // Outlined 规格画 1px 描边
    if (style_ == Style::Outlined) {
        QPen pen(theme_.outlineVariant, 1.0);
        p.setBrush(Qt::NoBrush);
        p.setPen(pen);
        p.drawRoundedRect(r, kRadius, kRadius);
    }

    // hover 状态层：on-surface 8% 叠加，透明度由动画驱动
    if (hoverAlpha_ > 0.0) {
        QColor layer = Md3Theme::blend(bg, theme_.onSurface, 0.08 * hoverAlpha_);
        p.setBrush(layer);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, kRadius, kRadius);
    }
}

void Md3Card::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    hovered_ = true;
    // 从当前透明度淡入到 1
    hoverAnim_.stop();
    hoverAnim_.setStartValue(hoverAlpha_);
    hoverAnim_.setEndValue(1.0);
    hoverAnim_.start();
}

void Md3Card::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hovered_ = false;
    // 从当前透明度淡出到 0
    hoverAnim_.stop();
    hoverAnim_.setStartValue(hoverAlpha_);
    hoverAnim_.setEndValue(0.0);
    hoverAnim_.start();
}
