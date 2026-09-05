#include "ba_page_tabs.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include "ba_style.h"

namespace {

constexpr int kHeight = 48;           // 页签条总高（含红旗悬浮）
constexpr qreal kCapsuleInsetY = 6;   // 白药丸距控件顶
constexpr int kCapsuleHeight = 40;

} // namespace

BaPageTabs::BaPageTabs(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(kHeight);
    setCursor(Qt::PointingHandCursor);
}

void BaPageTabs::addItem(const QString &text)
{
    items_ << text;
    update();
}

void BaPageTabs::setCurrentIndex(int index)
{
    if (index < 0 || index >= items_.size() || index == current_)
        return;
    current_ = index;
    emit currentChanged(index);
    update();
}

QSize BaPageTabs::sizeHint() const
{
    return QSize(items_.size() * 120, kHeight);
}

QRectF BaPageTabs::itemRect(int index) const
{
    if (items_.isEmpty())
        return QRectF();
    const qreal itemW = capsule_.width() / items_.size();
    return QRectF(capsule_.left() + index * itemW, capsule_.top(), itemW, capsule_.height());
}

int BaPageTabs::itemIndexAt(int x) const
{
    if (items_.isEmpty() || capsule_.width() <= 0)
        return -1;
    const qreal itemW = capsule_.width() / items_.size();
    const int idx = int((x - capsule_.left()) / itemW);
    if (idx < 0 || idx >= items_.size())
        return -1;
    return idx;
}

void BaPageTabs::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 置中白胶囊
    const qreal pad = 0;
    capsule_ = QRectF((width() - qMin<qreal>(width(), items_.size() * 132.0)) / 2.0,
                      kCapsuleInsetY,
                      qMin<qreal>(width(), items_.size() * 132.0), kCapsuleHeight);
    Q_UNUSED(pad);

    QPainterPath capPath;
    capPath.addRoundedRect(capsule_, kCapsuleHeight / 2.0, kCapsuleHeight / 2.0);
    p.fillPath(capPath, QColor(255, 255, 255, 240));
    p.setPen(QPen(QColor("#D5E4EF"), 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(capPath);

    for (int i = 0; i < items_.size(); ++i) {
        const QRectF rc = itemRect(i);
        const bool sel = (i == current_);

        if (sel) {
            // 顶部小三角红旗：骑在胶囊上缘
            const qreal cx = rc.center().x();
            QPainterPath flag;
            flag.moveTo(cx - 8, capsule_.top() - 1);
            flag.lineTo(cx + 8, capsule_.top() - 1);
            flag.lineTo(cx, capsule_.top() + 8);
            flag.closeSubpath();
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#F5821F"));
            p.drawPath(flag);

            // 橙渐变选中块：稍高出胶囊（上下各扩 2）
            const QRectF blk = rc.adjusted(5, -3, -5, -4);
            QPainterPath blkPath;
            blkPath.addRoundedRect(blk, 10, 10);
            QLinearGradient g(blk.topLeft(), blk.bottomLeft());
            g.setColorAt(0, QColor("#FFC95C"));
            g.setColorAt(1, QColor("#F5821F"));
            p.fillPath(blkPath, g);
            p.setPen(QPen(QColor(255, 255, 255, 150), 1.2));
            p.setBrush(Qt::NoBrush);
            p.drawPath(blkPath);
        }

        // 文字
        p.setFont(BaStyle::font(10, sel ? QFont::Bold : QFont::DemiBold));
        p.setPen(sel ? Qt::white : QColor("#48586B"));
        p.drawText(rc.adjusted(4, 4, -4, -4), Qt::AlignCenter, items_.at(i));
    }
}

void BaPageTabs::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
}

void BaPageTabs::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const int idx = itemIndexAt(event->pos().x());
        if (idx >= 0)
            setCurrentIndex(idx);
    }
}

void BaPageTabs::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
}
