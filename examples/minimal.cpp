// liquid_kit 最小示例：玻璃面板 + MD3/玻璃控件混合表单。
// 运行：cmake -B build && cmake --build build && ./build/liquid_kit_minimal

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCommandLineParser>
#include <QTimer>

#include "liquid_kit.h"

// 演示页：顶部按钮行（主题切换 / 莫奈取色）+ 玻璃面板 + 玻璃控件行 + 普通按钮
class Demo : public QWidget
{
public:
    Demo()
    {
        dark_ = true;
        theme_ = Md3Theme::dark();

        auto *lay = new QVBoxLayout(this);
        lay->setContentsMargins(36, 28, 36, 28);
        lay->setSpacing(18);

        auto *row = new QHBoxLayout;
        auto *toggleBtn = new Md3Button("切换主题");
        auto *monetBtn = new Md3Button("莫奈取色", this);
        monetBtn->setStyle(Md3Button::Style::Outlined);
        row->addWidget(toggleBtn);
        row->addWidget(monetBtn);
        row->addStretch();
        lay->addLayout(row);

        // 玻璃面板：自动折射窗口背景（GL 可用时离屏渲染，否则 CPU 软渲染）
        auto *glass = new LiquidGlassPanel;
        glass->setMinimumSize(360, 220);
        auto *gl = new QVBoxLayout(glass);
        gl->setContentsMargins(32, 28, 32, 28);
        gl->setSpacing(12);
        auto *glTitle = new QLabel("玻璃面板");
        QFont tf = glTitle->font();
        tf.setBold(true);
        glTitle->setFont(tf);
        auto *glDesc = new QLabel("折射窗口背景 + 半透明着色 + 中心透光\n拖动下方滑块可实时调折射强度");
        glDesc->setWordWrap(true);
        auto *glSlider = new LiquidGlassSlider;
        glSlider->setRange(0, 200);
        glSlider->setValue(100);
        auto *refractLabel = new QLabel("0.100");
        refractLabel->setMinimumWidth(44);
        refractLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        connect(glSlider, &LiquidGlassSlider::valueChanged, this, [=](int v) {
            glass->setRefraction(v / 1000.0);
            refractLabel->setText(QString::number(v / 1000.0, 'f', 3));
        });
        auto *sliderRow = new QHBoxLayout;
        sliderRow->addWidget(new QLabel("折射强度"));
        sliderRow->addWidget(glSlider, 1);
        sliderRow->addWidget(refractLabel);
        gl->addWidget(glTitle);
        gl->addWidget(glDesc);
        gl->addLayout(sliderRow);
        gl->addStretch();
        lay->addWidget(glass, 1);

        // 玻璃控件行：开关 / 进度条 / 玻璃按钮
        auto *glassRow = new QHBoxLayout;
        auto *sw = new LiquidGlassSwitch;
        auto *bar = new LiquidGlassProgressBar;
        bar->setRange(0, 100);
        bar->setValue(64);
        auto *gBtn = new LiquidGlassButton("玻璃按钮");
        glassRow->addWidget(new QLabel("开关"));
        glassRow->addWidget(sw);
        glassRow->addWidget(new QLabel("进度"));
        glassRow->addWidget(bar, 1);
        glassRow->addWidget(gBtn);
        lay->addLayout(glassRow);

        applyTheme();

        connect(toggleBtn, &QAbstractButton::clicked, this, [this] {
            toggleTheme();
        });
        connect(monetBtn, &QAbstractButton::clicked, this, [this] {
            applyMonetSeed(QColor("#6750A4"));   // 示例种子色，正式可用 extractMonetSeed(pixmap)
        });
    }

    void toggleTheme()
    {
        dark_ = !dark_;
        theme_ = dark_ ? Md3Theme::dark() : Md3Theme::light();
        applyTheme();
    }

    // 莫奈取色生成整套主题并应用（setTheme + 玻璃重抓帧）
    void applyMonetSeed(const QColor &seed)
    {
        theme_ = makeMonetTheme(seed, dark_);
        applyTheme();
    }

private:
    void applyTheme()
    {
        for (auto *b : findChildren<Md3Button *>())
            b->setTheme(theme_);
        for (auto *k : findChildren<LiquidGlassThemeKeeper *>()) {
            k->setTheme(theme_);
            k->applyGlassStyle(dark_);
            k->refreshBackdrop();   // 主题/背景变化后强制重抓窗口快照
        }
        for (auto *p : findChildren<LiquidGlassPanel *>()) {
            p->applyGlassStyle(dark_);
            p->refreshBackdrop();
        }
        auto pal = palette();
        pal.setColor(QPalette::Window, theme_.background);
        pal.setColor(QPalette::WindowText, theme_.onBackground);
        setPalette(pal);
    }

    Md3Theme theme_;
    bool dark_ = true;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    QCommandLineParser parser;
    parser.addOptions({
        {{"s", "screenshot"}, "截图保存到 <file> 后退出", "file"},
        {{"l", "light"}, "以亮色主题启动"},
    });
    parser.process(app);

    Demo w;
    if (parser.isSet("light"))
        w.toggleTheme();
    w.resize(800, 560);
    w.show();

    if (parser.isSet("screenshot")) {
        QTimer::singleShot(1200, &app, [&] {
            // 窗口 resize 稳定后强制重抓一次玻璃材质：
            // Hyprland 平铺下窗口尺寸异步变化，末次 resize 事件可能被
            // 抓帧限流跳过而滞留旧几何（玻璃板折射出旧布局文字残留）
            for (auto *k : w.findChildren<LiquidGlassThemeKeeper *>())
                k->refreshBackdrop();
            for (auto *p : w.findChildren<LiquidGlassPanel *>())
                p->refreshBackdrop();
            const QPixmap shot = w.grab();
            shot.save(parser.value("screenshot"));
            app.exit(0);
        });
    }

    return app.exec();
}
