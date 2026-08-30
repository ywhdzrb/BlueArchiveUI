#include "liquid_glass.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QSurfaceFormat>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QMouseEvent>
#include <QShowEvent>
#include <QResizeEvent>
#include <QRgb>
#include <QImage>
#include <qmath.h>
#include <QDateTime>
#include <QTimer>
#include <QFile>
#include <QTextStream>

#include <QLayout>  // grabBackdrop 抓帧前激活布局，避免 resize 期间抓到旧几何
#include <cmath>
#include <QDebug>  // 纹理上传 GL 错误输出（调试使用）

namespace {

// 诊断日志：Qt 的 stderr 消息在本机被静默（qWarning 0 字节输出），
// 故统一追加写文件，运行时可用 /tmp/opencode/liquid_glass_diag.txt 查询
void appendDiag(const QString &msg)
{
    QFile f("/tmp/opencode/liquid_glass_diag.txt");
    if (!f.open(QIODevice::Append | QIODevice::Text)) {
        return;
    }
    QTextStream ts(&f);
    ts << QDateTime::currentDateTime().toString("HH:mm:ss.zzz")
       << " " << msg << "\n";
}

// 玻璃面板顶点着色器：全屏四边形，vUV 为面板内归一化坐标(0~1)，
// 后续材质计算全部以 vUV 为基础，与设备像素比、坐标原点无关。
const char *kVertexShader = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTex;
out vec2 vUV;
void main() {
    vUV = aTex;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

// 玻璃面板片段着色器：折射采样背景纹理 + 半透明着色 + 中心透光。
// 折射采用「倒角弧面 + Snell 小角度近似」物理模型（参考 Apple Liquid Glass
// 复刻实现与 OverShifted/LiquidGlass）：玻璃中央可以认为表面近似平坦
// （几乎不折射），越靠近边缘玻璃面越陡，折射位移越强——采样点在边缘
// 被推向外沿，中央内容保持稳定。整体是「倒角玻璃板」而非「凸透镜放大镜」。
const char *kFragmentShader = R"(#version 330 core
uniform sampler2D uBackdrop;
uniform vec2  uBackdropSize;     // 窗口快照逻辑像素尺寸
uniform vec2  uPanelPos;         // 面板左上角在快照中的逻辑坐标
uniform vec2  uPanelSize;        // 面板逻辑像素尺寸
uniform float uRefraction;       // 折射强度（边缘位移系数）
uniform float uEdgeRefract;      // 边缘环带压边宽度比例（bevel/短边）
uniform float uCorner;           // 圆角半径(px)
uniform vec4  uTint;             // 半透明着色(rgba)
uniform float uGlowAlpha;        // 中心透光强度
in vec2 vUV;
out vec4 fragColor;

// 圆角矩形有向距离场(带符号)：p 为相对中心的坐标，halfSize 为半尺寸
float sdRoundRect(vec2 p, vec2 halfSize, float r) {
    vec2 q = abs(p) - halfSize + vec2(r);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    // 像素中心对齐：vUV=0 对应面板左上像素的左缘，像素中心在 0.5 处。
    // 不加偏移时 row0 的中心落在 px.y=0（顶边边界上），sd≈0.57 被裁成
    // 半透明灰，显示为面板顶边外一条 1px 灰/彩带（残影行）；
    // 偏移后 row0 中心在 px.y=0.5（顶边内侧 0.5px），与软件版行序一致。
    vec2 px = vUV * uPanelSize + vec2(0.5);
    vec2 c  = uPanelSize * 0.5;
    vec2 halfS = uPanelSize * 0.5;

    // 圆角裁剪：sdf<=0 在圆角矩形内，用 fwidth 切线宽度做边缘抗锯齿。
    // 对称窗口 [-fwidth, fwidth]：单边窗口 [0, fwidth] 在导数骤变处会偏置，
    // 导致切点处出现阶梯锯齿
    float sd = sdRoundRect(px - c, halfS, uCorner);
    // 光栅化 AA 宽度：固定等距窗口（不随 fwidth）。fwidth 仅在 4 片元
    // 导数采样上估计，圆弧斜率跨像素变化会让相邻片元的窗口宽窄抖动，
    // 输出色阶跳变成弧线上的「串珠彩点」；等距线 SDF 的空间梯度恒为 1，
    // 固定 1.0px 窗口对任意旋转角都是均匀一致的 AA，无噪声
    const float aa = 1.0;
    float clipA = 1.0 - smoothstep(-aa, aa, sd);

    // ---- 背景折射：倒角弧面 + Snell 近似(与软件版公式一致) ----
    // 把面板当作一块边缘倒角的玻璃板：设定一个「玻璃曲率高度场」，
    // 中央矮(接近平坦)、临近边缘的倒角带内快速升高。到边缘的归一化
    // 距离 t（0=边缘、1=中心）即决定表面坡度 slope = 高度场对 t 的导数：
    //   t 的矩形等值线（Chebyshev）保证上下/左右边缘的倒角宽度一致，
    //   与玻璃面板真实观感（四条边、带圆角）贴合
    vec2 n = (px - c) / halfS;                     // 椭圆归一化坐标
    // 矩形归一化半径：两轴除以半尺寸后取较大分量(Chebyshev 距离)，
    // 等值线为同心矩形而非椭圆。椭圆距离下同半径在宽/高两个方向
    // 对应不同物理长度，折射压边强弱会随宽高比失真
    float d = min(max(abs(n.x), abs(n.y)), 1.0);
    float t = 1.0 - d;                              // 0=边缘、1=中心
    // 倒角带外的中央区无需计算折射（表面平坦、无弯曲）
    float bevel = uEdgeRefract;                     // 倒角带宽度（0~1）
    float slope = 0.0;
    if (t < bevel) {
        // 圆形弧面高度场的坡度形状：h(t') = 1 - sqrt(1 - (1-t')^2)，
        // 圆心处坡度 0、边缘处坡度→竖直（真实镜片弧面）；
        // 对导数做钳制，避免 AA 半像素处斜率爆炸把采样点甩出边界
        float tt = clamp(t / bevel, 0.0, 1.0);
        float oneMinus = min(1.0 - tt, 0.93);
        slope = oneMinus / max(sqrt(1.0 - oneMinus * oneMinus), 0.002);
    }
    // 折射位移方向：沿「从面板中心指向当前像素」的径向。边缘倒角带内
    // 采样点朝面板中心偏移——透过玻璃边缘看到的背景内容被弯曲向内部，
    // 这是 Apple 复刻实现的主流方向（"warp inward at the edges"）。
    // 位移量 = 坡度 * (1 - 1/IOR) * 折射系数（IOR=1.5 玻璃）
    vec2 dir = n / max(length(n), 0.0012);          // 归一化径向
    float bend = slope * (1.0 - 1.0 / 1.5) * uRefraction;
    vec2 src = c + (n - dir * bend) * halfS + uPanelPos;   // 折射反演回窗口坐标
    vec2 uv = src / uBackdropSize;

    vec3 col = texture(uBackdrop, uv).rgb;

    // ---- 半透明着色：暗玻璃压黑、亮玻璃压白 ----
    col = mix(col, uTint.rgb, uTint.a);

    // ---- 中心透光：径向渐变提亮，位置/尺度与软件版一致 ----
    float gd = length((vUV - vec2(0.5, 0.42)) * (uPanelSize / (min(uPanelSize.x, uPanelSize.y) * 0.62)));
    col += vec3(uGlowAlpha * pow(clamp(1.0 - gd, 0.0, 1.0), 2.0));

    // 输出预乘 alpha：RGB 必须乘 clipA。否则边缘半透明像素的红/绿/蓝
    // 保持不饱和满值，合成到背景时白色溢出成 1px 锯齿边+彩色杂点
    fragColor = vec4(col * clipA, clipA);
}
)";

} // namespace

