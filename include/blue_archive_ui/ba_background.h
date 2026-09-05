#pragma once

#include <QPixmap>
#include <QString>
#include <QWidget>

// BA 大厅背景：默认 QPainter 手绘天空（渐变+云+山丘）。
// 若通过 setImagePath 指定官方提取图（BaAssets 素材），则按覆盖裁切铺满，
// 手绘层作为加载失败时的回退。
class BaBackground : public QWidget
{
    Q_OBJECT

public:
    explicit BaBackground(QWidget *parent = nullptr);

    // 指定素材相对路径（如 "img/bg/bg_office.jpg"），存在时优先绘制图片。
    void setImagePath(const QString &relPath);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawCloud(QPainter &p, const QPointF &center, qreal scale, qreal alpha);

    QString imagePath_;   // 素材相对路径（空表示手绘模式）
    QPixmap image_;       // 惰性加载缓存
};
