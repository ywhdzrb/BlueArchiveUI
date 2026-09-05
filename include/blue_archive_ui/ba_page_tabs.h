#pragma once

#include <QStringList>
#include <QWidget>

// BA 任务页页签条：白色圆角药丸长条 + 居中均分页签。
// 选中项 = 橙渐变立体块 + 白粗体字，顶部有小三角红旗装饰（图4 任务成就页式样）。
class BaPageTabs : public QWidget
{
    Q_OBJECT

public:
    explicit BaPageTabs(QWidget *parent = nullptr);

    void addItem(const QString &text);
    int currentIndex() const { return current_; }
    void setCurrentIndex(int index);

    QSize sizeHint() const override;

signals:
    void currentChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QRectF itemRect(int index) const;
    int itemIndexAt(int x) const;

    QStringList items_;
    int current_ = 0;
    QRectF capsule_ = QRectF();  // 当前整条置中后的矩形（paint 中计算）
};
