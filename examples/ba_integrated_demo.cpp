// BA 控件整合页：顶栏 + 按钮/声音/进度/卡片 四个分区（仅收录已调好的控件）
// 运行：./build/blue_archive_ui_integrated_demo --screenshot <png>
// 布局为绝对定位（无 WM xcb 下 QLayout 尺寸不可靠）

#include <QApplication>
#include <QLabel>
#include <QTimer>
#include <QWidget>

#include "blue_archive_ui.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("BlueArchive UI Integrated Demo"));

    BaAssets::enableCursor();

    QWidget win;
    win.resize(1180, 740);
    win.setWindowTitle(QStringLiteral("BlueArchive UI — Integrated"));
    win.setStyleSheet("background:#DFF1FA;");

    // 粒子装饰层（全屏、事件穿透、全局监听——不干扰控件交互）
    // 仅按下拖动/点击时出效果，鼠标随意移动不拉线
    BaSpark *spark = new BaSpark(&win);
    spark->setInteractive(false);
    spark->setFxOpacity(0.75);
    spark->setGeometry(0, 0, 1180, 740);

    // 顶栏（Page 形态）
    BaTopBar *top = new BaTopBar(QStringLiteral("學園日程"), &win);
    top->setMode(BaTopBar::Mode::Page);
    top->setWallet(QStringLiteral("152"), QStringLiteral("16,118,958"), QStringLiteral("685"));
    top->setGeometry(0, 0, 1180, 64);

    int y = 96;
    auto sect = [&](const QString &t) {
        BaSectionHeader *h = new BaSectionHeader(t, &win);
        h->setGeometry(40, y, 800, 34);
        y += 46;
    };
    auto row = [&](int hh) {
        int r = y;
        y += hh;
        return r;
    };

    // ① 按钮
    sect(QStringLiteral("按钮"));
    {
        const struct { const char *t; ba::SurfaceRole r; } items[] = {
            {"Sky", ba::SurfaceRole::Sky}, {"Green", ba::SurfaceRole::Green},
            {"Purple", ba::SurfaceRole::Purple}, {"Yellow", ba::SurfaceRole::Yellow},
            {"Red", ba::SurfaceRole::Red}, {"Deep", ba::SurfaceRole::Deep},
        };
        int x = 40;
        for (const auto &it : items) {
            BaButton *b = new BaButton(QString::fromUtf8(it.t), it.r, &win);
            b->setGeometry(x, y, 118, 40);
            b->setFixedSize(118, 40);
            x += 128;
        }
        BaButton *ghost = new BaButton(QStringLiteral("Ghost"), ba::SurfaceRole::Ghost, &win);
        ghost->setGeometry(40, y + 50, 118, 40);
        BaButton *dis = new BaButton(QStringLiteral("禁用"), ba::SurfaceRole::Sky, &win);
        dis->setGeometry(168, y + 50, 118, 40);
        dis->setEnabled(false);
        y += 100;
    }

    // ② 声音
    sect(QStringLiteral("声音"));
    {
        int r = row(96);
        BaSlider *s1 = new BaSlider(&win);
        s1->setGeometry(40, r, 380, 28);
        s1->setValue(68);
        BaCheckBox *mute = new BaCheckBox(QStringLiteral("静音"), &win);
        mute->setGeometry(444, r + 2, 120, 26);
        BaCheckBox *hint = new BaCheckBox(QStringLiteral("提示音"), &win);
        hint->setGeometry(576, r + 2, 120, 26);
        hint->setChecked(true);
        QLabel *vl = new QLabel(QStringLiteral("BGM  68"), &win);
        vl->setStyleSheet("color:#123A64; font-size:10px; font-weight:bold; background:transparent;");
        vl->setGeometry(40, r - 26, 160, 20);
    }

    // ③ 进度
    sect(QStringLiteral("进度"));
    {
        int r = row(68);
        BaProgressBar *p1 = new BaProgressBar(&win);
        p1->setGeometry(40, r, 500, 22);
        p1->setRange(0, 100);
        p1->setValue(62);
        BaProgressBar *p2 = new BaProgressBar(&win);
        p2->setGeometry(40, r + 34, 500, 20);
        p2->setRange(0, 100);
        p2->setValue(45);
        p2->setTrackColor(BaStyle::progressTrackDark());
        y = r + 64;
    }

    // ④ 卡片
    sect(QStringLiteral("卡片"));
    {
        int r = row(200);
        BaCard *card = new BaCard(&win);
        card->setGeometry(40, r, 360, 190);
        QLabel *ct = new QLabel(QStringLiteral("基礎卡片"), card);
        ct->setStyleSheet("color:#123A64; font-size:14px; font-weight:bold; background:transparent;");
        QLabel *cd = new QLabel(QStringLiteral("行式布局，内容随左斜边平行对齐。"), card);
        cd->setStyleSheet("color:#5C7182; font-size:10px; background:transparent;");
        BaButton *cb = new BaButton(QStringLiteral("前往"), ba::SurfaceRole::Sky, card);
        cb->setFixedWidth(120);
        card->addRow(ct);
        card->addRow(cd);
        card->addRow(cb);

        BaPanel *panel = new BaPanel(&win);
        panel->setGeometry(424, r, 360, 150);
        panel->setHeaderTitle(QStringLiteral("新手教學月卡"));
        QVBoxLayout *pl = panel->contentLayout();
        pl->addStretch();
        QHBoxLayout *ph = new QHBoxLayout();
        ph->addWidget(new BaChip(QStringLiteral("×10,000"), panel));
        ph->addWidget(new BaChip(QStringLiteral("×30"), panel));
        ph->addWidget(new BaChip(QStringLiteral("×1000"), panel));
        ph->addStretch();
        pl->addLayout(ph);
        pl->addSpacing(6);
        y = r + 190;
    }

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
