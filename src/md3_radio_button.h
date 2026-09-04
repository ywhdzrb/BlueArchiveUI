#pragma once

#include <QAbstractButton>
#include <QVariantAnimation>
#include "md3_theme.h"

// MD3 单选按钮：18x18 圆环，选中为 primary 描边环 + on-primary 中心圆点，
// 未选中为 2px outline 描边环，悬停 / 聚焦显示状态层，带右侧标签。
class Md3RadioButton : public QAbstractButton
{
    Q_OBJECT

public:
    explicit Md3RadioButton(QWidget *parent = nullptr);

    void setTheme(const Md3Theme &theme);
    void setText(const QString &text);   // 可选右侧标签
    void setLabelColor(const QColor &color);   // 可选自定义标签色

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    qreal targetStateAlpha() const
    {
        return (hovered_ || isDown()) ? kHoverAlpha : 0.0;
    }

    Md3Theme theme_;
    QString text_;
    QColor labelColor_;
    bool hovered_ = false;
    qreal stateAlpha_ = 0.0;   // 状态层当前透明度（百分比）
    QVariantAnimation stateAnim_;

    static constexpr qreal kSize = 18.0;      // 圆环直径
    static constexpr qreal kHoverAlpha = 10.0; // hover 状态层透明度
};
