#include "md3_icon.h"

#include <QPainterPath>
#include <QtMath>

namespace md3 {

// 在 (cx, cy) 处绘制基准 24x24 线性图标。几何沿用侧栏 / 导航条同款路径：
// 描边圆头、圆角，颜色与笔宽（默认 2px）由调用方传入。
void paintGlyph(QPainter &p, Glyph glyph, qreal cx, qreal cy, const QColor &color,
                qreal stroke)
{
    QPen pen(color, stroke);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);

    p.save();
    // 图标局部坐标系：原点平移到图标左上角（24x24 基准框）
    p.translate(cx - 12.0, cy - 12.0);

    switch (glyph) {
    case Glyph::Home: {
        // 屋顶 + 墙体 + 门
        p.setPen(pen);
        QPainterPath path;
        path.moveTo(2, 11);
        path.lineTo(12, 3);
        path.lineTo(22, 11);
        p.drawPath(path);

        path = QPainterPath();
        path.moveTo(6, 10);
        path.lineTo(6, 20);
        path.lineTo(18, 20);
        path.lineTo(18, 10);
        p.drawPath(path);

        path = QPainterPath();
        path.moveTo(10, 20);
        path.lineTo(10, 15);
        path.lineTo(14, 15);
        path.lineTo(14, 20);
        p.drawPath(path);
        break;
    }
    case Glyph::Search: {
        // 放大镜：圆 + 手柄
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(10, 10), 5, 5);
        QPainterPath handle;
        handle.moveTo(13.5, 13.5);
        handle.lineTo(20, 20);
        p.drawPath(handle);
        break;
    }
    case Glyph::Star: {
        // 五角星：内 / 外半径交替的十边形
        QPainterPath path;
        const qreal outer = 8.5;
        const qreal inner = 3.5;
        for (int k = 0; k < 10; ++k) {
            const qreal r = (k % 2 == 0) ? outer : inner;
            const qreal angle = qDegreesToRadians(-90.0 + k * 36.0);
            const QPointF pt(12.0 + r * qCos(angle), 12.0 + r * qSin(angle));
            if (k == 0) {
                path.moveTo(pt);
            } else {
                path.lineTo(pt);
            }
        }
        path.closeSubpath();
        p.setPen(pen);
        p.drawPath(path);
        break;
    }
    case Glyph::Person: {
        // 头部实心圆 + 肩部半圆弧
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(12, 8.5), 3.5, 3.5);

        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        QPainterPath shoulders;
        shoulders.moveTo(4, 21);
        shoulders.arcTo(QRectF(4, 14, 16, 14), 180, 180);
        p.drawPath(shoulders);
        break;
    }
    case Glyph::Palette: {
        // 调色板：圆盘 + 拇指孔 / 画笔孔 / 底部孔 / 左右孔
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(12, 12), 9, 9);

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(7, 7), 1.5, 1.5);
        p.drawEllipse(QPointF(17, 7), 1.5, 1.5);
        p.drawEllipse(QPointF(12, 18), 1.5, 1.5);
        p.drawEllipse(QPointF(5, 13), 1.5, 1.5);
        p.drawEllipse(QPointF(19, 13), 1.5, 1.5);
        break;
    }
    case Glyph::Drop: {
        // 水滴：顶部尖角 + 四段贝塞尔下摆成圆底
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        QPainterPath drop;
        drop.moveTo(12, 4);
        drop.cubicTo(16.4, 8.4, 19, 11.8, 19, 15);
        drop.cubicTo(19, 18.9, 15.9, 21, 12, 21);
        drop.cubicTo(8.1, 21, 5, 18.9, 5, 15);
        drop.cubicTo(5, 11.8, 7.6, 8.4, 12, 4);
        p.drawPath(drop);
        break;
    }
    case Glyph::Plus: {
        // 十字：水平 + 垂直圆笔直线
        p.setPen(pen);
        QPainterPath plus;
        plus.moveTo(5, 12);
        plus.lineTo(19, 12);
        plus.moveTo(12, 5);
        plus.lineTo(12, 19);
        p.drawPath(plus);
        break;
    }
    case Glyph::Check: {
        // 对勾：短臂 + 长臂
        p.setPen(pen);
        QPainterPath check;
        check.moveTo(4, 12.5);
        check.lineTo(10, 18.5);
        check.lineTo(20, 8);
        p.drawPath(check);
        break;
    }
    case Glyph::Close: {
        // 斜十字：两段对角圆笔直线
        p.setPen(pen);
        QPainterPath close;
        close.moveTo(6, 6);
        close.lineTo(18, 18);
        close.moveTo(18, 6);
        close.lineTo(6, 18);
        p.drawPath(close);
        break;
    }
    case Glyph::Info: {
        // 信息：外圆 + 顶部圆点 + 中心竖线
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(12, 12), 9, 9);

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(12, 8.6), 1.1, 1.1);

        p.setPen(pen);
        QPainterPath stem;
        stem.moveTo(12, 12.4);
        stem.lineTo(12, 16.6);
        p.drawPath(stem);
        break;
    }
    case Glyph::Bell: {
        // 铃铛：钟摆曲线 + 底横线 + 半圆弧拱
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        QPainterPath bell;
        bell.moveTo(5.5, 16.5);
        bell.cubicTo(7.2, 15.6, 7.2, 11.4, 7.2, 9.6);
        bell.cubicTo(7.2, 6.6, 9.4, 4.2, 12, 4.2);
        bell.cubicTo(14.6, 4.2, 16.8, 6.6, 16.8, 9.6);
        bell.cubicTo(16.8, 11.4, 16.8, 15.6, 18.5, 16.5);
        p.drawPath(bell);

        // 底沿横线
        QPainterPath rim;
        rim.moveTo(4.5, 16.5);
        rim.lineTo(19.5, 16.5);
        p.drawPath(rim);

        // 悬挂拱（小半圆弧）
        QPainterPath clapper;
        clapper.moveTo(10, 19.6);
        clapper.arcTo(QRectF(10, 17.6, 4, 4), 180, -180);
        p.drawPath(clapper);
        break;
    }
    case Glyph::Heart: {
        // 心形：两段对称贝塞尔勾勒左/右弧线与底部 V 汇合
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        QPainterPath heart;
        heart.moveTo(12, 20.5);
        heart.cubicTo(6.2, 16.2, 3.4, 13.0, 3.4, 9.8);
        heart.cubicTo(3.4, 6.9, 5.5, 4.6, 8.2, 4.6);
        heart.cubicTo(10.0, 4.6, 11.4, 5.6, 12, 7.0);
        heart.cubicTo(12.6, 5.6, 14.0, 4.6, 15.8, 4.6);
        heart.cubicTo(18.5, 4.6, 20.6, 6.9, 20.6, 9.8);
        heart.cubicTo(20.6, 13.0, 17.8, 16.2, 12, 20.5);
        p.drawPath(heart);
        break;
    }
    }

    p.restore();
}

} // namespace md3
