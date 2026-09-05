#include "ba_chip.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include "ba_style.h"

BaChip::BaChip(const QString &text, QWidget *parent)
    : QWidget(parent)
{
    setText(text);
}

void BaChip::setText(const QString &text)
{
    text_ = text;
    update();
    updateGeometry();
}

QSize BaChip::sizeHint() const
{
    const QFontMetrics fm(BaStyle::font(9, QFont::Bold));
    return QSize(fm.horizontalAdvance(text_) + 30, 24);
}

QSize BaChip::minimumSizeHint() const
{
    return sizeHint();
}

void BaChip::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r(QPointF(0, 0), QSizeF(width(), height()));
    // 左端 30° 斜切梯形 + 圆角 4
    QPainterPath path;
    const qreal cut = r.height() * 0.55;
    const qreal rad = 4;
    path.moveTo(r.left() + rad, r.top());
    path.lineTo(r.right() - rad, r.top());
    path.quadTo(r.right(), r.top(), r.right(), r.top() + rad);
    path.lineTo(r.right(), r.bottom() - rad);
    path.quadTo(r.right(), r.bottom(), r.right() - rad, r.bottom());
    path.lineTo(r.left() + cut + rad, r.bottom());
    path.quadTo(r.left() + cut, r.bottom(), r.left() + cut, r.bottom() - rad);
    path.lineTo(r.left() + cut, r.top() + rad);
    path.quadTo(r.left() + cut, r.top(), r.left() + cut - rad, r.top());
    path.closeSubpath();

    p.setPen(Qt::NoPen);
    p.setBrush(BaStyle::chipBlack());
    p.drawPath(path);

    // 白色数值
    p.setPen(Qt::white);
    p.setFont(BaStyle::font(9, QFont::Bold));
    p.drawText(r.adjusted(cut * 0.6, 0, -6, 0), Qt::AlignVCenter | Qt::AlignRight, text_);
}
