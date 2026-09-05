#pragma once

#include <QString>
#include <QWidget>

// BA 商店左侧菜单块（图5）：深蓝渐变斜切块 + 左端橙黄三角角标 + 白粗体文字。
// 块与块之间做错位排列（宿主用垂直叠放 + 手动偏移即可）。
class BaShopMenuBlock : public QWidget
{
    Q_OBJECT

public:
    explicit BaShopMenuBlock(const QString &text, QWidget *parent = nullptr);

    void setSelected(bool selected);
    bool isSelected() const { return selected_; }

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString text_{};
    bool selected_ = false;
};
