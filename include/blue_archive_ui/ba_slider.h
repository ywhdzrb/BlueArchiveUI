#pragma once

#include <QAbstractSlider>

#include "ba_anim.h"

// BA 风格水平滑块：浅灰圆角轨道 + 浅青填充段；
// thumb 为青蓝实心小圆点，嵌在略大的半透明灰圈内（游戏内音量滑块形态）。
// 两端可绘青色渐变喇叭图标（默认开，setVolumeIcons 关闭）。
// 点击时 thumb 从当前位置平滑飞向目标（帧驱动补间）。
class BaSlider : public QAbstractSlider
{
    Q_OBJECT

public:
    explicit BaSlider(QWidget *parent = nullptr);

    void setVolumeIcons(bool on);   // 两端喇叭图标开关（BGM/SE 行默认开）

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void applyValueFromPos(int x);
    qreal thumbStart() const;   // thumb 可移动域起点（与轨道绘制域一致）
    qreal thumbSpan() const;    // thumb 可移动域宽度
    qreal thumbCenter() const;  // 当前 thumb 中心坐标（始终按值映射，跟手）

    bool volumeIcons_ = true;
    bool dragging_ = false;
    qreal thumbAni_ = 0.0;  // thumb 中心 x 的动画值（从按下处平滑趋近目标）
    ba::Animator thumbAnim_;  // thumb 飞行补间器（帧驱动）
    bool hovered_ = false;
};
