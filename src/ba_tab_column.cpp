#include "ba_tab_column.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>

#include "ba_icon.h"
#include "ba_style.h"

namespace {

constexpr int kItemHeight = 46;
constexpr int kIconWidth = 22;

} // namespace

BaTabColumn::BaTabColumn(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(180);
    setCursor(Qt::PointingHandCursor);
    for (int &icon : icons_) icon = -1;

    hoverAnim_.setDuration(140);
    connect(&hoverAnim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        hoverProgress_ = v.toReal();
        update();
    });
}

void BaTabColumn::setStyle(Style s)
{
    style_ = s;
    update();
}

void BaTabColumn::addItem(const QString &text, bool selected)
{
    items_ << text;
    if (selected)
        current_ = items_.size() - 1;
    setFixedHeight(items_.size() * kItemHeight);
}

void BaTabColumn::setItemIcon(int index, int glyph)
{
    if (index >= 0 && index < 6)
        icons_[index] = glyph;
    update();
}

QSize BaTabColumn::sizeHint() const
{
    return QSize(180, items_.size() * kItemHeight);
}

QRectF BaTabColumn::itemRect(int index) const
{
    return QRectF(0, index * kItemHeight, width(), kItemHeight);
}

int BaTabColumn::hoverIndex() const
{
    return hover_;
}

void BaTabColumn::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform); // 图片缩放平滑（防锯齿）

    if (style_ == Style::Pill) {
        // 设置页整列浅蓝底
        p.fillRect(rect(), BaStyle::lightBlue());
    }

    for (int i = 0; i < items_.size(); ++i) {
        const QRectF rc = itemRect(i);
        const bool selected = (i == current_);

        if (style_ == Style::Pill) {
            // 选中项：白色圆角板；hover 项：半透明白
            if (selected) {
                QPainterPath plate;
                const qreal r = 8;
                plate.addRoundedRect(rc.adjusted(6, 3, -6, -3), r, r);
                p.fillPath(plate, Qt::white);
            } else if (i == hover_ && hoverProgress_ > 0.05) {
                p.setOpacity(0.55 * hoverProgress_);
                p.fillRect(rc, Qt::white);
                p.setOpacity(1.0);
            }
            // 虚线分隔（BA 的 dashed 间隔线）
            if (i > 0) {
                p.setPen(QPen(QColor(255, 255, 255, 130), 1, Qt::DashLine));
                p.drawLine(QPointF(14, rc.top()), QPointF(width() - 14, rc.top()));
            }
        } else {
            // Card：白卡列，选中项 = 深蓝渐变块 + 黄下划线 + 左上浅蓝三角装饰
            const QRectF card = rc.adjusted(4, 3, -4, -3);
            if (selected) {
                QLinearGradient g(card.topLeft(), card.bottomLeft());
                g.setColorAt(0, QColor("#2E6CA8"));
                g.setColorAt(1, QColor("#0F3761"));
                p.setPen(Qt::NoPen);
                p.setBrush(g);
                p.drawRoundedRect(card, 7, 7);
                // 左上浅蓝三角装饰
                p.setBrush(QColor(0x9C, 0xE0, 0xFA, 130));
                QPainterPath tri;
                tri.moveTo(card.left() + 2, card.top() + 2);
                tri.lineTo(card.left() + 14, card.top() + 2);
                tri.lineTo(card.left() + 2, card.top() + 14);
                tri.closeSubpath();
                p.drawPath(tri);
            } else {
                p.setPen(QPen(QColor(0xFF, 0xFF, 0xFF, 220), 1));
                p.setBrush(QColor(255, 255, 255, hover_ == i ? 150 + 80 * hoverProgress_ : 150));
                p.drawRoundedRect(card, 7, 7);
            }
        }

        // 图标（可选）与文字：Pill 选中加粗深蓝；Card 选中白字
        const bool isCardSel = (style_ == Style::Card) && selected;
        if (icons_[i] >= 0) {
            const QColor tint = isCardSel ? Qt::white
                                          : (selected ? BaStyle::accent() : BaStyle::muted());
            p.drawPixmap(QPoint(int(rc.left() + 18), int(rc.center().y() - 10)),
                         ba::pixmap(static_cast<ba::Glyph>(icons_[i]), QSize(20, 20), tint));
        }
        p.setFont(BaStyle::font(10, selected ? QFont::Bold : QFont::DemiBold));
        p.setPen(isCardSel ? Qt::white : BaStyle::deep());

        const qreal textX = icons_[i] >= 0 ? rc.left() + 46 : rc.left() + 26;
        if (isCardSel) {
            // Card 选中：黄下划线贴字底
            const QString &t = items_.at(i);
            const int tw = p.fontMetrics().horizontalAdvance(t);
            p.drawText(QRectF(textX, rc.top(), rc.width() - textX - 8, rc.height()),
                       Qt::AlignVCenter | Qt::AlignLeft, t);
            p.setBrush(BaStyle::yellow());
            p.drawRect(QRectF(textX, rc.bottom() - 12.0, tw, 3.0));
        } else {
            p.drawText(QRectF(textX, rc.top(), rc.width() - textX - 8, rc.height()),
                       Qt::AlignVCenter | Qt::AlignLeft, items_.at(i));
        }
    }
}

void BaTabColumn::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    const int idx = int(event->pos().y() / kItemHeight);
    if (idx >= 0 && idx < items_.size() && idx != current_) {
        current_ = idx;
        emit currentChanged(idx);
        update();
    }
}

void BaTabColumn::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    const int idx = int(event->pos().y() / kItemHeight);
    if (idx != hover_) {
        hover_ = idx;
        hoverAnim_.stop();
        hoverAnim_.setStartValue(hoverProgress_);
        hoverAnim_.setEndValue(idx >= 0 ? 1.0 : 0.0);
        hoverAnim_.start();
    }
}

void BaTabColumn::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    hover_ = -1;
    hoverAnim_.stop();
    hoverAnim_.setStartValue(hoverProgress_);
    hoverAnim_.setEndValue(0.0);
    hoverAnim_.start();
}

void BaTabColumn::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
}

void BaTabColumn::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}
