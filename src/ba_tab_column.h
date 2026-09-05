#pragma once

#include <QStringList>
#include <QVariantAnimation>
#include <QWidget>

// BA 侧栏 tab 列，支持两种形态（对应游戏内不同页面）：
// - Style::Pill：设置页浅蓝整列 + 白色选中板 + 虚线分隔
// - Style::Card：商店页白色圆角卡片列，选中项为深蓝渐变块 + 黄下划线 + 左上角浅蓝三角装饰
class BaTabColumn : public QWidget
{
    Q_OBJECT

public:
    enum class Style { Pill, Card };

    explicit BaTabColumn(QWidget *parent = nullptr);

    void setStyle(Style s);
    void addItem(const QString &text, bool selected = false);
    // 可配图标（BaIcon glyph），无则仅文字
    void setItemIcon(int index, int glyph);
    int currentIndex() const { return current_; }

    QSize sizeHint() const override;

signals:
    void currentChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QRectF itemRect(int index) const;  // 第 index 项所占矩形
    int hoverIndex() const;            // 当前鼠标悬停项（-1 无）

    QStringList items_;
    Style style_ = Style::Pill;
    int current_ = -1;
    int hover_ = -1;
    qreal hoverProgress_ = 0.0;    // hover 高亮进度动画
    QVariantAnimation hoverAnim_;
    int icons_[6];                 // 简单存 glyph int（-1 表示无）
    int iconCount_ = 0;
};
