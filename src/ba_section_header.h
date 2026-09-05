#pragma once

#include <QWidget>

// BA 内容区小标题：左侧 3px 圆角青蓝竖条 + 深藏蓝粗体文字，
// 底部贯穿一条 1px 虚线（#C9D8E2）。可选拖尾控件（置右对齐）。
class BaSectionHeader : public QWidget
{
    Q_OBJECT

public:
    explicit BaSectionHeader(const QString &title, QWidget *parent = nullptr);

    void setTitle(const QString &title);
    QString title() const { return title_; }

    // 在标题右侧追加一个控件（如“新增”按钮），自动布局右对齐
    void setTrailingWidget(QWidget *w);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QString title_;
    QWidget *trailing_ = nullptr;
};
