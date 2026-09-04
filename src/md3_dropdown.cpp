#include "md3_dropdown.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QHideEvent>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QScreen>

namespace {
constexpr int kHeight = 56;            // 与文本输入框等高
constexpr qreal kRadiusTop = 4.0;      // 顶部圆角，底部直角留给指示线
constexpr int kMenuRadius = 4;         // 菜单容器圆角
constexpr int kMenuItemHeight = 48;    // 单个菜单项高度
constexpr int kMenuPadding = 8;        // 菜单上下内边距
constexpr int kTextLeftPad = 16;       // 文字左内边距
constexpr int kArrowRightPad = 16;     // 箭头中心距右缘
}

// ---- Md3MenuPopup 弹出菜单窗口 ----

Md3MenuPopup::Md3MenuPopup(Md3Dropdown *owner)
    : QWidget(nullptr, Qt::Popup)
    , owner_(owner)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);

    // 层级阴影（blur 16 / 垂直偏移 2px，近似 MD3 Elevation Level 2）
    auto *effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(16.0);
    effect->setOffset(0, 2);
    effect->setColor(QColor(0, 0, 0, 90));
    setGraphicsEffect(effect);
}

void Md3MenuPopup::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

void Md3MenuPopup::setItems(const QStringList &items)
{
    items_ = items;
    update();
}

void Md3MenuPopup::setSelectedIndex(int index)
{
    selectedIndex_ = index;
    update();
}

// 宽度对齐宿主控件（至少 200px），高度由选项数量决定
QSize Md3MenuPopup::popupSize() const
{
    const int w = qMax(200, owner_ ? owner_->width() : 200);
    const int h = kMenuPadding * 2 + items_.size() * kMenuItemHeight;
    return QSize(w, h);
}

// 由像素坐标反算选项索引：基于菜单内边距与选项高度的除法
int Md3MenuPopup::itemAt(const QPoint &pos) const
{
    if (pos.x() < 0 || pos.x() >= width()) {
        return -1;
    }
    const int y = pos.y() - kMenuPadding;
    if (y < 0) {
        return -1;
    }
    const int idx = y / kMenuItemHeight;
    return idx < items_.size() ? idx : -1;
}

void Md3MenuPopup::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 菜单容器：surface 底色 + 圆角
    const QRectF r = rect().adjusted(1, 1, -1, -1);
    p.setBrush(theme_.surface);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(r, kMenuRadius, kMenuRadius);

    for (int i = 0; i < items_.size(); ++i) {
        const QRectF itemRect(0, kMenuPadding + i * kMenuItemHeight, width(), kMenuItemHeight);

        // hover 状态层：on-surface 8% 叠加
        if (i == hoverIndex_) {
            QColor layer = Md3Theme::blend(theme_.surface, theme_.onSurface, 0.08);
            p.setBrush(layer);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(itemRect.adjusted(1, 1, -1, -1), kMenuRadius, kMenuRadius);
        }

        // 选项文字：选中项 primary 加粗，其余 on-surface
        const bool selected = (i == selectedIndex_);
        QFont f = p.font();
        f.setPointSizeF(16.0);
        f.setWeight(selected ? QFont::Medium : QFont::Normal);
        p.setFont(f);
        p.setPen(selected ? theme_.primary : theme_.onSurface);
        const qreal textX = kMenuPadding + 16;
        const QString elided = QFontMetrics(f).elidedText(items_.at(i), Qt::ElideRight,
                                                          int(width() - textX - kMenuPadding));
        p.drawText(QRectF(textX, itemRect.y(), width() - textX - kMenuPadding, kMenuItemHeight),
                   Qt::AlignVCenter | Qt::AlignLeft, elided);

        // 选中标记：对勾折线，位于文字左侧图标区
        if (selected) {
            const qreal cx = kMenuPadding + 8;
            const qreal cy = itemRect.center().y();
            QPen checkPen(theme_.primary, 2.0);
            checkPen.setCapStyle(Qt::RoundCap);
            checkPen.setJoinStyle(Qt::RoundJoin);
            p.setPen(checkPen);
            QPainterPath check;
            check.moveTo(cx - 3, cy);
            check.lineTo(cx, cy + 3);
            check.lineTo(cx + 4, cy - 3);
            p.drawPath(check);
        }
    }
}

