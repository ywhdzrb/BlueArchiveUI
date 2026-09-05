#include "ba_top_bar.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include "ba_assets.h"
#include "ba_icon.h"
#include "ba_style.h"

namespace {

// 资源胶囊单元（真机样式）：独立白胶囊 + 官方图标 + 白色数字（深蓝描边保证浅底可读）
void drawResourcePill(QPainter &p, qreal &x, const QRectF &base, ba::Glyph glyph,
                      const QString &value, const QString &iconPath)
{
    QFont f = BaStyle::font(9, QFont::Bold);
    p.setFont(f);
    const int tw = p.fontMetrics().horizontalAdvance(value) + 4;
    const qreal w = 8 + 18 + 6 + tw + 8;
    // 胶囊改斜切平行四边形（同族 -16°）
    const QPainterPath ph = BaStyle::skewRectPath(
        QRectF(x, base.y(), w, base.height()), -16.0, qMin(5.0, base.height() * 0.16));
    p.fillPath(ph, QColor(255, 255, 255, 235));
    p.setPen(QPen(QColor(0xCF, 0xE5, 0xF0), 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(ph);
    // 官方图标
    p.setPen(Qt::NoPen);
    if (!iconPath.isEmpty()) {
        QPixmap img = BaAssets::image(iconPath);
        if (!img.isNull()) {
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            p.drawPixmap(QRectF(x + 7, base.y() + (base.height() - 18) / 2.0, 18, 18).toRect(),
                         img, QRectF(0, 0, img.width(), img.height()));
        }
    }
    p.drawPixmap(QPointF(x + 7, base.y() + (base.height() - 18) / 2.0),
                 ba::pixmap(glyph, QSize(18, 18)));
    // 深蓝粗字（白胶囊上最优可读）
    const QRectF tr(x + 8 + 18 + 6, base.y(), tw + 8, base.height());
    p.setPen(BaStyle::deep());
    p.drawText(tr, Qt::AlignVCenter | Qt::AlignLeft, value);
    x += w + 8;
}

} // namespace

BaTopBar::BaTopBar(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    title_ = title;
    setFixedHeight(64);
    setCursor(Qt::ArrowCursor);
}

void BaTopBar::setMode(Mode mode)
{
    mode_ = mode;
    update();
}

void BaTopBar::setTitle(const QString &title)
{
    title_ = title;
    update();
}

void BaTopBar::setTitleDark(bool dark)
{
    titleDark_ = dark;
    update();
}

void BaTopBar::setWallet(const QString &apText, const QString &coinText, const QString &gemText)
{
    apText_ = apText;
    coinText_ = coinText;
    gemText_ = gemText;
    update();
}

void BaTopBar::setWalletVisible(bool on)
{
    walletVisible_ = on;
    update();
}

void BaTopBar::setPlayer(const QString &name, int level, const QString &subText)
{
    playerName_ = name;
    playerLevel_ = level;
    playerSub_ = subText;
    playerKnown_ = true;
    update();
}

QRectF BaTopBar::backButtonRect() const
{
    return QRectF(8, 11, 42, 42);
}

QRectF BaTopBar::playerCardRect() const
{
    return QRectF(0, 0, 172, 48);
}

QRectF BaTopBar::homeRect() const
{
    return QRectF(width() - 44, 13, 38, 38);
}

QRectF BaTopBar::gearRect() const
{
    return QRectF(homeRect().left() - 42, 13, 38, 38);
}

QRectF BaTopBar::plusRect() const
{
    return QRectF(gearRect().left() - 42, 13, 38, 38);
}

QRectF BaTopBar::pillRect() const
{
    return QRectF(plusRect().left() - 10, 14, 0, 36);
}

void BaTopBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform); // 图片缩放平滑（防锯齿）

