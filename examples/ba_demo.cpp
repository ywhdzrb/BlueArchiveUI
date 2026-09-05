// BA 风格示例：模拟 Blue Archive 大厅 + 设置中心。
// 演示 BaBackground / BaTopBar / BaTabColumn / BaButton / BaSlider /
// BaCheckBox / BaProgressBar / BaSectionHeader / BaDialog 组合使用。
// 支持 --screenshot <path> 在渲染完成后截图退出（供验证与文档）。

#include <QApplication>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPixmap>

#include "blue_archive_ui.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setFont(BaStyle::font(10));
    // 官方素材可选：注册字体 + BA 白色鼠标指针（找不到素材自动跳过）
    BaAssets::enableCursor();

    QWidget *window = new QWidget;
    window->setWindowTitle(QStringLiteral("BlueArchive UI Demo - 学园日常"));
    window->resize(1180, 740);

    // 主树：背景 / 顶栏 / 可选侧栏 tab + 内容页 / 弹窗
    QVBoxLayout *root = new QVBoxLayout(window);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    BaBackground *bg = new BaBackground(window);
    bg->setParent(window);
    bg->lower();
    bg->move(0, 0);
    // 官方办公室大厅背景图（BaAssets 素材，缺失时自动回退手绘天空）
    bg->setImagePath(QStringLiteral("img/bg/bg_office.jpg"));
    QTimer::singleShot(0, window, [window, bg]() {
        bg->setGeometry(window->rect());
    });

    BaTopBar *top = new BaTopBar(QStringLiteral("学园日程"), window);
    top->setPlayer(QStringLiteral("灘橋"), 2, QStringLiteral("36/55"));
    root->addWidget(top);

    QWidget *body = new QWidget;
    body->setStyleSheet(QStringLiteral("background: transparent;"));
    QHBoxLayout *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(20, 22, 20, 20);
    bodyLayout->setSpacing(18);

    // 左侧设置式 tab 列（BA 侧栏形制）
    BaTabColumn *tabs = new BaTabColumn;
    tabs->addItem(QStringLiteral("大厅"), true);
    tabs->addItem(QStringLiteral("角色"));
    tabs->addItem(QStringLiteral("任务"));
    tabs->addItem(QStringLiteral("商店"));
    tabs->addItem(QStringLiteral("设置"));
    tabs->setItemIcon(0, int(ba::Glyph::Home));
    tabs->setItemIcon(1, int(ba::Glyph::Search));
    tabs->setItemIcon(2, int(ba::Glyph::Check));
    tabs->setItemIcon(3, int(ba::Glyph::Coin));
    tabs->setItemIcon(4, int(ba::Glyph::Settings));

    // 内容页：白色面板
    QWidget *content = new QWidget;
    content->setStyleSheet(QStringLiteral("background: rgba(255,255,255,0.92); border-radius: 14px;"));
    QVBoxLayout *cl = new QVBoxLayout(content);
    cl->setContentsMargins(26, 22, 26, 22);
    cl->setSpacing(12);

    // —— 段 1：按钮色板 ——
    cl->addWidget(new BaSectionHeader(QStringLiteral("快捷指令")));
    QHBoxLayout *btns = new QHBoxLayout;
    btns->setSpacing(10);
    BaButton *b1 = new BaButton(QStringLiteral("进行招募"), ba::SurfaceRole::Sky);
    BaButton *b2 = new BaButton(QStringLiteral("完成任务"), ba::SurfaceRole::Green);
    BaButton *b3 = new BaButton(QStringLiteral("学园手册"), ba::SurfaceRole::Purple);
    BaButton *b4 = new BaButton(QStringLiteral("奖励领取"), ba::SurfaceRole::Yellow);
    b4->setTextColor(BaStyle::deep());
    BaButton *b5 = new BaButton(QStringLiteral("删除好友"), ba::SurfaceRole::Red);
    BaButton *b6 = new BaButton(QStringLiteral("返回主页"), ba::SurfaceRole::Deep);
    BaButton *b7 = new BaButton(QStringLiteral("咨询教师"), ba::SurfaceRole::Ghost);
    for (BaButton *b : {b1, b2, b3, b4, b5, b6, b7})
        btns->addWidget(b);
    btns->addStretch(1);
    cl->addLayout(btns);

    // —— 段 2：音量细调与静音 ——
    cl->addWidget(new BaSectionHeader(QStringLiteral("声音设置")));
    QHBoxLayout *soundRow = new QHBoxLayout;
    soundRow->setSpacing(14);
    BaSlider *slider = new BaSlider;
    slider->setRange(0, 100);
    slider->setValue(68);
    BaCheckBox *mute = new BaCheckBox(QStringLiteral("静音"));
    mute->setChecked(false);
    soundRow->addWidget(slider, 1);
    soundRow->addWidget(mute);
    cl->addLayout(soundRow);

    // —— 段 3：成长进度 ——
    cl->addWidget(new BaSectionHeader(QStringLiteral("光之诗篇活动进度")));
    BaProgressBar *hbar = new BaProgressBar;
    hbar->setRange(0, 100);
    hbar->setValue(33);
    QHBoxLayout *prRow = new QHBoxLayout;
    prRow->setSpacing(14);
    prRow->addWidget(hbar, 1);
    BaButton *inc = new BaButton(QStringLiteral("推进 10%"), ba::SurfaceRole::Sky);
    prRow->addWidget(inc);
    cl->addLayout(prRow);

    // —— 段 4：白卡 + 黑晶片（商店形制） ——
    QHBoxLayout *shopRow = new QHBoxLayout;
    shopRow->setSpacing(14);
    BaPanel *card1 = new BaPanel;
    card1->setHeaderTitle(QStringLiteral("新手教学月卡"));
    QHBoxLayout *c1 = new QHBoxLayout;
    c1->addStretch();
    c1->addWidget(new BaChip(QStringLiteral("×10000")));
    c1->addWidget(new BaChip(QStringLiteral("30")));
    card1->contentLayout()->addLayout(c1);
    BaPanel *card2 = new BaPanel;
    card2->setHeaderTitle(QStringLiteral("青辉石补给包"));
    QHBoxLayout *c2 = new QHBoxLayout;
    c2->addStretch();
    c2->addWidget(new BaChip(QStringLiteral("10,000")));
    card2->contentLayout()->addLayout(c2);
    shopRow->addWidget(card1, 1);
    shopRow->addWidget(card2, 1);
    cl->addLayout(shopRow);

    // —— 段 5：商店左侧深蓝菜单块（题 5 图左侧菜单列形制） ——
    QHBoxLayout *menuRow = new QHBoxLayout;
    menuRow->setSpacing(10);
    BaShopMenuBlock *m1 = new BaShopMenuBlock(QStringLiteral("特定商品库"));
    BaShopMenuBlock *m2 = new BaShopMenuBlock(QStringLiteral("特选奖券"));
    m2->setSelected(true);
    BaShopMenuBlock *m3 = new BaShopMenuBlock(QStringLiteral("战利品商店"));
    menuRow->addWidget(m1, 1);
    menuRow->addWidget(m2, 1);
    menuRow->addWidget(m3, 1);
    cl->addLayout(menuRow);

    // —— 段 6：任务页签条 + 悬赏卡（题 3/4 图任务/悬赏页形制） ——
    cl->addWidget(new BaSectionHeader(QStringLiteral("委托 · 悬赏通缉")));
    BaPageTabs *pageTabs = new BaPageTabs;
    pageTabs->addItem(QStringLiteral("全体"));
    pageTabs->addItem(QStringLiteral("每天"));
    pageTabs->addItem(QStringLiteral("每周"));
    pageTabs->addItem(QStringLiteral("成就"));
    pageTabs->addItem(QStringLiteral("挑战任务"));
    pageTabs->setCurrentIndex(3);
    cl->addWidget(pageTabs);

    auto makeBounty = [&](const QString &title, const QString &tag,
                          const QString &desc, const QString &ticket) {
        BaBountyCard *bc = new BaBountyCard(title, ticket);
        bc->setTag(tag);
        bc->setDescription(desc);
        return bc;
    };
    cl->addWidget(makeBounty(QStringLiteral("高架公路"), QStringLiteral("活動進行中"),
                             QStringLiteral("可以获得必杀技升级所需的材料。"),
                             QStringLiteral("持有跳战券 7/2")));
    cl->addWidget(makeBounty(QStringLiteral("沙漠铁路"), QStringLiteral("活動進行中"),
                             QStringLiteral("可以获得技能升级所需的材料。"),
                             QStringLiteral("持有跳战券 8/2")));

    // —— 段 6.5：成就卡（题 4 图任务成就页：橙标签+黑轨进度条+奖励盒+立即前往） ——
    auto makeMission = [&](const QString &title) {
        BaMissionCard *mc = new BaMissionCard(title);
        mc->setTag(QStringLiteral("成就"));
        mc->setProgressText(QStringLiteral("次数 0/1"));
        mc->setProgressValue(0, 1);
        mc->setRewardCount(20);
        return mc;
    };
    cl->addWidget(makeMission(QStringLiteral("完成主线剧情第4篇第1章第18话")));
    cl->addWidget(makeMission(QStringLiteral("完成主线剧情第4篇第1章第19话")));

    // —— 段 7：对话气泡 ——
    BaVoiceBubble *bubble = new BaVoiceBubble(QStringLiteral("嘻嘻，等你很久咯。"));
    bubble->setSpeaker(QStringLiteral("純子"));
    cl->addWidget(bubble);

    // —— 段 8：打开设置弹窗 ——
    QHBoxLayout *openRow = new QHBoxLayout;
    openRow->addStretch();
    BaButton *openDlg = new BaButton(QStringLiteral("打开系统设置"), ba::SurfaceRole::Deep);
    openRow->addWidget(openDlg);
    cl->addLayout(openRow);
    cl->addStretch();

    bodyLayout->addWidget(tabs);
    bodyLayout->addWidget(content, 1);
    root->addWidget(body, 1);

    // —— 弹窗：底部滑入的设置面板 ——
    BaDialog *dlg = new BaDialog(QStringLiteral("系统设置"), window);
    dlg->setPanelWidth(560);
    QWidget *dlgBody = new QWidget;
    dlgBody->setStyleSheet(QStringLiteral("background: transparent;"));
    QVBoxLayout *dl = new QVBoxLayout(dlgBody);
    dl->setContentsMargins(0, 0, 0, 0);
    dl->setSpacing(16);
    dl->addWidget(new BaSectionHeader(QStringLiteral("杂项")));
    BaCheckBox *ch1 = new BaCheckBox(QStringLiteral("显示体力上限月历提醒"));
    BaCheckBox *ch2 = new BaCheckBox(QStringLiteral("今日已读新闻不再弹出"));
    ch1->setChecked(true);
    ch2->setChecked(false);
    dl->addWidget(ch1);
    dl->addWidget(ch2);
    BaSlider *dlgSlider = new BaSlider;
    dlgSlider->setRange(0, 100);
    dlgSlider->setValue(80);
    dl->addWidget(new BaSectionHeader(QStringLiteral("主界面 BGM 音量")));
    dl->addWidget(dlgSlider);
    QHBoxLayout *dlgBtns = new QHBoxLayout;
    dlgBtns->addStretch();
    BaButton *confirm = new BaButton(QStringLiteral("确定"), ba::SurfaceRole::Sky);
    BaButton *cancel = new BaButton(QStringLiteral("取消"), ba::SurfaceRole::Ghost);
    dlgBtns->addWidget(cancel);
    dlgBtns->addWidget(confirm);
    dl->addLayout(dlgBtns);
    dl->addStretch();
    dlg->contentLayout()->addWidget(dlgBody);

    // —— 交互 ——
    QObject::connect(inc, &QAbstractButton::clicked, [hbar]() {
        hbar->setValue(qMin(100, hbar->value() + 10));
    });
    QObject::connect(openDlg, &QAbstractButton::clicked, [dlg]() { dlg->showAsModal(); });
    QObject::connect(top, &BaTopBar::gearClicked, [dlg]() { dlg->showAsModal(); });
    QObject::connect(confirm, &QAbstractButton::clicked, [dlg]() { dlg->closeDialog(); });
    QObject::connect(cancel, &QAbstractButton::clicked, [dlg]() { dlg->closeDialog(); });
    QObject::connect(tabs, &BaTabColumn::currentChanged, [tabs, content](int idx) {
        Q_UNUSED(idx);
        Q_UNUSED(tabs);
        // 内容页预留：切换 tab 可换页面，此处仅刷新视觉
        content->update();
    });

    window->show();

    // 截图支持：--screenshot <path>，渲染 900ms 后再拍；
    // --show-dialog 在 400ms 时弹出设置对话框（与截图配合验证弹窗）
    QStringList args = app.arguments();
    if (args.contains(QStringLiteral("--show-dialog")))
        QTimer::singleShot(400, dlg, [dlg]() { dlg->showAsModal(); });
    const int si = args.indexOf(QStringLiteral("--screenshot"));
    if (si >= 0 && si + 1 < args.size()) {
        const QString path = args.at(si + 1);
        const int delay = args.contains(QStringLiteral("--show-dialog")) ? 1500 : 900;
        QTimer::singleShot(delay, window, [window, path]() {
            window->grab().save(path);
            QApplication::quit();
        });
    }

    return app.exec();
}
