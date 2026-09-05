#include "ba_bounty_card.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include "ba_style.h"

namespace {

constexpr qreal kCardRadius = 12.0;   // 白卡圆角
constexpr qreal kSheetW = 150.0;      // 右侧深蓝标题片宽度（长片≥卡宽 1/3）
constexpr qreal kChamfer = 16.0;      // 标题片左缘斜切宽度

} // namespace

BaBountyCard::BaBountyCard(const QString &title, const QString &ticketText, QWidget *parent)
    : QWidget(parent)
{
    title_ = title;
    ticketText_ = ticketText;
    setMinimumHeight(92);
    setFixedHeight(96);
}

void BaBountyCard::setTitle(const QString &title) { title_ = title; update(); }
void BaBountyCard::setTicketText(const QString &text) { ticketText_ = text; update(); }
void BaBountyCard::setTag(const QString &text) { tag_ = text; update(); }
void BaBountyCard::setDescription(const QString &text) { description_ = text; update(); }

QSize BaBountyCard::sizeHint() const
{
    return QSize(430, 96);
}

void BaBountyCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF rc(QPointF(0, 0), QSizeF(width(), height()));

    // 主体白色圆角卡 + 深蓝描边（对照悬赏页卡片）
    QPainterPath card;
    card.addRoundedRect(rc, kCardRadius, kCardRadius);
    p.fillPath(card, QColor(255, 255, 255, 242));
    p.setPen(QPen(QColor(0x6B, 0x9C, 0xC6, 150), 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(card);

    // 右侧深蓝渐变标题片：全高左斜梯形（顶上右、左下左剪角，对照悬赏页「高架公路」名片）
    const qreal sheetW = qMax(kSheetW, width() * 0.32);
    const QRectF sheet(rc.right() - sheetW - 6, -2, sheetW, rc.height() + 4);
    QPainterPath sp;
    sp.moveTo(sheet.left() + kChamfer, sheet.top());
    sp.lineTo(sheet.right(), sheet.top());
    sp.lineTo(sheet.right(), sheet.bottom());
    sp.lineTo(sheet.left(), sheet.bottom());
    sp.closeSubpath();
    QLinearGradient sg(sheet.topLeft(), sheet.bottomLeft());
    sg.setColorAt(0, QColor("#3E80BE"));
    sg.setColorAt(1, QColor("#0F3761"));
    p.fillPath(sp, sg);
    p.setPen(QPen(Qt::white, 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(sp);

    // 大标题白色粗体 + 深蓝描边（游戏内悬赏大标题）
    const QFont tf = BaStyle::font(12, QFont::Bold);
    p.setFont(tf);
    QPainterPath titlePath;
    titlePath.addText(QPointF(sheet.left() + 26, sheet.center().y() + fontMetrics().ascent() / 2.0 - 2), tf,
                      title_);
    p.setPen(QPen(QColor(0x0F, 0x37, 0x61, 230), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::white);
    p.drawPath(titlePath);
    p.setPen(Qt::NoPen);

    p.setFont(BaStyle::font(9));

    // 活动进行中橙红标签（左上角斜切角片）
    if (!tag_.isEmpty()) {
        const QRectF tagRc(12, 10, p.fontMetrics().horizontalAdvance(tag_) + 18, 22);
        QPainterPath tagPath;
        tagPath.addRoundedRect(tagRc, 4, 4);
        // 左端斜切
        QPainterPath shape;
        shape.moveTo(tagRc.left() + 8, tagRc.top());
        shape.lineTo(tagRc.right(), tagRc.top());
        shape.lineTo(tagRc.right(), tagRc.bottom());
        shape.lineTo(tagRc.left() + 8, tagRc.bottom());
        shape.lineTo(tagRc.left(), tagRc.center().y());
        shape.closeSubpath();
        QLinearGradient tg(QPointF(0, tagRc.top()), QPointF(0, tagRc.bottom()));
        tg.setColorAt(0, QColor("#FF8A5C"));
        tg.setColorAt(1, QColor("#F2573F"));
        p.fillPath(shape, tg);
        p.setFont(BaStyle::font(8, QFont::Bold));
        p.setPen(Qt::white);
        p.drawText(tagRc, Qt::AlignCenter, tag_);
    }

    // 描述文字（浅灰深蓝，两行内）
    if (!description_.isEmpty()) {
        p.setFont(BaStyle::font(9));
        p.setPen(QColor("#5C7182"));
        const QRectF descRc(12, 42, width() - kSheetW - 40, 34);
        p.drawText(descRc, Qt::AlignVCenter | Qt::AlignLeft, description_);
    }

    // 底部白胶囊「持有跳战券 n/n」（左端）
    if (!ticketText_.isEmpty()) {
        QFont f = BaStyle::font(9, QFont::DemiBold);
        p.setFont(f);
        const int tw = p.fontMetrics().horizontalAdvance(ticketText_);
        const QRectF cap(12, height() - 3 - 22, tw + 30, 22);
        QPainterPath capp;
        capp.addRoundedRect(cap, 11, 11);
        p.fillPath(capp, QColor(255, 255, 255, 245));
        p.setPen(QPen(QColor("#A7BFD1"), 1.2));
        p.setBrush(Qt::NoBrush);
        p.drawPath(capp);
        p.setPen(BaStyle::deep());
        p.drawText(cap, Qt::AlignCenter, ticketText_);
    }
}
