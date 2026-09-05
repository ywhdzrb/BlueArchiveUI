#pragma once

#include <QString>
#include <QWidget>

// BA 悬赏任务卡（图3）：右侧深蓝渐变标题窄条 + 左上部活动进行中红/橙标签 +
// 底部白胶囊「持有跳战券 n/n」+ 深蓝描边白卡主体（描述区由宿主布局填充）。
// 尺寸建议：宽 86%，高自适应（本控件只绘制骨架，内容用 hostLayout 嵌填）。
class BaBountyCard : public QWidget
{
    Q_OBJECT

public:
    explicit BaBountyCard(const QString &title = QString(),
                          const QString &ticketText = QString(),
                          QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setTicketText(const QString &text);   // 例如 "持有跳战券 7/2"
    void setTag(const QString &text);          // 例如 "活動進行中"（橙红标签）
    void setDescription(const QString &text);  // 例如 "可以获得技能升级所需的材料。"

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString title_{};
    QString tag_{};
    QString description_{};
    QString ticketText_{};
};
