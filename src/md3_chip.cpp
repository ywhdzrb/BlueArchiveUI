#include "md3_chip.h"
#include "md3_icon.h"

#include <QPainter>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QHBoxLayout>

namespace {
// 删除符号宽度占位（内边距的一部分）
constexpr qreal kDeletePad = 14.0;
}

Md3Chip::Md3Chip(const QString &text, QWidget *parent)
    : QAbstractButton(parent)
{
    setText(text);
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QFont f = font();
    f.setPointSizeF(14.0);
    setFont(f);

    stateAnim_.setDuration(150);
    stateAnim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&stateAnim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        stateAlpha_ = v.toReal();
        update();
    });
}

void Md3Chip::setStyle(Style style)
{
    if (style_ == style) {
        return;
    }
    style_ = style;
    // Assist 与 Input 不可勾选
    setCheckable(style_ == Style::Filter);
    updateGeometry();
    update();
}

void Md3Chip::setIcon(md3::Glyph glyph)
{
    glyph_ = glyph;
    iconVisible_ = true;
    updateGeometry();
    update();
}

void Md3Chip::setIconVisible(bool visible)
{
    iconVisible_ = visible;
    updateGeometry();
    update();
}

void Md3Chip::setShowDelete(bool showDelete)
{
    showDelete_ = showDelete;
    updateGeometry();
    update();
}

void Md3Chip::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

void Md3Chip::setChecked(bool checked)
{
    QAbstractButton::setChecked(checked);
    update();
}

QSize Md3Chip::sizeHint() const
{
    qreal w = kTextGap;
    if (iconVisible_) {
        w += 18.0 + kIconGap;   // 18px 图标 + 间隙
    }
    w += fontMetrics().horizontalAdvance(text());
    if (style_ == Style::Filter && isCheckable()) {
        // 选中预留对勾位：绘制时对勾后文本从 kTextGap+kIconGap+18+kIconGap 起
        w += 2 * kIconGap + 18.0;
    }
    if (showDelete_) {
        w += kDeletePad;
    }
    return QSize(static_cast<int>(w + kTextGap), static_cast<int>(kHeight));
}

QSize Md3Chip::minimumSizeHint() const
{
    return sizeHint();
}

// 绘制：底色（Filter 选中 secondary-container）→ 状态层 → 边框 → 图标 / 对勾 → 文本
void Md3Chip::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const bool disabled = !isEnabled();
    const bool checked = isChecked();
    const QRectF r = rect().adjusted(1, 1, -1, -1);
    const qreal radius = kHeight / 2.0;

    // 一：底色。Filter 选中态才填 secondary-container
    const bool filled = (style_ == Style::Filter && checked);
    if (filled) {
        p.setBrush(disabled ? theme_.disabledContainer() : theme_.secondaryContainer);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, radius, radius);
    }

    // 二：状态层覆盖（hover / 按下）
    if (!disabled && stateAlpha_ > 0.0) {
        const QColor base = filled ? theme_.secondaryContainer : QColor(Qt::transparent);
        const QColor layerColor = filled ? Md3Theme::blend(theme_.secondaryContainer,
                                                           theme_.onSecondaryContainer,
                                                           stateAlpha_ / 100.0)
                                         : QColor(theme_.onSurface.red(), theme_.onSurface.green(),
                                                  theme_.onSurface.blue(), stateAlpha_);
        p.setBrush(layerColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, radius, radius);
    }

    // 三：1px 边框。Filter 选中态 primary 加粗，其余 outline
    if (!filled) {
        QPen pen(disabled ? theme_.outlineVariant : theme_.outline, kStroke);
        p.setBrush(Qt::NoBrush);
        p.setPen(pen);
        p.drawRoundedRect(r, radius, radius);
    } else {
        QPen pen(disabled ? theme_.outlineVariant : theme_.primary, 2.0);
        p.setBrush(Qt::NoBrush);
        p.setPen(pen);
        p.drawRoundedRect(r, radius, radius);
    }

    // 四：图标区域（左起 kTextGap 处绘制 18px 图标）
    const qreal cy = r.center().y();
    qreal cursorX = kTextGap;

    // Filter 选中时最左侧换显示对勾
    if (style_ == Style::Filter && checked) {
        const QColor checkColor = disabled ? theme_.disabledContent() : theme_.onSecondaryContainer;
        md3::paintGlyph(p, md3::Glyph::Check, cursorX + 9.0, cy, checkColor, 2.0);
        cursorX = kTextGap + kIconGap + 18.0 + kIconGap;
    }
    // 其他变体：前置 icon
    else if (iconVisible_) {
        const QColor iconColor = disabled ? theme_.disabledContent() : theme_.primary;
        md3::paintGlyph(p, glyph_, cursorX + 9.0, cy, iconColor, 2.0);
        cursorX += 18.0 + kIconGap;
    }

    // 五：文本
    QColor textColor;
    if (style_ == Style::Filter && checked) {
        textColor = disabled ? theme_.disabledContent() : theme_.onSecondaryContainer;
    } else {
        textColor = disabled ? theme_.disabledContent() : theme_.onSurfaceVariant;
    }
    p.setPen(textColor);
    p.drawText(QRectF(cursorX, r.top(), r.width() - cursorX - kTextGap, r.height()),
               Qt::AlignLeft | Qt::AlignVCenter, text());

    // 六：Input 芯片的删除符号（最右侧 X）
    if (showDelete_) {
        md3::paintGlyph(p, md3::Glyph::Close, r.right() - kTextGap - 2.0, cy, textColor, 2.0);
    }
}

void Md3Chip::mousePressEvent(QMouseEvent *event)
{
    // Input 芯片：点击删除符号触发 deleteClicked 而非 toggle。
    // 命中时直接 return，避免基类再进入 down 状态导致松开后误发 clicked
    if (showDelete_ && style_ == Style::Input) {
        // 绘制符号中心在 adjusted(1,1,-1,-1) 后的圆角矩形右缘，此处需再偏 1px 对齐
        const qreal deleteCenter = rect().right() - kTextGap - 3.0;
        const qreal cy = height() / 2.0;
        if (qAbs(event->position().x() - deleteCenter) < 16.0
            && qAbs(event->position().y() - cy) < 16.0) {
            emit deleteClicked();
            return;
        }
    }
    QAbstractButton::mousePressEvent(event);
    if (isDown()) {
        stateAnim_.stop();
        stateAnim_.setStartValue(stateAlpha_);
        stateAnim_.setEndValue(targetStateAlpha());
        stateAnim_.start();
    }
}

// 松开后回落到 hover 档
void Md3Chip::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}

void Md3Chip::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    hovered_ = true;
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}

void Md3Chip::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hovered_ = false;
    stateAnim_.stop();
    stateAnim_.setStartValue(stateAlpha_);
    stateAnim_.setEndValue(targetStateAlpha());
    stateAnim_.start();
}
