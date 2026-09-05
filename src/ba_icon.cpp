#include "ba_icon.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <cmath>

#include "ba_assets.h"
#include "ba_style.h"

namespace ba {

namespace {

// 优先绘制官方解包图标素材：root 下找到即画，找不到返回 false 走手绘回退。
bool paintOfficial(QPainter &p, const QRectF &r, const QString &relPath)
{
    const QPixmap off = BaAssets::image(relPath);
    if (off.isNull())
        return false;
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawPixmap(r.toRect(), off);
    return true;
}

// 将单位坐标 (0~1) 缩放进目标框：绘制统一从小点阵放大，保证各尺寸一致。
QPointF u(const QRectF &r, qreal x, qreal y)
{
    return QPointF(r.left() + r.width() * x, r.top() + r.height() * y);
}

} // namespace

QPixmap pixmap(Glyph glyph, const QSize &size, const QColor &tint)
{
    QColor accent = tint.isValid() ? tint : BaStyle::accent();
    const QColor deep = BaStyle::deep();
    const QColor yellow = QColor("#FFD84D");

    QPixmap pm(size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r(QPointF(0, 0), QSizeF(size));
    const qreal edge = qMin(r.width(), r.height());

    auto filledCircle = [&](QPointF c, qreal d, const QColor &color) {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(c, d / 2, d / 2);
    };

    // 五角星：BA 金币中心星星（尖头朝上）
    auto starPath = [&](QPointF c, qreal R, qreal rInner) {
        QPainterPath path;
        for (int i = 0; i < 10; ++i) {
            const qreal ang = i * M_PI / 5 - M_PI / 2;
            const qreal rad = (i % 2 == 0) ? R : rInner;
            QPointF pt(c.x() + rad * std::cos(ang), c.y() + rad * std::sin(ang));
            if (i == 0) path.moveTo(pt); else path.lineTo(pt);
        }
        path.closeSubpath();
        return path;
    };

    switch (glyph) {
    case Glyph::Lightning: {
        // AP 体力：官方解包素材优先，无则手绘
        if (paintOfficial(p, r, QStringLiteral("img/icons/Common_Icon_Stamina.png")))
            break;
        // AP：绿色渐变圆灯 + 白色闪电（游戏内 AP 能源图标形态）
        QRadialGradient lg(QRectF(u(r, 0.16, 0.12), u(r, 0.66, 0.62)).center(), edge * 0.52);
        lg.setColorAt(0, QColor("#B9F7A8"));
        lg.setColorAt(0.62, QColor("#7CD95E"));
        lg.setColorAt(1, QColor("#3E9E35"));
        p.setBrush(Qt::NoBrush);
        p.setPen(Qt::NoPen);
        p.setBrush(lg);
        p.drawEllipse(r.center(), edge / 2, edge / 2);
        // 底缘反光
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(0, 90, 40, 110), qMax(1.0, edge * 0.05)));
        p.drawEllipse(r.center(), edge * 0.46, edge * 0.46);
        QPainterPath bolt;
        bolt.moveTo(u(r, 0.58, 0.16));
        bolt.lineTo(u(r, 0.32, 0.60));
        bolt.lineTo(u(r, 0.52, 0.60));
        bolt.lineTo(u(r, 0.44, 0.88));
        bolt.lineTo(u(r, 0.70, 0.42));
        bolt.lineTo(u(r, 0.50, 0.42));
        bolt.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawPath(bolt);
        // 左上高光点
        p.setBrush(QColor(255, 255, 255, 210));
        p.drawEllipse(u(r, 0.30, 0.30), edge * 0.09, edge * 0.06);
        break;
    }
    case Glyph::Coin: {
        // 金币：官方解包素材优先
        if (paintOfficial(p, r, QStringLiteral("img/icons/Common_Icon_Gold_Base.png")))
            break;
        // 金币：金渐变圆 + 白色 C 环 + 深金描边（游戏货币图标形态）
        QRadialGradient g(QRectF(u(r, 0.20, 0.14), u(r, 0.72, 0.66)).center(), edge * 0.52);
        g.setColorAt(0, QColor("#FFEFA9"));
        g.setColorAt(0.6, QColor("#FFD75E"));
        g.setColorAt(1, QColor("#F0A93B"));
        p.setBrush(g);
        p.setPen(Qt::NoPen);
        p.drawEllipse(r.center(), edge / 2, edge / 2);
        // 白色 C 环（偏右开口）+ 内高光
        p.setPen(QPen(QColor("#FFFDF4"), qMax(1.2, edge * 0.13), Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(u(r, 0.24, 0.24), u(r, 0.76, 0.76)), 60 * 16, 250 * 16);
        // 外缘描边
        p.setPen(QPen(QColor(180, 110, 30, 140), qMax(1.0, edge * 0.05)));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(r.center(), edge * 0.48, edge * 0.48);
        break;
    }
    case Glyph::Pyroxene: {
        // 青辉石：官方解包素材优先
        if (paintOfficial(p, r, QStringLiteral("img/icons/Common_Icon_Diamond.png")))
            break;
        // 青辉石：五棱晶体簇（近白色高光，青色半透明面）——游戏内奖励盒图标形态
        // 大晶体：五边形（顶尖 + 双肩 + 底），多面渐变；
        // 底侧两小晶体陪衬
        QLinearGradient g(u(r, 0.32, 0.06), u(r, 0.68, 0.90));
        g.setColorAt(0, QColor("#E8FAFF"));
        g.setColorAt(0.45, QColor("#96DFFA"));
        g.setColorAt(1, QColor("#4CA3DD"));
        QPainterPath crystal;
        crystal.moveTo(u(r, 0.50, 0.06));
        crystal.lineTo(u(r, 0.74, 0.34));
        crystal.lineTo(u(r, 0.66, 0.82));
        crystal.lineTo(u(r, 0.34, 0.82));
        crystal.lineTo(u(r, 0.26, 0.34));
        crystal.closeSubpath();
        p.setPen(QPen(QColor(255, 255, 255, 170), qMax(1.0, edge * 0.04)));
        p.setBrush(g);
        p.drawPath(crystal);
        // 左高光面
        QPainterPath facet;
        facet.moveTo(u(r, 0.50, 0.06));
        facet.lineTo(u(r, 0.26, 0.34));
        facet.lineTo(u(r, 0.34, 0.82));
        facet.lineTo(u(r, 0.50, 0.46));
        facet.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, 120));
        p.drawPath(facet);
        // 底部小晶体两枚
        QPainterPath small1;
        small1.moveTo(u(r, 0.80, 0.72));
        small1.lineTo(u(r, 0.94, 0.78));
        small1.lineTo(u(r, 0.88, 0.96));
        small1.lineTo(u(r, 0.74, 0.92));
        small1.closeSubpath();
        QPainterPath small2;
        small2.moveTo(u(r, 0.08, 0.66));
        small2.lineTo(u(r, 0.22, 0.70));
        small2.lineTo(u(r, 0.18, 0.94));
        small2.lineTo(u(r, 0.04, 0.90));
        small2.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x8E, 0xD4, 0xF6));
        p.drawPath(small1);
        p.drawPath(small2);
        break;
    }
    case Glyph::Return: {
        // BA 返回钮：深蓝渐变圆 + 白色左箭头 + 浅蓝外描边
        QLinearGradient bg(r.topLeft(), r.bottomLeft());
        bg.setColorAt(0, QColor("#2E6CA8"));
        bg.setColorAt(1, QColor("#0F3761"));
        p.setBrush(Qt::NoBrush);
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawEllipse(r.center(), edge / 2, edge / 2);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(0x9C, 0xD9, 0xF2, 190), qMax(1.0, edge * 0.05)));
        p.drawEllipse(r.center(), edge * 0.46, edge * 0.46);
        p.setPen(QPen(Qt::white, edge * 0.12, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath arrow;
        arrow.moveTo(u(r, 0.64, 0.22));
        arrow.lineTo(u(r, 0.36, 0.50));
        arrow.lineTo(u(r, 0.64, 0.78));
        p.drawPath(arrow);
        break;
    }
    case Glyph::Home: {
        // 白色线稿房子（tint 可指定颜色）：顶栏功能性图标
        p.setPen(QPen(accent, edge * 0.09, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        QPainterPath house;
        house.moveTo(u(r, 0.16, 0.50));
        house.lineTo(u(r, 0.50, 0.16));
        house.lineTo(u(r, 0.84, 0.50));
        house.moveTo(u(r, 0.26, 0.44));
        house.lineTo(u(r, 0.26, 0.82));
        house.lineTo(u(r, 0.74, 0.82));
        house.lineTo(u(r, 0.74, 0.44));
        house.moveTo(u(r, 0.44, 0.82));
        house.lineTo(u(r, 0.44, 0.60));
        house.lineTo(u(r, 0.58, 0.60));
        house.lineTo(u(r, 0.58, 0.82));
        p.drawPath(house);
        break;
    }
    case Glyph::Close: {
        filledCircle(r.center(), edge, QColor("#FF6E67"));
        p.setPen(QPen(Qt::white, edge * 0.11, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(u(r, 0.32, 0.32), u(r, 0.68, 0.68));
        p.drawLine(u(r, 0.68, 0.32), u(r, 0.32, 0.68));
        break;
    }
    case Glyph::Check: {
        p.setPen(QPen(accent, edge * 0.16, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath tick;
        tick.moveTo(u(r, 0.22, 0.52));
        tick.lineTo(u(r, 0.43, 0.74));
        tick.lineTo(u(r, 0.80, 0.28));
        p.drawPath(tick);
        break;
    }
    case Glyph::Plus: {
        // 白色线稿加号（tint 可指定颜色）：顶栏功能性图标
        p.setPen(QPen(accent, edge * 0.11, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(u(r, 0.50, 0.20), u(r, 0.50, 0.80));
        p.drawLine(u(r, 0.20, 0.50), u(r, 0.80, 0.50));
        break;
    }
    case Glyph::Speaker: {
        // 喇叭：矩形 + 斜筒 + 右侧两个声波弧
        p.setPen(Qt::NoPen);
        p.setBrush(accent);
        QPainterPath speaker;
        speaker.moveTo(u(r, 0.18, 0.40));
        speaker.lineTo(u(r, 0.38, 0.40));
        speaker.lineTo(u(r, 0.60, 0.20));
        speaker.lineTo(u(r, 0.60, 0.80));
        speaker.lineTo(u(r, 0.38, 0.62));
        speaker.lineTo(u(r, 0.18, 0.62));
        speaker.closeSubpath();
        p.drawPath(speaker);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(accent, edge * 0.07, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(QRectF(u(r, 0.60, 0.30), u(r, 0.74, 0.40)), -70 * 16, 140 * 16);
        p.drawArc(QRectF(u(r, 0.64, 0.10), u(r, 0.84, 0.25)), -70 * 16, 140 * 16);
        break;
    }
    case Glyph::SpeakerMini: {
        // 喇叭本体（无声波）：左端滑块的音量图标
        QPainterPath speaker;
        speaker.moveTo(u(r, 0.18, 0.38));
        speaker.lineTo(u(r, 0.38, 0.38));
        speaker.lineTo(u(r, 0.56, 0.20));
        speaker.lineTo(u(r, 0.56, 0.80));
        speaker.lineTo(u(r, 0.38, 0.62));
        speaker.lineTo(u(r, 0.18, 0.62));
        speaker.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(accent);
        p.drawPath(speaker);
        break;
    }
    case Glyph::Mute: {
        // 喇叭本体 + 红色斜杠
        QPainterPath speaker;
        speaker.moveTo(u(r, 0.18, 0.38));
        speaker.lineTo(u(r, 0.38, 0.38));
        speaker.lineTo(u(r, 0.56, 0.20));
        speaker.lineTo(u(r, 0.56, 0.80));
        speaker.lineTo(u(r, 0.38, 0.62));
        speaker.lineTo(u(r, 0.18, 0.62));
        speaker.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(accent);
        p.drawPath(speaker);
        p.setPen(QPen(QColor("#FF6E67"), edge * 0.10, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(u(r, 0.24, 0.74), u(r, 0.76, 0.22));
        break;
    }
    case Glyph::Search: {
        p.setPen(QPen(accent, edge * 0.10, Qt::SolidLine, Qt::RoundCap));
        p.drawEllipse(QRectF(u(r, 0.16, 0.16), u(r, 0.66, 0.66)));
        p.drawLine(u(r, 0.62, 0.62), u(r, 0.84, 0.84));
        break;
    }
    case Glyph::Settings: {
        // 白色线稿齿轮（tint 可指定颜色）：顶栏功能性图标（8 齿环）
        QPainterPath gear;
        const qreal cx = r.center().x(), cy = r.center().y();
        const qreal R = edge * 0.34;
        const qreal rIn = edge * 0.24;
        for (int i = 0; i < 8; ++i) {
            const qreal a = i * M_PI / 4;
            const QPointF c(cx + R * std::cos(a), cy + R * std::sin(a));
            // 齿：以角点为中心的小方片，随角旋转
            gear.addEllipse(c, edge * 0.075, edge * 0.075);
        }
        gear.addEllipse(cx, cy, rIn, rIn);
        p.setPen(QPen(accent, edge * 0.10, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        p.drawPath(gear);
        break;
    }
    case Glyph::Ticket: {
        // 证件 / 票卡：圆角矩形 + 横向剪切口
        const QRectF card(u(r, 0.16, 0.20), u(r, 0.84, 0.80));
        p.setBrush(accent);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(card, edge * 0.10, edge * 0.10);
        // 左侧两条白色虚线齿口（ticket 样式）
        p.setPen(QPen(Qt::white, qMax(1.0, edge * 0.06), Qt::SolidLine));
        p.drawLine(QPointF(card.left(), card.center().y() - edge * 0.12),
                   QPointF(card.left() + edge * 0.10, card.center().y() - edge * 0.12));
        p.drawLine(QPointF(card.left(), card.center().y() + edge * 0.12),
                   QPointF(card.left() + edge * 0.10, card.center().y() + edge * 0.12));
        break;
    }
    case Glyph::Mail: {
        // 信封：圆角矩形 + 内折线
        QPainterPath env;
        const QRectF card(u(r, 0.14, 0.20), u(r, 0.86, 0.80));
        p.setPen(QPen(accent, qMax(1.2, edge * 0.09), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(card, edge * 0.08, edge * 0.08);
        p.drawPolyline(QPolygonF({ card.topLeft() + QPointF(edge * 0.06, edge * 0.08),
                                   card.center(),
                                   card.bottomRight() - QPointF(edge * 0.06, edge * 0.08) }));
        break;
    }
    case Glyph::Grid: {
        // 棋盘格：四个象限，对角两格着色
        p.setBrush(accent);
        p.setPen(Qt::NoPen);
        p.fillRect(QRectF(u(r, 0.20, 0.20), u(r, 0.50, 0.50)).toRect(), accent);  // 左上
        p.fillRect(QRectF(u(r, 0.50, 0.50), u(r, 0.80, 0.80)).toRect(), accent);  // 右下
        break;
    }
    case Glyph::Star: {
        // 五角星：默认白色（お仕事左上角装饰）
        p.setBrush(tint.isValid() ? tint : QColor("#FFFFFF"));
        p.setPen(Qt::NoPen);
        p.drawPath(starPath(r.center(), edge * 0.46, edge * 0.19));
        break;
    }
    case Glyph::Arrow: {
        // 白色大三角（朝向由 tint 控制默认右）
        p.setBrush(tint.isValid() ? tint : Qt::white);
        p.setPen(Qt::NoPen);
        QPainterPath tri;
        tri.moveTo(u(r, 0.30, 0.14));
        tri.lineTo(u(r, 0.74, 0.50));
        tri.lineTo(u(r, 0.30, 0.86));
        tri.closeSubpath();
        p.drawPath(tri);
        break;
    }
    case Glyph::Music: {
        // 音符：竖直符杆 ×2 + 符头椭圆 + 横梁
        p.setPen(QPen(accent, qMax(1.4, edge * 0.11), Qt::SolidLine, Qt::RoundCap));
        const qreal x1 = edge * 0.36, x2 = edge * 0.60;
        p.drawLine(QPointF(x1, edge * 0.22), QPointF(x1, edge * 0.72));
        p.drawLine(QPointF(x2, edge * 0.16), QPointF(x2, edge * 0.66));
        p.setPen(Qt::NoPen);
        p.setBrush(accent);
        p.drawEllipse(QPointF(u(r, 0.30, 0.76)), edge * 0.11, edge * 0.08);
        p.drawEllipse(QPointF(u(r, 0.54, 0.70)), edge * 0.11, edge * 0.08);
        p.drawRect(QRectF(u(r, 0.36, 0.14), u(r, 0.60, 0.20)));
        break;
    }
    case Glyph::Pencil: {
        // 铅笔（账号信息页编辑钮）：45° 笔杆 + 笔尖三角 + 笔尾圆。tint 可指定（默认深蓝）
        const QColor pen = tint.isValid() ? tint : QColor(0x2E, 0x5F, 0x8A);
        p.setPen(Qt::NoPen);
        p.setBrush(pen);
        p.drawPolygon(QPolygonF({
            u(r, 0.24, 0.68), u(r, 0.66, 0.26), u(r, 0.74, 0.34), u(r, 0.32, 0.76)
        }));
        p.setBrush(pen.lighter(120));
        p.drawPolygon(QPolygonF({
            u(r, 0.28, 0.72), u(r, 0.36, 0.64), u(r, 0.32, 0.60), u(r, 0.24, 0.68)
        }));
        p.setBrush(QColor(0xF2, 0x99, 0x37));   // 橙色笔尖
        p.drawEllipse(QRectF(u(r, 0.14, 0.74), u(r, 0.32, 0.88)));
        break;
    }
    case Glyph::Copy: {
        // 复制（UID 胶囊）：两张叠错矩形纸片 + 右上对角折角
        p.setPen(QPen(accent, edge * 0.075, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(QPolygonF({ u(r, 0.18, 0.30), u(r, 0.70, 0.30),
                                  u(r, 0.70, 0.82), u(r, 0.18, 0.82) }));
        p.setPen(QPen(QColor(accent.red(), accent.green(), accent.blue(), 170),
                      edge * 0.075, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(QColor(0xF0, 0xF6, 0xFA));
        p.drawPolygon(QPolygonF({ u(r, 0.32, 0.12), u(r, 0.78, 0.12),
                                  u(r, 0.78, 0.62), u(r, 0.32, 0.62) }));
        p.drawPolygon(QPolygonF({ u(r, 0.32, 0.12), u(r, 0.62, 0.12),
                                  u(r, 0.62, 0.48), u(r, 0.32, 0.48) }));
        break;
    }
    case Glyph::MarkRing: {
        // 十字圆环徽章（稱號输入框左端装饰）：外圆环 + 中心点位 + 十字准星
        p.setPen(QPen(QColor(0x6E, 0x8B, 0xA0), edge * 0.07, Qt::SolidLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(u(r, 0.18, 0.18), u(r, 0.82, 0.82)));
        p.setPen(QPen(QColor(0x4E, 0x7A, 0x96), edge * 0.06, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(u(r, 0.50, 0.22), u(r, 0.50, 0.34));
        p.drawLine(u(r, 0.50, 0.66), u(r, 0.50, 0.78));
        p.drawLine(u(r, 0.22, 0.50), u(r, 0.34, 0.50));
        p.drawLine(u(r, 0.66, 0.50), u(r, 0.78, 0.50));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x4E, 0x7A, 0x96));
        p.drawEllipse(u(r, 0.50, 0.50), edge * 0.09, edge * 0.09);
        break;
    }
    }

    p.end();
    return pm;
}

} // namespace ba
