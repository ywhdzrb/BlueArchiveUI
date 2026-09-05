#include "ba_style.h"

#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QtMath>
#include <cmath>

#include "ba_assets.h"

QColor BaStyle::accent()      { return QColor("#4EC3F5"); }   // 青蓝高亮
QColor BaStyle::sky()         { return QColor("#77DEFF"); }   // 亮青主按钮
QColor BaStyle::deep()        { return QColor("#003153"); }   // 深藏蓝
QColor BaStyle::yellow()      { return QColor("#FFE433"); }   // 装饰黄
QColor BaStyle::lightBlue()   { return QColor("#CDE8FD"); }   // 浅蓝侧栏
QColor BaStyle::muted()       { return QColor("#6B7F8D"); }   // 灰蓝次文字
QColor BaStyle::dash()        { return QColor("#C9D8E2"); }   // 虚线
QColor BaStyle::track()       { return QColor("#DFE6EA"); }   // 轨道底
QColor BaStyle::panelBorder() { return QColor(0x6B, 0x7F, 0x8D, 102); } // 40% 灰蓝
QColor BaStyle::white()       { return QColor("#FFFFFF"); }
QColor BaStyle::dim()         { return QColor(0, 22, 40, 130); }

QPainterPath BaStyle::skewRectPath(const QRectF &rect, qreal skewDeg, qreal radius)
{
    // CSS skewX 语义：x' = x + tan(deg) * (y - cy)，围绕中心剪切（transform-origin: center）。
    // 修正：剪切后平行四边形的外接宽 = w + |tan|*h，超出控件矩形部分会被 QWidget 裁掉
    // （此前左右两侧总被切没）。故先沿 x 整体压缩 sx = 1 - |tan|*h/w，再剪切，
    // 使结果恰好落回矩形内——外形与 CSS skew 完整图形一致，只是按比例略窄。
    const qreal k = qTan(qDegreesToRadians(skewDeg));
    const qreal sx = qMax(0.2, 1.0 - qAbs(k) * rect.height() / qMax(rect.width(), 1.0));

    QPainterPath base;
    base.addRoundedRect(rect, radius, radius);
    QTransform t;
    t.translate(rect.center().x(), rect.center().y());
    t.scale(sx, 1.0);
    t.shear(k, 0.0);
    t.translate(-rect.center().x(), -rect.center().y());
    return t.map(base);
}

QPainterPath BaStyle::rightCutPath(const QRectF &rect, qreal cut, qreal radius)
{
    // 左下圆角 + 右上 45° 斜切（BA「お仕事」竖钮形态）
    const qreal r = qMin(radius, qMin(rect.width(), rect.height()) * 0.5);
    const qreal c = qMin(cut, rect.width() * 0.6);
    QPainterPath path;
    path.moveTo(rect.left() + r, rect.top());
    path.lineTo(rect.right() - c, rect.top());             // 上边直段
    path.lineTo(rect.right(), rect.top() + c);             // 45° 切角
    path.lineTo(rect.right(), rect.bottom() - r);
    path.quadTo(rect.right(), rect.bottom(), rect.right() - r, rect.bottom());
    path.lineTo(rect.left() + r, rect.bottom());
    path.quadTo(rect.left(), rect.bottom(), rect.left(), rect.bottom() - r);
    path.lineTo(rect.left(), rect.top() + r);
    path.quadTo(rect.left(), rect.top(), rect.left() + r, rect.top());
    path.closeSubpath();
    return path;
}