    // 主玻璃条：白色约 92% + 底部向下渐隐（BA 顶栏浮在背景上随页面滚动）
    QLinearGradient glass(0, 0, 0, height());
    glass.setColorAt(0, QColor(255, 255, 255, 243));
    glass.setColorAt(1, QColor(255, 255, 255, 210));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRectF(0, 0, width(), 60), 0, 0);

    if (mode_ == Mode::Hall && playerKnown_) {
        // ---------- 大厅态：左上玩家卡（真机样式：左端 42px 向斜切 + 右圆角 8 + Lv.+大数字+名+进度）----------
        const QRectF pc = playerCardRect();
        QPainterPath card;
        constexpr qreal cut = 42.0;
        constexpr qreal rr = 8.0;
        card.moveTo(pc.left() + cut, pc.top());
        card.lineTo(pc.right() - rr, pc.top());
        card.quadTo(pc.right(), pc.top(), pc.right(), pc.top() + rr);
        card.lineTo(pc.right(), pc.bottom() - rr);
        card.quadTo(pc.right(), pc.bottom(), pc.right() - rr, pc.bottom());
        card.lineTo(pc.left() + 2, pc.bottom());
        card.closeSubpath();
        p.setOpacity(0.94);
        p.fillPath(card, BaStyle::deepGradient(pc));
        p.setOpacity(1.0);
        // Lv. 黄字 + 白色大数字
        p.setFont(BaStyle::font(8, QFont::Bold));
        p.fillRect(QRectF(pc.left() + 12, pc.top() + 10, 30, 18), Qt::transparent);
        p.setPen(QColor(0xFF, 0xE4, 0x33));
        p.drawText(QRectF(pc.left() + 12, pc.top() + 7, 34, 14),
                   Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("Lv."));
        p.setPen(Qt::white);
        p.setFont(BaStyle::font(17, QFont::Bold));
        p.drawText(QRectF(pc.left() + 12, pc.top() + 15, 46, 26),
                   Qt::AlignVCenter | Qt::AlignLeft, QString::number(playerLevel_));
        // 白色粗名
        p.setFont(BaStyle::font(10, QFont::Bold));
        p.drawText(QRectF(pc.left() + 64, pc.top() + 8, 120, 16),
                   Qt::AlignVCenter | Qt::AlignLeft, playerName_);
        // 青蓝进度条 + 白色小字（subText 为空则省略）
        if (!playerSub_.isEmpty()) {
            const QRectF pw(pc.left() + 64, pc.top() + 28, 150, 5);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 255, 255, 60));
            p.drawRoundedRect(pw, 2.5, 2.5);
            p.setBrush(QColor(0x7F, 0xD8, 0xFA));
            p.drawRoundedRect(pw.adjusted(0, 0, -(pw.width() * 0.55), 0), 2.5, 2.5);
            p.setPen(Qt::white);
            p.setFont(BaStyle::font(7));
            p.drawText(QRectF(pw.left(), pw.bottom() + 3, 130, 12),
                       Qt::AlignVCenter | Qt::AlignLeft, playerSub_);
        }
        return;
    }

    // ---------- Page 态：返回钮 + 标题 ----------
    const QRectF back = backButtonRect();
    // 白底圆衬 + 深蓝渐变主圆 + 浅蓝描边
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 200));
    p.drawEllipse(back.adjusted(-4, -4, 4, 4));
    QPainterPath circ;
    circ.addEllipse(back);
    p.fillPath(circ, BaStyle::deepGradient(back));
    p.setPen(QPen(QColor(255, 255, 255, 150), 1.6));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(back.adjusted(1.5, 1.5, -1.5, -1.5));
    // 白箭头
    p.setPen(QPen(Qt::white, 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const QPointF c = back.center();
    QPainterPath arrow;
    arrow.moveTo(c.x() + 5, c.y() - 8);
    arrow.lineTo(c.x() - 5.5, c.y());
    arrow.lineTo(c.x() + 5, c.y() + 8);
    p.drawPath(arrow);

    // 标题：深蓝粗体（白描边）或白字蓝描边（深色条模式）
    QFont tf = BaStyle::font(14, QFont::Bold);
    p.setFont(tf);
    const QRectF titleRect(back.right() + 8, 10, width() - back.right() - 460, 30);
    if (titleDark_) {
        p.setPen(QPen(BaStyle::deep(), 3.6));
        p.drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, title_);
        p.setPen(Qt::white);
        p.drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, title_);
    } else {
        // 深藏蓝 + 白色细描边（白玻条上提清晰度）
        p.setPen(QPen(Qt::white, 1.6));
        p.drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, title_);
        p.setPen(BaStyle::deep());
        p.drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, title_);
    }
    // 标题底部黄色下划线（宽=字宽，贴字底）——与游戏页面大标题一致
    {
        const QFontMetrics fm(tf);
        const qreal tw = fm.horizontalAdvance(title_);
        const qreal ty = 40.5; // 标题垂直中心 ~25，词底 ~36，线 3.5px 放 40.5 贴近
        p.setPen(Qt::NoPen);
        p.setBrush(BaStyle::yellow());
        p.drawRect(QRectF(titleRect.left(), ty, qMin(tw, width() - titleRect.left() - 480.0), 3.5));
    }

    // ---------- 右侧：三个独立资源胶囊 + 加号片 + 三功能片（真机样式全白字）----------
    if (walletVisible_) {
        const QRectF base(0, 15, 0, 34);
        qreal x = plusRect().left() - 12;
        // 从右往左精确排：+片(AP后) Gem -> Coin+AP 组合从右铺开？真机顺序由左到右：AP、＋、Coin、Gem、＋
        // 简单按真机：AP 胶囊 → ＋片 → Coin 胶囊 → Gem 胶囊 → ＋片，右端紧贴功能片
        // 先算总宽
        QFont f = BaStyle::font(9, QFont::Bold);
        p.setFont(f);
        const QFontMetrics fm(f);
        const qreal apW = 8 + 18 + 6 + fm.horizontalAdvance(apText_) + 4 + 8;
        const qreal coinW = 8 + 18 + 6 + fm.horizontalAdvance(coinText_) + 4 + 8;
        const qreal gemW = 8 + 18 + 6 + fm.horizontalAdvance(gemText_) + 4 + 8;
        const qreal plusW = 22;
        qreal left = x - (gemW + 8 + plusW + 8 + coinW + 8 + plusW + 8 + apW);
        qreal xx = left;
        drawResourcePill(p, xx, base, ba::Glyph::Lightning, apText_, "img/icons/Common_Icon_Stamina.png");
        // ＋小片（斜切 -16°）
        const auto plusPath = [](qreal x0, qreal y0, qreal w0, qreal h0) {
            return BaStyle::skewRectPath(QRectF(x0, y0, w0, h0), -16.0, 4.0);
        };
        p.fillPath(plusPath(xx, 21, plusW, 22), QColor(255, 255, 255, 235));
        p.setPen(QPen(QColor(0xCF, 0xE5, 0xF0), 1.2));
        p.drawPath(plusPath(xx, 21, plusW, 22));
        p.drawPixmap(QPointF(xx + 2, 23), ba::pixmap(ba::Glyph::Plus, QSize(18, 18), BaStyle::deep()));
        xx += plusW + 8;
        drawResourcePill(p, xx, base, ba::Glyph::Coin, coinText_, "img/icons/Common_Icon_Gold_Base.png");
        drawResourcePill(p, xx, base, ba::Glyph::Pyroxene, gemText_, "img/icons/Common_Icon_Diamond.png");
        // Gem 后独立＋片（斜切 -16°）
        p.fillPath(plusPath(xx, 21, plusW, 22), QColor(255, 255, 255, 235));
        p.setPen(QPen(QColor(0xCF, 0xE5, 0xF0), 1.2));
        p.drawPath(plusPath(xx, 21, plusW, 22));
        p.drawPixmap(QPointF(xx + 2, 23), ba::pixmap(ba::Glyph::Plus, QSize(18, 18), BaStyle::deep()));
    }

    drawCircleButton(p, plusRect(), ba::Glyph::Ticket, BaStyle::deep());
    drawCircleButton(p, gearRect(), ba::Glyph::Mail, BaStyle::deep());
    drawCircleButton(p, homeRect(), ba::Glyph::Grid, BaStyle::deep());
}

