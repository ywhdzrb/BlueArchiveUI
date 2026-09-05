// BA 点击粒子演示（按用户规格：蓝圆渐入渐出 + 彩色圆角三角散射 + 月牙 + 渐细轨迹线）
// 运行：./build/blue_archive_ui_spark_demo --screenshot <png>
// 交互：鼠标按下点击 → 点击效果；按住拖动 → 轨迹线 + 随机三角

#include <QApplication>
#include <QLabel>
#include <QTimer>
#include <QWidget>

#include "blue_archive_ui.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("BlueArchive UI Spark Demo"));

    BaAssets::enableCursor(); // 官方 BA 白色鼠标光标

    QWidget win;
    win.resize(1160, 740);
    win.setWindowTitle(QStringLiteral("BlueArchive UI — BA Spark"));

    // 深色背景（对比度友好，粒子半透明下清晰可见）
    win.setStyleSheet("background:#0E2232;");

    // 粒子特效层（覆盖全屏，捕获鼠标）
    BaSpark *spark = new BaSpark(&win);
    spark->setGeometry(0, 0, 1160, 740);

    // 提示
    QLabel *tip = new QLabel(QStringLiteral("点击：蓝圆 + 彩色三角 + 月牙；按住拖动：细尾轨迹线"), &win);
    tip->setStyleSheet("color:#8FC6EE; font-size:14px; background:transparent;");
    tip->setGeometry(400, 30, 700, 30);
    tip->hide();

    // 截图时自动演示一组动作
    const QStringList args = app.arguments();
    if (args.contains("--screenshot")) {
        const QString path = args.at(args.indexOf("--screenshot") + 1);
        QTimer::singleShot(400, [spark]() {
            // 一次点击（按下后拖动：轨迹线 + 拖尾三角）
            spark->clickAt(QPointF(300, 460));
            spark->moveTo(QPointF(360, 400));
            spark->moveTo(QPointF(440, 340));
            spark->moveTo(QPointF(540, 300));
            spark->moveTo(QPointF(660, 280));
            spark->moveTo(QPointF(790, 290));
            spark->moveTo(QPointF(910, 330));
        });
        QTimer::singleShot(560, [spark]() {
            spark->clickAt(QPointF(920, 160));
            spark->clickAt(QPointF(560, 560));
        });
        QTimer::singleShot(700, [&win, &app, path]() {
            win.grab().save(path);
            app.exit(0);
        });
    }

    win.show();
    return app.exec();
}
