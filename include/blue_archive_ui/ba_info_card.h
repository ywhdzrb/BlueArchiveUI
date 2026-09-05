#pragma once

#include <QString>
#include <QWidget>
#include <vector>

// 账号信息页卡片（真机「稱呼」「簡介設定」形态）：
// 顶部深蓝渐变丝带标题条（白粗体居中，两端 45° 斜切）压在白卡圆角 8 上，
// 卡身白底 + 浅蓝描边；内容为 label + value + 尾部工具钮（铅笔/喇叭）的行模型。
// 行间以 1px 浅灰分隔线分隔。
class BaInfoCard : public QWidget
{
    Q_OBJECT

public:
    // 行尾部工具钮类型
    enum class Trailing {
        None,       // 无工具钮
        Edit,       // 白片 + 深蓝铅笔（编辑）
        Speaker     // 白片 + 带环喇叭（语音试听）
    };

    explicit BaInfoCard(const QString &ribbonText, QWidget *parent = nullptr);

    void setRibbonText(const QString &t);
    void addRow(const QString &label, const QString &value,
                Trailing trailing = Trailing::Edit);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // 单行数据
    struct Row {
        QString label;
        QString value;
        Trailing trailing = Trailing::None;
    };

    QRectF ribbonRect() const;   // 标题丝带区（骑在卡顶）
    QRectF buttonRectFor(const Row &row, qreal rowY) const;   // 行尾工具钮

    QString ribbonText_;
    std::vector<Row> rows_;
};
