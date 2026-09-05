#pragma once

#include <QPainter>
#include <QString>
#include <QWidget>

#include "ba_icon.h"

// BA 顶栏（对照任务页截图复刻）：
// - Page 模式：深蓝渐变圆返回钮 + 白玻条标题（深蓝粗体白描边）+ 右侧白胶囊资源区
//   （AP 绿字 / 金币金字 / 青辉石浅黄字 + / 白色细竖分隔线）+ 白片加号/齿轮/主页钮。
// - Hall 模式：左上角深蓝渐变玩家卡（Lv 白圆徽 + 白名 + 进度小字），无返回钮与标题。
// 顶栏为白色玻璃横条（约 92% 不透明），底部向下渐隐。
class BaTopBar : public QWidget
{
    Q_OBJECT

public:
    enum class Mode { Page, Hall };  // 页面态：返回钮+标题；大厅态：左上玩家卡

    explicit BaTopBar(const QString &title, QWidget *parent = nullptr);

    void setMode(Mode mode);
    Mode mode() const { return mode_; }

    void setTitle(const QString &title);
    QString title() const { return title_; }

    // 深色标题条（标题白字蓝描边，如悬赏通缉页）
    void setTitleDark(bool dark);

    // 三项钱包资源数值文本
    void setWallet(const QString &apText, const QString &coinText, const QString &gemText);
    void setWalletVisible(bool on);

    // 大厅态玩家卡（subText 为空则不显示进度小字）
    void setPlayer(const QString &name, int level, const QString &subText);

signals:
    void backClicked();
    void plusClicked();
    void gearClicked();
    void homeClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // 各元素几何
    QRectF backButtonRect() const;   // 深蓝渐变圆返回钮
    QRectF playerCardRect() const;   // 大厅态玩家卡
    QRectF pillRect() const;         // 右侧白胶囊资源区
    QRectF plusRect() const;         // 白片加号钮
    QRectF gearRect() const;         // 白片齿轮钮
    QRectF homeRect() const;         // 白片主页钮
    void drawCircleButton(QPainter &p, const QRectF &r, ba::Glyph glyph, const QColor &tint);

    Mode mode_ = Mode::Page;
    QString title_;
    bool titleDark_ = false;
    QString apText_ = "152/152";
    QString coinText_ = "109,195,252";
    QString gemText_ = "779";
    bool walletVisible_ = true;
    QString playerName_ = "貝吉塔王子";
    int playerLevel_ = 25;
    QString playerSub_ = "132/380";
    bool playerKnown_ = false;       // 是否由 setPlayer 显式设置过
};