void BaTopBar::drawCircleButton(QPainter &p, const QRectF &r, ba::Glyph glyph, const QColor &tint)
{
    // 白片斜切钮（BA 顶栏快捷片同族 -16° 平行四边形）：斜切白底 + 淡蓝描边 + 深蓝线稿图标
    const QRectF base = r.adjusted(2.0, 2.0, -2.0, -2.0);
    const QPainterPath path = BaStyle::skewRectPath(base, -16.0, qMin(4.0, base.height() * 0.12));
    p.fillPath(path, QColor(255, 255, 255, 235));
    p.setPen(QPen(QColor(0xCF, 0xE5, 0xF0), 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    const QSize s(18, 18);
    p.drawPixmap(QPointF(r.center().x() - s.width() / 2.0, r.center().y() - s.height() / 2.0),
                 ba::pixmap(glyph, s, tint));
}

void BaTopBar::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
}

void BaTopBar::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
    if (event->button() != Qt::LeftButton)
        return;
    const QPointF pos = event->pos();
    if (mode_ == Mode::Page && backButtonRect().adjusted(-4, -4, 4, 4).contains(pos)) {
        emit backClicked();
        return;
    }
    if (walletVisible_ && plusRect().contains(pos))
        emit plusClicked();
    if (gearRect().contains(pos))
        emit gearClicked();
    if (homeRect().contains(pos))
        emit homeClicked();
}
