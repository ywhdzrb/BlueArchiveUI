#pragma once
// 极简帧驱动补间器：QTimer 驱动插值回调，不依赖 QVariantAnimation。
// 背景：部分 Linux 环境（Qt6.8+/特定补丁）下 QVariantAnimation::start()
// 后从不产生 tick，导致所有补间动画静默失效；改用自建 QTimer 帧驱动可
// 在任何具备事件循环的平台上稳定运行，代价是每个动画一个 16ms 定时器。
// 用法：ba::Animator anim(180, ba::Animator::outCubic, [](qreal t){...}, []{...});
//       anim.start(0.0, 33.0);

#include <QTimer>
#include <QObject>
#include <functional>
#include <utility>
#include <cmath>

namespace ba {

class Animator
{
public:
    using Easing = std::function<qreal(qreal)>;
    using Step = std::function<void(qreal)>;
    using Done = std::function<void()>;

    // 构建补间器；durationMs 时长，easing 插值曲线，step 每帧回调终值进度，done 完成回调
    Animator(int durationMs, Easing easing, Step step, Done done = {})
        : duration_(durationMs)
        , easing_(std::move(easing))
        , step_(std::move(step))
        , done_(std::move(done))
    {
        timer_.setInterval(16);
        timer_.setTimerType(Qt::PreciseTimer);
        QObject::connect(&timer_, &QTimer::timeout, [this] {
            t_ += 16.0 / std::max(1, duration_);
            if (t_ <= 1.0) {
                step_(from_ + (to_ - from_) * easing_(t_));
                return;
            }
            timer_.stop();
            step_(to_);
            if (done_)
                done_();
        });
    }

    // 从 from 平滑补间到 to（调用即覆盖当前动画）
    void start(qreal from, qreal to)
    {
        from_ = from;
        to_ = to;
        t_ = 0.0;
        timer_.start();
    }

    void stop()
    {
        timer_.stop();
    }

    // 常用缓动：三次出（快速减速到 0 速）
    static qreal outCubic(qreal t)
    {
        const qreal u = 1.0 - t;
        return 1.0 - u * u * u;
    }

    // 常用缓动：回弹出（带过冲），仿 Qt OutBack
    static qreal outBack(qreal t)
    {
        const qreal c = 1.70158;
        const qreal u = t - 1.0;
        return 1.0 + (c + 1.0) * u * u * u + c * u * u;
    }

private:
    QTimer timer_;
    int duration_ = 0;
    Easing easing_;
    Step step_;
    Done done_;
    qreal from_ = 0.0;
    qreal to_ = 0.0;
    qreal t_ = 0.0;
};

} // namespace ba
