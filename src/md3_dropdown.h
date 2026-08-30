#pragma once

#include <QWidget>
#include <QStringList>
#include <QVariantAnimation>
#include "md3_theme.h"

// 前置声明，供弹出菜单窗口引用宿主控件
class Md3Dropdown;

// MD3 弹出菜单窗口：surface 背景、圆角 4px、层级阴影（Elevation Level 2 等效）。
// 选项 hover 时 on-surface 8% 高亮，选中项 primary 加粗并带对勾标记。
class Md3MenuPopup : public QWidget
{
    Q_OBJECT

public:
    explicit Md3MenuPopup(Md3Dropdown *owner);

    void setTheme(const Md3Theme &theme);
    void setItems(const QStringList &items);
    void setSelectedIndex(int index);
    // 依据宿主宽度与选项数量计算弹出窗口尺寸
    QSize popupSize() const;

signals:
    // 用户点击了第 index 项
    void itemClicked(int index);
    // 菜单被收起（点击外部自动隐藏时用于同步宿主状态）
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    // 返回坐标对应的选项索引，超出范围返回 -1
    int itemAt(const QPoint &pos) const;

    Md3Dropdown *owner_ = nullptr;
    Md3Theme theme_;
    QStringList items_;
    int selectedIndex_ = -1;
    int hoverIndex_ = -1;
};

// MD3 填充式下拉选择框（Exposed Dropdown）：
// 顶部圆角 4px、背景 surface-container-highest，与文本输入框同风格；
// 右侧下拉箭头，hover / 展开时指示线与箭头平滑过渡为 primary。
// 点击弹出菜单，支持占位提示与选中反馈。
class Md3Dropdown : public QWidget
{
    Q_OBJECT

public:
    explicit Md3Dropdown(const QStringList &items = QStringList(),
                         const QString &placeholder = QString(),
                         QWidget *parent = nullptr);

    void setTheme(const Md3Theme &theme);

    void setItems(const QStringList &items);
    void setPlaceholderText(const QString &placeholder);

    int currentIndex() const { return currentIndex_; }
    void setCurrentIndex(int index);
    // 当前选中项文本，未选中返回空串
    QString currentText() const;

    QSize sizeHint() const override;

signals:
    // 选中项变化时发出（index 从 0 开始，-1 表示无选中）
    void currentIndexChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void animateActive(bool active);
    void showPopup();
    void onItemClicked(int index);
    void onPopupClosed();

    Md3Theme theme_;
    QStringList items_;
    QString placeholder_;
    int currentIndex_ = -1;

    bool hovered_ = false;
    bool menuOpen_ = false;
    qreal activeProgress_ = 0.0;   // hover 或菜单展开的过渡进度，驱动指示线 / 箭头 / 状态层
    QVariantAnimation anim_;
    Md3MenuPopup *popup_ = nullptr;
};
