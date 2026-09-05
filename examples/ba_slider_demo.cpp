// BaSlider 独立预览页：复刻游戏设置页音量行（BGM/SE/ボイス）+ 基础/禁用变体，
// 用于逐项打磨滑块视觉。截图运行：blue_archive_ui_slider_demo --screenshot xxx.png

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "ba_check_box.h"
#include "ba_icon.h"
#include "ba_slider.h"
#include "ba_style.h"

// 音量设置行：左侧青蓝小竖条 + 深蓝粗体名称 + 滑块 + 右侧静音框（游戏设置页样式）
class VolumeRow : public QWidget
{
public:
    VolumeRow(const QString &name, int value, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        const auto h = new QHBoxLayout(this);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(8);

        // 名称左侧 3.5x16 青蓝圆角竖条
        auto *bar = new QWidget(this);
        bar->setFixedSize(4, 16);
        bar->setStyleSheet("background:#4EC3F5;border-radius:2px;");

        auto *label = new QLabel(name, this);
        label->setFont(BaStyle::font(10, QFont::Bold));
        label->setStyleSheet("color:#003153;");

        slider_ = new BaSlider(this);
        slider_->setValue(value);

        mute_ = new BaCheckBox("静音", this);

        h->addWidget(bar);
        h->addWidget(label);
        h->addWidget(slider_, 1);
        h->addWidget(mute_);
    }

    BaSlider *slider() const { return slider_; }

private:
    BaSlider *slider_ = nullptr;
    BaCheckBox *mute_ = nullptr;
};

class SliderPreview : public QWidget
{
public:
    explicit SliderPreview(QWidget *parent = nullptr) : QWidget(parent)
    {
        setStyleSheet("background:#FFFFFF;");

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(28, 24, 28, 24);
        layout->setSpacing(14);

        auto *title = new QLabel("<< Butterfly Slider Preview >>", this);
        title->setFont(BaStyle::font(14, QFont::Bold));
        title->setStyleSheet("color:#003153;");
        layout->addWidget(title);

        layout->addSpacing(6);

        // 设置页音量三段：BGM / SE / ボイス（官方样式：喇叭图标 + 白/灰双层 thumb）
        auto *bgm = new VolumeRow("BGM", 80, this);
        auto *se = new VolumeRow("SE", 60, this);
        auto *voice = new VolumeRow("ボイス", 30, this);
        layout->addWidget(bgm);
        layout->addWidget(se);
        layout->addWidget(voice);

        layout->addSpacing(14);

        // 无喇叭基础形态（进度式滑块）
        auto *plain = new BaSlider(this);
        plain->setVolumeIcons(false);
        plain->setValue(45);
        layout->addWidget(plain);

        // 禁用形态
        auto *disabled = new BaSlider(this);
        disabled->setVolumeIcons(false);
        disabled->setValue(70);
        disabled->setEnabled(false);
        layout->addWidget(disabled);

        layout->addStretch();
        setMinimumSize(760, 420);
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    const QStringList args = app.arguments();
    const int idx = args.indexOf("--screenshot");
    const QString shotPath = idx > 0 ? args.value(idx + 1) : QString();

    QMainWindow win;
    win.setWindowTitle("BaSlider Preview");
    win.resize(760, 420);
    win.setCentralWidget(new SliderPreview(&win));

    if (!shotPath.isEmpty()) {
        QTimer::singleShot(900, &win, [&] {
            win.grab().save(shotPath);
            app.quit();
        });
    }
    win.show();
    return app.exec();
}
