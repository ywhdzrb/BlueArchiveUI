#pragma once

#include <QDialog>
#include <QPointer>
#include "md3_theme.h"
#include "md3_button.h"

class QLabel;
class QVBoxLayout;
class QWidget;

// MD3 对话框：居中浮层，surface-container-high 底色 + 28px 圆角 + Level 3 阴影。
class Md3Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Md3Dialog(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setBody(const QString &body);

    // 内容区：替换正文下方区域为自定义 widget（如输入行 / 图片占位）
    void setContentWidget(QWidget *widget);

    // 追加操作按钮（右对齐），返回按钮索引
    int addAction(const QString &text, std::function<void()> onClick = {},
                  Md3Button::Style style = Md3Button::Style::Text);

    void setTheme(const Md3Theme &theme);

    // 显示对话框（模态居中）
    void showDialog();
    // 点击遮罩（对话框外部）关闭（默认 true）
    void setCloseOnBackdrop(bool closeOnBackdrop);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    Md3Theme theme_;
    QPointer<QLabel> titleLabel_;
    QPointer<QLabel> bodyLabel_;
    QPointer<QVBoxLayout> bodyLayout_;
    bool closeOnBackdrop_ = true;
};
