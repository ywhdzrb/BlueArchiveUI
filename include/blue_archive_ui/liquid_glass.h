#pragma once

#include <QWidget>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QAbstractButton>
#include <QPixmap>
#include <QImage>
#include <QSurfaceFormat>
#include "liquid_glass_widgets.h"   // LiquidGlassThemeKeeper（按钮基类）

// 磨砂玻璃（Frosted Glass）风格控件集：
// 以 iOS 磨砂观感做简化实现——背景模糊 + 半透明着色（无折射变形、无中心透光）。
// 包含两个控件：
//  - LiquidGlassPanel：玻璃面板。渲染架构参考 OverShifted/LiquidGlass（GLFW 独立
//    程序）：自建离屏 QOpenGLContext + FBO 渲染玻璃帧为 QImage，paintEvent 再画
//    出来。刻意不使用 QOpenGLWidget——Qt 平台性 GL 集成在 NVIDIA + Wayland +
//    EGL 等组合下创建上下文失败（3009 = EGL_BAD_MATCH），且失败后表现为面板黑屏
//    且每次重绘反复重试。离屏上下文创建失败则整条通道停用、改走 CPU 软渲染，
//    GL 与 CPU 两条路径同样将内容绘制进 paintEvent，面板永不黑屏。
//  - LiquidGlassButton：玻璃按钮，纯着色材质（不带背景折射，供浮于面板/页面使用）

// GL 探测（供 LiquidGlassPanel 与 main 使用）：
// 部分平台（NVIDIA 驱动 + Wayland EGL）创建 QOpenGLContext 会失败
// （错误码 3009 = EGL_BAD_MATCH，社区大量同型报告，属驱动/平台问题，
// 非应用 bug）。探测结果在首次调用时静默确定并缓存：
//  - probeGlFormat()：返回探测选定的可用 surface format（从未探测则先探测）
//  - isGlAvailable() ：全候选探测均不可用时返回 false（此时玻璃面板改用 CPU 软渲染）
QSurfaceFormat probeGlFormat();
bool isGlAvailable();

// ---- 供磨砂版 md3 控件（switch/slider/progress/card）复用的公共玻璃材质函数 ----

// 抓取窗口快照作为玻璃材质源。self 为抓帧者（玻璃控件），w 为顶层窗口：
// 抓帧期间隐藏所有带 lgGlass / md3FadeOverlay 属性的可见控件（玻璃控件
// 连同自身、主题过渡遮罩），w->render 后恢复。返回值 RGBA8888 全窗快照。
QImage grabGlassBackdrop(const QWidget *self, QWidget *w);

// 对窗口快照做两遍滑动窗口盒式模糊（磨砂素材源）。src 为 RGBA8888 快照，
// radius 为模糊半径（px）。返回同尺寸模糊图，供 GL/CPU 磨砂采样。
QImage frostedBackdrop(const QImage &src, int radius);

// 纯 CPU 玻璃板渲染：输出 size 大小的磨砂板（圆角 AA / 背景模糊采样 /
// tint 着色，公式与 GPU 片段着色器一致）。backdrop 为**已模糊**的窗口快照
// （frostedBackdrop 产物），panelRect 为本控件在窗口坐标系中的矩形。
// 磨砂版中 k / edgeK / glowMul 已弃用（保留仅为兼容旧调用），
// corner 为圆角半径 px。返回 Format_ARGB32。
QImage renderGlassPlateCPU(const QImage &backdrop, const QRect &panelRect,
                           const QSize &size, qreal corner, qreal k, qreal edgeK,
                           bool dark, qreal glowMul = 1.0);

// 玻璃面板：如实折射背景材质。showEvent / resizeEvent 时自动抓取窗口快照，
// 界面背景变动后可手动调用 refreshBackdrop() 更新。玻璃帧由离屏 GL 上下文
// 渲染为 QImage（可用时）或 CPU 软渲染管线绘制，paintEvent 统一画出。
class LiquidGlassPanel : public QWidget
{
    Q_OBJECT

public:
    explicit LiquidGlassPanel(QWidget *parent = nullptr);
    ~LiquidGlassPanel() override;

