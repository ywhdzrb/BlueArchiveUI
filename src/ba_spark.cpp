// BA 点击粒子组件实现（按用户规格：点击=渐入渐出蓝圆+彩色圆角三角散射+月牙波纹；
// 拖动=渐细渐隐轨迹线+随机三角散出。简单半透明绘制，亮背景不变白、复杂 GL 均废弃）

#include "ba_spark.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QStringList>
#include <QtMath>
#include <cmath>
#include <algorithm>

namespace {

// 圆形 / 三角 / 月牙生命周期（毫秒）
constexpr int kCircleLifeMs = 700;
constexpr int kTriLifeMs = 800;
constexpr int kCrescentLifeMs = 640;

// 轨迹线总寿命（秒），越老越细越淡
constexpr qreal kTrailLife = 0.22; // 拖尾总寿命（越短消失越快）

// 固定星色板（白 / 粉 #FF9ECF / 浅蓝 #7ED6FF）
const QColor kPalette[] = {
    QColor(255, 255, 255),
    QColor(0xFF, 0x9E, 0xCF),
    QColor(0x7E, 0xD6, 0xFF),
};
constexpr int kPaletteCount = int(sizeof(kPalette) / sizeof(kPalette[0]));

// 圆角等腰三角形路径（顶点朝上、底水平、三小圆角）
QPainterPath roundedTriPath(qreal s)
{
    QPainterPath path;
    path.moveTo(-s * 0.46, s * 0.37);
    path.lineTo(-s * 0.12, -s * 0.56);
    path.quadTo(0.0, -s * 0.68, s * 0.12, -s * 0.56);      // 顶角
    path.lineTo(s * 0.46, s * 0.37);
    path.quadTo(s * 0.62, s * 0.50, s * 0.36, s * 0.52);   // 右底角
    path.lineTo(-s * 0.36, s * 0.52);
    path.quadTo(-s * 0.62, s * 0.50, -s * 0.46, s * 0.37); // 左底角
    path.closeSubpath();
    return path;
}

// 月牙形（OddEven 挖孔：大圆 - 偏移小圆）
QPainterPath crescentPath(qreal s)
{
    QPainterPath path;
    path.setFillRule(Qt::OddEvenFill);
    path.addEllipse(QPointF(0, 0), s, s);
    path.addEllipse(QPointF(s * 0.62, 0), s * 0.86, s * 0.86);
    return path;
}

} // namespace

BaSpark::BaSpark(QWidget *parent)
    : QWidget(parent)
    , color_(0x4E, 0xC3, 0xF5)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setCursor(Qt::CrossCursor);
}

void BaSpark::setColor(const QString &rgb)
{
    // "r,g,b" 兼容 BASpark 旧调用
    const QStringList cs = rgb.split(',');
    if (cs.size() == 3)
        setColor(QColor(cs[0].toInt(), cs[1].toInt(), cs[2].toInt()));
}

void BaSpark::setColor(const QColor &color)
{
    color_ = color;
    update();
}

void BaSpark::setScale(qreal scale)
{
    scale_ = scale;
}

void BaSpark::setFxOpacity(qreal opacity)
{
    opacity_ = opacity;
}

void BaSpark::setAlwaysTrail(bool on)
{
    alwaysTrail_ = on;
}

void BaSpark::setInteractive(bool on)
{
    interactive_ = on;
    // 装饰层模式：事件透传 + 安装全局过滤器监听（任意控件上按下/移动都触发粒子）
    setAttribute(Qt::WA_TransparentForMouseEvents, !on);
    if (on)
        qApp->removeEventFilter(this);
    else
        qApp->installEventFilter(this);
}

bool BaSpark::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    if (event->type() == QEvent::MouseMove) {
        if (!alwaysTrail_ && !down_)
            return false; // 既非常驻又未按下：不跟
        auto *me = static_cast<QMouseEvent *>(event);
        moveTo(me->position());
    } else if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        clickAt(me->position());
    } else if (event->type() == QEvent::MouseButtonRelease) {
        releaseEffect();
    }
    return false; // 不拦截事件（其他控件照常接收）
}

QColor BaSpark::randomParticleColor() const
{
    // 星粒子三色随机：白 / 粉（#FF9ECF）/ 浅蓝（#7ED6FF）
    return kPalette[QRandomGenerator::global()->bounded(kPaletteCount)];
}

