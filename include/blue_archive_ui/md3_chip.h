#pragma once

#include <QAbstractButton>
#include <QVariantAnimation>
#include "md3_theme.h"
#include "md3_icon.h"

// MD3 芯片（Chip）：紧凑的交互式标签，四种变体共用一组几何。
//   Assist / Suggestion：带 icon 的辅助芯片，无选中态
//   Filter：可勾选的筛选芯片，checked 时显示对勾 + secondary-container 底色
//   Input：带 icon 的输入芯片
// 高度 32px 全圆角胶囊，选中态（Filter）显示 primary 边框 + 对勾图标，
// 悬停 / 按下有状态层，支持禁用。
class Md3Chip : public QAbstractButton
{
    Q_OBJECT

public:
    enum class Style {
        Assist,      // 多选辅助：primary outline，icon + 文本，不可勾选
        Filter,      // 筛选：可勾选，选中时 secondary-container 底 + 对勾
        Input,       // 输入：primary outline，icon + 文本，可删除
        Suggestion   // 建议：secondary outline，图标 + 文本，不可勾选
    };
    Q_ENUM(Style)

    explicit Md3Chip(const QString &text, QWidget *parent = nullptr);

    void setStyle(Style style);
    void setIcon(md3::Glyph glyph);           // 辅助 / 输入 / 建议芯片的前置图标
    void setIconVisible(bool visible);
    void setShowDelete(bool showDelete);      // Input 芯片右侧删除符号
    void setTheme(const Md3Theme &theme);
    void setChecked(bool checked);            // 仅 Filter 生效

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void deleteClicked();                     // 点击删除符号（Input 芯片）

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
    Style style_ = Style::Filter;
    md3::Glyph glyph_ = md3::Glyph::Check;
    bool iconVisible_ = false;
    bool showDelete_ = false;
    bool hovered_ = false;
    qreal stateAlpha_ = 0.0;
    QVariantAnimation stateAnim_;

    static constexpr qreal kHeight = 32.0;     // 芯片高度
    static constexpr qreal kStroke = 1.5;      // 边框解析度
    static constexpr qreal kIconGap = 8.0;     // 图标与文本间距
    static constexpr qreal kTextGap = 10.0;    // 文本与边缘间距
    static constexpr qreal kHoverAlpha = 10.0; // hover 状态层透明度
};
