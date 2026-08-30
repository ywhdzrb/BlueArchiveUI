#pragma once

#include <QWidget>
#include <QAbstractButton>
#include <QVariantAnimation>
#include <QImage>
#include "md3_theme.h"

// ---------------------------------------------------------------------------
// 液态玻璃控件集：玻璃材质的 md3 风格控件（开关 / 滑块 / 进度条 / 卡片）。
// 与 md3 原版**几何、尺寸、交互、语义完全一致**（52x32 开关、48 高滑块、
// 16px 胶囊轨道、44px 竖条 thumb、4px 进度条、12px 圆角卡片），
// 只是把「纯色填充」换成「玻璃材质」——背景经窗口快照折射 + 半透明着色 +
// 中心透光，色调仍以 Md3Theme 的角色色为主题。
// ---------------------------------------------------------------------------

// 玻璃基元：窗口快照抓取 + 主题/明暗状态 + 主题色玻璃帧存放。
// 所有玻璃控件公共逻辑（抓帧、重抓时机），小控件从它派生。
// 基类直接继承 QAbstractButton：同一套接口覆盖 checkable（开关）
// 与非按钮类控件（滑块/进度条/卡片，默认不可选中）。
class LiquidGlassThemeKeeper : public QAbstractButton
{
    Q_OBJECT

public:
    LiquidGlassThemeKeeper(QWidget *parent = nullptr);

    void setTheme(const Md3Theme &theme) { theme_ = theme; update(); }
    void applyGlassStyle(bool dark) { dark_ = dark; update(); }
    // 主题/背景/页面切换后手动刷新背景快照（供页面层遍历调用，强制：绕过节流）
    void refreshBackdrop() { grabBackdrop(/*force=*/true); }
    bool isDark() const { return dark_; }
    Md3Theme theme() const { return theme_; }

protected:
    // 抓取窗口快照（隐藏所有玻璃控件 + 遮罩），存 backdrop_，供子类渲染
    virtual void grabBackdrop(bool force = false);
    // show/resize 后的异步抓帧入口：layout pass 尚未落定，直接抓会抓到
    // 旧布局残影（玻璃板里折射出“自己”），用 zero-timer 延后再抓一次
    void backdropDeferred();
    // 本控件在窗口坐标系中的矩形（折射基线 & 抓帧判定）
    QRect mappedPanelRect() const;
    // 本控件要渲染的玻璃板：尺寸不满窗口时取自身矩形 + 参数化材质。
    // 取景矩形默认为控件自身矩形；轨道类（slider/progress）传轨道矩形，
    // 让小尺寸玻璃的倒角折射带相对轨道自身有足够占比
    QImage renderGlassPlate(qreal corner, qreal k, qreal edgeK, qreal glowMul = 1.0,
                            const QRect &viewRect = QRect());

protected:
    Md3Theme theme_;
    bool dark_ = false;
    QImage backdrop_;                // 全窗快照（RGBA8888）
    QSize backdropWinSize_;          // 快照时的窗口几何（变化即素材失效）
    qint64 lastGrabbedMs_ = 0;       // 实例级抓帧节流时间戳
    bool grabInProgress_ = false;    // 抓帧防重入（hide/show 触发 resize 再入）
};

// ---------------------------------------------------------------------------
// 玻璃开关：52x32 track + 动画 thumb，尺寸/动画与 md3_switch 完全一致
// ---------------------------------------------------------------------------
class LiquidGlassSwitch : public LiquidGlassThemeKeeper
{
    Q_OBJECT

public:
    explicit LiquidGlassSwitch(QWidget *parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void animateTo(bool checked);

    qreal progress_ = 0.0;       // 0=关闭、1=开启
    QVariantAnimation anim_;
};

// ---------------------------------------------------------------------------
// 玻璃滑块：几何与 md3_slider 完全一致（16px 胶囊轨道 / 44px 竖条 thumb /
// 10px 端距 / 48 高），轨道为玻璃材质，active 段/thumb 用主题色。
// ---------------------------------------------------------------------------
class LiquidGlassSlider : public LiquidGlassThemeKeeper
{
    Q_OBJECT

public:
    explicit LiquidGlassSlider(QWidget *parent = nullptr);

