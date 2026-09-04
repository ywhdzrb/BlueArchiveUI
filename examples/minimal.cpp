// blue_archive_ui 最小示例：玻璃面板 + MD3/玻璃控件混合表单。
// 运行：cmake -B build && cmake --build build && ./build/blue_archive_ui_minimal

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCommandLineParser>
#include <QTimer>
#include <QLineEdit>
#include <QPushButton>
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>

#include "blue_archive_ui.h"

// 演示页：顶部按钮行（主题切换 / 莫奈取色）+ 玻璃面板 + 玻璃控件行 + 普通按钮
class Demo : public QWidget
{
public:
    // bgPath: 外部背景图（可选），为空则用程序生成的霓虹渐变装饰图
    explicit Demo(const QString &bgPath = QString())
    {
        dark_ = true;
        theme_ = Md3Theme::dark();
        bgDark_ = bgPath.isEmpty() ? makeArtwork(true) : loadScaled(bgPath);
        bgLight_ = bgPath.isEmpty() ? makeArtwork(false) : loadScaled(bgPath);

        auto *lay = new QVBoxLayout(this);
        lay->setContentsMargins(36, 28, 36, 28);
        lay->setSpacing(18);

        auto *row = new QHBoxLayout;
        auto *toggleBtn = new Md3Button("切换主题");
        auto *monetBtn = new Md3Button("莫奈取色", this);
        monetBtn->setStyle(Md3Button::Style::Outlined);
        auto *dialogBtn = new Md3Button("对话框", this);
        dialogBtn->setStyle(Md3Button::Style::Text);
        row->addWidget(toggleBtn);
        row->addWidget(monetBtn);
        row->addWidget(dialogBtn);
        row->addStretch();
        lay->addLayout(row);

        // 玻璃面板：自动模糊窗口背景（GL 可用时离屏渲染，否则 CPU 软渲染）
        auto *glass = new LiquidGlassPanel;
        glass->setMinimumSize(360, 180);
        auto *gl = new QVBoxLayout(glass);
        gl->setContentsMargins(32, 24, 32, 24);
        gl->setSpacing(12);
        auto *glTitle = new QLabel("玻璃面板");
        QFont tf = glTitle->font();
        tf.setBold(true);
        glTitle->setFont(tf);
        auto *glDesc = new QLabel("磨砂窗口背景 + 半透明着色\n拖动下方滑块可实时调磨砂强度");
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
        sliderRow->addWidget(new QLabel("磨砂强度"));
        sliderRow->addWidget(glSlider, 1);
        sliderRow->addWidget(refractLabel);
        gl->addWidget(glTitle);
        gl->addWidget(glDesc);
        gl->addLayout(sliderRow);
        gl->addStretch();
        lay->addWidget(glass, 1);

        // 玻璃控件行：开关 / 进度条 / 玻璃按钮 / 玻璃 FAB / 玻璃下拉
        auto *glassRow = new QHBoxLayout;
        auto *sw = new LiquidGlassSwitch;
        auto *bar = new LiquidGlassProgressBar;
        bar->setRange(0, 100);
        bar->setValue(64);
        auto *gBtn = new LiquidGlassButton("玻璃按钮");
        auto *gFab = new LiquidGlassFab(md3::Glyph::Plus);
        auto *gDrop = new LiquidGlassDropdown({"选项 A", "选项 B", "选项 C"},
                                              "选择一个选项");
        glassRow->addWidget(new QLabel("开关"));
        glassRow->addWidget(sw);
        glassRow->addWidget(new QLabel("进度"));
        glassRow->addWidget(bar, 1);
        glassRow->addWidget(gBtn);
        glassRow->addWidget(gFab);
        lay->addLayout(glassRow);
        lay->addWidget(gDrop);

        // 玻璃输入框行：前后缀图标 + 聚焦指示线
        auto *gTextRow = new QHBoxLayout;
        auto *gText = new LiquidGlassTextField("玻璃输入框（试试聚焦）");
        gText->setTrailingIcon(md3::Glyph::Search);
        auto *gText2 = new LiquidGlassTextField("带前置图标");
        gText2->setLeadingIcon(md3::Glyph::Info);
        gTextRow->addWidget(gText, 1);
        gTextRow->addWidget(gText2, 1);
        lay->addLayout(gTextRow);

        // MD3 新增组件行：FAB + 角标 / 芯片 / 复选框 / 单选 / 分段按钮
        auto *md3Row1 = new QHBoxLayout;
        auto *fab = new Md3Fab(md3::Glyph::Plus);
        auto *badge = new Md3Badge;
        badge->attachTo(fab);
        badge->setCount(7);
        auto *chip = new Md3Chip("筛选芯片");
        chip->setStyle(Md3Chip::Style::Filter);
        auto *chip2 = new Md3Chip("辅助");
        chip2->setStyle(Md3Chip::Style::Assist);
        auto *check = new Md3CheckBox;
        check->setText("同意条款");
        auto *radio = new Md3RadioButton;
        radio->setText("选项一");
        auto *radio2 = new Md3RadioButton;
        radio2->setText("选项二");
        auto *seg = new Md3SegmentedButton;
        seg->addSegment("日");
        seg->addSegment("周");
        seg->addSegment("月", md3::Glyph::Drop);
        md3Row1->addWidget(new QLabel("FAB·角标"));
        md3Row1->addWidget(fab);
        md3Row1->addWidget(chip);
        md3Row1->addWidget(chip2);
        md3Row1->addWidget(check);
        md3Row1->addWidget(radio);
        md3Row1->addWidget(radio2);
        md3Row1->addWidget(seg);
        md3Row1->addStretch();
        lay->addLayout(md3Row1);

        // MD3 按钮图标行：带图标按钮 / 图标按钮普通行
        auto *md3Row2 = new QHBoxLayout;
        auto *btnIcon = new Md3Button("搜索", this);
        btnIcon->setIcon(md3::Glyph::Search);
        auto *btnIcon2 = new Md3Button("收藏", this);
        btnIcon2->setStyle(Md3Button::Style::FilledTonal);
        btnIcon2->setIcon(md3::Glyph::Star);
        auto *btnIcon3 = new Md3Button("关闭", this);
        btnIcon3->setStyle(Md3Button::Style::Outlined);
        btnIcon3->setIcon(md3::Glyph::Close);
        md3Row2->addWidget(new QLabel("图标按钮"));
        md3Row2->addWidget(btnIcon);
        md3Row2->addWidget(btnIcon2);
        md3Row2->addWidget(btnIcon3);
        md3Row2->addStretch();
        lay->addLayout(md3Row2);

        // MD3 输入框增强行：前后缀图标 + 错误态 + helper
        auto *md3Row3 = new QHBoxLayout;
        auto *tfA = new Md3TextField("带前后缀的输入框");
        tfA->setLeadingIcon(md3::Glyph::Search);
        tfA->setTrailingIcon(md3::Glyph::Close);
        tfA->setTrailingIconClicked([tfA] { tfA->setText(QString()); });
        auto *tfB = new Md3TextField("错误态示例");
        tfB->setError("错误：内容无效");
        md3Row3->addWidget(tfA, 1);
        md3Row3->addWidget(tfB, 1);
        lay->addLayout(md3Row3);

        // 对话框：打开演示
        connect(dialogBtn, &QAbstractButton::clicked, this, [this] {
            Md3Dialog d(this);
            d.setTitle("确认操作");
            d.setBody("即将执行不可逆的操作，是否继续？");
            d.addAction("取消", [&d] { d.reject(); }, Md3Button::Style::Text);
            d.addAction("确认", [&d] { d.accept(); }, Md3Button::Style::Filled);
            d.showDialog();
        });

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
        // 统一应用主题：Md3Xxx 控件（Reflection-driven 遍历不便枚举，
        // 这里按类逐个遍历；新组件都实现 setTheme）
        for (auto *b : findChildren<Md3Button *>())
            b->setTheme(theme_);
        for (auto *b : findChildren<Md3Fab *>())
            b->setTheme(theme_);
        for (auto *b : findChildren<Md3Badge *>())
            b->setTheme(theme_);
        for (auto *b : findChildren<Md3Chip *>())
            b->setTheme(theme_);
        for (auto *b : findChildren<Md3CheckBox *>())
            b->setTheme(theme_);
        for (auto *b : findChildren<Md3RadioButton *>())
            b->setTheme(theme_);
        for (auto *b : findChildren<Md3SegmentedButton *>())
            b->setTheme(theme_);
        for (auto *b : findChildren<Md3Dropdown *>())
            b->setTheme(theme_);
        for (auto *b : findChildren<Md3TextField *>())
            b->setTheme(theme_);
        for (auto *b : findChildren<Md3Slider *>())
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

    // 绘制窗口背景图（玻璃控件抓帧时会折射到它）
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        const QPixmap &bg = dark_ ? bgDark_ : bgLight_;
        if (!bg.isNull())
            p.drawPixmap(rect(), bg, bg.rect());
        // 主题色遮罩：压暗/提亮壁纸（模拟 iOS 26 桌面亮度补偿），
        // 使玻璃折射的观感与当前主题明暗一致
        QColor veil = theme_.background;
        veil.setAlphaF(0.30);
        p.fillRect(rect(), veil);
    }

