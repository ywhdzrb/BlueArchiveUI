// ba_progress_demo.cpp —— 纯进度条 demo：Daily Login 卡片整体复刻 + 变体，
// 供针对进度条样式进行指点与调优（浅白底、无背景素材干扰）。
#include <QLabel>
#include <QWidget>
#include <QApplication>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "ba_assets.h"
#include "ba_style.h"
#include "ba_progress_bar.h"
#include "ba_button.h"
#include "ba_icon.h"

namespace {

// Daily Login 卡片（复刻基准截图）：白卡深蓝描边，顶部浅青 Daily 标签+标题+分隔线，
// Count 1/1 + 全满亮青进度条 + 右侧闪电奖励盒 + Claim 黄按钮
class DailyLoginCard : public QWidget
{
public:
    explicit DailyLoginCard(QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedSize(560, 142);

        // 进度条（1/1 全满，mark 关）
        bar_ = new BaProgressBar(this);
        bar_->setValue(1);
        bar_->setRange(0, 1);
        bar_->setFixedHeight(16);
        bar_->setGeometry(24, 88, 300, 16);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // 白卡 + 深蓝描边 + 顶部浅蓝渐变晕
        const QRectF rc(0, 0, width(), height());
        QPainterPath card;
        card.addRoundedRect(rc, 10, 10);
        p.setPen(QPen(QColor(0x2F, 0x54, 0x78, 200), 1.2));
        p.setBrush(QColor(0xFF, 0xFF, 0xFF));
        p.drawPath(card);
        p.save();
        p.setClipPath(card);
        QLinearGradient g(rc.topLeft(), rc.bottomLeft());
        g.setColorAt(0, QColor(0xE8, 0xF3, 0xFC));
        g.setColorAt(0.45, QColor(0xFF, 0xFF, 0xFF, 0));
        p.fillRect(rc, g);
        p.restore();

        // 顶部「Daily」标签（浅青底 + 深蓝粗字，右端带小圆角）+ 深蓝标题
        const QRectF tag(18, 14, 68, 26);
        QPainterPath tagPath;
        tagPath.addRoundedRect(tag, 5, 5);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xB4, 0xE0, 0xFA));
        p.drawPath(tagPath);
        p.setPen(QColor(0x00, 0x31, 0x53));
        p.setFont(BaStyle::font(10, QFont::Bold));
        p.drawText(tag, Qt::AlignCenter, QStringLiteral("Daily"));

        p.setPen(QColor(0x00, 0x31, 0x53));
        p.setFont(BaStyle::font(15, QFont::Bold));
        p.drawText(QRectF(108, 8, 300, 36), Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("Daily Login"));

        // 标题下分隔线
        p.setPen(QPen(QColor(0xD8, 0xE4, 0xEC), 1.2));
        p.drawLine(QPointF(16, 46), QPointF(width() - 16, 46));

        // Count 文字 + 奖励盒 + Claim 按钮
        p.setPen(QColor(0x64, 0x76, 0x86));
        p.setFont(BaStyle::font(9, QFont::Bold));
        // 先画「Count」位置的占位（右侧实际文字叠加在 bar 上方）
        p.drawText(QRectF(24, 62, 120, 20), Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("Count"));
        p.setPen(QColor(0x00, 0x31, 0x53));
        p.setFont(BaStyle::font(12, QFont::Bold));
        p.drawText(QRectF(84, 62, 120, 20), Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("1 / 1"));

        // 右侧奖励盒：白卡 + 渐变闪电（官方图标）+ x50
        const QRectF box(width() - 176, 54, 78, 74);
        QPainterPath bp;
        bp.addRoundedRect(box, 8, 8);
        p.setPen(QPen(QColor(0x2F, 0x54, 0x78, 180), 1));
        p.setBrush(QColor(0xFD, 0xFF, 0xFF));
        p.drawPath(bp);
        const QPixmap zap = ba::pixmap(ba::Glyph::Lightning, QSize(30, 30));
        p.drawPixmap(QPointF(box.center().x() - 15, box.top() + 8), zap);
        p.setPen(QColor(0x6C, 0x7A, 0x88));
        p.setFont(BaStyle::font(9, QFont::Bold));
        p.drawText(QRectF(box.left() + 4, box.bottom() - 22, box.width() - 8, 16),
                   Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("x50"));

