#pragma once

#include <QString>
#include <QWidget>

// BA 成就/任务卡（对照任务成就页截图）：
// 白卡圆角 + 深蓝描边；左上橙渐变「成就」小标签；深蓝粗体标题；
// 「次数 n/m」灰字 + 黑灰圆头进度条（子控件 BaProgressBar）；
// 右侧奖励区：白框奖励盒（青辉石 + ×n）+ 青蓝渐变「立即前往」小按钮。
class BaMissionCard : public QWidget
{
    Q_OBJECT

public:
    explicit BaMissionCard(const QString &title, QWidget *parent = nullptr);

    void setTag(const QString &tag);            // 左上角标签文字（默认「成就」）
    void setProgressText(const QString &text);  // 「次数 0/1」样式字符串
    void setProgressValue(int value, int maximum = 1);  // 驱动内部进度条
    void setRewardCount(int count);             // 奖励 ×n
    void setActionText(const QString &text);    // 右侧按钮文字（默认「立即前往」）

    QSize sizeHint() const override;

signals:
    void actionClicked();                       // 右侧按钮按下

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QString tag_;
    QString title_;
    QString progressText_;
    int rewardCount_ = 0;
    QString actionText_;
};
