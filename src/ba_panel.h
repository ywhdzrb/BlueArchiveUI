#pragma once

#include <QString>
#include <QVBoxLayout>
#include <QWidget>

// BA 白色内容卡片（商店页/奖励面板形态）：
// 白底 + 圆角 12 + 深蓝细描边 1.2px，顶部可选浅蓝信息区（setHeaderTitle 显示
// 深蓝格名文字），其余部分为布局区（contentLayout）。hover 时描边加深并轻微上浮。
class BaPanel : public QWidget
{
    Q_OBJECT

public:
    explicit BaPanel(QWidget *parent = nullptr);

    // 顶部浅蓝信息区标题（不设置则无顶部区）
    void setHeaderTitle(const QString &title);
    QString headerTitle() const { return headerTitle_; }

    QVBoxLayout *contentLayout() const { return contentLayout_; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QRectF headerRect() const;

    QString headerTitle_;
    QVBoxLayout *contentLayout_ = nullptr;
    bool hovered_ = false;
};
