#pragma once

#include <QWidget>
#include <QVariantAnimation>
#include "md3_theme.h"

class QGraphicsDropShadowEffect;
class QVBoxLayout;

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

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QColor backgroundColor() const;
    QGraphicsDropShadowEffect *shadowEffect() const;

    Md3Theme theme_;
    Style style_ = Style::Elevated;
    bool hovered_ = false;
    // hover 状态层透明度（0~1），由动画驱动
    qreal hoverAlpha_ = 0.0;
    QVBoxLayout *contentLayout_ = nullptr;
    // hover 状态层淡入淡出动画
    QVariantAnimation hoverAnim_;
};
