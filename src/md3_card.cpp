#include "md3_card.h"
#include "md3_theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QLabel>
#include <QResizeEvent>
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

// 顶部图片区：空 pixmap 清除，否则创建/更新 QLabel 并驱动布局偏移
void Md3Card::setImage(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        if (imageLabel_) {
            imageLabel_->setVisible(false);
            imageLabel_->deleteLater();
            imageLabel_ = nullptr;
        }
        updateGeometry();
        update();
        return;
    }

    if (!imageLabel_) {
        // 图片子控件：自身绘制在卡片层之上，内容布局位移由 paintEvent 控制
        imageLabel_ = new QLabel(this);
        imageLabel_->setScaledContents(true);
        imageLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);   // 不拦截内容点击
        imageLabel_->show();
    }
    imageLabel_->setPixmap(pixmap);
    imageRadius_ = 12.0;
    updateGeometry();
    update();
}

// 图片圆角（仅顶部两角启用；底部贴合指示线）
void Md3Card::setImageRadius(qreal radius)
{
    imageRadius_ = radius;
    update();
}

// 更新图片区几何：全出血 0 边距，高度按 pixmap 原始比例拉伸到卡片宽度；
// 内容布局顶部内边距随图片高度下移（图片占去内容顶部空间）
void Md3Card::updateImageLayout()
{
    if (!imageLabel_) {
        contentLayout_->setContentsMargins(kPadding, kPadding, kPadding, kPadding);
        return;
    }
    // QLabel setScaledContents 后按自身几何等比拉伸，高度即给高
    const int h = imageLabel_->heightForWidth(width());
    imageLabel_->setGeometry(0, 0, width(), h);
    // 顶部内容相对图片底边让位（原 kPadding + 图片高 + 8px 间隙）
    contentLayout_->setContentsMargins(kPadding, kPadding + h + 8, kPadding, kPadding);
}

// 尺寸变化：图片区等比缩放，内容布局下移图片高度 + 间距
void Md3Card::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    if (imageLabel_) {
        updateImageLayout();
    }
    QWidget::resizeEvent(event);
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

    // 图片区：按 12px 圆角裁剪出圆角大图，紧贴卡片顶部全出血展开
    if (imageLabel_ && imageRadius_ > 0.0) {
        updateImageLayout();
        QPainterPath imagePath;
        imagePath.addRoundedRect(QRectF(0, 0, width(), imageLabel_->height()),
                                 imageRadius_, imageRadius_,
                                 Qt::AbsoluteSize);
        // 顶部两角圆角、底边直角：用路径重建 4 角
        QPainterPath topClip;
        topClip.addRoundedRect(QRectF(0, 0, width(), imageLabel_->height() + imageRadius_),
                               imageRadius_, imageRadius_);
        p.setClipPath(topClip);
        imageLabel_->render(&p);
        p.setClipping(false);
    }

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