// GLSL smoothstep(e0, e1, x) 的 CPU 等价：x 从 e0 过渡到 e1 返回 0→1
inline qreal smoothstep01(qreal x, qreal e0, qreal e1)
{
    const qreal t = qBound(0.0, (x - e0) / (e1 - e0), 1.0);
    return t * t * (3.0 - 2.0 * t);
}

namespace {

// 候选 GL surface format 列表，从高到低逐个探测创建 QOpenGLContext。
// 排序依据：community 案例显示 EGL_BAD_MATCH 常源于 format 与驱动 EGL
// config 不匹配（NVIDIA + Wayland 下尤为常见），降级顺序为
// 核心 + 采样 → 核心不带采样 → 降版本 → 驱动默认格式。
QList<QSurfaceFormat> probeCandidates()
{
    QList<QSurfaceFormat> list;
    auto base = QSurfaceFormat::defaultFormat();

    auto makeFmt = [&](int major, int minor, int samples) {
        QSurfaceFormat f = base;
        f.setRenderableType(QSurfaceFormat::OpenGL);
        f.setVersion(major, minor);
        f.setProfile(major >= 3 ? QSurfaceFormat::CoreProfile
                                : QSurfaceFormat::CompatibilityProfile);
        f.setAlphaBufferSize(8);   // 玻璃圆角外围需要真实 alpha 混合
        f.setDepthBufferSize(24);
        f.setStencilBufferSize(8);
        f.setSamples(samples);
        return f;
    };

    list << makeFmt(3, 3, 4);
    list << makeFmt(3, 3, 0);
    list << makeFmt(3, 2, 0);
    list << makeFmt(3, 0, 0);
    list << makeFmt(2, 0, 0);
    list << base;   // 兜底：驱动默认
    return list;
}

// 探测缓存：glProbeDone_ 首次成功后不再重复创建上下文
bool glProbeDone_ = false;
bool glProbeOk_ = false;
QSurfaceFormat glProbeFormat_;

// 逐个候选尝试创建裸 QOpenGLContext，取第一个成功者。
// 成功仅需 create() 返回真（不包括后续 surface 绑定，那部分由
// QOpenGLWidget 自身在真实窗口上完成，若仍失败面板走 CPU 降级）。
bool detectGl()
{
    if (glProbeDone_) {
        return glProbeOk_;
    }
    glProbeDone_ = true;

    const auto candidates = probeCandidates();
    for (const auto &fmt : candidates) {
        QOpenGLContext ctx;
        ctx.setFormat(fmt);
        if (ctx.create()) {
            glProbeOk_ = true;
            glProbeFormat_ = fmt;
            break;
        }
    }

    if (!glProbeOk_) {
        // 警告打一次：所有候选都失败，玻璃面板将使用 CPU 软件渲染
        qWarning("liquid_glass: no viable GL context format; "
                 "falling back to software rendering");
    }
    return glProbeOk_;
}

} // namespace

QSurfaceFormat probeGlFormat()
{
    detectGl();
    return glProbeFormat_;
}

bool isGlAvailable()
{
    return detectGl();
}

LiquidGlassPanel::LiquidGlassPanel(QWidget *parent)
    : QWidget(parent)
{
    // 圆角外区域需与页面背景真实混合（离屏 GL 混合与 CPU 降解路径一致）
    setAttribute(Qt::WA_TranslucentBackground);

    // 隐藏测试开关：MD3_GL_DISABLE=1 强制软件绘制，用于开发时对比
    // GPU / CPU 两条渲染管线的视觉一致性
    if (qEnvironmentVariableIsSet("MD3_GL_DISABLE")) {
        glOk_ = false;
        return;
    }

    // 启动探测（NVIDIA + Wayland + EGL 平台失败，3009 = EGL_BAD_MATCH）。
    // 探测失败不硬崩，降级 CPU 软件绘制；命中则应用探测选定的 format。
    // 注意：与 QOpenGLWidget 不同，此处通过自建离屏上下文绕开 Qt
    // Widgets 的平台 EGL 集成（参考 OverShifted/LiquidGlass 架构）。
    if (!isGlAvailable()) {
        glOk_ = false;
        appendDiag("GL unavailable -> CPU path");
    } else {
        appendDiag("GL available -> GPU path");
    }
}

