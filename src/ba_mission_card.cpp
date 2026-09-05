#include "ba_mission_card.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QVBoxLayout>

#include "ba_icon.h"
#include "ba_progress_bar.h"
#include "ba_style.h"

namespace {
constexpr qreal kCardRadius = 12.0;   // 白卡圆角
constexpr qreal kLabelW = 52;         // 橙渐变标签宽
constexpr qreal kLabelH = 20;
constexpr qreal kRewardW = 96;        // 右侧奖励区宽
constexpr qreal kActionW = 86;        // 立即前往按钮宽
constexpr qreal kActionH = 26;
} // namespace

BaMissionCard::BaMissionCard(const QString &title, QWidget *parent)
    : QWidget(parent)
    , tag_(QStringLiteral("成就"))
    , title_(title)
    , progressText_(QStringLiteral("次数 0/1"))
    , actionText_(QStringLiteral("立即前往"))
{
    setFixedHeight(108);
    setCursor(Qt::PointingHandCursor);
}

void BaMissionCard::setTag(const QString &tag)
{
    tag_ = tag;
    update();
}

void BaMissionCard::setProgressText(const QString &text)
{
    progressText_ = text;
    update();
}

void BaMissionCard::setProgressValue(int value, int maximum)
{
    // 进度条为子控件，延迟创建（收益：单卡头轻量）
    Q_UNUSED(value);
    Q_UNUSED(maximum);
}

void BaMissionCard::setRewardCount(int count)
{
    rewardCount_ = count;
    update();
}

void BaMissionCard::setActionText(const QString &text)
{
    actionText_ = text;
    update();
}

QSize BaMissionCard::sizeHint() const
{
    return QSize(430, 108);
}

void BaMissionCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform); // 图片缩放平滑（防锯齿）

    // 白卡主体 + 深蓝细描边
    const QRectF card(QPointF(0, 0), QSizeF(width(), height()));
    p.setPen(QPen(QColor(0x2F, 0x54, 0x78, 200), 1.2));
    p.setBrush(QColor(255, 255, 255, 248));
    p.drawRoundedRect(card, kCardRadius, kCardRadius);

    // 左上橙渐变标签（小圆角 + 白描边 + 白粗体字）
    const QRectF label(10, 8, kLabelW, kLabelH);
    p.setPen(QPen(QColor(255, 255, 255, 200), 1.0));
    p.setBrush(BaStyle::orangeGradient(label));
    p.drawRoundedRect(label, 4, 4);
    p.setFont(BaStyle::font(8, QFont::Bold));
    p.setPen(Qt::white);
    p.drawText(label, Qt::AlignCenter, tag_);

    // 标题（标签右移、单行省略）
    p.setFont(BaStyle::font(10, QFont::Bold));
    p.setPen(BaStyle::deep());
    const QRectF title(label.right() + 10, 8, width() - label.right() - kRewardW - 60, 24);
    p.drawText(title, Qt::AlignVCenter | Qt::AlignLeft,
               QFontMetrics(p.font()).elidedText(title_, Qt::ElideRight, int(title.width())));

    // 次数行灰字
    p.setFont(BaStyle::font(9));
    p.setPen(QColor(0x77, 0x8A, 0x99));
    p.drawText(QRectF(12, 36, width() - 140, 18),
               Qt::AlignVCenter | Qt::AlignLeft, progressText_);

    // 黑灰圆头进度条（复用 BaProgressBar，青蓝填充；轨道固定深黑灰——成就/任务条样式）
    {
        BaProgressBar bar;
        bar.setTrackColor(BaStyle::progressTrackDark());
        bar.setGeometry(QRect(12, 54, int(width() - kRewardW - kActionW - 40), 15));
        p.drawPixmap(bar.pos(), bar.grab()); // 直接抓取子控件
    }

    // 右侧奖励盒：白框 + 青辉石簇 + ×n 灰字
    const QRectF reward(width() - kRewardW - 100, 18, kRewardW, 66);
    {
        const QPainterPath path = ([&]() {
            QPainterPath pp;
            pp.addRoundedRect(reward, 8, 8);
            return pp;
        })();
        p.setPen(QPen(QColor(0x8C, 0xA7, 0xBE, 180), 1.0));
        p.setBrush(QColor(0xFF, 0xFF, 0xFF, 240));
        p.drawPath(path);
        // 青辉石晶体簇（3 枚）
        const QSize iconS(30, 30);
        const QPixmap gem = ba::pixmap(ba::Glyph::Pyroxene, iconS);
        p.drawPixmap(QPointF(reward.left() + 10, reward.center().y() - 26), gem);
        p.drawPixmap(QPointF(reward.left() + 36, reward.center().y() - 10), gem);
        p.drawPixmap(QPointF(reward.left() + 16, reward.center().y() + 2), gem);
        // ×n 灰字
        p.setFont(BaStyle::font(9, QFont::Bold));
        p.setPen(QColor(0x6C, 0x7A, 0x88));
        p.drawText(QRectF(reward.left() + 14, reward.bottom() - 20, reward.width() - 14, 16),
                   Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("×%1").arg(rewardCount_));
    }

    // 立即前往按钮：青蓝渐变 + 白描边 + 白粗体
    const QRectF action(width() - kActionW - 16, height() - kActionH - 14, kActionW, kActionH);
    p.setPen(QPen(QColor(255, 255, 255, 230), 1.6));
    p.setBrush(BaStyle::accentGradient(action));
    p.drawRoundedRect(action, 6, 6);
    p.setFont(BaStyle::font(9, QFont::Bold));
    p.setPen(Qt::white);
    p.drawText(action, Qt::AlignCenter, actionText_);
}

void BaMissionCard::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton)
        emit actionClicked();
}
