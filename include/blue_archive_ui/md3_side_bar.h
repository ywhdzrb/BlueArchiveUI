#pragma once

#include <QWidget>
#include <QVector>
#include "md3_theme.h"
#include "md3_icon.h"

// MD3 侧边导航栏（Navigation Rail）：固定在窗口左侧的窄竖条。
// 图标 + 标签的导航项，选中项以 primary-container 胶囊指示器高亮，
// 图标 / 标签过渡为 primary；悬停时显示 on-surface 状态层。
class Md3SideBar : public QWidget
{
    Q_OBJECT

public:
    // 图标枚举直接取公共图标库（md3_icon.h），避免私有副本漂移
    using Glyph = md3::Glyph;

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
    struct Item {
        QString label;
        Glyph glyph = Glyph::Home;
    };

    Md3Theme theme_;
    QVector<Item> items_;
    int currentIndex_ = -1;
    int hoverIndex_ = -1;
};