LiquidGlassPanel::~LiquidGlassPanel()
{
    // 离屏 GL 资源必须在当前上下文中释放；context 无效时跳过
    if (glResourcesReady_) {
        if (glContext_.isValid()) {
            glContext_.makeCurrent(&glSurface_);
            vao_.destroy();
            vertexBuffer_.destroy();
            bgTexture_.destroy();
            program_.removeAllShaders();
            glContext_.doneCurrent();
        }
        if (fbo_) {
            delete fbo_;
            fbo_ = nullptr;
        }
    }
}

// 离屏 GL 上下文 + shader + VAO/VBO + 纹理懒初始化。
// 创建失败返回 false，同时 glOk_ 置 false，此后面板全走 CPU 软渲染。
// 返回 true 仅代表离屏上下文可用（成功与否与 Qt Widgets 平台无关，
// 这正是本架构选型的原因）。
bool LiquidGlassPanel::ensureGlResources()
{
    if (glResourcesReady_) {
        return true;
    }

    // 使用启动探测选定的 surface format（见文件头注释）
    glContext_.setFormat(probeGlFormat());
    if (!glContext_.create()) {
        glOk_ = false;
        return false;
    }
    glSurface_.setFormat(glContext_.format());
    glSurface_.create();
    if (!glSurface_.isValid()) {
        glOk_ = false;
        return false;
    }
    if (!glContext_.makeCurrent(&glSurface_)) {
        glOk_ = false;
        return false;
    }

    // 顶点数据：位置(-1..1) + 面板内归一化纹理坐标。
    // V 分量纵向翻转：QImage 首行是顶部，上传后 GL 纹理 v=0 即顶行；
    // 而 GL 视口 y=-1 在底部，故底部顶点用 v=1。QOpenGLFramebufferObject::
    // toImage() 内部也会做一次 Y 翻转（glReadPixels 底部行写入 QImage
    // 末行），两次翻转抵消后输出与快照方向一致，否则画面上下颠倒。
    static const GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
    };

    if (!program_.addShaderFromSourceCode(QOpenGLShader::Vertex, kVertexShader)
        || !program_.addShaderFromSourceCode(QOpenGLShader::Fragment, kFragmentShader)
        || !program_.link()) {
        qWarning("LiquidGlassPanel shader compile/link failed: %s",
                 qPrintable(program_.log()));
        glOk_ = false;
        glContext_.doneCurrent();
        return false;
    }

    vao_.create();
    vertexBuffer_.create();
    vertexBuffer_.bind();
    vertexBuffer_.allocate(vertices, sizeof(vertices));
    vao_.bind();
    program_.bind();
    program_.enableAttributeArray(0);
    program_.setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
    program_.enableAttributeArray(1);
    program_.setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
    vao_.release();
    vertexBuffer_.release();
    program_.release();

    // 背景纹理：RGBA8、钳制到边缘、线性滤波（折射为亚像素采样）
    bgTexture_.setFormat(QOpenGLTexture::RGBA8_UNorm);
    bgTexture_.create();
    bgTexture_.setWrapMode(QOpenGLTexture::ClampToEdge);
    bgTexture_.setMinificationFilter(QOpenGLTexture::Linear);
    bgTexture_.setMagnificationFilter(QOpenGLTexture::Linear);

    glContext_.doneCurrent();
    glResourcesReady_ = true;
    return true;
}

