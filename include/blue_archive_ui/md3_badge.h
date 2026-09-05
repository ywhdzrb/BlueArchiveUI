#pragma once

#include <QWidget>
#include "md3_theme.h"

// MD3 徽章（Badge）：附着于其他控件右上角的小圆点 / 数字气泡。
// 常规尺寸 16px 圆，带 99+ 溢出显示；支持用 setCount(0) 隐藏。
// 挂载方式：setParent(宿主) 后调用 attachTo(宿主控件)，自动跟随宿主移动
// （宿主 resize / move 时经 event filter 自动重定位到右上角）。
class Md3Badge : public QWidget
{
    Q_OBJECT

public:
    explicit Md3Badge(QWidget *parent = nullptr);

    // 角标内容：count > 99 显示 99+；count <= 0 隐藏
    void setCount(int count);
    int count() const { return count_; }

    // 切换为无数字小圆点模式（dot badge），或恢复数字徽章
    void setDot(bool dot);

    void setTheme(const Md3Theme &theme);

    // 挂到宿主控件右上角（角标为宿主子控件，覆盖在宿主之上）
    void attachTo(QWidget *host);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;   // 监听宿主移动 / 缩放

private:
    // 重新定位到宿主控件右上角。
    // 偏移量 = 角标尺寸的一半减去 2px 收进宿主圆角（仅正则徽章；小圆点全凸出）
    void reposition();

    Md3Theme theme_;
    QWidget *host_ = nullptr;
    int count_ = 0;       // 当前显示数字，0 为隐藏
    bool dot_ = false;    // true 为无数字小圆点模式
};
