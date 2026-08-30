#include "md3_side_bar.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QtMath>

namespace {
constexpr int kRailWidth = 80;       // 导航栏宽度
constexpr int kItemHeight = 64;      // 单个导航项高度
constexpr qreal kIndicatorW = 56.0;  // 选中指示器胶囊尺寸
constexpr qreal kIndicatorH = 32.0;
constexpr int kTopPad = 8;           // 顶部留白
constexpr qreal kStroke = 2.0;       // 线性图标描边宽度
}

Md3SideBar::Md3SideBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(kRailWidth);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
}

void Md3SideBar::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

int Md3SideBar::addItem(const QString &label, Glyph glyph)
{
    items_.append({label, glyph});
    if (currentIndex_ < 0) {
        currentIndex_ = 0;
    }
    update();
    return items_.size() - 1;
}

void Md3SideBar::clearItems()
{
    items_.clear();
    currentIndex_ = -1;
    hoverIndex_ = -1;
    update();
}

void Md3SideBar::setCurrentIndex(int index)
{
    if (index < 0 || index >= items_.size() || index == currentIndex_) {
        return;
    }
    currentIndex_ = index;
    update();
    emit itemSelected(index);
}

QSize Md3SideBar::sizeHint() const
{
    return QSize(kRailWidth, kTopPad + items_.size() * kItemHeight);
}

// 绘制：容器背景 → 每项（选中胶囊 / 悬停状态层 → 图标 → 标签）
void Md3SideBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 容器背景 surface-container-low
    p.fillRect(rect(), theme_.surfaceContainerLow);

    for (int i = 0; i < items_.size(); ++i) {
        const qreal itemTop = kTopPad + i * kItemHeight;
        const qreal iconCx = kRailWidth / 2.0;
        const qreal iconCy = itemTop + kIndicatorH / 2.0;   // 图标 / 指示器中心
        const bool selected = (i == currentIndex_);

        // 选中项：primary-container 胶囊；悬停项：on-surface 8% 状态层
        const QRectF indicator(iconCx - kIndicatorW / 2.0, iconCy - kIndicatorH / 2.0,
                               kIndicatorW, kIndicatorH);
        if (selected) {
            p.setBrush(theme_.primaryContainer);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(indicator, kIndicatorH / 2.0, kIndicatorH / 2.0);
        } else if (i == hoverIndex_) {
            QColor layer = Md3Theme::blend(theme_.surfaceContainerLow, theme_.onSurface, 0.08);
            p.setBrush(layer);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(indicator, kIndicatorH / 2.0, kIndicatorH / 2.0);
        }

        // 图标：选中 primary，否则 on-surface-variant
        const QColor iconColor = selected ? theme_.primary : theme_.onSurfaceVariant;
        paintGlyph(p, items_.at(i).glyph, iconCx, iconCy, iconColor);

        // 标签：选中 primary 加粗，否则 on-surface-variant（Label Medium 12px）
        QFont f = p.font();
        f.setPointSizeF(12.0);
        f.setWeight(selected ? QFont::Medium : QFont::Normal);
        p.setFont(f);
        p.setPen(selected ? theme_.primary : theme_.onSurfaceVariant);
        const QString elided = QFontMetrics(f).elidedText(items_.at(i).label, Qt::ElideRight,
                                                          kRailWidth - 8);
        p.drawText(QRectF(0, iconCy + kIndicatorH / 2.0, kRailWidth, kItemHeight - kIndicatorH),
                   Qt::AlignHCenter | Qt::AlignVCenter, elided);
    }
}

void Md3SideBar::mousePressEvent(QMouseEvent *event)
{
    const int idx = (event->pos().y() - kTopPad) / kItemHeight;
    if (idx >= 0 && idx < items_.size()) {
        setCurrentIndex(idx);
    }
}

void Md3SideBar::mouseMoveEvent(QMouseEvent *event)
{
    const int idx = (event->pos().y() - kTopPad) / kItemHeight;
    const int target = (idx >= 0 && idx < items_.size()) ? idx : -1;
    if (target != hoverIndex_) {
        hoverIndex_ = target;
        update();
    }
}

void Md3SideBar::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hoverIndex_ = -1;
    update();
}

// 绘制 24x24 基准的线性图标，居中于 (cx, cy)，描边色 / 填充色由调用方指定。
// 角度约定与 arcTo 一致：0°=右, 90°=上, 180°=左, 270°=下。
void Md3SideBar::paintGlyph(QPainter &p, Glyph glyph, qreal cx, qreal cy, const QColor &color) const
{
    QPen pen(color, kStroke);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);

    p.save();
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
        // 调色板：圆盘 + 拇指孔 + 画笔孔
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(12, 12), 9, 9);

        // 拇指孔（左上）
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(7, 7), 1.5, 1.5);

        // 画笔孔（右上）
        p.drawEllipse(QPointF(17, 7), 1.5, 1.5);

        // 底部孔
        p.drawEllipse(QPointF(12, 18), 1.5, 1.5);

        // 左侧孔
        p.drawEllipse(QPointF(5, 13), 1.5, 1.5);

        // 右侧孔
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
    }

    p.restore();
}
