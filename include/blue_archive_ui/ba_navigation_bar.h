#pragma once

#include <QStringList>
#include <QWidget>

#include "ba_icon.h"

// BA 底部导航（对照大厅截图）：
// 白色半透明胶囊长条，8 项等宽排布；未选中＝浅蓝线稿图标 + 深蓝文字；
// 选中＝深蓝渐变圆角块 + 浅蓝描边 + 白色图标文字（块略微高出胶囊顶）。
class BaNavigationBar : public QWidget
{
    Q_OBJECT

public:
    explicit BaNavigationBar(QWidget *parent = nullptr);

    void addItem(const QString &text, ba::Glyph glyph);
    void setCurrentIndex(int index);
    int currentIndex() const { return current_; }

    QSize sizeHint() const override;

signals:
    void currentChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QRectF itemRect(int index) const;  // 第 index 项所占矩形（含内侧边距）

    QStringList items_;
    QVector<ba::Glyph> glyphs_;
    int current_ = 0;
};
