// BA Options 设置弹窗复刻 demo：对照真机（Mi 10, 2340x1080）截图逐像素还原。
// 布局：暗背景 + 中央 900x430 白卡片弹窗
//   ├─ 顶部标题条（Options 居中 + 黄线贴字底 + 右上深灰 X + 左上浅蓝几何装饰）
//   ├─ 左栏浅蓝渐变 tab 列（Games/Graphics/Volume/Notice/Language + 选中白块 + 底部游戏键位图例）
//   └─ 右内容白板：4 行音量（Master/BGM/SFX/Voice）竖条+滑块+Mute 方框 + 右下 Default 青蓝钮

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QLabel>
#include <QPainter>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

#include "ba_assets.h"
#include "ba_background.h"
#include "ba_slider.h"
#include "ba_style.h"

// 内容区单行：蓝 I 竖条 + 名称 + BaSlider + Mute 文字 + 白描边方框
class OptionsRow : public QWidget
{
public:
    explicit OptionsRow(const QString &name, int value, QWidget *parent = nullptr)
    {
        // 行首青蓝竖条
        auto *bar = new QWidget;
        bar->setFixedSize(4, 16);
        bar->setStyleSheet("background:#4BB3E8;border-radius:2px;");

        auto *lbl = new QLabel(name);
        lbl->setFont(BaStyle::font(10, QFont::Bold));
        lbl->setStyleSheet("color:#002F6E;background:transparent;");

        slider_ = new BaSlider;
        slider_->setValue(value);

        auto *mute = new QLabel(QStringLiteral("Mute"));
        mute->setFont(BaStyle::font(10, QFont::DemiBold));
        mute->setStyleSheet("color:#002F6E;background:transparent;");

        auto *box = new MuteBox;

        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(26, 0, 26, 0);
        lay->setSpacing(10);
        lay->addWidget(bar);
        lay->addSpacing(2);
        lay->addWidget(lbl);
        lay->addWidget(slider_, 2);
        lay->addWidget(mute);
        lay->addWidget(box);
    }

private:
    // 白描边空方框（真机 Mute 复选：白底 + 深蓝灰描边 2px，无圆角小）
    class MuteBox : public QWidget
    {
    public:
        explicit MuteBox(QWidget *parent = nullptr) : QWidget(parent)
        {
            setFixedSize(40, 32);
            setCursor(Qt::PointingHandCursor);
        }

    protected:
        void paintEvent(QPaintEvent *event) override
        {
            Q_UNUSED(event);
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(QPen(QColor(0x7A, 0xA8, 0xC4), 2));
            p.setBrush(QColor(255, 255, 255, 240));
            p.drawRoundedRect(rect(), 2, 2);
        }
    };

    BaSlider *slider_ = nullptr;
};

// 左栏 tab 项
class OptionsTabs : public QWidget
{
public:
    explicit OptionsTabs(QWidget *parent = nullptr)
    {
        auto *lay = new QVBoxLayout(this);
        lay->setContentsMargins(0, 78, 0, 0);
        lay->setSpacing(0);
        for (const QString &name : {QStringLiteral("Games"), QStringLiteral("Graphics"),
                                    QStringLiteral("Volume"), QStringLiteral("Notice"),
                                    QStringLiteral("Language"), QStringLiteral("Title")}) {
            auto *tab = new QLabel(name);
            tab->setFixedHeight(46);
            tab->setAlignment(Qt::AlignCenter);
            const bool sel = (name == QStringLiteral("Volume"));
            tab->setStyleSheet(sel
                ? QStringLiteral("background:#FFFFFF;color:#00325B;font-weight:bold;border-radius:3px;")
                : QStringLiteral("background:transparent;color:#00325B;"));
            tab->setFont(BaStyle::font(12, sel ? QFont::Bold : QFont::DemiBold));
            lay->addWidget(tab);
        }
        lay->addStretch(1);
    }
};