    void setRange(int min, int max);
    void setValue(int value);
    int value() const { return value_; }

    QSize sizeHint() const override;

signals:
    void valueChanged(int value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setValueFromPos(int x);

    int min_ = 0;
    int max_ = 100;
    int value_ = 50;
    qreal displayValue_ = 50.0;
    bool pressed_ = false;
    QVariantAnimation anim_;
};

// ---------------------------------------------------------------------------
// 玻璃进度条：几何与 md3_progress_bar 一致（4px 高 / 胶囊圆角），
// track 为玻璃材质，指示条用主题 primary。
// ---------------------------------------------------------------------------
class LiquidGlassProgressBar : public LiquidGlassThemeKeeper
{
    Q_OBJECT

public:
    explicit LiquidGlassProgressBar(QWidget *parent = nullptr);

    void setRange(int min, int max);
    void setValue(int value);
    int value() const { return value_; }
    void setIndeterminate(bool indeterminate);
    bool isIndeterminate() const { return indeterminate_; }

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    int min_ = 0;
    int max_ = 100;
    int value_ = 0;
    bool indeterminate_ = false;
    qreal slide_ = 0.0;          // 不确定模式动画相位
    QVariantAnimation anim_;
};

// ---------------------------------------------------------------------------
// 玻璃卡片：圆角 12px + hover 状态层，几何与 md3_card 一致；背景玻璃材质。
// 内容由 setContent 挂载（16px 内边距）。
// ---------------------------------------------------------------------------
class LiquidGlassCard : public LiquidGlassThemeKeeper
{
    Q_OBJECT

public:
    explicit LiquidGlassCard(QWidget *parent = nullptr);

    void setContent(QWidget *content);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    bool hovered_ = false;
    qreal hoverAlpha_ = 0.0;
    QVariantAnimation hoverAnim_;
};

// ---------------------------------------------------------------------------
// 玻璃导航栏：iOS 26 悬浮胶囊玻璃条（64 高 / 每项 68 宽 / 整条不拉伸），
// 整体为一长条玻璃板（折射窗口背景）。项图标 + label，选中项
// 「玻璃泡」= 二次玻璃材质（折射背景 + secondaryContainer 着色 + 高光），
// 切换时玻璃泡沿条滑动（250ms 动画）。
// ---------------------------------------------------------------------------
class LiquidGlassNavigationBar : public LiquidGlassThemeKeeper
{
    Q_OBJECT

public:
    // 内置线性图标，与 Md3SideBar 同一组（独立枚举避免类间依赖）
    enum class Glyph {
        Home,     // 首页
        Search,   // 搜索
        Star,     // 收藏
        Person,   // 我的
        Palette,  // 调色板
        Drop,     // 水滴（液态玻璃）
    };
    Q_ENUM(Glyph)

    explicit LiquidGlassNavigationBar(QWidget *parent = nullptr);

    // 追加导航项，返回索引
    int addItem(const QString &label, Glyph glyph);
    void clearItems();

    int currentIndex() const { return currentIndex_; }
    void setCurrentIndex(int index);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    void currentIndexChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    // 选中玻璃泡的绘制（材质 = 玻璃板 + 着色 + 高光带 + 底暗缘）
    void paintSelectedBubble(QPainter &p, const QRectF &bubble);

    // 在 (cx, cy) 处绘制 24x24 基准线性图标（与 md3_side_bar 一致）
    void paintGlyph(QPainter &p, Glyph glyph, qreal cx, qreal cy, const QColor &color) const;

    struct Item {
        QString label;
        Glyph glyph = Glyph::Home;
    };

    QVector<Item> items_;
    int currentIndex_ = -1;
    int hoverIndex_ = -1;
    qreal floatIndex_ = -1.0;    // 玻璃泡滑动动画中的连续索引
    QVariantAnimation bubbleAnim_;
};
