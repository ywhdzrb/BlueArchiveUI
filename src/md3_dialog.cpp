#include "md3_dialog.h"
#include "md3_button.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainterPath>

namespace {
constexpr int kRadius = 28;        // 对话框圆角（MD3 大对话框规范 28px）
constexpr int kPad = 24;           // 内容四周内边距
constexpr int kTitleGap = 10;      // 标题与正文间距
constexpr int kActionGap = 12;     // 按钮间间距
constexpr int kActionTopPad = 16;  // 操作区与正文间隔
}

Md3Dialog::Md3Dialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_TranslucentBackground);   // 圆角背景靠 paintEvent 绘制
    setModal(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(kPad, kPad, kPad, kPad);
    root->setSpacing(kTitleGap);

    // 标题（Headline Small：22px Medium）
    titleLabel_ = new QLabel(this);
    QFont titleFont = font();
    titleFont.setPointSizeF(22.0);
    titleFont.setWeight(QFont::Medium);
    titleLabel_->setFont(titleFont);
    titleLabel_->setWordWrap(true);
    root->addWidget(titleLabel_);

    // 正文（Body Large：16px）
    bodyLabel_ = new QLabel(this);
    QFont bodyFont = font();
    bodyFont.setPointSizeF(16.0);
    bodyLabel_->setFont(bodyFont);
    bodyLabel_->setWordWrap(true);
    bodyLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    root->addWidget(bodyLabel_);

    // 内容区（可被 setContentWidget 替换成自定义 widget）
    bodyLayout_ = new QVBoxLayout;
    bodyLayout_->setSpacing(kTitleGap);
    root->addLayout(bodyLayout_);

    // 操作按钮排：右对齐
    root->addSpacing(kActionTopPad);
    auto *actions = new QHBoxLayout;
    actions->setSpacing(kActionGap);
    actions->addStretch(1);
    root->addLayout(actions);
}

void Md3Dialog::setTitle(const QString &title)
{
    setWindowTitle(title);
    titleLabel_->setText(title);
    titleLabel_->setVisible(!title.isEmpty());
    update();
}

void Md3Dialog::setBody(const QString &body)
{
    bodyLabel_->setText(body);
    bodyLabel_->setVisible(!body.isEmpty());
    update();
}

void Md3Dialog::setContentWidget(QWidget *widget)
{
    if (widget) {
        widget->setParent(this);
        bodyLayout_->addWidget(widget);
    }
}

int Md3Dialog::addAction(const QString &text, std::function<void()> onClick, Md3Button::Style style)
{
    auto *button = new Md3Button(text, this);
    button->setStyle(style);
    // 挂到根布局最后一行的操作排（构造函数末尾 addStretch 之前的按钮区）
    if (QVBoxLayout *root = qobject_cast<QVBoxLayout *>(layout())) {
        const int last = root->count() - 1;
        if (QHBoxLayout *row = qobject_cast<QHBoxLayout *>(root->itemAt(last)->layout())) {
            row->insertWidget(row->count() - 1, button);
        }
    }
    if (onClick) {
        connect(button, &Md3Button::clicked, this, [onClick]() { onClick(); });
    }
    updateGeometry();
    return 0;
}

void Md3Dialog::setTheme(const Md3Theme &theme)
{
    theme_ = theme;

    // 文字颜色按主题调整
    titleLabel_->setStyleSheet(QStringLiteral("color: %1;").arg(theme_.onSurface.name()));
    bodyLabel_->setStyleSheet(QStringLiteral("color: %1;").arg(theme_.onSurfaceVariant.name()));

    // 按钮全量更新主题
    const auto children = findChildren<Md3Button *>();
    for (Md3Button *b : children) {
        b->setTheme(theme_);
    }
    update();
}

void Md3Dialog::showDialog()
{
    show();
    raise();
    activateWindow();
}

QSize Md3Dialog::sizeHint() const
{
    // 最小宽 280（MD3 spec），高度按布局需求
    QSize hint = QDialog::sizeHint();
    hint.setWidth(qMax(280, hint.width()));
    return hint;
}

// 绘制圆角背景（surface-container-high）+ 顶部/底部淡阴影凹凸
void Md3Dialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 投影（Level 3）
    QColor shadow(0, 0, 0, 64);
    constexpr int kShadowBlur = 40;
    p.setPen(Qt::NoPen);
    p.setBrush(shadow);
    p.drawRoundedRect(rect().adjusted(0, 4, 0, 8), kRadius + 4, kRadius + 4);

    // 主背景（surfaceContainerHigh）
    p.setBrush(theme_.surfaceContainerHigh);
    p.drawRoundedRect(rect(), kRadius, kRadius);
}

void Md3Dialog::keyPressEvent(QKeyEvent *event)
{
    // Esc 关闭
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}

void Md3Dialog::mousePressEvent(QMouseEvent *event)
{
    // 点击圆角外区域（透明遮罩区）关闭
    if (closeOnBackdrop_) {
        QPainterPath path;
        path.addRoundedRect(rect(), kRadius, kRadius);
        if (!path.contains(event->position())) {
            reject();
            return;
        }
    }
    QDialog::mousePressEvent(event);
}
