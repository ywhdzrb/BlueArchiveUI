#include "ba_section_header.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QHBoxLayout>

#include "ba_style.h"

BaSectionHeader::BaSectionHeader(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    title_ = title;
    setFixedHeight(34);
}

void BaSectionHeader::setTitle(const QString &title)
{
    title_ = title;
    update();
}

void BaSectionHeader::setTrailingWidget(QWidget *w)
{
    if (!w)
        return;
    trailing_ = w;
    w->setParent(this);
}

QSize BaSectionHeader::sizeHint() const
{
    return QSize(300, 34);
}

void BaSectionHeader::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);

    // 左竖条：3px 宽、14px 高的青蓝圆角短条
    p.setBrush(BaStyle::accent());
    p.drawRoundedRect(QRectF(0, (height() - 16) / 2.0, 3.5, 16), 1.75, 1.75);

    // 标题
    p.setPen(QColor("#0F3A5C"));
    p.setFont(BaStyle::font(11, QFont::Bold));
    int trailingW = trailing_ ? trailing_->width() + 10 : 0;
    p.drawText(QRectF(12, 0, width() - 12 - trailingW, height() - 6),
               Qt::AlignVCenter | Qt::AlignLeft, title_);

    // 底部虚线
    p.setPen(QPen(BaStyle::dash(), 1, Qt::DashLine));
    p.drawLine(QPointF(0, height() - 5), QPointF(width(), height() - 5));
}

void BaSectionHeader::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (trailing_) {
        trailing_->move(width() - trailing_->width() - 6, height() - trailing_->height() - 8);
    }
}
