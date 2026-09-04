#include "md3_progress_bar.h"
#include "md3_theme.h"

#include <QPainter>

namespace {
constexpr qreal kBarHeight = 4.0;       // MD3 线性进度条高度
constexpr qreal kBarRadius = 2.0;       // 半高胶囊圆角
constexpr qreal kIndicatorWidth = 0.25; // 不确定模式指示条相对宽度
}

Md3ProgressBar::Md3ProgressBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(kBarHeight);

    // 不确定模式滑动动画：1.6s 线性循环
    anim_.setDuration(1600);
    anim_.setStartValue(0.0);
    anim_.setEndValue(1.0);
    anim_.setLoopCount(-1);
    anim_.setEasingCurve(QEasingCurve::Linear);
    connect(&anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        slide_ = v.toReal();
        update();
    });
}

// 切换主题并重绘
void Md3ProgressBar::setTheme(const Md3Theme &theme)
{
    theme_ = theme;
    update();
}

// 设置确定模式取值范围，越界值就近截断
void Md3ProgressBar::setRange(int min, int max)
{
    // 与滑块版一致：入参 min > max 时交换，保证后序 qBound 与比例计算正确
    if (min > max) {
        qSwap(min, max);
    }
    min_ = min;
    max_ = max;
    value_ = qBound(min_, value_, max_);
    update();
}

void Md3ProgressBar::setValue(int value)
{
    value_ = qBound(min_, value, max_);
    update();
}

int Md3ProgressBar::value() const
{
    return value_;
}

// 切换不确定模式，动画随模式启停
void Md3ProgressBar::setIndeterminate(bool indeterminate)
{
    if (indeterminate_ == indeterminate) {
        return;
    }
    indeterminate_ = indeterminate;
    if (indeterminate_) {
        anim_.start();
    } else {
        anim_.stop();
        slide_ = 0.0;
    }
    update();
}

bool Md3ProgressBar::isIndeterminate() const
{
    return indeterminate_;
}

QSize Md3ProgressBar::sizeHint() const
{
    return QSize(200, qRound(kBarHeight));
}

// 绘制 track 与指示条：track 用 surface-container-highest，
// 指示条用 primary，两端半高胶囊圆角
void Md3ProgressBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF bar(0, 0, width(), kBarHeight);

    p.setPen(Qt::NoPen);
    p.setBrush(theme_.surfaceContainerHighest);
    p.drawRoundedRect(bar, kBarRadius, kBarRadius);

    if (indeterminate_) {
        // 不确定模式：固定 1/4 宽的指示条从左侧滑出、右侧滑入，循环往复
        const qreal indW = width() * kIndicatorWidth;
        const qreal x = -indW + slide_ * (width() + indW);
        p.setBrush(theme_.primary);
        p.drawRoundedRect(QRectF(x, 0, indW, kBarHeight), kBarRadius, kBarRadius);
    } else {
        // 确定模式：按 value 占比从左侧填充
        const qreal ratio = max_ > min_ ? qreal(value_ - min_) / (max_ - min_) : 0.0;
        if (ratio > 0.0) {
            p.setBrush(theme_.primary);
            p.drawRoundedRect(QRectF(0, 0, width() * ratio, kBarHeight), kBarRadius, kBarRadius);
        }
    }
}