QPainterPath BaStyle::baButtonPath(const QRectF &rect, qreal cut, qreal radius)
{
    // BA 官方按钮真实形态：矩形 + 左上/右下两处 45° 小切角 + 右上/左下小圆角
    // （对照游戏「確認/デフォルト」按钮轮廓——不是对称平行四边形，而是「削角八边形」）
    const qreal r = qMin(radius, qMin(rect.width(), rect.height()) * 0.5);
    const qreal c = qMin(cut, qMin(rect.width(), rect.height()) * 0.42);
    QPainterPath path;
    path.moveTo(rect.left(), rect.top() + c);              // 左边缘上端（切角起点）
    path.lineTo(rect.left() + c, rect.top());              // 左上 45° 斜切
    path.lineTo(rect.right() - r, rect.top());             // 顶边
    path.quadTo(rect.right(), rect.top(), rect.right(), rect.top() + r);   // 右上小圆角
    path.lineTo(rect.right(), rect.bottom() - c);          // 右边缘（接下切角起点）
    path.lineTo(rect.right() - c, rect.bottom());          // 右下 45° 斜切
    path.lineTo(rect.left() + r, rect.bottom());           // 底边
    path.quadTo(rect.left(), rect.bottom(), rect.left(), rect.bottom() - r); // 左下小圆角
    path.lineTo(rect.left(), rect.top() + c);              // 左边缘闭合
    path.closeSubpath();
    return path;
}

QLinearGradient BaStyle::accentGradient(const QRectF &r)
{
    QLinearGradient g(r.topLeft(), r.bottomLeft());
    g.setColorAt(0, QColor("#9FE7FB"));
    g.setColorAt(1, QColor("#5FC6F2"));
    return g;
}

QLinearGradient BaStyle::deepGradient(const QRectF &r)
{
    QLinearGradient g(r.topLeft(), r.bottomLeft());
    g.setColorAt(0, QColor("#2E6CA8"));
    g.setColorAt(1, QColor("#0F3761"));
    return g;
}

QLinearGradient BaStyle::panelGradient(const QRectF &r)
{
    QLinearGradient g(r.topLeft(), r.bottomLeft());
    g.setColorAt(0, QColor("#FFFFFF"));
    g.setColorAt(1, QColor("#F1F8FD"));
    return g;
}

QLinearGradient BaStyle::cardInfoGradient(const QRectF &r)
{
    QLinearGradient g(r.topLeft(), r.bottomLeft());
    g.setColorAt(0, QColor("#FFFFFF"));
    g.setColorAt(1, QColor("#EDF3F9"));
    return g;
}

QColor BaStyle::roleColor(ba::SurfaceRole role)
{
    switch (role) {
    case ba::SurfaceRole::Sky:     return QColor("#77DEFF");
    case ba::SurfaceRole::Green:   return QColor("#93EE9D");
    case ba::SurfaceRole::Purple:  return QColor("#A98FF7");
    case ba::SurfaceRole::Yellow:  return QColor("#FFE433");
    case ba::SurfaceRole::Red:     return QColor("#FF6E67");
    case ba::SurfaceRole::Deep:    return QColor("#003153");
    case ba::SurfaceRole::Ghost:   return QColor(0, 0, 0, 0);
    }
    return sky();
}

QColor BaStyle::onRoleColor(ba::SurfaceRole role)
{
    // 深色底（紫 / 红 / 深蓝）配白字，其余亮色底配深藏蓝正文。
    switch (role) {
    case ba::SurfaceRole::Purple:
    case ba::SurfaceRole::Red:
    case ba::SurfaceRole::Deep:
        return QColor("#FFFFFF");
    case ba::SurfaceRole::Ghost:
        return deep();
    default:
        return deep();
    }
}

std::pair<QColor, QColor> BaStyle::roleGradientPair(ba::SurfaceRole role)
{
    // 角色色立体渐变：亮段在上、深段在下，模拟 BA 钮的玻璃凸面。
    switch (role) {
    case ba::SurfaceRole::Sky:    return { QColor("#B8E1F5"), QColor("#70BEE4") };
    case ba::SurfaceRole::Green:  return { QColor("#C8FAD1"), QColor("#7BDF8B") };
    case ba::SurfaceRole::Purple: return { QColor("#D3C4FF"), QColor("#9B7FF2") };
    case ba::SurfaceRole::Yellow: return { QColor("#FFF2A8"), QColor("#FFD84D") };
    case ba::SurfaceRole::Red:    return { QColor("#FFA9A4"), QColor("#F46760") };
    case ba::SurfaceRole::Deep:   return { QColor("#2E6CA8"), QColor("#0F3761") };
    case ba::SurfaceRole::Ghost:  return { QColor("#FFFFFF"), QColor("#EDF3F9") };
    }
    return { QColor("#9FE7FB"), QColor("#5FC6F2") };
}

