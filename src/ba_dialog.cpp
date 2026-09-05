#include "ba_dialog.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QShowEvent>

#include "ba_icon.h"
#include "ba_style.h"

BaDialog::BaDialog(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    title_ = title;
    // 作为宿主的子 widget 显示：覆盖父容器整个区域，无需顶层窗口标志，
    // 语义等同游戏内弹窗（遮罩+面板都在宿主画布内）。
    setAttribute(Qt::WA_DeleteOnClose, false);

    panel_ = new QWidget(this);
    panel_->setObjectName(QStringLiteral("baDialogPanel"));
    contentLayout_ = new QVBoxLayout(panel_);
    constexpr int m = 24;
    contentLayout_->setContentsMargins(m, 56, m, 66);
    contentLayout_->setSpacing(14);
}

void BaDialog::setTitle(const QString &title)
{
    title_ = title;
    update();
}

void BaDialog::setPanelWidth(int w)
{
    panelWidth_ = w;
    resize(size());
}

QRectF BaDialog::panelRect() const
{
    // 面板贴底、左右居中，宽度固定
    const qreal left = (width() - panelWidth_) / 2.0;
    return QRectF(left, 0, panelWidth_, height());
}

QRectF BaDialog::closeButtonRect() const
{
    const QRectF pr = panelRect();
    return QRectF(pr.right() - 44, 14, 30, 30);
}

void BaDialog::showAsModal()
{
    QWidget *host = parentWidget();
    if (host) {
        move(0, 0);
        resize(host->size());
        show();
        // 白色面板单独从底部滑入，遮罩不动（BA 弹窗入场）
        BaStyle::slideInFromBottom(panel_, 400);
    } else {
        show();
    }
}

void BaDialog::closeDialog()
{
    hide();
    emit closed();
}

void BaDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 遮罩（BA 弹窗使用更深的海蓝遮罩）
    p.fillRect(rect(), QColor(0, 18, 36, 148));

    // 白色渐变面板（白→微蓝）+ 圆角 10 + 浅蓝描边 + 底部梯形收窄
    const QRectF pr = panelRect();
    QPainterPath panelPath;
    panelPath.addRoundedRect(pr, 10, 10);
    p.fillPath(panelPath, BaStyle::panelGradient(pr));

    const qreal cut = 22;
    QPainterPath footer;
    footer.moveTo(pr.left() + 2, pr.bottom() - 34);
    footer.lineTo(pr.left() + cut, pr.bottom() - 4);
    footer.lineTo(pr.right() - cut, pr.bottom() - 4);
    footer.lineTo(pr.right() - 2, pr.bottom() - 34);
    footer.closeSubpath();
    p.save();
    p.setClipPath(panelPath);
    // 底部渐深的灰蓝收窄条（BA 对话框底部的梯形手柄）
    QLinearGradient g(QPointF(0, pr.bottom() - 40), QPointF(0, pr.bottom()));
    g.setColorAt(0, QColor(0xF0, 0xF5, 0xF9, 0));
    g.setColorAt(1, QColor(0xDA, 0xE3, 0xEC, 210));
    p.fillRect(QRectF(pr.left(), pr.bottom() - 40, pr.width(), 40), g);
    p.restore();

    // 顶部渐隐装饰线（左上短白亮线，BA 玻璃质感点缀）
    QLinearGradient topGlow(pr.left() + 28, 0, pr.left() + 180, 0);
    topGlow.setColorAt(0, QColor(255, 255, 255, 0));
    topGlow.setColorAt(0.25f, QColor(255, 255, 255, 0));
    topGlow.setColorAt(1, QColor(255, 255, 255, 0));
    p.fillRect(QRectF(pr.left() + 28, pr.top() + 4, 120, 2), QColor(255, 255, 255, 30));

    // 面板描边：细浅蓝一圈
    p.setPen(QPen(QColor(0x9C, 0xD9, 0xF2, 170), 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(panelPath);

    // 标题：深蓝粗体完全居中 + 黄色下划线贴文字底（宽度=文字宽）
    p.setFont(BaStyle::font(13, QFont::Bold));
    p.setPen(BaStyle::deep());
    const int titleW = p.fontMetrics().horizontalAdvance(title_);
    const QRectF titleRect(pr.left() + 26, 16, pr.width() - 52, 26);
    p.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter, title_);
    const qreal underlineY = 44;
    p.setBrush(BaStyle::yellow());
    p.drawRect(QRectF(titleRect.center().x() - titleW / 2.0, underlineY, titleW, 3.5));

    // 右上关闭：白色小方片 + 深蓝 X（BA 弹窗的细腻关闭示意）
    const QRectF cb = closeButtonRect();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#F2F8FD"));
    p.drawRoundedRect(cb.adjusted(2, 2, -2, -2), 5, 5);
    p.setPen(QPen(BaStyle::deep(), 2.2, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(cb.center() + QPointF(-5, -5), cb.center() + QPointF(5, 5));
    p.drawLine(cb.center() + QPointF(5, -5), cb.center() + QPointF(-5, 5));
}

void BaDialog::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

void BaDialog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // 白色面板铺满高度、贴底，内容随其布局流动
    panel_->setGeometry(panelRect().toRect());
}

void BaDialog::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
}

void BaDialog::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton) {
        if (closeButtonRect().contains(event->pos())) {
            closeDialog();
            return;
        }
        if (!panelRect().contains(event->pos()))
            closeDialog();   // 点击遮罩关闭
    }
}
