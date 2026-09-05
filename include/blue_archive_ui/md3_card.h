#pragma once

#include <QWidget>
#include <QVariantAnimation>
#include "md3_theme.h"

class QGraphicsDropShadowEffect;
class QVBoxLayout;
class QLabel;
class QPixmap;

// MD3 卡片：覆盖 Elevated / Filled / Outlined 三种规格，圆角 12px。
// Elevated 带层级阴影，悬停时状态层淡入淡出动画，内容通过 setContent 挂载。
class Md3Card : public QWidget
{
    Q_OBJECT

public:
    enum class Style {
        Elevated,  // 表面色 + 层级阴影
        Filled,    // surface-container-highest 填充
        Outlined   // 表面色 + 1px 描边
    };
    Q_ENUM(Style)

    explicit Md3Card(QWidget *parent = nullptr);

    void setStyle(Style style);
    void setTheme(const Md3Theme &theme);
    // 将内容控件挂载到卡片内（16px 内边距）
    void setContent(QWidget *content);

    // 顶部图片区：传入 pixmap 后卡片顶部展示图片（全出血大图），
    // 标题/正文自动下移；空 pixmap 移除图片区
    void setImage(const QPixmap &pixmap);
    // 图片区圆角（默认上圆角与卡片圆角同源 12px，仅顶部圆角）
    void setImageRadius(qreal radius);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QColor backgroundColor() const;
    QGraphicsDropShadowEffect *shadowEffect() const;
    // 更新图片区几何与内容偏移
    void updateImageLayout();

    Md3Theme theme_;
    Style style_ = Style::Elevated;
    bool hovered_ = false;
    // hover 状态层透明度（0~1），由动画驱动
    qreal hoverAlpha_ = 0.0;
    QVBoxLayout *contentLayout_ = nullptr;
    // hover 状态层淡入淡出动画
    QVariantAnimation hoverAnim_;

    QLabel *imageLabel_ = nullptr;
    qreal imageRadius_ = 0.0;   // 0 = 未启用图片区
};