void BaSpark::clickAt(const QPointF &pos)
{
    down_ = true;
    lastPos_ = pos;
    hasLast_ = true;
    // 开始新一轮轨迹：清掉旧点，只留当前断点（旧马尾立即淡出，绝不与新段连线）
    trail_.clear();
    trail_.append(TrailPt{pos, 0.0, true});
    spawnClick(pos);
    ensureTimer();
}

void BaSpark::spawnClick(const QPointF &pos)
{
    // ① 原版 filledCircle：三次缓动半径 + 线性淡出（无白芯，原版即无色芯）
    if (parts_.size() < 64) {
        Particle c;
        c.p = pos;
        c.kind = 0;
        c.T = 16.0 / 60.0;  // maxLife 16 帧
        c.s = scale_;
        c.c = color_;
        parts_.append(c);
    }

    // ② 原版双光弧环：白起点→青终点、半径=球半径+随机偏移、随机角速度
    for (int i = 0; i < 2; i++) {
        Particle r;
        r.p = pos;
        r.kind = 3;
        r.T = 23.0 / 60.0;  // maxLife 23 帧
        r.s = scale_;
        r.rot = (i == 0) ? 0.0 : (QRandomGenerator::global()->generateDouble() * 3.0 - 1.5) * M_PI; // 弧 off 防对称
        const qreal rsList[3] = { 0.0, 0.08, 0.1 };                 // 原版角速度（rad/帧）
        r.vr = rsList[QRandomGenerator::global()->bounded(3)] * 60.0; // 转 rad/s
        const qreal rrList[4] = { 0.0, 1.0, 1.5, 2.0 };             // 原版半径偏移
        r.rr = rrList[QRandomGenerator::global()->bounded(4)];
        parts_.append(r);
    }

    // ③ 星 4 颗（白/粉/蓝随机）：锐三角 (0,-s)→(0.6s,0.6s)→(-0.6s,0.6s)，速度/旋转/摩擦全帧制换算
    const qreal speedAdjust = scale_ / 1.5;
    for (int i = 0; i < 4; i++) {
        const qreal a = QRandomGenerator::global()->generateDouble() * 2.0 * M_PI;
        Particle s;
        s.p = pos;
        s.v = QPointF(std::cos(a), std::sin(a)) *
              (4.8 + QRandomGenerator::global()->generateDouble() * 2.0) * speedAdjust * 60.0; // 原版 4.8~6.8 px/帧
        s.kind = 4;
        s.c = randomParticleColor();
        s.T = 31.0 / 60.0; // a -= 0.032/帧 → 31 帧寿命
        s.s = (4.0 + QRandomGenerator::global()->generateDouble() * 3.0) * scale_;
        s.rot = QRandomGenerator::global()->generateDouble() * 2.0 * M_PI;
        s.vr = (QRandomGenerator::global()->generateDouble() - 0.5) * 0.28 * 60.0; // rs rad/帧
        parts_.append(s);
    }
}

// 拖动喷星：点击星同款（白色锐三角、同尺寸/速度/旋转/衰减）
void BaSpark::spawnTri(const QPointF &pos, qreal speedBase, qreal sizeMul)
{
    if (parts_.size() >= 64)
        return;
    const qreal speedAdjust = scale_ / 1.5;
    const qreal a = QRandomGenerator::global()->generateDouble() * 2.0 * M_PI;
    const qreal speed = (4.8 + QRandomGenerator::global()->generateDouble() * 2.0) *
                        speedAdjust * 60.0 * speedBase;
    Particle star;
    star.p = pos;
    star.v = QPointF(std::cos(a), std::sin(a)) * speed;
    star.c = randomParticleColor();;
    star.kind = 4;
    star.T = 0.55 + QRandomGenerator::global()->generateDouble() * 0.15;
    star.s = (4.0 + QRandomGenerator::global()->generateDouble() * 3.0) * scale_ * sizeMul;
    star.rot = QRandomGenerator::global()->generateDouble() * 2.0 * M_PI;
    star.vr = (QRandomGenerator::global()->generateDouble() - 0.5) * 0.28 * 60.0;
    parts_.append(star);
}


