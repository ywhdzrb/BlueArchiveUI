#pragma once

#include <QWidget>
#include <QVariantAnimation>
#include <QPointer>
#include "md3_theme.h"
#include "md3_icon.h"

// MD3 分段按钮（Segmented Button）：单行多段互斥选择器。
// 每段内部点击切换；选中段以 secondaryContainer 底 + onSecondaryContainer 内容显示，
// 分段之间以 1px 分隔线相连，整体最外圈保持 outline 描边，
// 全组统一高度 40px，全圆角胶囊外轮廓。
// 使用示例：
//   auto *seg = new Md3SegmentedButton(this);
//   seg->addSegment(QStringLiteral("Day"), md3::Glyph::Star);
//   seg->addSegment(QStringLiteral("Week"));
//   seg->setCurrentIndex(0);
//   connect(seg, &Md3SegmentedButton::currentIndexChanged, ...);
class Md3SegmentedButton : public QWidget
{
    Q_OBJECT

public:
    explicit Md3SegmentedButton(QWidget *parent = nullptr);

    // 追加分段，文本 + 可选图标（nullopt 无图标）
    int addSegment(const QString &text, const std::optional<md3::Glyph> &glyph = std::nullopt);
    void clearSegments();

    // 当前选中索引，-1 为无选中
    int currentIndex() const { return currentIndex_; }
    void setCurrentIndex(int index);

    void setTheme(const Md3Theme &theme);

    // 允许全部取消（默认为 false：点击已选段不取消选中）
    void setDeselectable(bool deselectable) { deselectable_ = deselectable; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void currentIndexChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    struct Segment {
        QString text;
        std::optional<md3::Glyph> glyph;
        qreal width = 0.0;          // 计算的段宽
        qreal hoverAlpha = 0.0;     // hover 状态层（0~10 百分比）
    };
    // 段内所有控件统一状态动画（hover 用）
    void animateHover(int index, qreal target);

    Md3Theme theme_;
    QVector<Segment> segments_;
    // 每段独立的 hover 动画器（与 segments_ 一一对应，QPointer 指向 this 的
    // QObject child，生命周期由段按钮托管）：
    // 若共用单个动画器，换段时 stop 后旧段 hoverAlpha 会停在中间值导致状态层残留
    QVector<QPointer<QVariantAnimation>> hoverAnims_;
    int currentIndex_ = -1;
    bool deselectable_ = false;
    bool hovered_ = false;
    int hoverIndex_ = -1;           // 当前 hover 的段

    static constexpr qreal kHeight = 40.0;       // 全组高度
    static constexpr qreal kPad = 16.0;          // 段内左右内边距
    static constexpr qreal kGap = 3.0;           // 图标与文本间距
    static constexpr qreal kStroke = 1.0;        // 描边宽
    static constexpr qreal kHoverAlpha = 10.0;   // hover 状态层透明度
};
