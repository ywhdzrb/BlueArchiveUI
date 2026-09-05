// BA 全部控件总汇演示：一个页面展示所有 BA 风格控件 + 弹窗交互
// 运行：./build/blue_archive_ui_all_demo --screenshot <png>
// 布局为绝对定位（无 WM xcb 下 QLayout 尺寸不可靠，均手动 setGeometry）

#include <QApplication>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QWidget>

#include "blue_archive_ui.h"

// 内容区标题（深蓝粗体 + 前导蓝条）
static QLabel *genSectionTitle(const QString &text, QWidget *parent)
{
    QLabel *t = new QLabel(text, parent);
    t->setStyleSheet(QStringLiteral(
        "color:#123A64; font-size:13px; font-weight:bold; background:transparent;"));
    t->setGeometry(0, 0, 300, 26);
    return t;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("BlueArchive UI All Demo"));

    // 官方素材（字体/背景/鼠标/图标）
    BaAssets::enableCursor();

    QWidget win;
    win.resize(1180, 740);
    win.setWindowTitle(QStringLiteral("BlueArchive UI — All Controls"));

    // 全场背景（官方大厅图）
    BaBackground *bg = new BaBackground(&win);
    bg->setImagePath("img/bg/mainBG.jpeg");
    bg->setGeometry(0, 0, 1180, 740);
    QTimer::singleShot(150, [bg, &win]() { bg->setGeometry(win.rect()); });

    // 顶栏（Hall 模式：玩家卡 + 资源 + home）
    BaTopBar *top = new BaTopBar(QStringLiteral("学园日程"), &win);
    top->setMode(BaTopBar::Mode::Hall);
    top->setPlayer(QStringLiteral("灘橋"), 35, QStringLiteral("444/982"));
    top->setWallet(QStringLiteral("152"), QStringLiteral("16,118,958"), QStringLiteral("685"));
    top->setGeometry(0, 0, 1180, 64);

    // —— 内容滚动区 ——
    QScrollArea *scroll = new QScrollArea(&win);
    scroll->setGeometry(0, 64, 940, 610);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea{background:transparent;}"
                          "QScrollBar:vertical{background:rgba(255,255,255,140);width:8px;border-radius:4px;}"
                          "QScrollBar::handle:vertical{background:#7EC9F0;border-radius:4px;min-height:30px;}"
                          "QScrollBar::add-line,QScrollBar::sub-line{height:0;}");
    scroll->viewport()->setStyleSheet("background:transparent;");
    QWidget *canvas = new QWidget();
    canvas->resize(940, 1660);
    canvas->setStyleSheet("background:transparent;");
    scroll->setWidget(canvas);
    scroll->setWidgetResizable(false);

    const auto sect = [&](const QString &text, int y) {
        QLabel *t = genSectionTitle(text, canvas);
        t->setGeometry(20, y, 500, 26);
        return t;
    };

    // ============ 1. 按钮 ============
    sect(QStringLiteral("按钮 (BaButton) — 七角色 / Ghost / 禁用"), 14);
    {
        int x = 24;
        const struct { const char *text; ba::SurfaceRole role; } items[] = {
            { "Sky",   ba::SurfaceRole::Sky },
            { "Green", ba::SurfaceRole::Green },
            { "Purple", ba::SurfaceRole::Purple },
            { "Yellow", ba::SurfaceRole::Yellow },
            { "Red",   ba::SurfaceRole::Red },
            { "Deep",  ba::SurfaceRole::Deep },
        };
        for (const auto &it : items) {
            BaButton *b = new BaButton(QString::fromUtf8(it.text), it.role, canvas);
            b->setGeometry(x, 46, 118, 40);
            b->setFixedSize(118, 40);
            x += 126;
        }
        BaButton *ghost = new BaButton(QStringLiteral("Ghost"), ba::SurfaceRole::Ghost, canvas);
        ghost->setGeometry(24, 96, 118, 40);
        BaButton *dis = new BaButton(QStringLiteral("禁用"), ba::SurfaceRole::Sky, canvas);
        dis->setGeometry(160, 96, 118, 40);
        dis->setEnabled(false);
    }

    // ============ 2. 滑块 / 复选框 / 进度条 ============
    sect(QStringLiteral("声音 (BaSlider / BaCheckBox) — 音量设定"), 150);
    {
        BaSectionHeader *sh = new BaSectionHeader(QStringLiteral("音量设定"), canvas);
        sh->setGeometry(24, 176, 500, 34);
        QLabel *vLabel = new QLabel(QStringLiteral("BGM  68"), canvas);
        vLabel->setStyleSheet("color:#123A64; font-size:10px; font-weight:bold; background:transparent;");
        vLabel->setGeometry(24, 214, 200, 20);
        BaSlider *s1 = new BaSlider(canvas);
        s1->setGeometry(24, 238, 380, 28);
        s1->setValue(68);
        BaCheckBox *mute = new BaCheckBox(QStringLiteral("静音"), canvas);
        mute->setGeometry(430, 238, 120, 26);
        mute->setChecked(false);
        BaCheckBox *hint = new BaCheckBox(QStringLiteral("提示音"), canvas);
        hint->setGeometry(560, 238, 120, 26);
        hint->setChecked(true);
    }

    sect(QStringLiteral("进度 (BaProgressBar)"), 246);
    {
        BaProgressBar *p1 = new BaProgressBar(canvas);
        p1->setGeometry(24, 276, 500, 22);
        p1->setRange(0, 100);
        p1->setValue(62);
        BaProgressBar *p2 = new BaProgressBar(canvas);
        p2->setGeometry(24, 310, 500, 20);
        p2->setRange(0, 100);
        p2->setValue(45);
        p2->setTrackColor(BaStyle::progressTrackDark());
        QLabel *tip = new QLabel(QStringLiteral("上一行为浅蓝轨 / 下一行为黑灰轨"), canvas);
        tip->setStyleSheet("color:#6B7F8D; font-size:9px; background:transparent;");
        tip->setGeometry(24, 336, 300, 18);
    }

    // ============ 3. 卡片 ============
    sect(QStringLiteral("卡片 (BaCard / BaPanel / BaInfoCard / BaPlayerCard / BaChip)"), 366);
    {
        BaCard *card = new BaCard(canvas);
        card->setGeometry(24, 392, 360, 230);
        QLabel *ct = new QLabel(QStringLiteral("基礎卡片"), card);
        ct->setStyleSheet("color:#123A64; font-size:14px; font-weight:bold; background:transparent;");
        QLabel *cd = new QLabel(QStringLiteral("行式布局，内容随左斜边平行对齐。"), card);
        cd->setStyleSheet("color:#5C7182; font-size:10px; background:transparent;");
        BaButton *cb = new BaButton(QStringLiteral("前往"), ba::SurfaceRole::Sky);
        cb->setFixedWidth(120);
        card->addRow(ct);
        card->addRow(cd);
        card->addRow(cb);

        BaPanel *panel = new BaPanel(canvas);
        panel->setGeometry(410, 392, 360, 170);
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

        BaInfoCard *info = new BaInfoCard(QStringLiteral("稱呼"), canvas);
        info->addRow(QStringLiteral("稱呼"), QStringLiteral("aaazhe"), BaInfoCard::Trailing::Edit);
        info->addRow(QStringLiteral("問候"), QStringLiteral("泥嚎"), BaInfoCard::Trailing::None);
        info->setGeometry(24, 640, 520, info->sizeHint().height());

        BaPlayerCard *pc = new BaPlayerCard(canvas);
        pc->setLevel(35);
        pc->setNameText(QStringLiteral("灘橋"));
        pc->setProgress(444, 982);
        pc->setGeometry(580, 640, 300, 76);

        QLabel *ctip = new QLabel(QStringLiteral("Chip ×10,000 / ×30 / ×1000（BaChip）"), canvas);
        ctip->setStyleSheet("color:#6B7F8D; font-size:9px; background:transparent;");
        ctip->setGeometry(24, 728, 400, 18);
    }

    // ============ 4. 页签 / 任务卡 ============
    sect(QStringLiteral("页签与任务卡 (BaPageTabs / BaBountyCard / BaMissionCard)"), 762);
    {
        BaPageTabs *tabs = new BaPageTabs(canvas);
        tabs->addItem(QStringLiteral("全体"));
        tabs->addItem(QStringLiteral("每天"));
        tabs->addItem(QStringLiteral("每周"));
        tabs->addItem(QStringLiteral("成就"));
        tabs->addItem(QStringLiteral("挑战任务"));
        tabs->setGeometry(24, 788, 760, 56);
        tabs->setCurrentIndex(3);

        BaBountyCard *bc = new BaBountyCard(QStringLiteral("高架公路"), QString(), canvas);
        bc->setTag(QStringLiteral("活動進行中"));
        bc->setDescription(QStringLiteral("可以获得必杀技能升级所需的材料。"));
        bc->setTicketText(QStringLiteral("持有跳战券 7/2"));
        bc->setGeometry(24, 860, 430, 96);

        BaMissionCard *mc = new BaMissionCard(
            QStringLiteral("完成主线剧情第4篇第1章第18话"), canvas);
        mc->setTag(QStringLiteral("成就"));
        mc->setProgressText(QStringLiteral("次数 0/1"));
        mc->setRewardCount(20);
        mc->setGeometry(470, 860, 430, 108);
    }

    // ============ 5. 菜单块 / 气泡 / 弹窗触发 ============
    sect(QStringLiteral("商店菜单 (BaShopMenuBlock) / 气泡 (BaVoiceBubble) / 弹窗 (BaDialog)"), 990);
    {
        const int y = 1024;
        BaShopMenuBlock *m1 = new BaShopMenuBlock(QStringLiteral("特定商品库"), canvas);
        m1->setGeometry(24, y, 150, 40);
        BaShopMenuBlock *m2 = new BaShopMenuBlock(QStringLiteral("特选奖券"), canvas);
        m2->setGeometry(182, y, 150, 40);
        m2->setSelected(true);
        BaShopMenuBlock *m3 = new BaShopMenuBlock(QStringLiteral("战利品商店"), canvas);
        m3->setGeometry(340, y, 150, 40);

        BaVoiceBubble *vb = new BaVoiceBubble(QStringLiteral("嘻嘻，等你很久咯。"), canvas);
        vb->setSpeaker(QStringLiteral("純子"));
        vb->setGeometry(24, 1080, 300, 92);

        BaButton *dlgBtn = new BaButton(QStringLiteral("打开系统设置弹窗"), ba::SurfaceRole::Deep, canvas);
        dlgBtn->setGeometry(360, 1096, 240, 44);
        BaDialog *dlg = new BaDialog(QStringLiteral("系统设置"), &win);
        dlg->setPanelWidth(560);
        {
            QVBoxLayout *dl = dlg->contentLayout();
            BaCheckBox *d1 = new BaCheckBox(QStringLiteral("自动播放语音"), dlg);
            BaCheckBox *d2 = new BaCheckBox(QStringLiteral("显示 BGM 曲名"), dlg);
            d2->setChecked(true);
            dl->addWidget(d1);
            dl->addSpacing(8);
            dl->addWidget(d2);
            dl->addSpacing(12);
            BaSlider *ds = new BaSlider(dlg);
            ds->setGeometry(0, 0, 320, 28);
            ds->setValue(80);
            dl->addWidget(ds);
            QHBoxLayout *db = new QHBoxLayout();
            db->addStretch();
            BaButton *ok = new BaButton(QStringLiteral("确定"), ba::SurfaceRole::Sky, dlg);
            BaButton *ce = new BaButton(QStringLiteral("取消"), ba::SurfaceRole::Deep, dlg);
            db->addWidget(ok);
            db->addSpacing(12);
            db->addWidget(ce);
            dl->addSpacing(16);
            dl->addLayout(db);
            QObject::connect(ok, &BaButton::clicked, dlg, &BaDialog::closeDialog);
            QObject::connect(ce, &BaButton::clicked, dlg, &BaDialog::closeDialog);
        }
        QObject::connect(dlgBtn, &BaButton::clicked, dlg, &BaDialog::showAsModal);
    }

    // ============ 6. 侧边 tab 列 ============
    sect(QStringLiteral("侧栏 (BaTabColumn)"), 1180);
    {
        BaTabColumn *tc = new BaTabColumn(canvas);
        tc->addItem(QStringLiteral("大厅"), true);
        tc->addItem(QStringLiteral("角色"));
        tc->addItem(QStringLiteral("任务"));
        tc->addItem(QStringLiteral("商店"));
        tc->addItem(QStringLiteral("设置"));
        tc->setItemIcon(0, int(ba::Glyph::Home));
        tc->setItemIcon(1, int(ba::Glyph::Search));
        tc->setItemIcon(2, int(ba::Glyph::Check));
        tc->setItemIcon(3, int(ba::Glyph::Coin));
        tc->setItemIcon(4, int(ba::Glyph::Settings));
        tc->setGeometry(24, 1206, 180, 246);
        QLabel *tip = new QLabel(QStringLiteral("设置弹窗左栏形态：浅蓝底白选中板"), canvas);
        tip->setStyleSheet("color:#6B7F8D; font-size:9px; background:transparent;");
        tip->setGeometry(224, 1216, 300, 18);
    }

    // ============ 7. 底部导航 ============
    {
        BaNavigationBar *nav = new BaNavigationBar(&win);
        nav->addItem(QStringLiteral("咖啡厅"), ba::Glyph::Home);
        nav->addItem(QStringLiteral("日程"), ba::Glyph::Settings);
        nav->addItem(QStringLiteral("成员"), ba::Glyph::Search);
        nav->addItem(QStringLiteral("编队"), ba::Glyph::Check);
        nav->addItem(QStringLiteral("小组"), ba::Glyph::Grid);
        nav->addItem(QStringLiteral("制造"), ba::Glyph::Settings);
        nav->addItem(QStringLiteral("商店"), ba::Glyph::Coin);
        nav->addItem(QStringLiteral("招募"), ba::Glyph::Search);
        nav->setGeometry(0, 740 - 66, 1180, 66);
        nav->setCurrentIndex(0);
    }

    // 截图（--screenshot 顶部；--shot-bottom 先滚到底再截，两张覆盖全部内容）
    const QStringList args = app.arguments();
    if (args.contains("--screenshot")) {
        const QString path = args.at(args.indexOf("--screenshot") + 1);
        const bool bottom = args.contains("--shot-bottom");
        const bool mid = args.contains("--shot-mid");
        if (bottom || mid) {
            QTimer::singleShot(900, [scroll, mid, bottom, &win, &app, path]() {
                QScrollBar *bar = scroll->verticalScrollBar();
                bar->setValue(bottom ? bar->maximum() : bar->maximum() / 2);
                QTimer::singleShot(300, [&win, &app, path]() {
                    win.grab().save(path);
                    app.exit(0);
                });
            });
        } else {
            QTimer::singleShot(900, [&win, &app, path]() {
                win.grab().save(path);
                app.exit(0);
            });
        }
    }
    win.show();
    return app.exec();
}
