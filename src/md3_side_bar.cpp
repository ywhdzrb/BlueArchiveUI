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
        md3::paintGlyph(p, items_.at(i).glyph, iconCx, iconCy, iconColor, kStroke);

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
