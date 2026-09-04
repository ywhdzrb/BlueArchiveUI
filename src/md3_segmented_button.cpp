#include "md3_segmented_button.h"
#include "md3_icon.h"

#include <QPainter>
#include <QMouseEvent>
#include <QEnterEvent>
#include <optional>

Md3SegmentedButton::Md3SegmentedButton(QWidget *parent)
    : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    QFont f = font();
    f.setPointSizeF(14.0);
    setFont(f);
}

int Md3SegmentedButton::addSegment(const QString &text, const std::optional<md3::Glyph> &glyph)
{
    Segment seg;
    seg.text = text;
    seg.glyph = glyph;
    seg.width = fontMetrics().horizontalAdvance(text) + 2 * kPad + (glyph.has_value() ? 18.0 + kGap : 0.0);
    segments_.append(seg);

    // 为新增段配一个独立动画器（挂 this 为 QObject child，QPointer 托管），
    // 捕获固定索引，写入该段自己的 hoverAlpha
    const int idx = segments_.size() - 1;
    auto *anim = new QVariantAnimation(this);
    anim->setDuration(150);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &QVariantAnimation::valueChanged, this, [this, idx](const QVariant &v) {
        if (idx >= 0 && idx < segments_.size()) {
            segments_[idx].hoverAlpha = v.toReal();
        }
        update();
    });
    hoverAnims_.append(anim);

    updateGeometry();
    update();
    return segments_.size() - 1;
}

void Md3SegmentedButton::clearSegments()
{
    segments_.clear();
    hoverAnims_.clear();   // 动画仍是 this 的 QObject child，析构时统一回收
    currentIndex_ = -1;
    updateGeometry();
    update();
}

void Md3SegmentedButton::setCurrentIndex(int index)
{
    if (index == currentIndex_) {
        return;
    }
    currentIndex_ = index;
    emit currentIndexChanged(currentIndex_);
    update();
}

void Md3SegmentedButton::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

QSize Md3SegmentedButton::sizeHint() const
{
    qreal total = 0.0;
    for (const Segment &seg : segments_) {
        total += seg.width;
    }
    return QSize(static_cast<int>(total), static_cast<int>(kHeight));
}

QSize Md3SegmentedButton::minimumSizeHint() const
{
    return sizeHint();
}

// 绘制：外层轮廓 → 选中段底色 → 段间分隔线 → 图标 + 文本
void Md3SegmentedButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (segments_.isEmpty()) {
        return;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const QRectF r = rect().adjusted(1, 1, -1, -1);
    const qreal radius = kHeight / 2.0;

    // 一：选中段底色（secondaryContainer）
    if (currentIndex_ >= 0 && currentIndex_ < segments_.size()) {
        qreal left = 0.0;
        for (int i = 0; i < currentIndex_; ++i) {
            left += segments_[i].width;
        }
        const QRectF sel(left, r.top(), segments_[currentIndex_].width, r.height());
        p.setBrush(theme_.secondaryContainer);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(sel, radius, radius);
    }

    // 二：hover 状态层（blend 于选中底色之上，未选中用 on-surface 低透明度）
    if (hoverIndex_ >= 0 && hoverIndex_ < segments_.size()) {
        const qreal alpha = segments_[hoverIndex_].hoverAlpha;
        if (alpha > 0.0) {
            qreal left = 0.0;
            for (int i = 0; i < hoverIndex_; ++i) {
                left += segments_[i].width;
            }
            const QRectF hover(left, r.top(), segments_[hoverIndex_].width, r.height());
            const bool inSelected = (hoverIndex_ == currentIndex_);
            QColor layer = inSelected ? Md3Theme::blend(theme_.secondaryContainer,
                                                        theme_.onSecondaryContainer,
                                                        alpha / 100.0)
                                      : QColor(theme_.onSurface.red(), theme_.onSurface.green(),
                                               theme_.onSurface.blue(), static_cast<int>(alpha));
            p.setBrush(layer);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(hover, radius, radius);
        }
    }

    // 三：整体外轮廓描边
    {
        QPen pen(theme_.outline, kStroke);
        p.setBrush(Qt::NoBrush);
        p.setPen(pen);
        p.drawRoundedRect(r, radius, radius);
    }

    // 四：段间分隔线（1px vertical，outline-variant）
    {
        QPen pen(theme_.outlineVariant, kStroke);
        p.setPen(pen);
        qreal left = 0.0;
        for (int i = 0; i < segments_.size() - 1; ++i) {
            left += segments_[i].width;
            p.drawLine(QPointF(left, r.top() + 6.0), QPointF(left, r.bottom() - 6.0));
        }
    }

    // 五：图标 + 文本（选中段 on-secondary-container，否则 on-surface-variant）
    {
        qreal left = 0.0;
        for (int i = 0; i < segments_.size(); ++i) {
            const Segment &seg = segments_[i];
            const bool selected = (i == currentIndex_);
            const QColor content = selected ? theme_.onSecondaryContainer : theme_.onSurfaceVariant;

            qreal x = left;
            if (seg.glyph.has_value()) {
                md3::paintGlyph(p, *seg.glyph, x + kPad + 9.0, r.center().y(), content, 2.0);
                x += kPad + 18.0 + kGap;
            } else {
                x += kPad;
            }

            p.setPen(content);
            p.drawText(QRectF(x, r.top(), seg.width - (x - left) - kPad, r.height()),
                       Qt::AlignVCenter, seg.text);
            left += seg.width;
        }
    }
}

void Md3SegmentedButton::mousePressEvent(QMouseEvent *event)
{
    // 命中检测：按段宽切入对应索引
    qreal left = 0.0;
    int hit = -1;
    for (int i = 0; i < segments_.size(); ++i) {
        if (event->position().x() >= left && event->position().x() < left + segments_[i].width) {
            hit = i;
            break;
        }
        left += segments_[i].width;
    }
    if (hit < 0) {
        return;
    }
    if (hit == currentIndex_) {
        if (deselectable_) {
            setCurrentIndex(-1);
        }
        return;
    }
    setCurrentIndex(hit);
}

void Md3SegmentedButton::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    hovered_ = true;
    setMouseTracking(true);
    update();
}

void Md3SegmentedButton::mouseMoveEvent(QMouseEvent *event)
{
    // 按段宽定位 hover 段；换段时旧段状态层回落、新段抬升
    qreal left = 0.0;
    int hit = -1;
    for (int i = 0; i < segments_.size(); ++i) {
        if (event->position().x() >= left && event->position().x() < left + segments_[i].width) {
            hit = i;
            break;
        }
        left += segments_[i].width;
    }
    if (hit != hoverIndex_) {
        animateHover(hoverIndex_, 0.0);
        hoverIndex_ = hit;
        animateHover(hit, kHoverAlpha);
    }
}

void Md3SegmentedButton::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    hovered_ = false;
    animateHover(hoverIndex_, 0.0);
    hoverIndex_ = -1;
    update();
}

// hover 透出：鼠标移动跟踪（继承 QWidget，mouseMoveEvent 未激活需要 setMouseTracking）。
// 动画器按索引唯一对应段，换段时新旧段动画互不打断，旧段能平滑淡出。
void Md3SegmentedButton::animateHover(int index, qreal target)
{
    if (index < 0 || index >= segments_.size()) {
        return;
    }
    QVariantAnimation *anim = hoverAnims_.at(index);
    anim->stop();
    anim->setStartValue(segments_[index].hoverAlpha);
    anim->setEndValue(target);
    anim->start();
}
