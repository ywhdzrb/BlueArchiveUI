#include "ba_shop_menu_block.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include "ba_style.h"

namespace {
constexpr int kHeight = 40;
}

BaShopMenuBlock::BaShopMenuBlock(const QString &text, QWidget *parent)
    : QWidget(parent)
{
    text_ = text;
    setFixedHeight(kHeight);
}

void BaShopMenuBlock::setSelected(bool selected)
{
    selected_ = selected;
    update();
}

QSize BaShopMenuBlock::sizeHint() const
{
    const QFontMetrics fm(BaStyle::font(10, QFont::Bold));
    return QSize(fm.horizontalAdvance(text_) + 48, kHeight);
}

void BaShopMenuBlock::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF rc(QPointF(0, 0), QSizeF(width(), height()));

    // 深蓝渐变斜切体（右端大斜切 + 圆角 4）
    QPainterPath body;
    const qreal cut = 12;
    body.moveTo(rc.left(), rc.top());
    body.lineTo(rc.right() - cut, rc.top());
    body.lineTo(rc.right() - cut + 8, rc.center().y() + 6);
    body.lineTo(rc.right() - cut + 8, rc.bottom());
    body.lineTo(rc.left(), rc.bottom());
    body.closeSubpath();
    QLinearGradient g(rc.topLeft(), rc.bottomLeft());
    g.setColorAt(0, selected_ ? QColor("#3F86C4") : QColor("#2E6CA8"));
    g.setColorAt(1, selected_ ? QColor("#174979") : QColor("#0F3761"));
    p.fillPath(body, g);
    // 右侧浅蓝描边轮廓（游戏内轮廓亮线）
    p.setPen(QPen(QColor(126, 201, 240, 170), 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(body);

    // 左端橙黄三角角标（骑在左缘，选中时才显示；未选保持淡黄小块）
    if (selected_ || true) {
        QPainterPath tri;
        tri.moveTo(rc.left() - 1, rc.top() + 5);
        tri.lineTo(rc.left() + 10, rc.center().y() - 2);
        tri.lineTo(rc.left() - 1, rc.center().y() + 4);
        tri.closeSubpath();
        QLinearGradient tg(QPointF(0, rc.top()), QPointF(0, rc.bottom()));
        tg.setColorAt(0, QColor("#FFC95C"));
        tg.setColorAt(1, QColor("#F5821F"));
        p.fillPath(tri, tg);
    }

    // 文本：白色粗体
    p.setFont(BaStyle::font(10, QFont::Bold));
    p.setPen(Qt::white);
    p.drawText(rc.adjusted(14, 0, -16, 0), Qt::AlignVCenter | Qt::AlignLeft, text_);
}
