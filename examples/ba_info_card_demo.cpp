#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "ba_assets.h"
#include "ba_background.h"
#include "ba_info_card.h"
#include "ba_icon.h"
#include "ba_player_card.h"
#include "ba_style.h"

// —— 预览页：账号信息页（帳號資訊）卡片效果还原 ——

// 進修中信息条：深蓝渐变 + 白图标 + 白字两行
class BoostNotice : public QWidget
{
public:
    BoostNotice(QWidget *parent = nullptr) : QWidget(parent) { setMinimumSize(300, 56); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r(0, 0, width(), height());
        p.setPen(Qt::NoPen);
        p.setBrush(BaStyle::ribbonGradient(QRectF(0, 0, width(), height() / 2)));
        p.drawRoundedRect(r, 6, 6);

        // 白框小图标（進修中 badge：圆+绿芽示意）
        p.setPen(QPen(Qt::white, 1.4));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(10, 10, 22, 22));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xA8, 0xE4, 0x8B));
        p.drawEllipse(QPointF(21, 21), 5.0, 5.0);

        // 两行白色文字
        p.setPen(Qt::white);
        p.setFont(BaStyle::font(9, QFont::DemiBold));
        p.drawText(QRectF(42, 4, width() - 50, 20), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("進修中"));
        p.setPen(QColor(0xD8, 0xEA, 0xF7));
        p.setFont(BaStyle::font(8));
        p.drawText(QRectF(42, 26, width() - 50, 20), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("消耗AP时，将获得2.5倍(+150%)的账号经验值。(至Lv.40为止)"));
    }
};

// UID 胶囊：白底 + 深蓝描边 + 深蓝粗字 + Copy 图标
class UidCapsule : public QWidget
{
public:
    UidCapsule(QWidget *parent = nullptr) : QWidget(parent) { setMinimumSize(170, 34); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r(0, 0, width(), height());
        // 尾端圆弧造型（左直角右半圆）
        QPainterPath cap;
        cap.addRoundedRect(r, height() / 2.0, height() / 2.0);
        p.setPen(QPen(QColor(0xB8, 0xD0, 0xE4), 1.2));
        p.setBrush(QColor(0xFF, 0xFF, 0xFF, 235));
        p.drawPath(cap);

        p.setPen(QColor(0x26, 0x45, 0x6C));
        p.setFont(BaStyle::font(10, QFont::DemiBold));
        p.drawText(QRectF(12, 0, width() - 44, height()), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("UID: 19279245"));
        p.drawPixmap(width() - 32, height() / 2 - 8,
                     ba::pixmap(ba::Glyph::Copy, QSize(16, 16), QColor(0x3B, 0xA0, 0x8E)));
    }
};

// 亮青渐变大按钮（玩家資訊 / ID卡）
class BigGradientButton : public QWidget
{
public:
    BigGradientButton(const QString &text, QWidget *parent = nullptr) : text_(text), QWidget(parent)
    {
        setMinimumSize(250, 62);
        setCursor(Qt::PointingHandCursor);
    }

    void setText2(const QString &t) { text_ = t; update(); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r(0, 0, width(), height());
        QPainterPath path = BaStyle::skewRectPath(r, -8.0, 9.0);

        p.setPen(QPen(Qt::white, 1.6));
        QLinearGradient g(r.topLeft(), r.bottomLeft());
        g.setColorAt(0.0, QColor(0x7E, 0xD8, 0xFA));
        g.setColorAt(1.0, QColor(0x3C, 0xB4, 0xEE));
        p.setBrush(g);
        p.drawPath(path);

        // 左上白色高光弧
        p.save();
        p.setClipPath(path);
        p.setPen(QPen(QColor(255, 255, 255, 120), 2.4, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(QRectF(r.left() + 14, r.top() + 6, r.width() - 28, r.height() - 14),
                  200 * 16, 140 * 16);
        p.restore();

        QFont f = BaStyle::font(15, QFont::Bold);
        p.setFont(f);
        p.setPen(QColor(0x1C, 0x4C, 0x7C));
        p.drawText(r, Qt::AlignCenter, text_);
    }

private:
    QString text_;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    BaAssets::enableCursor();

    QWidget win;
    win.resize(1180, 740);
    win.setWindowTitle(QStringLiteral("BA Info Card Preview"));

    // 浅蓝渐变背景天空（绝对定位 + show 后跟随）
    BaBackground *bg = new BaBackground(&win);
    bg->setImagePath("img/bg/mainBG.jpeg");
    bg->setGeometry(0, 0, 1180, 740);
    QTimer::singleShot(150, [bg, &win]() { bg->setGeometry(win.rect()); });

    // —— 绝对定位布局（无 WM xcb 下 QLayout 尺寸不可靠，全部手动摆放）——
    // 左列：進修中 + UID + 玩家卡
    BoostNotice *bn = new BoostNotice(&win);
    bn->setGeometry(64, 64, 300, 56);
    UidCapsule *uc = new UidCapsule(&win);
    uc->setGeometry(64, 148, 300, 34);
    BaPlayerCard *pl = new BaPlayerCard(&win);
    pl->setLevel(35);
    pl->setNameText(QStringLiteral("啊这"));
    pl->setProgress(444, 982);
    pl->setGeometry(64, 190, 300, 76);

    // 右列：兩张信息卡
    BaInfoCard *c1 = new BaInfoCard(QStringLiteral("稱呼"), &win);
    c1->addRow(QStringLiteral("稱呼"), QStringLiteral("aaazhe"), BaInfoCard::Trailing::Speaker);
    c1->setFixedSize(680, c1->sizeHint().height());
    c1->move(452, 36);

    BaInfoCard *c2 = new BaInfoCard(QStringLiteral("簡介設定"), &win);
    c2->addRow(QStringLiteral("稱號"), QStringLiteral("夏萊的老師"), BaInfoCard::Trailing::Edit);
    c2->addRow(QStringLiteral("値日生"), QStringLiteral("阳奈(礼服)"), BaInfoCard::Trailing::Edit);
    c2->addRow(QStringLiteral("問候"), QStringLiteral("泥嚎"), BaInfoCard::Trailing::Edit);
    c2->setFixedSize(680, c2->sizeHint().height());
    c2->move(452, c1->height() + 36 + 16);

    // 底部亮青渐变大按钮
    BigGradientButton *b1 = new BigGradientButton(QStringLiteral("玩家資訊"), &win);
    b1->setGeometry(452, c2->y() + c2->height() + 24, 326, 62);
    BigGradientButton *b2 = new BigGradientButton(QStringLiteral("ID卡"), &win);
    b2->setGeometry(806, c2->y() + c2->height() + 24, 326, 62);

    // 截图
    const QStringList args = app.arguments();
    if (args.contains("--screenshot")) {
        const QString path = args.at(args.indexOf("--screenshot") + 1);
        QTimer::singleShot(900, [&win, &app, path]() {
            win.grab().save(path);
            app.exit(0);
        });
    }

    win.show();
    return app.exec();
}
