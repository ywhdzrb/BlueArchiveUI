#include "ba_check_box.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include "ba_style.h"

BaCheckBox::BaCheckBox(const QString &text, QWidget *parent)
    : QAbstractButton(parent)
    , checkAnim_(150, ba::Animator::outCubic,
                 // 每帧更新勾生长进度并重绘
                 [this](qreal t) {
                     checkAni_ = t;
                     update();
                 })
{
    setText(text);
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFixedHeight(24);

    // 勾生长动画：点击与程序化 setChecked 都走 toggled → 帧驱动补间
    connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
        setCheckedAnimated(checked);
    });
}

void BaCheckBox::setCheckedAnimated(bool checked)
{
    checkAnim_.start(checkAni_, checked ? 1.0 : 0.0);
}

QSize BaCheckBox::sizeHint() const
{
    const QFontMetrics fm(font());
    return QSize(26 + fm.horizontalAdvance(text()) + 6, 24);
}

void BaCheckBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 静音框为横长圆角矩形（26x20 圆角 5），白底 + 浅蓝描边
    const QRectF box(2, (height() - 20) / 2.0, 26, 20);

    // 底：白填充，选中后转入浅青蓝
    QColor fill = isChecked() ? QColor(0x8E, 0xD8, 0xFA, 120) : Qt::white;
    if (hovered_ && !isChecked())
        fill = QColor(0x4E, 0xC3, 0xF5, 24);
    p.setBrush(fill);

    // 描边：未选浅蓝灰，选中青蓝加粗
    const QColor stroke = isChecked() ? BaStyle::accent() : QColor("#9FC5DD");
    p.setPen(QPen(stroke, isChecked() ? 2.2 : 1.5));
    p.drawRoundedRect(box, 5, 5);

    // 选中中心青蓝椭圆块（随 checkAni_ 淡入）
    if (checkAni_ > 0.01) {
        QColor inner = BaStyle::accent();
        inner.setAlphaF(0.95 * checkAni_);
        p.setPen(Qt::NoPen);
        p.setBrush(inner);
        p.drawEllipse(box.center(), 5.5, 5.5);
    }

    // 文字：BA 正文深藏蓝
    p.setPen(isEnabled() ? BaStyle::deep() : BaStyle::muted());
    p.setFont(BaStyle::font(10));
    p.drawText(QRectF(box.right() + 8, 0, width() - box.right() - 8, height()),
               Qt::AlignVCenter | Qt::AlignLeft, text());
}

void BaCheckBox::enterEvent(QEnterEvent *event)
{
    QAbstractButton::enterEvent(event);
    hovered_ = true;
    update();
}

void BaCheckBox::leaveEvent(QEvent *event)
{
    QAbstractButton::leaveEvent(event);
    hovered_ = false;
    update();
}

void BaCheckBox::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    if (isChecked())
        setCheckedAnimated(true);
    else
        setCheckedAnimated(false);
}