    // 程序生成的霓虹渐变装饰图（无外部图片时的兜底背景）
    static QPixmap makeArtwork(bool dark)
    {
        const QSize size(1600, 1000);
        QPixmap pm(size);
        pm.fill(dark ? QColor(0x0d, 0x11, 0x26) : QColor(0xea, 0xee, 0xfa));
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);

        // 底色对角渐变：暗色深蓝紫，亮色浅蓝
        QLinearGradient g(0, 0, size.width(), size.height());
        if (dark) {
            g.setColorAt(0.0, QColor(0x12, 0x19, 0x3a));
            g.setColorAt(0.55, QColor(0x1a, 0x14, 0x38));
            g.setColorAt(1.0, QColor(0x0d, 0x11, 0x26));
        } else {
            g.setColorAt(0.0, QColor(0xe3, 0xe9, 0xfa));
            g.setColorAt(0.55, QColor(0xee, 0xe4, 0xf5));
            g.setColorAt(1.0, QColor(0xe9, 0xee, 0xfa));
        }
        p.fillRect(pm.rect(), g);

        // 霓虹光斑：径向渐变圆提供折射可观察的低频色差
        const struct { QPointF c; qreal r; QColor col; } blobs[] = {
            {{260, 200}, 620, QColor(0x2e, 0x6b, 0xff)},
            {{1350, 150}, 520, QColor(0x8b, 0x5c, 0xf6)},
            {{165, 880}, 560, QColor(0xec, 0x48, 0x99)},
            {{1430, 760}, 500, QColor(0x06, 0xb6, 0xd4)},
            {{820, 480}, 700, QColor(0x4a, 0x6b, 0xdd)},
        };
        for (const auto &b : blobs) {
            QRadialGradient rg(b.c, b.r);
            QColor c = b.col;
            const qreal a = dark ? 0.46 : 0.22;
            rg.setColorAt(0.0, liftAlpha(c, a));
            rg.setColorAt(0.55, liftAlpha(c, a * 0.45));
            rg.setColorAt(1.0, liftAlpha(c, 0.0));
            p.fillRect(pm.rect(), rg);
        }

