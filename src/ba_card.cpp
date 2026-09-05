// 基础卡片实现
// 卡体：-16° 平行四边形（与按钮/进度条同族，BaStyle::skewRectPath）
// 内容：addRow 手动行式布局——每行 x 随自身 y 沿斜边偏移，保证水平文字与左斜边
// 等距对齐（切线补偿公式 x = margin + tan(角) * (卡中心y - 行中心y)）

#include "ba_card.h"

#include "ba_style.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

namespace {

constexpr qreal kSkewDeg = -16.0; // 与按钮/进度条同族斜切角
constexpr qreal kRadius = 12.0;   // 卡圆角
constexpr qreal kRowMargin = 20.0; // 行距左斜边水平距离
constexpr qreal kEdgeInset = 2.0;  // 底座内缩（投影防裁）

} // namespace

BaCard::BaCard(QWidget *parent)
    : QWidget(parent)
    , content_(new QVBoxLayout(this))
{
    content_->setContentsMargins(20, 16, 16, 16);
    // 无 WM/截图环境布局不自动激活，进事件循环后主动激活一次
    QTimer::singleShot(0, this, [this]() {
        content_->invalidate();
        content_->activate();
    });
}

QVBoxLayout *BaCard::contentLayout() const
{
    return content_;
}

void BaCard::addRow(QWidget *widget)
{
    widget->setParent(this);
    rows_.push_back(widget);
    layoutRows();
}

void BaCard::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutRows();
}

void BaCard::layoutRows()
{
    if (rows_.isEmpty())
        return;

    // 与 paintEvent 完全一致的卡体几何（skewRectPath：先 sx 压缩再 shear）
    const QRectF base(2.0, 2.5, width() - 4.0, height() - 5.5);
    const qreal k = qTan(qDegreesToRadians(kSkewDeg)); // 负角
    const qreal sx = qMax(0.2, 1.0 - qAbs(k) * base.height() / qMax(base.width(), 1.0));
    const qreal cxB = base.center().x();
    const qreal cyB = base.center().y();
    const qreal cxS = cxB + sx * (base.left() - cxB); // 压缩后矩形左缘
    const auto xEdge = [&](qreal y) -> qreal {
        // 压缩后左缘 + shear 位移：x = cxS + k*(y - cyB)
        return cxS + k * (y - cyB);
    };

    // 行高累加（sizeHint 高度，至少 24）
    qreal yTop = 14.0;
    QVector<qreal> heights;
    qreal total = 0.0;
    for (QWidget *w : rows_) {
        const qreal h = qMax(24.0, qreal(w->sizeHint().height()));
        heights.push_back(h);
        total += h;
    }
    // 行间距：均匀填充剩余空间（至少 10）
    const qreal spacing = rows_.size() > 1
        ? qMax(10.0, (height() - 28.0 - total) / (rows_.size() - 1))
        : 0.0;

    for (int i = 0; i < rows_.size(); ++i) {
        QWidget *w = rows_.at(i);
        const qreal h = heights.at(i);
        const qreal ycY = yTop + h / 2.0;
        // 行左缘 = 该 y 处真实斜边 x + 恒定留白（与斜边平行等距）
        const qreal x = xEdge(ycY) + kRowMargin;
        w->setGeometry(int(x), int(yTop), width() - int(x) - 16, int(h));
        yTop += h + spacing;
    }
}

void BaCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 底座内缩使投影完整落在控件内
    const QRectF base(kEdgeInset, kEdgeInset + 0.5, width() - 2 * kEdgeInset, height() - 2 * kEdgeInset - 1.5);

    // 投影（两层半透明偏移描摹）
    p.setPen(Qt::NoPen);
    QPainterPath shadow1 = BaStyle::skewRectPath(base.translated(0, 1.6), kSkewDeg, kRadius);
    p.setBrush(QColor(0x6B, 0x7F, 0x8D, 45));
    p.drawPath(shadow1);
    QPainterPath shadow2 = BaStyle::skewRectPath(base.translated(0, 1.4), kSkewDeg, kRadius);
    p.setBrush(QColor(0x6B, 0x7F, 0x8D, 30));
    p.drawPath(shadow2);

    // 白卡体 + 深蓝描边
    const QPainterPath body = BaStyle::skewRectPath(base, kSkewDeg, kRadius);
    p.setBrush(Qt::white);
    p.setPen(QPen(QColor(0x2F, 0x54, 0x78, 200), 1.2));
    p.drawPath(body);
}