class OptionsWindow : public QWidget
{
public:
    explicit OptionsWindow(QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedSize(900, 430);
        lay_ = new QVBoxLayout(this);
        lay_->setContentsMargins(0, 0, 0, 0);
        lay_->setSpacing(0);

        // 主体：左右分栏（左栏固定 190，右内容随伸展）
        auto *body = new QWidget;
        body->setStyleSheet("background:transparent;");
        auto *h = new QHBoxLayout(body);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(0);
        tabs_ = new OptionsTabs;
        tabs_->setFixedWidth(190);
        tabs_->setStyleSheet("background:qlineargradient(x1:0,y1:0,x2:0.7,y2:0,stop:0 #A9DCF7,stop:1 #DFF1FA);border-top-right-radius:8px;border-bottom-right-radius:8px;");
        h->addWidget(tabs_);
        auto *right = new QWidget;
        right->setStyleSheet("background:transparent;");
        auto *v = new QVBoxLayout(right);
        v->setContentsMargins(0, 80, 0, 16);
        v->setSpacing(0);
        for (const QString &name : {QStringLiteral("Master"), QStringLiteral("BGM"),
                                    QStringLiteral("SFX"), QStringLiteral("Voice")}) {
            v->addWidget(new OptionsRow(name, name == QStringLiteral("BGM") ? 30 : 80));
            // 行间隔 1px 淡分隔线
            QFrame *sep = new QFrame;
            sep->setFixedHeight(1);
            sep->setStyleSheet("background:#E8EFF5;");
            v->addWidget(sep);
        }
        v->addStretch(1);
        h->addWidget(right);
        lay_->addWidget(body);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // 阴影底座（外扩 6px 两段灰晕模拟 drop-shadow）
        for (int i = 6; i >= 1; --i) {
            const qreal alpha = i == 6 ? 30 : 52 - i * 5;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(60, 80, 110, int(alpha)));
            p.drawRoundedRect(QRectF(-i, -i, width() + 2 * i, height() + 2 * i), 16 + i, 16 + i);
        }

