#pragma once

#include <QWidget>
#include <QVariantAnimation>
#include "md3_theme.h"

// MD3 2024 版滑块：16px 胶囊轨道（inactive=secondaryContainer、active=primary）
// + 6px gap 分段 + 44px 高圆角竖条 thumb + 右端 stop indicator。
// 支持鼠标点击/拖动取值，press/focus 时 thumb 变细。
// thumb 位置带平滑动画：点击轨道/程序设值时 200ms 滑动过渡，拖动时以 100ms 短动画平滑跟随。
class Md3Slider : public QWidget
{
    Q_OBJECT

public:
    explicit Md3Slider(QWidget *parent = nullptr);

    void setTheme(const Md3Theme &theme);

    void setRange(int min, int max);
    void setValue(int value);
    int value() const { return value_; }

    QSize sizeHint() const override;

signals:
    void valueChanged(int value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setValueFromPos(int x);

    Md3Theme theme_;
    int min_ = 0;
    int max_ = 100;
    int value_ = 50;
    // 绘制使用的平滑插值值，动画驱动；拖动时与 value_ 同步
    qreal displayValue_ = 50.0;
    bool pressed_ = false;
    // thumb 移动动画：点击轨道 / setValue 时平滑滑动
    QVariantAnimation anim_;
};