// 离屏渲染玻璃帧：FBO 上执行玻璃 shader，产物 glFrame_ 由 paintEvent 统一绘出。
// 任一步失败（FBO 创建、着色器运行）即整条通道降级 CPU 软渲染。
void LiquidGlassPanel::renderGlFrame()
{
    const QSize sz = size();
    if (sz.width() <= 0 || sz.height() <= 0) {
        glFrame_ = QImage();
        return;
    }

    if (!glContext_.makeCurrent(&glSurface_)) {
        qWarning("liquid_glass: renderGlFrame early-return: makeCurrent FAILED");
        glOk_ = false;
        return;
    }

    // 纹理由窗口快照填充：只在快照更新后上传一次，避免每帧重复传输。
    // 用原生 GL 命令上传（glPixelStorei 显式声明行距与对齐），
    // QOpenGLTexture 高层封装在此场景行为不可靠（采样得到空纹理）
    if (backdropDirty_ && !backdrop_.isNull()) {
        const GLuint texId = bgTexture_.textureId();
        glBindTexture(GL_TEXTURE_2D, texId);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, backdrop_.bytesPerLine() / 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     backdrop_.width(), backdrop_.height(), 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, backdrop_.constBits());
        glBindTexture(GL_TEXTURE_2D, 0);
        hasTexture_ = true;
        backdropDirty_ = false;
    }

    // FBO 按面板尺寸惰性重建：着色附件为 32bit RGBA texture。
    // 开启 4x MSAA 硬件抗锯齿：弧线几何由 GPU 多点采样圆滑，
    // 消除 shader fwidth 窗口在 45° 弧线上产生的串珠彩点。
    if (!fbo_ || fbo_->width() != sz.width() || fbo_->height() != sz.height()) {
        delete fbo_;
        QOpenGLFramebufferObjectFormat fmt;
        fmt.setSamples(4);
        fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        fbo_ = new QOpenGLFramebufferObject(sz, fmt);
    }
    if (!fbo_ || !fbo_->isValid() || !fbo_->bind()) {
        glOk_ = false;
        glContext_.doneCurrent();
        return;
    }
    glViewport(0, 0, sz.width(), sz.height());
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const QWidget *w = window();
    // 背景快照尚未就绪：不渲染（否则 FBO 全透明，且转出的是非空图，
    // 会挡住 paintEvent 的 CPU 兜底路径，面板显示为空白）。
    // paint 循环内严禁同步抓帧（grabBackdrop 会 hide/show + render 顶层窗口，
    // 在绘画期间执行会无限递归 repaint）。这里只调度一次异步抓帧。
    if (!hasTexture_ || !w || w->width() <= 0 || w->height() <= 0) {
        scheduleBackdropGrab();
        glContext_.doneCurrent();
        glFrame_ = QImage();
        return;
    }
    // 面板在窗口坐标系中的逻辑矩形（窗口左侧还有导航栏，mapTo 保证对齐）
    const QPoint pos = mapTo(const_cast<QWidget *>(w), QPoint(0, 0));
    const QRect panelRect(pos, size());
    // 快照布局与当前窗口几何不一致（Hyprland 异步 resize）时素材失效，
    // 直接采样会折射出旧位置内容（背景错位），本帧置空走 CPU 兜底并调度重抓
    const bool stale = backdropWinSize_.isValid() && (w->size() != backdropWinSize_);
    if (!backdrop_.rect().contains(panelRect) || stale) {
        // 背景快照尺寸与面板不匹配（show/resize 竞态）：本帧不产生 GL 内容，
        // 置空走 CPU 兜底（其内也不抓帧，仅调度异步重抓）。
        scheduleBackdropGrab();
        glContext_.doneCurrent();
        glFrame_ = QImage();
        return;
    }
        program_.bind();
        program_.setUniformValue("uBackdrop", 0);
        program_.setUniformValue("uBackdropSize", QVector2D(backdrop_.width(), backdrop_.height()));
        program_.setUniformValue("uPanelPos", QVector2D(pos.x(), pos.y()));
        program_.setUniformValue("uPanelSize", QVector2D(sz.width(), sz.height()));
        program_.setUniformValue("uRefraction", float(refractionK_));
        program_.setUniformValue("uEdgeRefract", float(edgeRefractK_));
        program_.setUniformValue("uCorner", float(cornerRadius_));
        program_.setUniformValue("uTint", dark_
            ? QVector4D(0.0f, 0.0f, 0.0f, 52.0f / 255.0f)
            : QVector4D(1.0f, 1.0f, 1.0f, 72.0f / 255.0f));
        program_.setUniformValue("uGlowAlpha", dark_ ? 34.0f / 255.0f : 52.0f / 255.0f);

        glActiveTexture(GL_TEXTURE0);
        bgTexture_.bind();
        vao_.bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        vao_.release();
        bgTexture_.release();
        program_.release();

    fbo_->release();

    // toImage 必须挂在 current context 上：先转 QImage（其内部处理 Y 翻转），
    // 再 doneCurrent。此前顺序为 doneCurrent 后 toImage，Qt 打印
    // "called without a current context" 并返回空图，导致 GPU 通道每帧失效
    // 落到 CPU 兜底（折射因抓帧竞态常表现为无折射黑板）。
    // toImage 内容实为未预乘字节（fmt_check 实证），但 QOpenGLFramebufferObject
    // 将其标记为 Premultiplied；QPainter 混读该标定会把 AA 半透明像素算偏
    // （弧线外缘黄/黑杂点）。显式转为非预乘格式，语义与内容一致。
    glFrame_ = fbo_->toImage().convertToFormat(QImage::Format_ARGB32);
    if (qEnvironmentVariableIsSet("MD3_GL_FRAME_DEBUG")) {
        glFrame_.save("/tmp/opencode/glframe_dump.png");
    }
    glContext_.doneCurrent();
}

void LiquidGlassPanel::applyGlassStyle(bool dark)
{
    if (dark_ == dark) {
        return;
    }
    dark_ = dark;
    update();
}

void LiquidGlassPanel::refreshBackdrop()
{
    grabBackdrop();
}

void LiquidGlassPanel::setRefraction(qreal k)
{
    refractionK_ = k;
    // 倒角带宽度占短边比例：双轴等宽（上/下/左/右同值），
    // 与 shader/CPU 的 bevel 参数一致
    edgeRefractK_ = 0.18;
    update();
}

void LiquidGlassPanel::setCornerRadius(qreal radius)
{
    cornerRadius_ = radius;
    update();
}

// 抓取顶层窗口的整幅快照，作为玻璃材质的背景源（CPU 一次性抓帧，上传 GPU 纹理）。
// 快照必须不含玻璃控件自身的渲染（否则折射出来的是旧帧且重影错位——「看到
// 自己」），因此复用 grabGlassBackdrop：抓帧期间将所有带 lgGlass / md3FadeOverlay
// 属性的控件（所有玻璃控件 + 主题过渡遮罩）全部隐藏，render 后恢复。
// render 期间会同步处理子控件事件（含 resize），必须用标志防重入以免无限递归。
// 注意：本函数禁止在 paint 循环内调用（paint 期间 hide/show + render 顶层窗口
// 会递归 repaint 卡死），paint 内的重抓走 scheduleBackdropGrab() 异步调度。
void LiquidGlassPanel::grabBackdrop()
{
    if (backdropGrabbing_) {
        return;
    }
    backdropGrabbing_ = true;

    QWidget *w = window();
    if (!w || w->width() <= 0 || w->height() <= 0) {
        backdropGrabbing_ = false;
        return;
    }
    // 抓帧前先落定布局：Hyprland 平铺下窗口 resize 是异步的，面板隐藏后
    // 布局可能还停留在旧几何（面板顶行叠在页眉文字上），会导致玻璃顶边
    // 折射出「文字行」黑线 + 次像素彩点。强制 layout->activate 一次再抓。
    if (w->layout()) {
        w->layout()->activate();
    }
    const QImage snap = grabGlassBackdrop(this, w);
    backdrop_ = snap;
    backdropWinSize_ = w->size();   // 记录快照时的窗口几何，用于失效检测
    if (qEnvironmentVariableIsSet("MD3_BACKDROP_DEBUG")) {
        backdrop_.save(QString("/tmp/opencode/bd_%1_%2.png")
                           .arg(w->width()).arg(w->height()));
    }
    backdropDirty_ = true;
    lastGrabbedMs_ = QDateTime::currentMSecsSinceEpoch();
    update();
    backdropGrabbing_ = false;
}

