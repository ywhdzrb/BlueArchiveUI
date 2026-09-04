#include "md3_badge.h"
#include "md3_theme.h"

#include <QPainter>
#include <QEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QFontMetricsF>

namespace {
// 徽章尺寸（像素）
constexpr qreal kBadgeSize = 16.0;    // 常规徽章直径
constexpr qreal kDotSize = 7.0;       // 小圆点直径（count 未设置时）
constexpr qreal kFontSize = 11.0;     // 数字字号
constexpr int kTopPad = 10;           // 徽章圈延伸部分
}

Md3Badge::Md3Badge(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);   // 不拦鼠标
    hide();
}

void Md3Badge::setCount(int count)
{
    count_ = count;
    if (count_ <= 0) {
        hide();
        return;
    }
    // 角标自身按内容尺寸固定，保证贴角定位与点画圆以中心为基准
    setFixedSize(qRound(kBadgeSize), qRound(kBadgeSize));
    show();
    updateGeometry();
    update();
    reposition();
}

// dot 模式：只显示小圆点，忽略数字；切换时同步尺寸与可见性
void Md3Badge::setDot(bool dot)
{
    if (dot_ == dot) {
        return;
    }
    dot_ = dot;
    const qreal size = dot_ ? kDotSize : kBadgeSize;
    setFixedSize(qRound(size), qRound(size));
    if (dot_) {
        show();
    } else if (count_ <= 0) {
        hide();
    }
    updateGeometry();
    update();
    reposition();
}

void Md3Badge::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

// 挂到宿主控件：作为其子控件，始终浮于宿主上层（加高 z 值）
void Md3Badge::attachTo(QWidget *host)
{
    if (host_ == host) {
        return;
    }
    if (host_) {
        host_->removeEventFilter(this);
    }
    host_ = host;
    if (host_) {
        host_->installEventFilter(this);
        setParent(host_);
    }
    reposition();
}

// 宿主移动 / 缩放 / 属性变化时重新对位
bool Md3Badge::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == host_ && event->type() == QEvent::Move) {
        reposition();
    } else if (watched == host_ && event->type() == QEvent::Resize) {
        reposition();
    }
    return QWidget::eventFilter(watched, event);
}

// 定位：徽章中心贴在宿主控件右上角。
// 角标中心坐标 = 宿主右上角 + 偏移 (4, 3)（视觉上轻微超出宿主右缘）
void Md3Badge::reposition()
{
    if (!host_) {
        return;
    }
    const QPointF center(host_->width() - 4.0, 4.0);
    move(QPoint(static_cast<int>(center.x() - width() / 2.0),
                static_cast<int>(center.y() - height() / 2.0)));
}

void Md3Badge::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal size = dot_ ? kDotSize : kBadgeSize;

    // 气泡圆形底（error 色）
    p.setPen(Qt::NoPen);
    p.setBrush(theme_.error);
    p.drawEllipse(QPointF(width() / 2.0, height() / 2.0), size / 2.0, size / 2.0);

    // 数字（仅常规徽章），on-error 色，11px
    if (!dot_ && count_ > 0) {
        const QString text = count_ > 99 ? QStringLiteral("99+") : QString::number(count_);
        QFont f = font();
        f.setPointSizeF(kFontSize);
        f.setWeight(QFont::Medium);
        QFontMetricsF fm(f);
        p.setPen(theme_.onError);
        p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter, text);
    }
}
