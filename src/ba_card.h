// 基础卡片：白 -16° 平行四边形卡体 + 深蓝描边 + 圆角 + 投影
// 内容行沿斜边平行等距对齐（addRow 手动行式布局，行左缘距斜边恒等 20px）
// 若用户需要自由布局，可用 contentLayout()（但不会随斜边偏移）

#pragma once

#include <QWidget>
#include <QVector>

class QVBoxLayout;

class BaCard : public QWidget
{
    Q_OBJECT

public:
    explicit BaCard(QWidget *parent = nullptr);

    // 内容布局（基础边距，不随斜边偏移）
    QVBoxLayout *contentLayout() const;

    // 手动行式布局：行沿左斜边等距对齐，行高按 widget sizeHint
    void addRow(QWidget *widget);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // 重排所有 addRow 行：x = margin + tan(斜切角)*(卡中心y - 行中心y)（行与斜边平行）
    void layoutRows();

    QVBoxLayout *content_;
    QVector<QWidget *> rows_;
};
