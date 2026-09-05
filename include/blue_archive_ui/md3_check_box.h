#pragma once

#include <QAbstractButton>
#include <QVariantAnimation>
#include "md3_theme.h"

// MD3 复选框：18x18 圆角框，选中为 primary 底 + on-primary 对勾，
// 未选中为 2px outline 描边，支持部分选中（partially-checked）菱形相位。
// 单击切换；悬停 / 聚焦显示状态层。
class Md3CheckBox : public QAbstractButton
{
    Q_OBJECT

public:
    explicit Md3CheckBox(QWidget *parent = nullptr);

    void setTheme(const Md3Theme &theme);
    // 部分选中态：勾不显示，显示 on-primary 小圆点
    void setPartially(bool partially);
    void setText(const QString &text);   // 可选右侧标签

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
    bool partially_ = false;
    bool hovered_ = false;
    qreal stateAlpha_ = 0.0;   // 状态层当前透明度（百分比）
    QVariantAnimation stateAnim_;

    static constexpr qreal kSize = 18.0;      // 勾选框边长
    static constexpr qreal kHoverAlpha = 10.0; // hover 状态层透明度
};