// paint 循环内的重抓调度：zero-timer 延后 + 300ms 实例限流。
// grabBackdrop 在绘画期间执行会 hide/show 控件并 render 顶层窗口，
// 引发 Qt 递归 repaint（QWidget::repaint: Recursive repaint detected），
// 且面板位置随页面动画移动时每帧都同步抓帧 → 卡死。
// 限流内被跳过的请求进入补抓：Hyprland 下窗口 resize 分多步到达，
// 最后一步若是窗口 resize 事件本身、且距上次抓帧不足 300ms，
// 会被原样忽略 → 材质停留在旧几何（面板折射出旧帧文字残留）。
void LiquidGlassPanel::scheduleBackdropGrab()
{
    const QSize winNow = window() ? window()->size() : QSize();
    if (backdropWinSize_ != winNow && !retryTimerActive_) {
        // 尺寸已变但上次抓帧距今不足 300ms：挂一个 320ms 后的补抓，
        // 期间若再有新请求（retryTimerActive_）不重复挂
        retryTimerActive_ = true;
        QTimer::singleShot(340, this, [this]() {
            retryTimerActive_ = false;
            if (isVisible() && backdropWinSize_ != window()->size()) {
                grabBackdrop();
            }
        });
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastGrabbedMs_ < 300) {
        return;   // 限流：距上次抓帧不足 300ms 不重抓（防止动画期间风暴）
    }
    lastGrabbedMs_ = now;
    QTimer::singleShot(0, this, [this]() {
        if (isVisible()) {
            grabBackdrop();
        }
    });
}

void LiquidGlassPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // 离屏 GL 通道：可用则渲染玻璃帧，任一步失败（context/FBO/shader）
    // 立即降级并停用（glOk_ = false），不反复重试——照抄参考项目
    // OverShifted/LiquidGlass 的「自建 context + 离屏 FBO」架构，
    // 与 Qt Widgets 平台 EGL 集成（3009 黑屏）彻底解耦。
    if (glOk_) {
        if (ensureGlResources()) {
            renderGlFrame();
        }
    }
    if (!glOk_ || glFrame_.isNull()) {
        renderSoftwareFrame();
    }

    QPainter p(this);
    if (qEnvironmentVariableIsSet("MD3_GL_FRAME_FINAL")) {
        QFile dbgPre("/tmp/opencode/pre.txt");
        if (dbgPre.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream tsPre(&dbgPre);
            tsPre << "PRE frame=" << glFrame_.size().width() << "x" << glFrame_.size().height()
                  << " c0,300=" << glFrame_.pixelColor(0, 300).name() << glFrame_.pixelColor(0, 300).alpha()
                  << " c2,300=" << glFrame_.pixelColor(2, 300).name() << glFrame_.pixelColor(2, 300).alpha()
                  << "\n";
            dbgPre.close();
        }
    }
    p.drawImage(0, 0, glFrame_);
    // 最终上屏帧 dump：与 drawImage 完全同一对象
    if (qEnvironmentVariableIsSet("MD3_GL_FRAME_FINAL")) {
        glFrame_.save("/tmp/opencode/glframe_final.png");
        QFile dbg("/tmp/opencode/paint_samples.txt");
        if (dbg.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream ts(&dbg);
            const QPoint pw = mapTo(window(), QPoint(0, 0));
            ts << "PAINT frame=" << glFrame_.size().width() << "x" << glFrame_.size().height()
               << " pos=" << pw.x() << "," << pw.y()
               << " c0,300=" << glFrame_.pixelColor(0, 300).name() << glFrame_.pixelColor(0, 300).alpha()
               << " c2,300=" << glFrame_.pixelColor(2, 300).name() << glFrame_.pixelColor(2, 300).alpha()
               << " ok=" << glOk_ << "\n";
            dbg.close();
        }
    }
}