void BaSpark::moveTo(const QPointF &pos)
{
    if (!down_ && !alwaysTrail_)
        return;
    if (hasLast_ && (pos - lastPos_).manhattanLength() < 2.0)
        return;

    lastPos_ = pos;
    hasLast_ = true;

    // 轨迹点：新点入列，超长弹头；按最小步长抽稀防段间圆头重叠成斑块
    if (trail_.isEmpty() || (pos - trail_.last().p).manhattanLength() >= 6.0) {
        trail_.append(TrailPt{pos, 0.0});
        if (trail_.size() > 24)
            trail_.removeFirst();
    }

    // 拖动喷星：30% 概率 1 颗（每帧限 1 颗 trailQuota_ 节流），与点击星同款
    if (!trailQuota_ && QRandomGenerator::global()->generateDouble() < 0.3) {
        trailQuota_ = true;
        spawnTri(pos, 1.0, 1.0);
    }
    ensureTimer();
}

void BaSpark::releaseEffect()
{
    down_ = false;
    hasLast_ = false;
}

void BaSpark::clearEffects()
{
    parts_.clear();
    trail_.clear();
    update();
}

void BaSpark::ensureTimer()
{
    if (!timer_) {
        timer_ = new QTimer(this);
        timer_->setInterval(16);
        connect(timer_, &QTimer::timeout, this, &BaSpark::tick);
    }
    if (!timer_->isActive())
        timer_->start();
}

void BaSpark::tick()
{
    trailQuota_ = false; // 每帧重置轨迹喷三角配额

    const qreal dt = 16.0 / 1000.0;

    // 演进粒子
    for (auto &pt : parts_) {
        pt.t += dt;
        pt.p += pt.v * dt;
        pt.rot += pt.vr * dt;
    }
    // 过期弹掉（保留顺序）
    {
        QVector<Particle> alive;
        alive.reserve(parts_.size());
        for (const Particle &pt : parts_)
            if (pt.t < pt.T)
                alive.append(pt);
        parts_ = std::move(alive);
    }

    // 演进轨迹
    for (auto &tp : trail_)
        tp.age += dt;
    {
        QVector<TrailPt> alive;
        alive.reserve(trail_.size());
        for (const TrailPt &tp : trail_)
            if (tp.age < kTrailLife)
                alive.append(tp);
        trail_ = std::move(alive);
    }

    const bool hasWork = !parts_.isEmpty() || !trail_.isEmpty();
    if (!hasWork) {
        timer_->stop();
        update();
        return;
    }
    update();
}