QLinearGradient BaStyle::orangeGradient(const QRectF &r)
{
    // 橙色立体渐变：任务「成就」标签 / 页签选中块（#FFC95C → #F5821F）
    QLinearGradient g(r.topLeft(), r.bottomLeft());
    g.setColorAt(0, QColor("#FFC95C"));
    g.setColorAt(1, QColor("#F5821F"));
    return g;
}

QColor BaStyle::progressTrackDark()
{
    // 任务进度条轨道：深黑灰（图上近乎黑，带轻微红棕夹角）
    return QColor("#262A31");
}

QColor BaStyle::progressFill()
{
    // 任务进度条填充：亮青蓝，头端微白
    return QColor("#A5E3FF");
}

QLinearGradient BaStyle::ribbonGradient(const QRectF &r)
{
    // 丝带标题条：顶亮底深的藏蓝渐变（账号信息页「稱呼」等卡头）
    QLinearGradient g(r.topLeft(), r.bottomLeft());
    g.setColorAt(0.0, QColor("#4E86BE"));
    g.setColorAt(1.0, QColor("#12396B"));
    return g;
}

qreal BaStyle::easeOutBack(qreal t)
{
    // 参数化对 cubic-bezier(0.3, 1.3, 0.3, 1) 采样：二分逼近参数求 y。
    // 该曲线 y 在 x=0.6 附近顶点约 1.05，呈轻微过冲后回落，符合 BA 面板入场合奏。
    const qreal x1 = 0.3, y1 = 1.3, x2 = 0.3, y2 = 1.0;
    t = qBound(qreal(0), t, qreal(1));
    qreal lo = 0, hi = 1;
    for (int i = 0; i < 24; ++i) {
        qreal mid = (lo + hi) / 2;
        const qreal mt = 1 - mid;
        qreal xt = 3 * mt * mt * mid * x1 + 3 * mt * mid * mid * x2 + mid * mid * mid;
        if (xt < t) lo = mid; else hi = mid;
    }
    qreal s = (lo + hi) / 2;
    const qreal ms = 1 - s;
    return 3 * ms * ms * s * y1 + 3 * ms * s * s * y2 + s * s * s;
}

void BaStyle::slideInFromBottom(QWidget *widget, int durationMs)
{
    QWidget *host = widget->parentWidget();
    if (!host || durationMs <= 0)
        return;
    const int fromY = host->height();               // 移出到父容器下边界外
    const int toY = widget->y();
    widget->move(widget->x(), fromY);

    QVariantAnimation *anim = new QVariantAnimation(widget);
    anim->setDuration(durationMs);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    QObject::connect(anim, &QVariantAnimation::valueChanged, widget,
                     [widget, toY, host](const QVariant &v) {
                         const qreal k = v.toReal();
                         const qreal y = toY + (host->height() - toY) * (1.0 - easeOutBack(k));
                         widget->move(widget->x(), static_cast<int>(y));
                     });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

QFont BaStyle::font(int pointSize, QFont::Weight weight)
{
    // 优先已注册的 BA 官方字体（BaAssets ensureInit 注册 Blueaka / Mushin / Gyeonggi），
    // 取不到再退回系统无衬线列表。
    QFont f;
    const QString fam = BaAssets::uiFontFamily();
    if (!fam.isEmpty()) {
        f.setFamily(fam);
    } else {
        f.setFamilies({QStringLiteral("Nunito Sans"),
                       QStringLiteral("Noto Sans JP"),
                       QStringLiteral("Noto Sans"),
                       QStringLiteral("Source Han Sans Rounded"),
                       QStringLiteral("Source Han Sans SC"),
                       QStringLiteral("Microsoft YaHei")});
    }
    f.setPointSize(pointSize);
    f.setWeight(weight);
    return f;
}
