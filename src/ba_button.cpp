#include "ba_button.h"

#include "ba_icon.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>

namespace {

constexpr qreal kSkewDeg = -16.0;  // CodePen 标准左倾角
constexpr qreal kRadius = 6.0;     // 角部小圆角（本体还随高度比例微调）
constexpr int kHeight = 34;

// CodePen@mangopomelo 变体色板：纯色底 + 前景文字色（与 .btn-primary 等一一对应）
QColor flatColorOf(ba::SurfaceRole role)
{
    switch (role) {
    case ba::SurfaceRole::Sky:     return QColor(0, 200, 242);      // btn-primary
    case ba::SurfaceRole::Green:   return QColor(63, 201, 72);      // btn-success
    case ba::SurfaceRole::Purple:  return QColor(150, 120, 240);    // 扩展紫色
    case ba::SurfaceRole::Yellow:  return QColor(245, 232, 74);     // btn-warning
    case ba::SurfaceRole::Red:     return QColor(198, 66, 66);      // btn-danger
    case ba::SurfaceRole::Deep:    return QColor(45, 70, 100);      // btn-dark
    case ba::SurfaceRole::Ghost:   return QColor(212, 238, 238);    // btn-light/secondary
    }
    return QColor(237, 237, 237);  // 默认 .btn
}

QColor flatTextOf(ba::SurfaceRole role)
{
    switch (role) {
    case ba::SurfaceRole::Sky:     return QColor(45, 70, 99);       // 深蓝字
    case ba::SurfaceRole::Green:   return Qt::white;
    case ba::SurfaceRole::Purple:  return Qt::white;
    case ba::SurfaceRole::Yellow:  return QColor(75, 33, 22);       // 深褐字
    case ba::SurfaceRole::Red:     return QColor(253, 226, 5);      // 亮黄字
    case ba::SurfaceRole::Deep:    return QColor(255, 215, 0);      // 金字
    case ba::SurfaceRole::Ghost:   return QColor(0, 43, 79);        // 深藏蓝字
    }
    return QColor(95, 107, 125);   // 默认灰蓝字
}

} // namespace

BaButton::BaButton(const QString &text, ba::SurfaceRole role, QWidget *parent)
    : QAbstractButton(parent)
    , scalePress_(100, ba::Animator::outCubic,
                  // 每帧写入按压缩放值并重绘
                  [this](qreal t) {
                      scale_ = t;
                      update();
                  })
    , scaleRelease_(100, ba::Animator::outCubic,
                    // 每帧写入回弹缩放值并重绘（ease-out，与 CSS transition 100ms ease-out 对应）
                    [this](qreal t) {
                        scale_ = t;
                        update();
                    })
{
    role_ = role;
    setText(text);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);

    // CodePen 变体色：纯色底自动搭配深蓝/白字
    setTextColor(flatTextOf(role));
}

void BaButton::setSkewDeg(qreal deg)
{
    skewDeg_ = deg;
    update();
}

void BaButton::setRole(ba::SurfaceRole role)
{
    role_ = role;
    if (!customTextColor_)
        setTextColor(flatTextOf(role));
    update();
}

void BaButton::setTextColor(const QColor &c)
{
    customTextColor_ = true;
    QPalette pal = palette();
    pal.setColor(QPalette::WindowText, c);
    setPalette(pal);
}

void BaButton::setFixedHeight(int h)
{
    QAbstractButton::setFixedHeight(h);
}

void BaButton::setIconGlyph(ba::Glyph glyph)
{
    icon_.glyph = glyph;
    update();
}

void BaButton::setIconVisible(bool visible)
{
    icon_.visible = visible;
    update();
}

QSize BaButton::sizeHint() const
{
    const QFontMetrics fm(font());
    const int w = fm.horizontalAdvance(text()) + (icon_.visible ? 26 : 0) + 32;
    return QSize(w, kHeight);
}

QSize BaButton::minimumSizeHint() const
{
    return sizeHint();
}

void BaButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform); // 图片缩放平滑（防锯齿）

    // 整体 opacity 0.8（CodePen .btn 属性）
    p.setOpacity(0.8);

    // 以中心为锚做按压缩放：先平移原点再缩放，保证几何中心不动
    const QRectF base(QPointF(0, 0), QSizeF(width(), height()));
    p.translate(rect().center());
    p.scale(scale_, scale_);
    p.translate(-rect().center());

    const QPainterPath path = currentPath();

    // 底部投影：box-shadow 0px 2px 5px rgb(200,200,200)（两层偏移淡化模拟模糊）
    p.save();
    p.translate(0, 1.6);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(200, 200, 200, 130));
    p.drawPath(path);
    p.translate(0, 1.4);
    p.setBrush(QColor(200, 200, 200, 60));
    p.drawPath(path);
    p.restore();

    // 填充 bg：纯色（disabled 置灰），hover 不加减亮（CodePen 无 hover 效果）
    const bool enabled = isEnabled();
    p.setPen(QPen(QColor(200, 200, 200), 1));   // 1px 灰边框
    p.setBrush(enabled ? flatColorOf(role_) : QColor(216, 216, 216));
    p.drawPath(path);

    if (focused_) {
        p.setPen(QPen(BaStyle::accent(), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }

    // 文字水平绘制（不随形状斜切 = CSS .btn>* 反向 skew 效果）
    QColor textColor = palette().color(QPalette::WindowText);
    if (!enabled)
        textColor = QColor(153, 153, 153);      // btn-disabled 灰字

    // 可选前置小图标
    const int textW = fontMetrics().horizontalAdvance(text());
    const int iconW = icon_.visible ? 22 : 0;
    const qreal cx = rect().center().x();
    qreal tx = cx - (textW + (icon_.visible ? iconW + 6 : 0)) / 2.0;
    if (icon_.visible) {
        p.drawPixmap(QPoint(int(tx), int(rect().center().y() - 11)),
                     ba::pixmap(icon_.glyph, QSize(22, 22), textColor));
        tx += iconW + 6;
    }

    const QFont tf = BaStyle::font(10, QFont::Bold);
    p.setFont(tf);
    p.setPen(textColor);
    p.drawText(base, Qt::AlignCenter, text());
}

QPainterPath BaButton::currentPath() const
{
    // CodePen@mangopomelo 方案：skewX(-10deg) 左倾平行四边形（CSS 1:1 复刻）。
    // 用 BaStyle::skewRectPath 的 QTransform shear 实现：圆角平滑、无折角。
    // 底座先在矩形内缩 2.5px：让 box-shadow 投影完整落在 widget 边界内
    // （投影画到控件矩形外会被 QWidget 裁掉，造成「上下缺边」观感）。
    const qreal radius = qBound(4.0, height() * 0.08, 8.0);
    const QRectF base(2.5, 2.5, width() - 5.0, height() - 5.0);
    return BaStyle::skewRectPath(base, skewDeg_, radius);
}

void BaButton::enterEvent(QEnterEvent *event)
{
    QAbstractButton::enterEvent(event);
    hovered_ = true;
    update();
}

void BaButton::leaveEvent(QEvent *event)
{
    QAbstractButton::leaveEvent(event);
    hovered_ = false;
    update();
}

void BaButton::focusInEvent(QFocusEvent *event)
{
    QAbstractButton::focusInEvent(event);
    // 鼠标点击不给 focus 环（BA 原版无鼠标焦点描边），仅键盘 Tab 导航显示
    focused_ = event->reason() != Qt::MouseFocusReason;
    update();
}

void BaButton::focusOutEvent(QFocusEvent *event)
{
    QAbstractButton::focusOutEvent(event);
    focused_ = false;
    update();
}

void BaButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        scaleTo(0.95);   // :active { --scale: .95 }
    }
    QAbstractButton::mousePressEvent(event);
}

void BaButton::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton) {
        scaleTo(1.0);
    }
}

// 平滑缩放到目标值：按下走按压补间器，回弹走回弹补间器（均 ease-out）
void BaButton::scaleTo(qreal target)
{
    if (target < scale_) {
        scaleRelease_.stop();
        scalePress_.start(scale_, target);
    } else {
        scalePress_.stop();
        scaleRelease_.start(scale_, target);
    }
}
