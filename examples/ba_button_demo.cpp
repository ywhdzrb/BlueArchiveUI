// ba_button_demo.cpp —— 纯按钮 demo：大尺寸展示 + 全角色色板一排，
// 供针对按钮样式进行指点与调优（浅白底、无背景素材干扰）。
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QApplication>
#include <QTimer>

#include "ba_assets.h"
#include "ba_style.h"
#include "ba_button.h"

namespace {

// 一个带说明标签的按钮展示行（标签在左，按钮在右）
QWidget *labeledRow(const QString &labelText, BaButton *btn)
{
    QWidget *row = new QWidget;
    QHBoxLayout *lay = new QHBoxLayout(row);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(16);
    QLabel *lab = new QLabel(labelText);
    lab->setStyleSheet(QStringLiteral(
        "font-size: 13px; color: #003153; font-weight: 700; padding-right: 8px;"));
    lab->setFixedWidth(110);
    lay->addWidget(lab);
    lay->addWidget(btn, 0, Qt::AlignLeft | Qt::AlignVCenter);
    lay->addStretch();
    return row;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    BaAssets::enableCursor();  // 注册官方字体 + 白色指针

    auto win = new QWidget;
    win->setWindowTitle(QStringLiteral("BlueArchive - Button Demo"));
    win->resize(760, 700);
    win->setStyleSheet(QStringLiteral("background: #F5FAFF;"));  // 极浅蓝白底，无干扰
    auto *root = new QVBoxLayout(win);
    root->setContentsMargins(48, 40, 48, 40);
    root->setSpacing(18);

    auto *cap = new QLabel(QStringLiteral("<< Butterfly Button Preview >>"));
    cap->setStyleSheet(QStringLiteral(
        "font-size: 16px; color: #003153; font-weight: 800; letter-spacing: 1px;"));
    root->addWidget(cap);

    // 大号单按钮：主操作（Sky 纯色 + 斜切 + 投影，CodePen 方案）
    // 大尺寸下用与 CodePen 一致的 -16° 确保斜切明显
    auto *big = new BaButton(QStringLiteral("立即前往"), ba::SurfaceRole::Sky);
    big->setFixedSize(340, 96);
    big->setFont(BaStyle::font(16, QFont::Bold));
    root->addWidget(labeledRow(QStringLiteral("Big / Sky"), big));

    // 不同角色色一排
    auto *row1 = new QHBoxLayout;
    const struct { const char *name; ba::SurfaceRole role; } roles[] = {
        { "Sky", ba::SurfaceRole::Sky },
        { "Green", ba::SurfaceRole::Green },
        { "Purple", ba::SurfaceRole::Purple },
        { "Yellow", ba::SurfaceRole::Yellow },
        { "Red", ba::SurfaceRole::Red },
        { "Deep", ba::SurfaceRole::Deep },
    };
    for (const auto &r : roles) {
        auto *b = new BaButton(QString::fromUtf8(r.name), r.role);
        b->setFixedSize(110, 40);
        row1->addWidget(b);
    }
    root->addLayout(row1);

    // Ghost（白底描边）+ 禁用态
    auto *row2 = new QHBoxLayout;
    auto *ghost = new BaButton(QStringLiteral("Ghost"), ba::SurfaceRole::Ghost);
    ghost->setFixedSize(110, 40);
    auto *disabled = new BaButton(QStringLiteral("Disabled"), ba::SurfaceRole::Sky);
    disabled->setFixedSize(110, 40);
    disabled->setEnabled(false);
    row2->addWidget(ghost);
    row2->addWidget(disabled);
    row2->addStretch();
    root->addLayout(row2);

    root->addStretch();
    win->show();

    // 截图支持：--screenshot <path>，渲染 900ms 后自拍退出（供验证）
    QStringList args = app.arguments();
    const int si = args.indexOf(QStringLiteral("--screenshot"));
    if (si >= 0 && si + 1 < args.size()) {
        const QString path = args.at(si + 1);
        QTimer::singleShot(900, win, [win, path]() {
            win->grab().save(path);
            QApplication::quit();
        });
    }

    return app.exec();
}