// 鼠标移动时跟踪悬停项，驱动 hover 高亮
void Md3MenuPopup::mouseMoveEvent(QMouseEvent *event)
{
    const int idx = itemAt(event->pos());
    if (idx != hoverIndex_) {
        hoverIndex_ = idx;
        update();
    }
}

void Md3MenuPopup::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
}

// 松开鼠标时选中当前悬停项并通知宿主
void Md3MenuPopup::mouseReleaseEvent(QMouseEvent *event)
{
    const int idx = itemAt(event->pos());
    if (idx >= 0) {
        emit itemClicked(idx);
    }
}

void Md3MenuPopup::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hoverIndex_ = -1;
    update();
}

// 菜单隐藏（手动收起或点击外部）时通知宿主复位状态
void Md3MenuPopup::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    emit closed();
}

// ---- Md3Dropdown 填充式下拉选择框 ----

Md3Dropdown::Md3Dropdown(const QStringList &items, const QString &placeholder, QWidget *parent)
    : QWidget(parent)
    , items_(items)
    , placeholder_(placeholder)
{
    setFixedHeight(kHeight);
    setCursor(Qt::PointingHandCursor);

    // MD3 Body Large 排版：16px / Regular 字重
    QFont f = font();
    f.setPointSizeF(16.0);
    setFont(f);

    // 指示线 / 箭头 / 状态层过渡动画，150ms 符合 MD3 短动效时长
    anim_.setDuration(150);
    anim_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        activeProgress_ = v.toReal();
        update();
    });

    popup_ = new Md3MenuPopup(this);
    connect(popup_, &Md3MenuPopup::itemClicked, this, &Md3Dropdown::onItemClicked);
    connect(popup_, &Md3MenuPopup::closed, this, &Md3Dropdown::onPopupClosed);
}

// 弹出菜单是 Qt::Popup 顶层窗口（构造时忽略父指针），
// 必须随宿主显式释放，否则每次构造泄漏一个窗口且宿主销毁后残留屏幕
Md3Dropdown::~Md3Dropdown()
{
    delete popup_;
}

// 切换主题并同步弹出菜单
void Md3Dropdown::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    if (popup_) {
        popup_->setTheme(theme);
    }
    update();
}

void Md3Dropdown::setItems(const QStringList &items)
{
    items_ = items;
    if (currentIndex_ >= items_.size()) {
        currentIndex_ = -1;
        emit currentIndexChanged(currentIndex_);
    }
    update();
}

void Md3Dropdown::setPlaceholderText(const QString &placeholder)
{
    placeholder_ = placeholder;
    update();
}

void Md3Dropdown::setCurrentIndex(int index)
{
    if (index == currentIndex_) {
        return;
    }
    currentIndex_ = index;
    update();
    emit currentIndexChanged(index);
}

QString Md3Dropdown::currentText() const
{
    if (currentIndex_ >= 0 && currentIndex_ < items_.size()) {
        return items_.at(currentIndex_);
    }
    return QString();
}

QSize Md3Dropdown::sizeHint() const
{
    return QSize(200, kHeight);
}

