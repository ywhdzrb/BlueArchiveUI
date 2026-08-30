#pragma once

#include <QWidget>
#include <QVector>
#include "md3_theme.h"

class QPainter;

// MD3 侧边导航栏（Navigation Rail）：固定在窗口左侧的窄竖条。
// 图标 + 标签的导航项，选中项以 primary-container 胶囊指示器高亮，
// 图标 / 标签过渡为 primary；悬停时显示 on-surface 状态层。
class Md3SideBar : public QWidget
{
    Q_OBJECT

public:
    // 内置线性图标，避免外部图标资源依赖
    enum class Glyph {
        Home,     // 首页
        Search,   // 搜索
        Star,     // 收藏
        Person,   // 我的
        Palette,  // 调色板
        Drop,     // 水滴（液态玻璃）
    };
    Q_ENUM(Glyph)

    explicit Md3SideBar(QWidget *parent = nullptr);

    void setTheme(const Md3Theme &theme);

    // 追加一个导航项，返回其索引
    int addItem(const QString &label, Glyph glyph);
    // 移除全部导航项
    void clearItems();

    int currentIndex() const { return currentIndex_; }
    void setCurrentIndex(int index);

    QSize sizeHint() const override;

signals:
    void itemSelected(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    // 在 (cx, cy) 处绘制指定线性图标（24x24 基准，居中于该点）
    void paintGlyph(QPainter &p, Glyph glyph, qreal cx, qreal cy, const QColor &color) const;

    struct Item {
        QString label;
        Glyph glyph = Glyph::Home;
    };

    Md3Theme theme_;
    QVector<Item> items_;
    int currentIndex_ = -1;
    int hoverIndex_ = -1;
};
