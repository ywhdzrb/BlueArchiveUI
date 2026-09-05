#pragma once

#include <QColor>

// MD3 主题：持有全部色彩角色，分亮色 / 暗色两套，并提供状态层混合辅助。
// 色板依据 Material Design 3 规范（种子色 Google Blue #1A73E8）。
class Md3Theme
{
public:
    static Md3Theme light();
    static Md3Theme dark();

    // 主色系
    QColor primary;
    QColor onPrimary;
    QColor primaryContainer;
    QColor onPrimaryContainer;

    // 辅助色系
    QColor secondary;
    QColor onSecondary;
    QColor secondaryContainer;
    QColor onSecondaryContainer;

    // 第三色系
    QColor tertiary;
    QColor onTertiary;
    QColor tertiaryContainer;
    QColor onTertiaryContainer;

    // 错误色系
    QColor error;
    QColor onError;
    QColor errorContainer;
    QColor onErrorContainer;

    // 表面与背景
    QColor background;
    QColor onBackground;
    QColor surface;
    QColor onSurface;
    QColor surfaceVariant;
    QColor onSurfaceVariant;
    QColor surfaceContainerLow;
    QColor surfaceContainer;
    QColor surfaceContainerHigh;
    QColor surfaceContainerHighest;

    // 轮廓
    QColor outline;
    QColor outlineVariant;

    // 把 tint 以指定透明度叠加到 base 上，用于 hover / 按下等状态层反馈
    static QColor blend(const QColor &base, const QColor &tint, qreal alpha);

    // 禁用态背景：on-surface 以 12% 透明度叠在表面上
    QColor disabledContainer() const;
    // 禁用态内容：on-surface 以 38% 透明度叠在表面上
    QColor disabledContent() const;
};