void BaSpark::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // —— 轨迹线（后段越细越淡，带加粗低透明眩光层）——
    for (int i = 1; i < trail_.size(); i++) {
        const TrailPt &a = trail_[i - 1];
        const TrailPt &b = trail_[i];
        if (a.gap)
            continue; // 断点段：跨两次点击等场景不连线
        const qreal ti = b.age / kTrailLife; // 0 新 → 1 老
        if (ti >= 1.0)
            continue;
        const qreal alpha = (1.0 - ti) * 0.7;
        if (alpha < 0.03)
            continue;
        QColor c = color_;
        c.setAlphaF(alpha * opacity_);
        const qreal w = std::max(0.8, (1.0 - ti) * 3.0 * scale_);
        QPen pen(c, w, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.drawLine(a.p, b.p);
    }

    // —— 粒子 ——
    for (const Particle &pt : parts_) {
        const qreal prog = pt.t / pt.T; // 0 → 1
        p.save();
        p.translate(pt.p);
        p.rotate(qRadiansToDegrees(pt.rot));
        switch (pt.kind) {
        case 0: { // 原版 filledCircle：半径 = 26*scale*(1-(1-prog)^3) 三次缓动、alpha 线性 1-prog；无眩光
            const qreal ease = 1.0 - std::pow(1.0 - prog, 3);
            const qreal r = 26.0 * pt.s * ease;
            QColor c = pt.c;
            c.setAlphaF(qMax(0.0, 1.0 - prog) * opacity_);
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawEllipse(QPointF(0, 0), r, r);
            break;
        }
        case 1: { // 彩色圆角三角：渐出（无眩光）
            QColor c = pt.c;
            c.setAlphaF(c.alphaF() * (1.0 - prog) * opacity_);
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawPath(roundedTriPath(pt.s * 22.0));
            break;
        }
        case 2: { // 月牙（纯色渐出，无眩光）
            QColor c = pt.c;
            c.setAlphaF(c.alphaF() * (1.0 - prog) * opacity_);
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawPath(crescentPath(pt.s * 16.0));
            break;
        }
        case 3: { // 原版 ring 光弧：白(250,252,252)→终点(主色+510)/3、弧长 0.1 生长/0.4 收缩、线宽中间粗
            const qreal p3 = prog;
            const qreal ease = 1.0 - std::pow(1.0 - p3, 3);
            const qreal waveR = 26.0 * pt.s * ease;
            const qreal r = waveR + pt.rr * pt.s; // 半径 = 球半径 + 随机偏移
            qreal ratio = 1.0;
            if (p3 < 0.1)
                ratio = p3 * 10.0;
            else if (p3 > 0.4)
                ratio = qMax(0.0, 1.0 - (p3 - 0.4) / 0.6);
            const qreal len = 1.1 * M_PI * ratio;
            if (len < 0.02)
                break;
            const qreal wProp = qMin(2.0 - std::fabs(4.0 * (p3 - 0.5)), 1.0); // 保留（整体强度乘子）
            const qreal widthMul = qMin(-0.8 * (p3 - 0.8) + 1.0, 1.0);
            const qreal ang = pt.rot + pt.vr * pt.t;
            // 弧色：近白起点 → 终点色 (色+510)/3（逐通道 2:1 向白拉）
            const int e1r = (pt.c.red() + 510) / 3;
            const int e1g = (pt.c.green() + 510) / 3;
            const int e1b = (pt.c.blue() + 510) / 3;
            auto mix_i = [p3](int a, int b) { return qRound(a + (b - a) * p3); };
            const int cw = mix_i(250, e1r);
            const int cg = mix_i(252, e1g);
            const int cb = mix_i(252, e1b);
            QColor arc(cw, cg, cb);
            arc.setAlphaF(qBound(0.0, qMin(1.1 - 0.3 * p3, 1.0) * opacity_, 1.0));
            p.setPen(Qt::NoPen);
            p.setBrush(arc);
            const QRectF arcRect(-r, -r, r * 2.0, r * 2.0);
            const int a0 = qRound(qRadiansToDegrees(ang) * 16.0);
            const int aLen = qRound(qRadiansToDegrees(len) * 16.0);
            // 分段绘制：弧上位置 t 中段最粗、两端细（getWeightProp），整体随 prog 收缩（widthMul）
            const int kSeg = 10;
            for (int i = 0; i < kSeg; i++) {
                const qreal t = (i + 0.5) / qreal(kSeg);
                const qreal wProp = qMin(2.0 - std::fabs(4.0 * (t - 0.5)), 1.0);
                const qreal w = (0.4 + (3.3 - 0.4) * wProp) * widthMul;
                p.setPen(QPen(arc, w, Qt::SolidLine, Qt::RoundCap));
                p.drawArc(arcRect,
                          a0 + qRound(qRadiansToDegrees(len / kSeg * i) * 16.0),
                          qRound(qRadiansToDegrees(len / kSeg) * 16.0));
            }
            break;
        }
        case 4: { // 原版白星：锐利三角，alpha 每帧 -0.032 线性衰减（带放大概率眩光）
            const qreal a = qMax(0.0, 1.0 - 0.032 * (pt.t * 60.0));
            if (a <= 0.02)
                break;
            QColor c = pt.c;
            c.setAlphaF(a * opacity_);
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            const qreal s = pt.s;
            QPainterPath tri;
            tri.moveTo(0, -s);
            tri.lineTo(s * 0.6, s * 0.6);
            tri.lineTo(-s * 0.6, s * 0.6);
            tri.closeSubpath();
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawPath(tri);
            break;
        }
        default:
            break;
        }
        p.restore();
    }
}

void BaSpark::mousePressEvent(QMouseEvent *event)
{
    clickAt(event->position());
    event->accept();
}

void BaSpark::mouseMoveEvent(QMouseEvent *event)
{
    moveTo(event->position());
    event->accept();
}

void BaSpark::mouseReleaseEvent(QMouseEvent *event)
{
    releaseEffect();
    event->accept();
}