// 绘制：填充背景 → 状态层 → 文本 → 箭头 → 底部指示线
void Md3Dropdown::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 填充背景，仅顶部圆角。
    // Qt 的 arcTo 角度约定：0°=右, 90°=上, 180°=左, 270°=下；sweep 正值 = 角度值增大。
    QPainterPath path;
    path.moveTo(0, kRadiusTop);
    path.arcTo(QRectF(0, 0, 2 * kRadiusTop, 2 * kRadiusTop), 180, -90);
    path.lineTo(width() - 2 * kRadiusTop, 0);
    path.arcTo(QRectF(width() - 2 * kRadiusTop, 0, 2 * kRadiusTop, 2 * kRadiusTop), 90, -90);
    path.lineTo(width(), height());
    path.lineTo(0, height());
    path.closeSubpath();
    p.fillPath(path, theme_.surfaceContainerHighest);

    // hover / 展开状态层：on-surface 10%，透明度随过渡进度
    if (activeProgress_ > 0.0) {
        QColor layer = Md3Theme::blend(theme_.surfaceContainerHighest, theme_.onSurface,
                                       0.10 * activeProgress_);
        p.fillPath(path, layer);
    }

    // 文本：已选中显示选项内容，否则显示占位提示
    const bool hasValue = currentIndex_ >= 0 && currentIndex_ < items_.size();
    p.setFont(font());
    p.setPen(hasValue ? theme_.onSurface : theme_.onSurfaceVariant);
    const QString text = hasValue ? items_.at(currentIndex_) : placeholder_;
    const qreal textRight = width() - 2 * kArrowRightPad - 8;
    const QString elided = QFontMetrics(font()).elidedText(text, Qt::ElideRight,
                                                           qMax(0, int(textRight - kTextLeftPad)));
    p.drawText(QRectF(kTextLeftPad, 0, textRight - kTextLeftPad, height()),
               Qt::AlignVCenter | Qt::AlignLeft, elided);

    // 下拉箭头：V 形折线，激活时过渡为 primary
    if (!items_.isEmpty()) {
        const QColor arrowColor = Md3Theme::blend(theme_.onSurfaceVariant, theme_.primary, activeProgress_);
        QPen pen(arrowColor, 2.0);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        const qreal cx = width() - kArrowRightPad - 5;
        const qreal cy = height() / 2.0;
        QPainterPath arrow;
        arrow.moveTo(cx - 4, cy - 2);
        arrow.lineTo(cx, cy + 2);
        arrow.lineTo(cx + 4, cy - 2);
        p.drawPath(arrow);
    }

    // 底部指示线：未激活 1px outline-variant，激活 2px primary，颜色渐变过渡
    const QColor lineColor = Md3Theme::blend(theme_.outlineVariant, theme_.primary, activeProgress_);
    p.setPen(QPen(lineColor, 1.0 + activeProgress_));
    p.drawLine(QPointF(0, height() - 0.5), QPointF(width(), height() - 0.5));
}

// 点击：菜单未开则弹出，已开则收起
void Md3Dropdown::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    if (items_.isEmpty()) {
        return;
    }
    if (menuOpen_) {
        popup_->hide();
        return;
    }
    showPopup();
}

void Md3Dropdown::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    hovered_ = true;
    animateActive(hovered_ || menuOpen_);
}

void Md3Dropdown::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hovered_ = false;
    animateActive(hovered_ || menuOpen_);
}

// 驱动指示线 / 箭头 / 状态层的过渡动画
void Md3Dropdown::animateActive(bool active)
{
    anim_.stop();
    anim_.setStartValue(activeProgress_);
    anim_.setEndValue(active ? 1.0 : 0.0);
    anim_.start();
}

// 在控件正下方弹出菜单，空间不足时向上弹出
void Md3Dropdown::showPopup()
{
    popup_->setTheme(theme_);
    popup_->setItems(items_);
    popup_->setSelectedIndex(currentIndex_);
    const QSize sz = popup_->popupSize();

    const QPoint anchor = mapToGlobal(QPoint(0, height()));
    QPoint pos = anchor;
    if (const QScreen *screen = QGuiApplication::screenAt(anchor)) {
        if (anchor.y() + sz.height() > screen->availableGeometry().bottom()) {
            pos = mapToGlobal(QPoint(0, -sz.height()));
        }
    }

    popup_->resize(sz);
    popup_->move(pos);
    menuOpen_ = true;
    animateActive(true);
    popup_->show();
    popup_->raise();
    popup_->setFocus();
}

// 用户选中某项：更新索引并收起菜单
void Md3Dropdown::onItemClicked(int index)
{
    setCurrentIndex(index);
    popup_->hide();
}

// 菜单收起（含点击外部自动关闭）时复位展开状态
void Md3Dropdown::onPopupClosed()
{
    menuOpen_ = false;
    animateActive(hovered_ || menuOpen_);
}
