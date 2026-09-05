// BaTopBar 演示：单一 Bar，自适应窗口宽度
// 运行：./build/blue_archive_ui_topbar_demo --screenshot <png>

#include <QApplication>
#include <QTimer>
#include <QWidget>

#include "blue_archive_ui.h"

// 宿主窗口：把 Bar 钉在顶部并跟宽（无 WM 截图环境 QLayout 不可靠，手动 setGeometry）
class HostWin : public QWidget
{
public:
    explicit HostWin(QWidget *parent = nullptr) {}

    void setBar(BaTopBar *bar, int h) { bar_ = bar; barH_ = h; }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        if (bar_)
            bar_->setGeometry(0, 0, width(), barH_);
    }

private:
    BaTopBar *bar_ = nullptr;
    int barH_ = 64;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("BlueArchive UI TopBar Demo"));

    HostWin win;
    win.resize(1180, 200);
    win.setWindowTitle(QStringLiteral("BlueArchive UI — TopBar"));
    win.setStyleSheet("background:#DFF1FA;");

    // 唯一一条 Bar：返回钮 + 页面标题 + 资源胶囊 + 斜切白片（随窗口宽度自适应）
    BaTopBar *bar = new BaTopBar(QStringLiteral("任務"), &win);
    bar->setMode(BaTopBar::Mode::Page);
    bar->setWallet(QStringLiteral("152"), QStringLiteral("16,118,958"), QStringLiteral("685"));
    bar->setGeometry(0, 0, win.width(), 64);
    win.setBar(bar, 64);

    QObject::connect(bar, &BaTopBar::backClicked, [&]() { qInfo("bar: back"); });
    QObject::connect(bar, &BaTopBar::plusClicked, [&]() { qInfo("bar: plus"); });
    QObject::connect(bar, &BaTopBar::gearClicked, [&]() { qInfo("bar: gear"); });
    QObject::connect(bar, &BaTopBar::homeClicked, [&]() { qInfo("bar: home"); });

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
