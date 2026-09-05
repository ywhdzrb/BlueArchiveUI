#include "ba_player_card.h"

#include "ba_icon.h"
#include "ba_style.h"

#include <QPainter>
#include <QPaintEvent>

namespace {

constexpr qreal kRadius = 10.0;   // 右端圆角
constexpr qreal kBackCut = 42.0;  // 左端大斜切（左上尖角）
constexpr qreal kPBarH = 6.0;     // 进度条高

} // namespace

BaPlayerCard::BaPlayerCard(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(300);
    setMinimumHeight(76);
}

void BaPlayerCard::setLevel(int lv)
{
    level_ = lv;
    update();
}

void BaPlayerCard::setNameText(const QString &name)
{
    name_ = name;
    update();
}

void BaPlayerCard::setProgress(int value, int max)
{
    value_ = value;
    max_ = qMax(1, max);
    update();
}

void BaPlayerCard::setSubText(const QString &text)
{
    subText_ = text;
    update();
}

void BaPlayerCard::setEditVisible(bool visible)
{
    editVisible_ = visible;
    update();
}

QSize BaPlayerCard::sizeHint() const
{
    return QSize(300, 76);
}

QSize BaPlayerCard::minimumSizeHint() const
{
    return QSize(280, 72);
}

void BaPlayerCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform); // 图片缩放平滑（防锯齿）
    p.setRenderHint(QPainter::TextAntialiasing);

    const QRectF r(QPointF(0, 0), QSizeF(width(), height()));

    // 深蓝渐变板：左端大斜切（左上尖角）+ 右端圆角（真机玩家卡样式）
    QPainterPath body;
    body.moveTo(r.left() + kBackCut, r.top());
    body.lineTo(r.right() - kRadius, r.top());
    body.lineTo(r.right(), r.top() + kRadius);
    body.lineTo(r.right(), r.bottom() - kRadius);
    body.lineTo(r.right() - kRadius, r.bottom());
    body.lineTo(r.left() + kBackCut * 0.28, r.bottom());
    body.closeSubpath();
    QLinearGradient bodyG(r.topLeft(), r.bottomLeft());
    bodyG.setColorAt(0.0, QColor(0x1B, 0x4A, 0x7A));
    bodyG.setColorAt(1.0, QColor(0x0C, 0x2A, 0x4C));
    p.fillPath(body, bodyG);

    // 左区：黄 Lv. + 白色大数字（Lv. 在数字上方，错位排版同真机）
    p.setPen(QColor(0xF5, 0xC4, 0x45));
    p.setFont(BaStyle::font(7, QFont::Bold));
    p.drawText(QRectF(14, 6, 70, 14), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Lv."));
    p.setPen(Qt::white);
    p.setFont(BaStyle::font(19, QFont::Bold));
    p.drawText(QRectF(12, 14, 70, 32), Qt::AlignLeft | Qt::AlignVCenter,
               QString::number(level_));

    // 名称：白色粗体（数值右侧）
    const qreal nameX = 88;
    p.setFont(BaStyle::font(11, QFont::Bold));
    p.drawText(QRectF(nameX, 10, width() - nameX - 40, 22), Qt::AlignLeft | Qt::AlignVCenter, name_);

    // 进度条：亮青圆头条 + 暗蓝轨道
    const qreal pbY = 40;
    const qreal pbW = 130;
    const QRectF pbRect(nameX, pbY, pbW, kPBarH);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x0A, 0x23, 0x42));
    p.drawRoundedRect(pbRect, kPBarH / 2.0, kPBarH / 2.0);
    const qreal ratio = value_ / qreal(max_);
    if (ratio > 0.01) {
        QRectF fill = pbRect;
        fill.setWidth(pbRect.width() * ratio);
        QLinearGradient fillG(fill.topLeft(), fill.bottomLeft());
        fillG.setColorAt(0.0, QColor(0x7F, 0xD8, 0xFA));
        fillG.setColorAt(1.0, QColor(0x4D, 0xC2, 0xF5));
        p.setBrush(fillG);
        p.drawRoundedRect(fill, kPBarH / 2.0, kPBarH / 2.0);
    }

    // 数值小字（subText 空时自动 "value/max"）
    p.setPen(QColor(0xBF, 0xDC, 0xF2));
    p.setFont(BaStyle::font(8));
    const QString sub = subText_.isEmpty()
                            ? QStringLiteral("%1/%2").arg(value_).arg(max_)
                            : subText_;
    p.drawText(QRectF(nameX, pbY + 10, pbW + 40, 20), Qt::AlignLeft | Qt::AlignVCenter, sub);

    // 右上铅笔圆钮：深蓝渐变圆片 + 白铅笔
    if (editVisible_) {
        const QRectF btn(width() - 34, 6, 28, 28);
        p.setPen(QPen(QColor(0xCF, 0xE0, 0xEC), 1));
        QLinearGradient btnG(btn.topLeft(), btn.bottomLeft());
        btnG.setColorAt(0.0, QColor(0x46, 0x81, 0xB8));
        btnG.setColorAt(1.0, QColor(0x1C, 0x4A, 0x7E));
        p.setBrush(btnG);
        p.drawEllipse(btn);
        p.drawPixmap(btn.center().toPoint() + QPoint(-8, -8),
                     ba::pixmap(ba::Glyph::Pencil, QSize(16, 16), Qt::white));
    }
}
