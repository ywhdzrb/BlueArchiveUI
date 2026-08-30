#pragma once

#include <QWidget>
#include <QVariantAnimation>
#include "md3_theme.h"

// MD3 线性进度条：4px 高胶囊条。
// 确定模式（determinate）：指示条按 value/range 比例填充 primary；
// 不确定模式（indeterminate）：指示条循环滑动，适用于未知耗时任务。
// track 统一用 surface-container-highest，指示条用 primary。
class Md3ProgressBar : public QWidget
{
    Q_OBJECT

public:
    explicit Md3ProgressBar(QWidget *parent = nullptr);

    void setTheme(const Md3Theme &theme);

    // 确定模式取值范围，默认 0~100
    void setRange(int min, int max);
    void setValue(int value);
    int value() const;

    // 切换不确定模式：true 时启动循环滑动动画
    void setIndeterminate(bool indeterminate);
    bool isIndeterminate() const;

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Md3Theme theme_;
    int min_ = 0;
    int max_ = 100;
    int value_ = 0;
    bool indeterminate_ = false;
    qreal slide_ = 0.0;          // 不确定模式下滑动位置，0~1
    QVariantAnimation anim_;
};
