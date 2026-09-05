#include "ba_navigation_bar.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include "ba_style.h"

namespace {
constexpr int kBarHeight = 66;   // 胶囊条总高
constexpr int kPillH = 58;       // 白胶囊实际高（上下留 4px 呼吸）
} // namespace

BaNavigationBar::BaNavigationBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(kBarHeight);
    setCursor(Qt::PointingHandCursor);
}

void BaNavigationBar::addItem(const QString &text, ba::Glyph glyph)
{
    items_ << text;
    glyphs_ << glyph;
    update();
}

void BaNavigationBar::setCurrentIndex(int index)
{
    if (index >= 0 && index < items_.size()) {
        current_ = index;
        update();
    }
}

QSize BaNavigationBar::sizeHint() const
{
    return QSize(qMax(560, items_.size() * 96), kBarHeight);
}

QRectF BaNavigationBar::itemRect(int index) const
{
    if (items_.isEmpty())
        return {};
    const qreal w = width() / qreal(items_.size());
    return QRectF(index * w, 0, w, kBarHeight);
}

void BaNavigationBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform); // 图片缩放平滑（防锯齿）

    // 白色半透明胶囊底（BA 大厅底部导航）
    const QRectF pill(2, (kBarHeight - kPillH) / 2.0, width() - 4, kPillH);
    QPainterPath pillPath;
    pillPath.addRoundedRect(pill, kPillH / 2, kPillH / 2);
    p.fillPath(pillPath, QColor(255, 255, 255, 225));
    p.setPen(QPen(QColor(0xCF, 0xE5, 0xF0, 160), 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(pillPath);

    if (items_.isEmpty())
        return;

    for (int i = 0; i < items_.size(); ++i) {
        const QRectF rc = itemRect(i);
        const bool selected = (i == current_);

        if (selected) {
            // 选中块：深蓝渐变 + 浅蓝描边，略微高出胶囊顶
            QRectF block = rc.adjusted(8, (kBarHeight - kPillH) / 2.0 - 6, -8, -5);
            QPainterPath bp;
            bp.addRoundedRect(block, 10, 10);
            p.fillPath(bp, BaStyle::deepGradient(block));
            p.setPen(QPen(QColor(0x7E, 0xC9, 0xF0, 190), 2));
            p.setBrush(Qt::NoBrush);
            p.drawPath(bp);
            // 底部渐隐约出来一点（浅蓝端头）
            QLinearGradient tail(0, block.bottom() - 6, 0, block.bottom() + 4);
            tail.setColorAt(0, QColor(0x2E, 0x6C, 0xA8, 0));
            tail.setColorAt(1, QColor(0x6B, 0xC3, 0xEE, 90));
            p.setPen(Qt::NoPen);
            p.setBrush(tail);
        }

        // 图标
        const qreal iconS = 22;
        const QPointF iconPos(rc.center().x() - iconS / 2, rc.top() + (kBarHeight - 42) / 2.0 - 2);
        p.drawPixmap(iconPos, ba::pixmap(glyphs_.at(i), QSize(22, 22),
                                         selected ? QColor(Qt::white) : QColor(0x77, 0x8C, 0x9B)));
        // 文字：选中白色，其余深蓝
        p.setFont(BaStyle::font(9, selected ? QFont::Bold : QFont::DemiBold));
        p.setPen(selected ? QColor(Qt::white) : BaStyle::deep());
        p.drawText(QRectF(rc.left(), rc.top() + 42, rc.width(), 18),
                   Qt::AlignHCenter | Qt::AlignTop, items_.at(i));
    }
}

void BaNavigationBar::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
    if (event->button() != Qt::LeftButton || items_.isEmpty())
        return;
    const int idx = int(event->pos().x() / (width() / qreal(items_.size())));
    if (idx >= 0 && idx < items_.size() && idx != current_) {
        current_ = idx;
        emit currentChanged(idx);
        update();
    }
}
