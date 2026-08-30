#pragma once

#include <QAbstractButton>
#include <QVariantAnimation>
#include "md3_theme.h"

// MD3 按钮：覆盖 Filled / FilledTonal / Outlined / Text 四种规格。
// 高度 40px，全圆角，带状态层（hover / 按下 / 聚焦）动效，支持禁用态。
class Md3Button : public QAbstractButton
{
    Q_OBJECT

public:
    // 按钮规格，对应 MD3 规范中的四种变体
    enum class Style {
        Filled,        // 实心：primary 背景
        FilledTonal,   // 色调实心：secondary-container 背景
        Outlined,      // 描边：透明背景 + 1px 轮廓
        Text           // 文本：无背景无边框
    };
    Q_ENUM(Style)

    explicit Md3Button(const QString &text, QWidget *parent = nullptr);

    void setStyle(Style style);
    void setTheme(const Md3Theme &theme);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    // 计算当前应呈现的状态层透明度百分比（0~12），并驱动动画趋近
    qreal targetStateAlpha() const;
    // 各规格的背景色 / 文字色 / 状态层基色
    QColor backgroundColor() const;
    QColor contentColor() const;
    QColor stateLayerColor() const;

    Md3Theme theme_;
    Style style_ = Style::Filled;
    bool hovered_ = false;
    bool focused_ = false;
    qreal stateAlpha_ = 0.0;   // 状态层当前透明度（0~12 的百分比值）
    QVariantAnimation stateAnim_;
};