        // 主体白卡
        p.setBrush(Qt::white);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect(), 14, 14);

        // 顶部标题条：白 → 浅蓝微渐变 + 顶部 3px 装饰带（左 12 浅蓝 → 右白，右端有 45° 斜收口）
        const QRectF titleRect(16, 14, width() - 32, 66);
        QLinearGradient tg(titleRect.topLeft(), titleRect.bottomLeft());
        tg.setColorAt(0, QColor(255, 255, 255));
        tg.setColorAt(1, QColor(0xF2, 0xFA, 0xFE));
        p.setBrush(tg);
        p.drawRoundedRect(titleRect, 10, 10);
        QLinearGradient band(titleRect.topLeft(), QPointF(titleRect.right(), titleRect.top()));
        band.setColorAt(0, QColor(0x8F, 0xD8, 0xF5));
        band.setColorAt(0.55, QColor(0xBF, 0xE6, 0xF8));
        band.setColorAt(1, QColor(0xE8, 0xF4, 0xFC));
        p.setBrush(band);
        p.drawRoundedRect(QRectF(titleRect.x(), titleRect.y() - 2, titleRect.width(), 4), 2, 2);

        // 左上浅蓝几何装饰：40° 斜切小平行四边形色块（真机标题条左侧）
        p.setBrush(QColor(0x9C, 0xDA, 0xF4));
        QPainterPath deco;
        deco.moveTo(titleRect.x() + 6, titleRect.y() + 8);
        deco.lineTo(titleRect.x() + 40, titleRect.y() + 8);
        deco.lineTo(titleRect.x() + 24, titleRect.y() + 40);
        deco.lineTo(titleRect.x() + 6, titleRect.y() + 40);
        deco.closeSubpath();
        p.drawPath(deco);

        // 标题 Options + 黄线贴字底
        const QFont tf = BaStyle::font(17, QFont::Bold);
        p.setFont(tf);
        QFontMetricsF fm(tf);
        const qreal tcx = width() / 2.0;
        const qreal ty = titleRect.center().y() + 3;
        p.setPen(QColor("#00325B"));
        p.drawText(QPointF(tcx - fm.horizontalAdvance(QStringLiteral("Options")) / 2.0, ty + 8),
                   QStringLiteral("Options"));
        const qreal lineW = fm.horizontalAdvance(QStringLiteral("Options")) - 4;
        p.setPen(QPen(QColor(0xF5, 0xC6, 0x42), 4, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(tcx - lineW / 2.0, ty + 18), QPointF(tcx + lineW / 2.0, ty + 18));

        // 右上深灰 X（线条，无底座）
        const qreal x = width() - 40, y2 = titleRect.center().y();
        p.setPen(QPen(QColor(0x5B, 0x73, 0x86), 3, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(x - 9, y2 - 9), QPointF(x + 9, y2 + 9));
        p.drawLine(QPointF(x - 9, y2 + 9), QPointF(x + 9, y2 - 9));

        // 左下按键位图例 △ × + ○
        p.setPen(QPen(QColor(0x8A, 0xA8, 0xC0), 2, Qt::SolidLine, Qt::RoundCap));
        const qreal bx = 30, by = height() - 26;
        // ×
        p.drawLine(QPointF(bx - 5, by - 5), QPointF(bx + 5, by + 5));
        p.drawLine(QPointF(bx - 5, by + 5), QPointF(bx + 5, by - 5));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x8A, 0xA8, 0xC0));
        // △
        p.drawPolygon(QPolygonF({QPointF(bx + 22, by - 9), QPointF(bx + 31, by + 5), QPointF(bx + 13, by + 5)}));
        // +
        p.setPen(QPen(QColor(0x8A, 0xA8, 0xC0), 2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(bx + 53, by - 6), QPointF(bx + 53, by + 6));
        p.drawLine(QPointF(bx + 47, by), QPointF(bx + 59, by));
        // ○
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(bx + 78, by + 2), 11, 11);

        // 右下 Default 青蓝渐变钮：左上高亮 + 白描边 + 白粗体
        const QRectF dbtn(width() - 158, height() - 58, 126, 40);
        QLinearGradient dg(dbtn.topLeft(), dbtn.bottomLeft());
        dg.setColorAt(0, QColor(0x7E, 0xD2, 0xF8));
        dg.setColorAt(1, QColor(0x3F, 0xB6, 0xEE));
        p.setBrush(dg);
        p.setPen(QPen(QColor(255, 255, 255, 210), 1.4));
        p.drawRoundedRect(dbtn, 8, 8);
        // 左上白色高光弧
        p.setPen(QPen(QColor(255, 255, 255, 130), 2.4, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(dbtn.adjusted(5, 4, -5, -6), 70 * 16, 110 * 16);
        const QFont df = BaStyle::font(11, QFont::Bold);
        p.setFont(df);
        p.setPen(Qt::white);
        p.drawText(dbtn, Qt::AlignCenter, QStringLiteral("Default"));
    }

private:
    QVBoxLayout *lay_ = nullptr;
    OptionsTabs *tabs_ = nullptr;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    BaAssets::enableCursor();

    QMainWindow win;
    win.setWindowTitle(QStringLiteral("BA Options Preview"));
    win.resize(1280, 720);

    // 背景：默认背景图铺满 + 轻微压暗（弹窗模态感）
    auto *bg = new BaBackground;
    bg->setImagePath(QStringLiteral("img/bg/mainBG.jpeg"));
    win.setCentralWidget(bg);

    auto *panel = new OptionsWindow(bg);
    panel->setGeometry((1280 - 900) / 2, (720 - 430) / 2, 900, 430);
    panel->show();
    win.show();

    if (app.arguments().contains(QStringLiteral("--screenshot"))) {
        // 直接抓主窗口内容（不依赖 X 根窗口合成，任何 WM 环境下都稳定）
        const QString path = app.arguments().at(app.arguments().indexOf(QStringLiteral("--screenshot")) + 1);
        QTimer::singleShot(900, [&win, &app, path]() {
            win.grab().save(path);
            app.exit(0);
        });
    }

    return app.exec();
}
