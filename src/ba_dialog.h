#pragma once

#include <QString>
#include <QVBoxLayout>
#include <QWidget>

// BA 模态对话框：黑色遮罩 + 白色圆角面板。
// 面板：标题（深蓝粗体 + 3px 黄色下划线）+ 内容区 + 底部梯形收窄区。
// 播放从底部滑入（0.4s 过冲弹性），点击遮罩或右上红圆按钮关闭。
class BaDialog : public QWidget
{
    Q_OBJECT

public:
    explicit BaDialog(const QString &title, QWidget *parent = nullptr);

    void setTitle(const QString &title);
    QString title() const { return title_; }

    // 面板内容区（默认垂直布局，中空留白）
    QVBoxLayout *contentLayout() const { return contentLayout_; }

    // 面板宽度（默认 520）
    void setPanelWidth(int w);
    int panelWidth() const { return panelWidth_; }

    // 覆盖顶层 show 语义：显示为模态（parent 被遮罩）
    void showAsModal();
    void closeDialog();

signals:
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QRectF panelRect() const;
    QRectF closeButtonRect() const;

    QString title_;
    int panelWidth_ = 520;
    QVBoxLayout *contentLayout_ = nullptr;
    QWidget *panel_ = nullptr;
};
