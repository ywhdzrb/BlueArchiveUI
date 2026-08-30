#pragma once

#include <QWidget>
#include <QVariantAnimation>
#include "md3_theme.h"

class QLineEdit;

// MD3 填充式输入框：顶部圆角 4px，背景 surface-container-highest，
// 底部指示线聚焦时从 outline-variant 平滑过渡为 2px primary。
class Md3TextField : public QWidget
{
    Q_OBJECT

public:
    explicit Md3TextField(const QString &placeholder, QWidget *parent = nullptr);

    void setTheme(const Md3Theme &theme);

    QString text() const;
    void setText(const QString &text);
    void setPlaceholderText(const QString &placeholder);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    Md3Theme theme_;
    QLineEdit *editor_ = nullptr;
    bool focused_ = false;
    qreal focusProgress_ = 0.0;   // 0 = 未聚焦，1 = 聚焦，驱动指示线过渡
    QVariantAnimation anim_;
};
