#pragma once

#include <QAbstractButton>
#include <QVariantAnimation>
#include "md3_theme.h"
#include "md3_icon.h"

// MD3 浮动按钮（Floating Action Button）：悬浮于内容之上的主操作。
// 支持三种规格：Regular 56x56 / Small 40x40 / Large 96x96（带图标 + 文字）。
// 层级颜色：primary 底 + on-primary 图标，阴影 Level 3，hover 状态层 8%，
// 可切换 tonal 规格（secondary-container 色调）。
class Md3Fab : public QAbstractButton
{
    Q_OBJECT

public:
    enum class Size {
        Regular,   // 56x56，默认
        Small,     // 40x40，小 FAB
        Large      // 96x96，扩展 FAB（图标 + 文字）
    };
    Q_ENUM(Size)

    explicit Md3Fab(md3::Glyph glyph = md3::Glyph::Plus, QWidget *parent = nullptr);

    void setGlyph(md3::Glyph glyph);
    void setSize(Size size);
    void setTonal(bool tonal);            // true 用 secondary-container 色调
    void setText(const QString &text);    // 仅 Large 显示
    void setTheme(const Md3Theme &theme);

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
    md3::Glyph glyph_ = md3::Glyph::Plus;
    Size size_ = Size::Regular;
    bool tonal_ = false;
    QString text_;
    bool hovered_ = false;
    qreal stateAlpha_ = 0.0;   // 状态层当前透明度（百分比）
    QVariantAnimation stateAnim_;

    static constexpr qreal kHoverAlpha = 12.0;  // hover 状态层透明度
};
