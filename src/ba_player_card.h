#pragma once

#include <QString>
#include <QWidget>

// 账号信息页玩家卡（真机 Lv.35 卡形态）：
// 深蓝渐变板（左端大斜切剪角）+ 黄「Lv.」小标 + 白色大数字 + 白色粗体名称 +
// 亮青头圆进度条（青蓝亮条）+ 数值小字（444/982）+ 右上铅笔圆钮。
class BaPlayerCard : public QWidget
{
    Q_OBJECT

public:
    explicit BaPlayerCard(QWidget *parent = nullptr);

    void setLevel(int lv);
    void setNameText(const QString &name);          // 白色粗体名
    void setProgress(int value, int max);           // 进度条显示值
    void setSubText(const QString &text);           // 进度数值小字（默认空则自动 "v/m"）
    void setEditVisible(bool visible);              // 右上铅笔圆钮

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int level_ = 35;
    QString name_ = QStringLiteral("老师");
    int value_ = 0;
    int max_ = 1;
    QString subText_;
    bool editVisible_ = true;
};
