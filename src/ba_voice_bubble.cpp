#include "ba_voice_bubble.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include "ba_style.h"

namespace {
constexpr qreal kRadius = 18.0;
}

BaVoiceBubble::BaVoiceBubble(const QString &text, QWidget *parent)
    : QWidget(parent)
{
    text_ = text;
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void BaVoiceBubble::setText(const QString &text) { text_ = text; updateGeometry(); update(); }
void BaVoiceBubble::setSpeaker(const QString &name) { speaker_ = name; updateGeometry(); update(); }

QSize BaVoiceBubble::sizeHint() const
{
    const QFontMetrics fm(BaStyle::font(11));
    const int textW = fm.horizontalAdvance(text_);
    return QSize(qMax(220, textW + 48), 52 + (speaker_.isEmpty() ? 0 : 20));
}

QSize BaVoiceBubble::minimumSizeHint() const
{
    return QSize(160, 44);
}

void BaVoiceBubble::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal top = speaker_.isEmpty() ? 0 : 22;
    const QRectF rc(0, top, width(), height() - top - 14);

    // 白色大圆角板 + 浅蓝描边
    QPainterPath body;
    body.addRoundedRect(rc, kRadius, kRadius);
    p.fillPath(body, QColor(255, 255, 255, 236));
    p.setPen(QPen(QColor(0x9C, 0xD9, 0xF2, 200), 1.4));
    p.setBrush(Qt::NoBrush);
    p.drawPath(body);

    // 右下角小尾巴（小圆三角）
    QPainterPath tail;
    tail.moveTo(rc.right() - 26, rc.bottom() - 1);
    tail.lineTo(rc.right() - 6, rc.bottom() - 1);
    tail.lineTo(rc.right() - 2, rc.bottom() + 12);
    tail.closeSubpath();
    p.fillPath(tail, QColor(255, 255, 255, 236));

    // 深蓝文字（WordWrap）
    p.setFont(BaStyle::font(11));
    p.setPen(BaStyle::deep());
    p.drawText(rc.adjusted(20, 6, -20, -8), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, text_);

    // 名字小条（可选）：左上角浅蓝渐变圆角条 + 白字
    if (!speaker_.isEmpty()) {
        QFont f = BaStyle::font(9, QFont::Bold);
        p.setFont(f);
        const int tw = p.fontMetrics().horizontalAdvance(speaker_);
        const QRectF nameRc(0, 0, tw + 22, 18);
        QPainterPath npath;
        npath.addRoundedRect(nameRc, 9, 9);
        QLinearGradient g(nameRc.topLeft(), nameRc.bottomLeft());
        g.setColorAt(0, QColor("#4A9AD8"));
        g.setColorAt(1, QColor("#1C5A8F"));
        p.fillPath(npath, g);
        p.setPen(Qt::white);
        p.drawText(nameRc, Qt::AlignCenter, speaker_);
    }
}