        // 白色描边圆环：为玻璃折射提供高频细节参照
        p.setPen(QPen(QColor(255, 255, 255, dark ? 52 : 96), 3));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(370, 300), 190, 190);
        p.drawEllipse(QPointF(1240, 250), 120, 120);
        p.drawEllipse(QPointF(1100, 780), 230, 230);
        p.drawEllipse(QPointF(240, 820), 90, 90);
        return pm;
    }

    // 提高 alpha 而不改变色相：光斑颜色取自透明色叠加前
    static QColor liftAlpha(const QColor &c, qreal a)
    {
        QColor r = c;
        r.setAlphaF(a);
        return r;
    }

    // 外部图片加载：按 1600 宽等比预缩放，避免每帧对超大图重采样
    static QPixmap loadScaled(const QString &path)
    {
        QImage img(path);
        if (img.isNull()) {
            qWarning("背景图 %s 加载失败，使用内置装饰图", qPrintable(path));
            return {};
        }
        if (img.width() > 1600)
            img = img.scaledToWidth(1600, Qt::SmoothTransformation);
        return QPixmap::fromImage(img);
    }

    Md3Theme theme_;
    bool dark_ = true;
    QPixmap bgDark_;
    QPixmap bgLight_;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    QCommandLineParser parser;
    parser.addOptions({
        {{"s", "screenshot"}, "截图保存到 <file> 后退出", "file"},
        {{"l", "light"}, "以亮色主题启动"},
        {{"b", "background"}, "显示指定的背景图片 <file>", "file"},
    });
    parser.process(app);

    // 外部背景图路径：空则用内置装饰图
    Demo w(parser.value("background"));
    if (parser.isSet("light"))
        w.toggleTheme();
    w.resize(820, 760);
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