        // Claim 黄色按钮（BaButton 由布局？直接本卡内手绘出金色渐变条即可,
        // 这里保持纯展示：按钮类已经在主 demo 验证，卡内用简化绘制）
        const QRectF claim(width() - 92, 54, 78, 74);
        QPainterPath cp;
        cp.addRoundedRect(claim, 8, 8);
        QLinearGradient cg(claim.topLeft(), claim.bottomLeft());
        cg.setColorAt(0, QColor(0xFF, 0xF1, 0x9C));
        cg.setColorAt(1.0, QColor(0xF5, 0xC5, 0x45));
        p.setPen(QPen(QColor(0xC9, 0xA8, 0x2F), 1.4));
        p.setBrush(cg);
        p.drawPath(cp);
        p.setPen(QColor(0x4A, 0x33, 0x0A));
        p.setFont(BaStyle::font(12, QFont::Bold));
        p.drawText(claim, Qt::AlignCenter, QStringLiteral("Claim"));
    }

private:
    BaProgressBar *bar_ = nullptr;
};

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    BaAssets::enableCursor();  // 注册官方字体 + 白色指针

    auto win = new QWidget;
    win->setWindowTitle(QStringLiteral("BlueArchive - Progress Demo"));
    win->resize(760, 680);
    win->setStyleSheet(QStringLiteral("background: #F5FAFF;"));  // 极浅蓝白底，无干扰
    auto *root = new QVBoxLayout(win);
    root->setContentsMargins(48, 40, 48, 40);
    root->setSpacing(18);

    auto *cap = new QLabel(QStringLiteral("<< Butterfly Progress Preview >>"));
    cap->setStyleSheet(QStringLiteral(
        "font-size: 16px; color: #003153; font-weight: 800; letter-spacing: 1px;"));
    root->addWidget(cap);

    // 场景 1：Daily Login 卡片整体复刻（浅蓝底 + 亮青全满）
    root->addWidget(new DailyLoginCard);

    // 场景 2：各值进度条（浅蓝白轨道 + 亮青填充）
    auto *row1 = new QHBoxLayout;
    const int vals[] = { 100, 68, 33, 10 };
    for (int v : vals) {
        auto *pb = new BaProgressBar;
        pb->setFixedSize(150, 16);
        pb->setValue(v);
        row1->addWidget(pb);
    }
    root->addLayout(row1);
    auto *lbl1 = new QLabel(QStringLiteral("100% / 68% / 33% / 10%  ·  浅蓝白轨道（Daily）"));
    lbl1->setStyleSheet(QStringLiteral("font-size: 12px; color: #6B7F8D;"));
    root->addWidget(lbl1);

    // 场景 3：成就卡深黑灰轨道（任务条样式）
    auto *row2 = new QHBoxLayout;
    for (int v : vals) {
        auto *pb = new BaProgressBar;
        pb->setFixedSize(150, 16);
        pb->setTrackColor(QColor("#262A31"));
        pb->setValue(v);
        row2->addWidget(pb);
    }
    root->addLayout(row2);
    auto *lbl2 = new QLabel(QStringLiteral("100% / 68% / 33% / 10%  ·  深黑灰轨道（成就/任务）"));
    lbl2->setStyleSheet(QStringLiteral("font-size: 12px; color: #6B7F8D;"));
    root->addWidget(lbl2);

    // 场景 4：右端格位 mark 版本
    auto *row3 = new QHBoxLayout;
    auto *pbMark = new BaProgressBar;
    pbMark->setFixedSize(150, 16);
    pbMark->setValue(42);
    row3->addWidget(pbMark);
    auto *lbl3 = new QLabel(QStringLiteral("经典浅蓝 + 右上角格位标记（菜单格位风格）"));
    lbl3->setStyleSheet(QStringLiteral("font-size: 12px; color: #6B7F8D;"));
    row3->addWidget(lbl3);
    row3->addStretch();
    root->addLayout(row3);

    root->addStretch();
    win->show();

    // 截图支持：--screenshot <path>，渲染 900ms 后自拍退出（供验证）
    QStringList args = app.arguments();
    const int si = args.indexOf(QStringLiteral("--screenshot"));
    if (si >= 0 && si + 1 < args.size()) {
        const QString path = args.at(si + 1);
        QTimer::singleShot(900, win, [win, path]() {
            win->grab().save(path);
            QApplication::quit();
        });
    }

    return app.exec();
}
