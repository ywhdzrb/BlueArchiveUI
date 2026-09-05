#pragma once

#include <QAbstractButton>

#include "ba_anim.h"

// BA 风格复选框（静音框形态）：横长圆角矩形（26x20，圆角 5）+ 白底浅蓝描边；
// 选中时描边变青蓝、内部淡入青蓝实心圆。带 0.15s 的淡入动画（帧驱动）。
class BaCheckBox : public QAbstractButton
{
    Q_OBJECT

public:
    explicit BaCheckBox(const QString &text, QWidget *parent = nullptr);

    void setCheckedAnimated(bool checked);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool hovered_ = false;
    qreal checkAni_ = 0.0;
    ba::Animator checkAnim_;  // 勾生长补间器（帧驱动）
};
