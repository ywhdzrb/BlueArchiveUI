#pragma once

#include <QAbstractButton>
#include <QPair>
#include <QPainterPath>

#include "ba_anim.h"
#include "ba_icon.h"
#include "ba_style.h"

// BA 平行四边形按钮（CodePen@mangopomelo 方案重制）：skew(-16deg) 左倾 + 小圆角，
// 纯色底 + 1px 灰边框 + 底部投影 + 整体 opacity 0.8，文字水平不斜（等效 CSS > * 反斜切）。
// 交互：按下整钮 scale(0.95)（100ms ease-out 回弹）；聚焦描青蓝外框。
class BaButton : public QAbstractButton
{
    Q_OBJECT

public:
    explicit BaButton(const QString &text, ba::SurfaceRole role = ba::SurfaceRole::Sky,
                      QWidget *parent = nullptr);

    void setRole(ba::SurfaceRole role);
    ba::SurfaceRole role() const { return role_; }
    void setTextColor(const QColor &c);          // 默认按 role 自动搭配
    void setFixedHeight(int h);                  // BA 按钮多用短矮尺寸
    void setIconGlyph(ba::Glyph glyph);          // 前置小图标（非必选）
    void setIconVisible(bool visible);
    void setSkewDeg(qreal deg);                  // 斜切角：-16 CodePen 标准，0 直角矩形


    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPainterPath currentPath() const;

    ba::SurfaceRole role_ = ba::SurfaceRole::Sky;
    qreal skewDeg_ = -16.0;          // 斜切角（-16 当前试用值，负=左倾平行四边形）
    bool hovered_ = false;  
    bool focused_ = false;
    bool customTextColor_ = false;   // 用户显式 setTextColor 后不再跟随角色色
    qreal scale_ = 1.0;              // 按压缩放值（动画驱动）
    ba::Animator scalePress_;        // 按下压缩补间器（100ms OutCubic）
    ba::Animator scaleRelease_;      // 松开回补间器（100ms OutCubic，ease-out）
    void scaleTo(qreal target);      // 平滑缩放：按下走 press、回弹走 release

    struct {
        bool visible = false;
        ba::Glyph glyph = ba::Glyph::Plus;
    } icon_;
};
