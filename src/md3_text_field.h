#pragma once

#include <QWidget>
#include <QVariantAnimation>
#include "md3_theme.h"
#include "md3_icon.h"

class QLineEdit;

// MD3 填充式输入框：顶部圆角 4px，背景 surface-container-highest，
// 底部指示线聚焦时从 outline-variant 平滑过渡为 2px primary。
// 增强能力：
//   - 前后缀图标（18x18 线性图标，位于编辑框左右外侧）
//   - 错误态：指示线变 error 色 + 后置图标变 error（若已启用）
//   - helper 文本框：显示在指示线下方（可选 error 关键字自定义色）
class Md3TextField : public QWidget
{
    Q_OBJECT

public:
    explicit Md3TextField(const QString &placeholder, QWidget *parent = nullptr);

    void setTheme(const Md3Theme &theme);

    QString text() const;
    void setText(const QString &text);
    void setPlaceholderText(const QString &placeholder);

    // 前后缀图标：传入 std::nullopt 表示移除；setLeadingIconVisible 控制显隐
    void setLeadingIcon(std::optional<md3::Glyph> glyph);
    void setTrailingIcon(std::optional<md3::Glyph> glyph);
    void setLeadingIconVisible(bool visible);
    void setTrailingIconVisible(bool visible);

    // 错误态：传入空串清除错误，否则显示 error 辅助文本
    void setError(const QString &errorText = QStringLiteral(""));
    bool error() const { return !errorText_.isEmpty(); }

    // helper 文本（通常用于提示）；error 激活时被错误文本替代
    void setHelperText(const QString &helperText);

    // 后置图标点击回调（例如清空 / 密码显示切换）
    void setTrailingIconClicked(std::function<void()> callback);

    QSize sizeHint() const override;

signals:
    void trailingIconClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    // 顶部填充区矩形（主体恒 56px）与辅助文本显示区
    QRectF textFieldRect() const;
    // helper / 错误文本激活时动态加高控件
    void updateAreaHeight();

    Md3Theme theme_;
    QLineEdit *editor_ = nullptr;
    bool focused_ = false;
    qreal focusProgress_ = 0.0;   // 0 = 未聚焦，1 = 聚焦，驱动指示线过渡
    QVariantAnimation anim_;

    std::optional<md3::Glyph> leadingGlyph_;
    std::optional<md3::Glyph> trailingGlyph_;
    bool leadingVisible_ = false;
    bool trailingVisible_ = false;
    QString errorText_;
    QString helperText_;
    std::function<void()> trailingCallback_;
};
