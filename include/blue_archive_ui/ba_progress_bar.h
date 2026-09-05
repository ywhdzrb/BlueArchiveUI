#pragma once

#include <QProgressBar>

#include "ba_anim.h"

// BA 风格进度条：圆角轨道 + 官方亮青饱和填充（顶白高光线），
// 轨道色可换（默认浅蓝白 Daily 登录样式；成就/任务条用深黑灰 setTrackColor）。
// 数值变化 0.18s 平滑过渡；轨道色可换。
class BaProgressBar : public QProgressBar
{
    Q_OBJECT

public:
    explicit BaProgressBar(QWidget *parent = nullptr);

    // 平滑过渡是否开启（默认开）
    void setAnimated(bool on);
    // 是否绘制 BA 特色的右上角格位标记（默认画）
    // 轨道底色（默认浅蓝白 #D8E9FA；任务卡用深黑灰）
    void setTrackColor(const QColor &c);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    qreal shownValue_ = 0.0;          // 当前展示值（动画联动）
    ba::Animator valueAnim_;          // 数值蠕动补间器（帧驱动，不依赖 Qt 动画框架）
    bool animate_ = true;
    bool markVisible_ = true;
    QColor trackColor_ = QColor("#D8E9FA");  // Default: Daily 登录条 浅蓝底
};
