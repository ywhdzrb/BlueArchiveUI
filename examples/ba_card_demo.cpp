// 基础卡片演示：单张 BaCard + 简单内容（标题/说明/按钮）
// 运行：./build/blue_archive_ui_card_demo --screenshot <png>

#include <QApplication>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "ba_button.h"
#include "ba_card.h"
#include "ba_style.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("BA Card Preview"));

    QWidget win;
    win.setStyleSheet("background:#EAF3FA;");
    win.resize(520, 320);
    win.setWindowTitle(QStringLiteral("BA Card Preview"));

    // 单张基础卡：行式布局（每行沿左斜边平行等距对齐）
    BaCard *card = new BaCard(&win);
    card->setGeometry(60, 40, 400, 240);

    QLabel *title = new QLabel(QStringLiteral("基礎卡片"));
    title->setStyleSheet("color:#123A64; font-size:15px; font-weight:bold; background:transparent;");

    QLabel *desc = new QLabel(
        QStringLiteral("白色 -16° 平行四边形卡体，深蓝描边，圆角 12，双层投影。\n"
                       "内容每行沿左斜边平行等距对齐（addRow 行式布局）。"));
    desc->setStyleSheet("color:#5C7182; font-size:11px; background:transparent;");
    desc->setWordWrap(true);

    BaButton *btn = new BaButton(QStringLiteral("前往"), ba::SurfaceRole::Sky);
    btn->setFixedWidth(140);

    card->addRow(title);
    card->addRow(desc);
    card->addRow(btn);

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
