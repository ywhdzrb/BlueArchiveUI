#include "ba_progress_bar.h"

#include <QPainter>
#include <QPaintEvent>
#include <QtMath>

#include "ba_style.h"

namespace {

constexpr qreal kSkewDeg = -16.0;  // 与 BaButton 同族：左倾平行四边形斜切角

// 进度条平行四边形路径：与按钮一致的小圆角 skew 斜切（BA 官方同款）
QPainterPath barPath(const QRectF &rect, qreal radius)
{
    return BaStyle::skewRectPath(rect, kSkewDeg, radius);
}

} // namespace

BaProgressBar::BaProgressBar(QWidget *parent)
    : QProgressBar(parent)
    , valueAnim_(180, ba::Animator::outCubic,
                 // 每帧把插值写入展示值并重绘
                 [this](qreal t) {
                     shownValue_ = t;
                     update();
                 })
{
    setRange(0, 100);
    setTextVisible(false);
    setFixedHeight(14);

    // 值变化时从当前展示值平滑滚到新值（BA 进度条的蠕动感）
    connect(this, &QProgressBar::valueChanged, this, [this](int v) {
        if (!animate_) {
            shownValue_ = v;
            update();
            return;
        }
        valueAnim_.start(shownValue_, v);
    });
}

void BaProgressBar::setAnimated(bool on)
{
    animate_ = on;
}

void BaProgressBar::setTrackColor(const QColor &c)
{
    trackColor_ = c;
    update();
}

QSize BaProgressBar::sizeHint() const
{
    return QSize(220, 14);
}

void BaProgressBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal radius = qBound(4.0, height() * 0.15, 6.0);
    const QRectF track(QPointF(0, 0), QSizeF(width(), height()));

    // 轨道底：浅蓝白平行四边形（Daily 登录样式；任务卡换深黑灰）
    const QPainterPath trackPath = barPath(track, radius);
    p.setPen(Qt::NoPen);
    p.setBrush(trackColor_);
    p.fillPath(trackPath, trackColor_);

    // 填充：官方亮青（与 Sky 按钮同系 rgb(0,200,242)），垂直三段上亮下深。
    // 填充自身也是平行四边形（两端斜边与轨道平行）——绘制在剪切坐标系中：
    // clip 到轨道路径，再按同样的剪切变换画普通矩形，输出即斜边填充（CSS 同款）
    const qreal fillW = track.width() * qBound(qreal(0), shownValue_ / (maximum() - minimum()), qreal(1));
    if (fillW > 2) {
        p.save();
        p.setClipPath(trackPath);
        const qreal yc = height() / 2.0;
        const qreal s = qTan(qDegreesToRadians(kSkewDeg));
        p.translate(0, yc);
        p.shear(s, 0);
        p.translate(0, -yc);

        // 纯色填充（官方 Sky 亮青），四周与轨道留隙（左右 5px、上下 2px），圆角小一档
        const qreal insetX = 5.0;
        const qreal insetY = 2.0;
        const qreal fillRadius = qMin(3.0, radius * 0.55);
        const QRectF fillRect(insetX, insetY,
                              qMax(fillW - insetX - insetX, insetX),
                              height() - insetY * 2);
        p.setBrush(QColor("#00C8F2"));
        p.drawRoundedRect(fillRect, fillRadius, fillRadius);
        p.restore();
    }
}
