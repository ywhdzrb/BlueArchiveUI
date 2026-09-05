#pragma once

#include <QWidget>
#include <QAbstractButton>
#include <QVariantAnimation>
#include <QImage>
#include <QStringList>
#include <optional>
#include <functional>
#include "md3_theme.h"
#include "md3_icon.h"

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
    QImage frosted_;                 // 模糊后快照（磨砂采样源，随 backdrop_ 更新）
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
    // 图标枚举直接取公共图标库，与 Md3SideBar 共用一份实现
    using Glyph = md3::Glyph;

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

class QLineEdit;
class LiquidGlassDropdown;

// ---------------------------------------------------------------------------
// 玻璃弹出菜单：与 Md3MenuPopup 同构（surface 底 / 4px 圆角 / hover 高亮 /
// 选中对勾），但独立窗口抓不到背景快照，菜单体直接画半透磨砂底色，
// 不参与玻璃折射（折射仅在宿主窗口内生效）。
// ---------------------------------------------------------------------------
class LiquidGlassMenuPopup : public QWidget
{
    Q_OBJECT

public:
    explicit LiquidGlassMenuPopup(LiquidGlassDropdown *owner);

    void setTheme(const Md3Theme &theme);
    void setItems(const QStringList &items);
    void setSelectedIndex(int index);
    QSize popupSize() const;

signals:
    void itemClicked(int index);
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    int itemAt(const QPoint &pos) const;

    LiquidGlassDropdown *owner_ = nullptr;
    Md3Theme theme_;
    QStringList items_;
    int selectedIndex_ = -1;
    int hoverIndex_ = -1;
};

// ---------------------------------------------------------------------------
// 玻璃输入框：几何/交互与 md3_text_field 一致（56 高 / 顶部圆角 4 / 底部
// 指示线 / 前后缀图标），背景改为玻璃材质（折射窗口背景 + 主题着色）。
// 内部 QLineEdit 透明叠加，聚焦驱动指示线渐变（150ms）。
// ---------------------------------------------------------------------------
class LiquidGlassTextField : public LiquidGlassThemeKeeper
{
    Q_OBJECT

public:
    explicit LiquidGlassTextField(const QString &placeholder = QString(),
                                  QWidget *parent = nullptr);

    void setTheme(const Md3Theme &theme);

    QString text() const;
    void setText(const QString &text);
    void setPlaceholderText(const QString &placeholder);

    // 前后缀图标：传入 std::nullopt 表示移除
    void setLeadingIcon(std::optional<md3::Glyph> glyph);
    void setTrailingIcon(std::optional<md3::Glyph> glyph);

    // 后置图标点击回调
    void setTrailingIconClicked(std::function<void()> callback);

    QSize sizeHint() const override;

signals:
    void trailingIconClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    QRectF textFieldRect() const;

    QLineEdit *editor_ = nullptr;
    std::optional<md3::Glyph> leadingGlyph_;
    std::optional<md3::Glyph> trailingGlyph_;
    std::function<void()> trailingCallback_;
    qreal focusProgress_ = 0.0;   // 聚焦过渡进度（驱动指示线渐变）
    QVariantAnimation anim_;
};

// ---------------------------------------------------------------------------
// 玻璃浮动按钮：几何与 md3_fab 一致（Regular 56x56 / Small 40x40，大号
// 文字版暂不提供），圆形玻璃板 + 主题色 tint + 顶部高光，图标 on-primary。
// 替代纯色胶囊：点击即 QAbstractButton 语义，直接复用 clicked() 信号。
// ---------------------------------------------------------------------------
class LiquidGlassFab : public LiquidGlassThemeKeeper
{
    Q_OBJECT

public:
    enum class Size {
        Regular,   // 56x56，默认
        Small      // 40x40，小 FAB
    };
    Q_ENUM(Size)

    explicit LiquidGlassFab(md3::Glyph glyph = md3::Glyph::Plus,
                            QWidget *parent = nullptr);

    void setGlyph(md3::Glyph glyph);
    void setSize(Size size);
    void setTonal(bool tonal);   // true 用 secondary-container 色调

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    qreal targetStateAlpha() const
    {
        return (hovered_ || isDown()) ? hkHoverAlpha : 0.0;
    }

    Md3Theme theme_;
    md3::Glyph glyph_ = md3::Glyph::Plus;
    Size size_ = Size::Regular;
    bool tonal_ = false;
    bool hovered_ = false;
    qreal stateAlpha_ = 0.0;     // 状态层当前透明度（百分比）
    QVariantAnimation stateAnim_;
    static constexpr qreal hkHoverAlpha = 12.0;  // hover/按压状态层透明度
};

// ---------------------------------------------------------------------------
// 玻璃下拉选择框：几何/交互与 md3_dropdown 一致（56 高 / 顶部圆角 4 /
// 底部指示线 / 右侧下拉箭头），背景为玻璃材质。菜单用 LiquidGlassMenuPopup。
// ---------------------------------------------------------------------------
class LiquidGlassDropdown : public LiquidGlassThemeKeeper
{
    Q_OBJECT

public:
    explicit LiquidGlassDropdown(const QStringList &items = QStringList(),
                                 const QString &placeholder = QString(),
                                 QWidget *parent = nullptr);
    ~LiquidGlassDropdown() override;

    void setItems(const QStringList &items);
    void setPlaceholderText(const QString &placeholder);

    int currentIndex() const { return currentIndex_; }
    void setCurrentIndex(int index);
    QString currentText() const;

    QSize sizeHint() const override;

signals:
    void currentIndexChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void animateActive(bool active);
    void showPopup();
    void onItemClicked(int index);
    void onPopupClosed();

    Md3Theme theme_;
    QStringList items_;
    QString placeholder_;
    int currentIndex_ = -1;
    bool hovered_ = false;
    bool menuOpen_ = false;
    qreal activeProgress_ = 0.0;   // hover / 展开过渡进度，驱动指示线与箭头
    QVariantAnimation anim_;
    LiquidGlassMenuPopup *popup_ = nullptr;
};