// 软件降级绘制：与片段着色器逐项等价的 CPU 管线，产物写入 glFrame_
// 后由 paintEvent 统一绘出。逐像素计算桶形折射采样 + tint + 透光 +
// 高光 + 菲涅耳 + 白描边 + 圆角抗锯齿。仅在 GL context 不可用时用，
// 视觉结果与 GPU 版一致。
void LiquidGlassPanel::renderSoftwareFrame()
{
    const int W = width();
    const int H = height();
    if (W <= 0 || H <= 0) {
        glFrame_ = QImage();
        return;
    }

    const QWidget *w = window();
    const QPoint pos = mapTo(const_cast<QWidget *>(w), QPoint(0, 0));
    const QRect panelRect(pos, size());
    // 窗口几何已变：快照反映的是旧布局（控件位置/文字与当前不符），
    // 必须重抓。与取景区越界同样视为素材失效
    const bool stale = w && backdropWinSize_.isValid() && (w->size() != backdropWinSize_);
    const bool haveSrc = !backdrop_.isNull() && backdrop_.rect().contains(panelRect) && !stale;

    // show/resize 竞态兜底：窗口在首次抓帧后又被放大（面板矩形溢出快照），
    // 下一帧采样全部越界会导致整板纯黑。paint 循环内不能同步抓帧
    // （grabBackdrop 的 hide/show + render 在绘画期间递归 repaint 卡死），
    // 此处仅调度异步重抓（300ms 限流在调度函数内部做）。
    if (!haveSrc && width() > 0 && height() > 0 && w && w->width() > 0) {
        scheduleBackdropGrab();
    }

    const qreal cx = W * 0.5, cy = H * 0.5;
    const qreal hx = W * 0.5, hy = H * 0.5;

    // 与 shader uniform 保持一致的材质参数
    const qreal tintR = dark_ ? 0.0 : 255.0;
    const qreal tintA = (dark_ ? 52 : 72) / 255.0;
    const qreal glowA = (dark_ ? 34 : 52) / 255.0;
    const qreal glowScale = qMin(W, H) * 0.62;
    const qreal glowCy = H * 0.42;
    const qreal k = refractionK_;
    const qreal corner = cornerRadius_;

    // 快捷 lambda：读取 backdrop_ 中窗口坐标 (sx, sy) 处的双线性 RGB
    auto sampleAt = [&](qreal sx, qreal sy, qreal &r, qreal &g, qreal &b) {
        sx = qBound(0.0, sx, qreal(backdrop_.width() - 2));
        sy = qBound(0.0, sy, qreal(backdrop_.height() - 2));
        const int x0 = int(sx);
        const int y0 = int(sy);
        const qreal fx = sx - x0;
        const qreal fy = sy - y0;
        const uchar *row0 = backdrop_.constScanLine(y0) + x0 * 4;
        const uchar *row1 = backdrop_.constScanLine(y0 + 1) + x0 * 4;
        r = 0; g = 0; b = 0;
        for (int ch = 0; ch < 3; ++ch) {
            const qreal c00 = row0[ch];
            const qreal c01 = row0[4 + ch];
            const qreal c10 = row1[ch];
            const qreal c11 = row1[4 + ch];
            const qreal a = c00 + (c01 - c00) * fx;
            const qreal bb = c10 + (c11 - c10) * fx;
            const qreal v = a + (bb - a) * fy;
            if (ch == 0) r = v; else if (ch == 1) g = v; else b = v;
        }
    };

    QImage out(W, H, QImage::Format_ARGB32);
    for (int y = 0; y < H; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(out.scanLine(y));
        const qreal v = qreal(y) / H;
        for (int x = 0; x < W; ++x) {
            const qreal u = qreal(x) / W;

            // 圆角 SDF：与 shader sdRoundRect 一致（qAbs + 距离组装）
            const qreal qx = qAbs(x - cx) - hx + corner;
            const qreal qy = qAbs(y - cy) - hy + corner;
            const qreal sdx = qMax(qx, 0.0);
            const qreal sdy = qMax(qy, 0.0);
            const qreal sd = std::sqrt(sdx * sdx + sdy * sdy)
                             + qMin(qMax(qx, qy), 0.0) - corner;
            // 对称窗口 AA：与 GPU 版 fwidth 窗口一致（-1~+1 过渡），
            // 单边窗口在弧线切线处会留下可见锯齿（真机 3009 降级 CPU 版主诉）
            const qreal clipA = 1.0 - smoothstep01(sd, -1.0, 1.0);

            // 折射反查：倒角弧面 + Snell 近似(与 shader 同公式)。
            // 中央平坦无弯曲（slope=0），倒角带内采样点朝面板中心偏移
            const qreal nx = (x - cx) / hx;
            const qreal ny = (y - cy) / hy;
            const qreal d = qMin(qMax(qAbs(nx), qAbs(ny)), 1.0);
            const qreal t = 1.0 - d;                 // 0=边缘、1=中心
            const qreal bevel = edgeRefractK_;       // 倒角带宽(0~1)
            qreal slope = 0.0;
            if (t < bevel) {
                const qreal tt = qBound(0.0, t / bevel, 1.0);
                const qreal oneMinus = qMin(1.0 - tt, 0.93);
                slope = oneMinus / qMax(std::sqrt(1.0 - oneMinus * oneMinus), 0.002);
            }
            const qreal len = std::sqrt(nx * nx + ny * ny);
            const qreal dirx = len > 0.0012 ? nx / len : 0.0;
            const qreal diry = len > 0.0012 ? ny / len : 0.0;
            const qreal bend = slope * (1.0 - 1.0 / 1.5) * k;
            const qreal sx = cx + (nx - dirx * bend) * hx + pos.x();
            const qreal sy = cy + (ny - diry * bend) * hy + pos.y();

            qreal cr = 0, cg = 0, cb = 0;
            if (haveSrc) {
                sampleAt(sx, sy, cr, cg, cb);
            }

            // tint 混合：暗玻璃压黑、亮玻璃压白
            cr = cr + (tintR - cr) * tintA;
            cg = cg + (tintR - cg) * tintA;
            cb = cb + (tintR - cb) * tintA;

            // 中心透光：径向提亮（中心 (0.5, 0.42)，尺度 min*0.62）
            const qreal gdx = (x - cx) / glowScale;
            const qreal gdy = (y - glowCy) / glowScale;
            const qreal gd = std::sqrt(gdx * gdx + gdy * gdy);
            const qreal g = qBound(0.0, 1.0 - gd, 1.0);
            cr += glowA * g * g * 255.0;
            cg += glowA * g * g * 255.0;
            cb += glowA * g * g * 255.0;

            const int ar = qBound(0, int(cr + 0.5), 255);
            const int ag = qBound(0, int(cg + 0.5), 255);
            const int ab = qBound(0, int(cb + 0.5), 255);
            const int aa = qBound(0, int(clipA * 255.0 + 0.5), 255);
            line[x] = qRgba(ar, ag, ab, aa);
        }
    }

    glFrame_ = out;
}

