#pragma once

#include <QString>
#include <QWidget>

// BA「数值黑晶片」：深蓝黑梯形片（左端 30° 斜切 + 圆角 4），白色数值文字。
// 出现于商店/奖励面板底部（如「×10000」、奖体力「30」、青辉石数量）。
class BaChip : public QWidget
{
    Q_OBJECT

public:
    explicit BaChip(const QString &text, QWidget *parent = nullptr);

    void setText(const QString &text);
    QString text() const { return text_; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString text_;
};
