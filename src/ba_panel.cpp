#include "ba_panel.h"

#include <QPainter>
#include <QPaintEvent>

#include "ba_style.h"

BaPanel::BaPanel(QWidget *parent)
    : QWidget(parent)
{
    contentLayout_ = new QVBoxLayout(this);
    contentLayout_->setContentsMargins(16, 16, 16, 16);
    contentLayout_->setSpacing(10);
    contentLayout_->setSizeConstraint(QLayout::SetMinimumSize);
}

void BaPanel::setHeaderTitle(const QString &title)
{
    headerTitle_ = title;
    update();
    // 有标题才为顶部信息区预留高度
    const int h = headerTitle_.isEmpty() ? 0 : 34;
    contentLayout_->setContentsMargins(16, h + 8, 16, 16);
}

QRectF BaPanel::headerRect() const
{
    return QRectF(0, 0, width(), 34);
}

QSize BaPanel::sizeHint() const
{
    return QSize(240, 100);
}

QSize BaPanel::minimumSizeHint() const
{
    return QSize(120, 40);
}

void BaPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 白底卡 + 圆角 12
    const QRectF r(QPointF(0, 0), QSizeF(width(), height()));
    const QPainterPath path = [&r]() {
        QPainterPath path;
        path.addRoundedRect(r, 12, 12);
        return path;
    }();
    p.fillPath(path, Qt::white);

    // 顶部浅蓝信息区（可选）
    if (!headerTitle_.isEmpty()) {
        QPainterPath head;
        QRectF hr = headerRect();
        hr.setRight(hr.right());
        head.addRoundedRect(hr, 12, 12);
        // 使得头部与卡片圆角一致而底边为直线
        QPainterPath clip = path;
        QPainterPath lower;
        lower.addRect(QRectF(hr.left(), hr.bottom() - 12, hr.width(), 12));
        clip = clip.subtracted(lower);
        QLinearGradient g(hr.topLeft(), hr.bottomLeft());
        g.setColorAt(0, QColor("#F2F9FE"));
        g.setColorAt(1, QColor("#EDF3F9"));
        p.fillPath(clip, g);
        // 头部分隔线
        p.setPen(QPen(QColor(0xBF, 0xD6, 0xE8, 180), 1));
        p.drawLine(QPointF(hr.left() + 1, hr.bottom()), QPointF(hr.right() - 1, hr.bottom()));

        p.setPen(BaStyle::deep());
        p.setFont(BaStyle::font(10, QFont::Bold));
        p.drawText(hr.adjusted(16, 2, -16, -2), Qt::AlignVCenter | Qt::AlignLeft, headerTitle_);
        // 标题下贴黄色小尺线
        p.setPen(Qt::NoPen);
        p.setBrush(BaStyle::yellow());
        const int tw = p.fontMetrics().horizontalAdvance(headerTitle_);
        p.drawRect(QRectF(hr.left() + 16, hr.bottom() - 8, tw, 2.5));
    }

    // 深蓝细描边
    p.setPen(QPen(hovered_ ? QColor(0x1F, 0x4E, 0x79, 230) : QColor(0x28, 0x5B, 0x8C, 160), 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
}

void BaPanel::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    hovered_ = true;
    update();
}

void BaPanel::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    hovered_ = false;
    update();
}
