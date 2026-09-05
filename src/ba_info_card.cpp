#include "ba_info_card.h"

#include "ba_icon.h"
#include "ba_style.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>

namespace {

constexpr qreal kRibbonH = 30.0;    // 丝带高度
constexpr qreal kRowH = 44.0;       // 单行高
constexpr qreal kPad = 12.0;        // 卡内边距
constexpr qreal kButton = 28.0;     // 尾部工具钮尺寸
constexpr qreal kLabelW = 96.0;     // label 列宽
constexpr qreal kRadius = 8.0;      // 卡圆角

} // namespace

BaInfoCard::BaInfoCard(const QString &ribbonText, QWidget *parent)
    : QWidget(parent)
    , ribbonText_(ribbonText)
{
    setMinimumWidth(420);
}

void BaInfoCard::setRibbonText(const QString &t)
{
    ribbonText_ = t;
    update();
}

void BaInfoCard::addRow(const QString &label, const QString &value, Trailing trailing)
{
    Row row;
    row.label = label;
    row.value = value;
    row.trailing = trailing;
    rows_.push_back(row);
    updateGeometry();
    update();
}

QSize BaInfoCard::sizeHint() const
{
    // 宽：label + value + 尾部钮余量；高：丝带 + 行数 × 行高 + 底部内边距
    const QFont f = BaStyle::font(10, QFont::DemiBold);
    const QFontMetrics fm(f);
    int w = int(kLabelW + 90 + kButton + kPad * 3);
    for (const Row &row : rows_) {
        w = qMax(w, int(kLabelW + fm.horizontalAdvance(row.value) + kButton + kPad * 3 + 24));
    }
    const int h = int(kRibbonH + kRowH * rows_.size() + kPad);
    return QSize(w, h);
}

QSize BaInfoCard::minimumSizeHint() const
{
    return sizeHint();
}

QRectF BaInfoCard::ribbonRect() const
{
    return QRectF(0, 8, width(), kRibbonH);
}

QRectF BaInfoCard::buttonRectFor(const Row &row, qreal rowY) const
{
    Q_UNUSED(row);
    return QRectF(width() - kButton - kPad, rowY + (kRowH - kButton) / 2.0, kButton, kButton);
}

void BaInfoCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform); // 图片缩放平滑（防锯齿）
    p.setRenderHint(QPainter::TextAntialiasing);

    const QRectF card(0, kRibbonH / 2.0, width(), height() - kRibbonH / 2.0);

    // 白卡体：直角圆角 8（真机基线，内容天然对齐）+ 浅蓝描边
    p.setPen(QPen(BaStyle::infoCardEdge(), 1.2));
    p.setBrush(Qt::white);
    p.drawRoundedRect(card, kRadius, kRadius);

    // 丝带标题条：深蓝渐变 + 两端 45° 斜切（左上尖 / 右下尖）
    const QRectF rb = ribbonRect();
    const qreal cut = kRibbonH * 0.55;
    QPainterPath ribbon;
    ribbon.moveTo(rb.left(), rb.top() + 1);
    ribbon.lineTo(rb.right(), rb.top() + 1);
    ribbon.lineTo(rb.right() - cut, rb.bottom());
    ribbon.lineTo(rb.left() + cut, rb.bottom());
    ribbon.closeSubpath();
    p.fillPath(ribbon, BaStyle::ribbonGradient(ribbon.boundingRect()));

    // 丝带文字：白思粗体居中 + 深蓝描边
    QFont rf = BaStyle::font(11, QFont::Bold);
    p.setFont(rf);
    const QFontMetricsF rfm(rf);
    const qreal baseY = rb.center().y() + (rfm.ascent() - rfm.descent()) / 2.0;
    QPainterPath tp;
    tp.addText(QPointF(rb.center().x() - rfm.horizontalAdvance(ribbonText_) / 2.0, baseY),
               rf, ribbonText_);
    p.setPen(QPen(QColor(0x0B, 0x2A, 0x50, 160), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::white);
    p.drawPath(tp);

    // 行模型：label + value + 尾部工具钮 + 行间分隔线
    p.setFont(BaStyle::font(10));
    for (size_t i = 0; i < rows_.size(); ++i) {
        const Row &row = rows_[i];
        const qreal y = kRibbonH + i * kRowH;

        // label（深蓝常规）
        p.setPen(QColor(0x33, 0x55, 0x7B));
        p.drawText(QRectF(kPad, y, kLabelW, kRowH), Qt::AlignVCenter | Qt::AlignLeft, row.label);

        // value（深蓝思粗体）
        const QFont vf = BaStyle::font(10, QFont::DemiBold);
        p.setFont(vf);
        const QColor vc(0x1E, 0x3B, 0x58);
        p.setPen(vc);
        p.drawText(QRectF(kPad + kLabelW, y, width() - kPad * 2 - kLabelW - kButton - 8, kRowH),
                   Qt::AlignVCenter | Qt::AlignLeft, row.value);

        // 行间分隔线（最后一行不画）
        if (i + 1 < rows_.size()) {
            p.setPen(QPen(QColor(0xE5, 0xEA, 0xF0), 1));
            p.drawLine(QPointF(kPad, y + kRowH), QPointF(width() - kPad, y + kRowH));
        }

        // 尾部工具钮：白片圆角 6 + 铅笔 / 喇叭图标
        if (row.trailing != Trailing::None) {
            const QRectF btn = buttonRectFor(row, y);
            p.setPen(QPen(QColor(0xC9, 0xDB, 0xE8), 1));
            p.setBrush(QColor(0xF7, 0xFB, 0xFE));
            p.drawRoundedRect(btn, 6, 6);
            if (row.trailing == Trailing::Edit) {
                p.drawPixmap(btn.center().toPoint() + QPoint(-9, -9),
                             ba::pixmap(ba::Glyph::Pencil, QSize(18, 18)));
            } else if (row.trailing == Trailing::Speaker) {
                p.drawPixmap(btn.center().toPoint() + QPoint(-9, -9),
                             ba::pixmap(ba::Glyph::Speaker, QSize(18, 18), QColor(0x33, 0x5C, 0x8A)));
            }
        }
    }
}