void LiquidGlassPanel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 异步抓帧：hide/show 引起的布局 resize 事件为 posted 延迟分发，
    // 同步在该事件内抓帧会在下一事件循环再次 resize→递归成抓帧风暴。
    // 延迟 0ms 让布局先稳定一轮后再取素材
    scheduleBackdropGrab();
}

void LiquidGlassPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    scheduleBackdropGrab();
}

LiquidGlassButton::LiquidGlassButton(const QString &text, QWidget *parent)
    : LiquidGlassThemeKeeper(parent)
{
    setText(text);
    setCursor(Qt::PointingHandCursor);
}

QSize LiquidGlassButton::sizeHint() const
{
    const qreal w = fontMetrics().horizontalAdvance(text()) + 48;
    return QSize(qMax(96, qRound(w)), 44);
}

// 玻璃按钮：真玻璃材质——背景折射 + 半透明着色 + 中心透光（renderGlassPlate），
// 悬停 / 按下时在玻璃上叠加亮（暗色）/ 暗（亮色）状态层，提亮手感到位。
void LiquidGlassButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal radius = 16.0;
    const QRectF r(rect());
    const QPainterPath buttonPath = [&]() {
        QPainterPath path;
        path.addRoundedRect(r, radius, radius);
        return path;
    }();

    // 玻璃板：与卡片/开关同一材质管线（折射窗口背景 + tint + 透光）。
    // 底后再叠一层实色 tint（暗色偏黑 / 亮色偏白，比卡片浓一点）——
    // 按钮需要比玻璃卡片更「实体」的轮廓感，否则与背景玻璃融为一体
    QImage glass = renderGlassPlate(radius, 0.10, 0.18, 1.0);
    if (!glass.isNull()) {
        p.setClipPath(buttonPath);
        p.drawImage(r, glass, r);
        p.fillRect(r, dark_ ? QColor(0, 0, 0, 62)
                            : QColor(255, 255, 255, 96));
        p.setClipping(false);
    } else {
        p.setBrush(dark_ ? QColor(48, 48, 50) : QColor(255, 255, 255, 152));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, radius, radius);
    }

    // 状态层：暗色悬停/按下提亮、亮色悬停/按下压暗，glass 之上简单混合
    int stateAlpha = 0;
    if (hovered_) {
        stateAlpha = 18;
    }
    if (pressed_) {
        stateAlpha = 40;
    }
    if (stateAlpha > 0) {
        p.setClipPath(buttonPath);
        p.fillRect(r, dark_ ? QColor(255, 255, 255, stateAlpha)
                            : QColor(0, 0, 0, stateAlpha));
        p.setClipping(false);
    }

    // 标题文字：Label Large 风格，暗色玻璃用白字、亮色玻璃用近黑字
    QFont f = font();
    f.setPointSizeF(14.0);
    f.setWeight(QFont::Medium);
    p.setFont(f);
    p.setPen(dark_ ? QColor(255, 255, 255, 232) : QColor(24, 24, 28, 232));
    p.drawText(r, Qt::AlignCenter, text());
}

void LiquidGlassButton::enterEvent(QEnterEvent *event)
{
    LiquidGlassThemeKeeper::enterEvent(event);
    hovered_ = true;
    update();
}

void LiquidGlassButton::leaveEvent(QEvent *event)
{
    LiquidGlassThemeKeeper::leaveEvent(event);
    hovered_ = false;
    pressed_ = false;
    update();
}

void LiquidGlassButton::showEvent(QShowEvent *event)
{
    LiquidGlassThemeKeeper::showEvent(event);
    backdropDeferred();
}

void LiquidGlassButton::resizeEvent(QResizeEvent *event)
{
    LiquidGlassThemeKeeper::resizeEvent(event);
    backdropDeferred();
}

void LiquidGlassButton::mousePressEvent(QMouseEvent *event)
{
    QAbstractButton::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        pressed_ = true;
        update();
    }
}

void LiquidGlassButton::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton) {
        pressed_ = false;
        update();
    }
}

// ---------------------------------------------------------------------------
// 公共玻璃材质函数：供玻璃版 md3 控件（switch/slider/progress/card）复用。
// 与 LiquidGlassPanel 同一材质管线（倒角折射 + tint + 中心透光 + 圆角 AA），
// 小控件直接走 CPU 渲染即可（尺寸小、成本可忽略），不引入离屏 GL。
// ---------------------------------------------------------------------------

// 抓取窗口快照：隐藏玻璃控件自身与 md3FadeOverlay 遮罩后 render 顶层窗口。
// 抓帧期间所有玻璃控件（含自身）都被隐藏，保证快照内不含玻璃帧；
// lgGlass 属性由 grabGlassBackdrop 调用方（玻璃控件构造时）设置。
// 注意 activate() 必须在 hide 之前：hide 后调用会触发布局重排，
// 隐藏玻璃控件占位被回收、周边文字上移，快照录到「布局中间态」
// （玻璃板折射出悬浮的旧几何文字残留）。
QImage grabGlassBackdrop(const QWidget *self, QWidget *w)
{
    if (!w || w->width() <= 0 || w->height() <= 0) {
        return QImage();
    }
    // 抓帧前先落定布局（Hyprland 平铺 resize 异步期间布局可能滞后）
    if (w->layout()) {
        w->layout()->activate();
    }

    QList<QWidget *> hidden;
    const auto masks = w->findChildren<QWidget *>();
    for (QWidget *mask : masks) {
        const bool glass = mask->property("lgGlass").toBool();
        const bool overlay = mask->property("md3FadeOverlay").toBool();
        if ((glass || overlay) && mask->isVisible()) {
            mask->hide();
            hidden.append(mask);
        }
    }
    if (self && self->isVisible()) {
        const_cast<QWidget *>(self)->hide();
        if (!hidden.contains(const_cast<QWidget *>(self))) {
            hidden.append(const_cast<QWidget *>(self));
        }
    }
    QPixmap snap(w->size());
    w->render(&snap);
    for (QWidget *mask : hidden) {
        mask->show();
    }
    return snap.toImage().convertToFormat(QImage::Format_RGBA8888);
}

