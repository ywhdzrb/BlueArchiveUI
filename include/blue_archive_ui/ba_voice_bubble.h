#pragma once

#include <QString>
#include <QWidget>

// BA 对话气泡（图2/3/4 左下角）：白色圆角大板 + 浅蓝描边 + 右下小尾巴 + 深蓝文字。
// setText 多行自动换行（WordWrap）。
class BaVoiceBubble : public QWidget
{
    Q_OBJECT

public:
    explicit BaVoiceBubble(const QString &text, QWidget *parent = nullptr);

    void setText(const QString &text);
    void setSpeaker(const QString &name);  // 可选：气泡上方名字小条（贝吉塔名条式）

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString text_{};
    QString speaker_{};
};
