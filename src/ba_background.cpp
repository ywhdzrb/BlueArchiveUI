#include "ba_background.h"

#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QtMath>

#include "ba_assets.h"

BaBackground::BaBackground(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(false);
}

void BaBackground::setImagePath(const QString &relPath)
{
    imagePath_ = relPath;
    image_ = QPixmap();   // 重置缓存，下次 paint 再按需加载
    update();
}

void BaBackground::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 素材图优先：等比放大裁切铺满（BA 游戏背景适配标准做法）
    if (!imagePath_.isEmpty()) {
        if (image_.isNull())
            image_ = BaAssets::image(imagePath_);
        if (!image_.isNull()) {
            const QSizeF src(image_.width(), image_.height());
            const qreal scale = qMax(qreal(width()) / src.width(),
                                     qreal(height()) / src.height());
            const QSizeF dst(src.width() * scale, src.height() * scale);
            const QRectF target((width() - dst.width()) / 2.0,
                                (height() - dst.height()) / 2.0,
                                dst.width(), dst.height());
            // 平滑放大输出整幅
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            p.drawPixmap(target, image_, QRectF(0, 0, src.width(), src.height()));
            return;
        }
    }

    // 手绘天空三档渐变：湛蓝 → 浅青 → 乳白（BA 登录大厅的天色）
    QLinearGradient sky(QPointF(0, 0), QPointF(0, height()));
    sky.setColorAt(0.0, QColor("#6EBFF2"));
    sky.setColorAt(0.55, QColor("#BFE9FA"));
    sky.setColorAt(0.82, QColor("#E8F7F7"));
    sky.setColorAt(1.0, QColor("#FFFFFF"));
    p.fillRect(rect(), sky);

    // 云层：三档大小，白 alpha 递减，位置相对窗口尺寸
    drawCloud(p, QPointF(width() * 0.22, height() * 0.24), 0.9, 170);
    drawCloud(p, QPointF(width() * 0.72, height() * 0.16), 1.15, 150);
    drawCloud(p, QPointF(width() * 0.50, height() * 0.40), 0.7, 120);
    drawCloud(p, QPointF(width() * 0.88, height() * 0.46), 0.85, 110);

    // 底部远山剪影：两条青蓝色带弧丘
    const QColor hillA(0x8F, 0xD4, 0xF0, 90);
    const QColor hillB(0x6F, 0xB7, 0xE8, 110);
    auto hills = [&](qreal base, qreal amp, const QColor &c) {
        QPainterPath path;
        path.moveTo(0, height());
        path.lineTo(0, base);
        // 两座对称山丘：正弦包络
        const int n = 16;
        for (int i = 0; i <= n; ++i) {
            const qreal x = width() * i / n;
            const qreal y = base - amp * qSin(M_PI * i / n);
            path.lineTo(x, y);
        }
        path.lineTo(width(), height());
        path.closeSubpath();
        p.fillPath(path, c);
    };
    hills(height() * 0.94, height() * 0.085, hillA);
    hills(height() * 0.90, height() * 0.055, hillB);
}

void BaBackground::drawCloud(QPainter &p, const QPointF &center, qreal scale, qreal alpha)
{
    // 云：一组相交圆的白团（简化三个圆 + 底弧）
    const qreal r = 46 * scale;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, alpha));
    p.drawEllipse(center + QPointF(-r * 1.1, r * 0.35), r * 0.75, r * 0.45);
    p.drawEllipse(center + QPointF(0, r * 0.15), r, r * 0.62);
    p.drawEllipse(center + QPointF(r * 1.1, r * 0.35), r * 0.75, r * 0.45);
    // 补底成整团
    p.drawRect(QRectF(center.x() - r * 1.85, center.y() + r * 0.30, r * 3.7, r * 0.5));
}