    // 切换亮 / 暗玻璃风格（着色、透光强度随之变化）
    void applyGlassStyle(bool dark);
    // 重新抓取窗口背景并刷新玻璃材质（主题切换、背景变动后调用）
    void refreshBackdrop();

    // 磨砂强度（模糊半径系数），默认 0.10，范围 0~0.2。
    // 兼容旧几何：改名后内部映射为模糊半径（clamp 后）。
    void setRefraction(qreal k);
    // 面板圆角半径，默认 24
    void setCornerRadius(qreal radius);

    // 是否经由 GPU 通道渲染玻璃帧（离屏上下文创建成功与否）。
    // CPU 软渲染与 GPU 均画进 paintEvent，仅用于诊断/日志。
    bool isGpuRendering() const { return glOk_; }

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void grabBackdrop();
    // paint 循环内严禁直接抓帧（hide/show + render 在绘画期间递归 repaint），
    // 此函数用 zero-timer 延后调度一次抓帧（内部 300ms 限流防风暴）
    void scheduleBackdropGrab();
    bool ensureGlResources();     // 离屏上下文 + shader + 纹理懒初始化
    void renderGlFrame();         // 离屏 FBO 渲染玻璃帧到 glFrame_（失败降级 CPU）
    void renderSoftwareFrame();   // CPU 软渲染玻璃帧到 glFrame_

    bool glOk_ = true;                // GL 通道可用；context 创建失败置 false
    QImage backdrop_;                 // 抓到的窗口快照（grabBackdrop 产物）
    QImage frosted_;                  // 模糊后快照（磨砂采样源，随 backdrop_ 更新）
    QSize backdropWinSize_;           // 快照时的窗口几何（变化即素材失效）
    QImage glFrame_;                  // 渲染产物（GL 或 CPU 管线生成）
    bool backdropDirty_ = false;      // 快照已更新、glFrame_ 待重渲染
    bool glResourcesReady_ = false;   // 离屏 GL 资源（shader/VAO 等）已初始化
    bool hasTexture_ = false;         // 纹理已创建且非空

    QOpenGLContext glContext_;        // 离屏渲染上下文（与 Qt Widgets 平台无关）
    QOffscreenSurface glSurface_;     // 离屏上下文挂载的伪 surface
    QOpenGLFramebufferObject *fbo_ = nullptr;   // 离屏 FBO（按面板尺寸惰性重建）
    QOpenGLShaderProgram program_;
    QOpenGLTexture bgTexture_{QOpenGLTexture::Target2D};
    QOpenGLBuffer vertexBuffer_{QOpenGLBuffer::VertexBuffer};
    QOpenGLVertexArrayObject vao_;

    bool backdropGrabbing_ = false;   // 抓图期间防重入：render 同步会回调 resizeEvent
    qreal refractionK_ = 0.10;        // 磨砂强度（模糊半径系数，0~0.2）
    qreal edgeRefractK_ = 0.18;       // 保留兼容（磨砂版弃用）
    qreal cornerRadius_ = 24.0;
    bool dark_ = false;

    qint64 lastGrabbedMs_ = 0;        // 上次成功抓帧时间戳（重抓节流用）
    bool retryTimerActive_ = false;   // 限流补偿补抓定时器已挂起标志
};

// 玻璃按钮：真玻璃材质（背景折射 + tint + 透光，同 LiquidGlassCard），
// 悬停 / 按下时叠加亮/暗状态层提亮。继承 LiquidGlassThemeKeeper
// 自动纳入玻璃抓帧体系与主题遍历。
class LiquidGlassButton : public LiquidGlassThemeKeeper
{
    Q_OBJECT

public:
    explicit LiquidGlassButton(const QString &text, QWidget *parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    bool hovered_ = false;
    bool pressed_ = false;
};
