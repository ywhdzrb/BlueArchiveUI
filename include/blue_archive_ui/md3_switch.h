#pragma once

#include <QAbstractButton>
#include <QVariantAnimation>
#include "md3_theme.h"

// MD3 开关：52x32 track + 动画 thumb。
// 未选中：track 用 surface-container-highest，边框 outline；
// 选中：track 变 primary，thumb 放大并右移，支持 200ms 过渡动画。
class Md3Switch : public QAbstractButton
{
    Q_OBJECT

public:
    explicit Md3Switch(QWidget *parent = nullptr);

    void setTheme(const Md3Theme &theme);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void animateTo(bool checked);

    Md3Theme theme_;
    qreal progress_ = 0.0;   // 0 = 关闭，1 = 开启，供插值使用
    QVariantAnimation anim_;
};