// 纯 CPU 玻璃板渲染：与 LiquidGlassPanel::renderSoftwareFrame 同公式，
// 但参数化（corner / k / edgeK / dark / glowMul），panelRect 提供窗口坐标基线。
QImage renderGlassPlateCPU(const QImage &backdrop, const QRect &panelRect,
                           const QSize &size, qreal corner, qreal k, qreal edgeK,
                           bool dark, qreal glowMul)
{
    const int W = size.width();
    const int H = size.height();
    if (W <= 0 || H <= 0) {
        return QImage();
    }

    const bool haveSrc = !backdrop.isNull() && backdrop.rect().contains(panelRect);
    const QPoint pos = panelRect.topLeft();

    const qreal cx = W * 0.5, cy = H * 0.5;
    const qreal hx = W * 0.5, hy = H * 0.5;

    const qreal tintR = dark ? 0.0 : 255.0;
    const qreal tintA = (dark ? 52 : 72) / 255.0;
    const qreal glowA = (dark ? 34 : 52) / 255.0 * glowMul;
    const qreal glowScale = qMin(W, H) * 0.62;
    const qreal glowCy = H * 0.42;

    auto sampleAt = [&](qreal sx, qreal sy, qreal &r, qreal &g, qreal &b) {
        sx = qBound(0.0, sx, qreal(backdrop.width() - 2));
        sy = qBound(0.0, sy, qreal(backdrop.height() - 2));
        const int x0 = int(sx);
        const int y0 = int(sy);
        const qreal fx = sx - x0;
        const qreal fy = sy - y0;
        const uchar *row0 = backdrop.constScanLine(y0) + x0 * 4;
        const uchar *row1 = backdrop.constScanLine(y0 + 1) + x0 * 4;
        r = 0; g = 0; b = 0;
        for (int ch = 0; ch < 3; ++ch) {
            const qreal c00 = row0[ch];
            const qreal c01 = row0[4 + ch];
            const qreal c10 = row1[ch];
            const qreal c11 = row1[4 + ch];
            const qreal a = c00 + (c01 - c00) * fx;
            const qreal bb = c10 + (c11 - c10) * fx;
            const qreal v = a + (bb - a) * fy;
            if (ch == 0) r = v; else if (ch == 1) g = v; else b = v;
        }
    };

    QImage out(W, H, QImage::Format_ARGB32);
    for (int y = 0; y < H; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(out.scanLine(y));
        for (int x = 0; x < W; ++x) {
            // 圆角 SDF（与 shader / 面板 CPU 版一致）
            const qreal qx = qAbs(x - cx) - hx + corner;
            const qreal qy = qAbs(y - cy) - hy + corner;
            const qreal sdx = qMax(qx, 0.0);
            const qreal sdy = qMax(qy, 0.0);
            const qreal sd = std::sqrt(sdx * sdx + sdy * sdy)
                             + qMin(qMax(qx, qy), 0.0) - corner;
            const qreal clipA = 1.0 - smoothstep01(sd, -1.0, 1.0);

            // 倒角弧面折射反查（与 shader 同公式）
            const qreal nx = (x - cx) / hx;
            const qreal ny = (y - cy) / hy;
            const qreal d = qMin(qMax(qAbs(nx), qAbs(ny)), 1.0);
            const qreal t = 1.0 - d;
            qreal slope = 0.0;
            if (t < edgeK) {
                const qreal tt = qBound(0.0, t / edgeK, 1.0);
                const qreal oneMinus = qMin(1.0 - tt, 0.93);
                slope = oneMinus / qMax(std::sqrt(1.0 - oneMinus * oneMinus), 0.002);
            }
            const qreal len = std::sqrt(nx * nx + ny * ny);
            const qreal dirx = len > 0.0012 ? nx / len : 0.0;
            const qreal diry = len > 0.0012 ? ny / len : 0.0;
            const qreal bend = slope * (1.0 - 1.0 / 1.5) * k;
            const qreal sx = cx + (nx - dirx * bend) * hx + pos.x();
            const qreal sy = cy + (ny - diry * bend) * hy + pos.y();

            qreal cr = 0, cg = 0, cb = 0;
            if (haveSrc) {
                sampleAt(sx, sy, cr, cg, cb);
            }

            // tint 混合
            cr = cr + (tintR - cr) * tintA;
            cg = cg + (tintR - cg) * tintA;
            cb = cb + (tintR - cb) * tintA;

            // 中心透光
            const qreal gdx = (x - cx) / glowScale;
            const qreal gdy = (y - glowCy) / glowScale;
            const qreal gd = std::sqrt(gdx * gdx + gdy * gdy);
            const qreal g = qBound(0.0, 1.0 - gd, 1.0);
            cr += glowA * g * g * 255.0;
            cg += glowA * g * g * 255.0;
            cb += glowA * g * g * 255.0;

            const int ar = qBound(0, int(cr + 0.5), 255);
            const int ag = qBound(0, int(cg + 0.5), 255);
            const int ab = qBound(0, int(cb + 0.5), 255);
            const int aa = qBound(0, int(clipA * 255.0 + 0.5), 255);
            line[x] = qRgba(ar, ag, ab, aa);
        }
    }
    return out;
}
