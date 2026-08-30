#include "md3_theme.h"

// 亮色主题：背景为浅色，色板取自 MD3 规范亮色方案
Md3Theme Md3Theme::light()
{
    Md3Theme t;
    t.primary = QColor("#1A73E8");
    t.onPrimary = QColor("#FFFFFF");
    t.primaryContainer = QColor("#D2E3FC");
    t.onPrimaryContainer = QColor("#041E49");

    t.secondary = QColor("#0061A4");
    t.onSecondary = QColor("#FFFFFF");
    t.secondaryContainer = QColor("#CCE5FF");
    t.onSecondaryContainer = QColor("#001C3D");

    t.tertiary = QColor("#7A5600");
    t.onTertiary = QColor("#FFFFFF");
    t.tertiaryContainer = QColor("#FFDEAD");
    t.onTertiaryContainer = QColor("#261800");

    t.error = QColor("#BA1A1A");
    t.onError = QColor("#FFFFFF");
    t.errorContainer = QColor("#FFDAD6");
    t.onErrorContainer = QColor("#410002");

    t.background = QColor("#FDF8FD");
    t.onBackground = QColor("#1D1B20");
    t.surface = QColor("#FEFBFF");
    t.onSurface = QColor("#1D1B20");
    t.surfaceVariant = QColor("#E1E0EC");
    t.onSurfaceVariant = QColor("#44464F");
    t.surfaceContainerLow = QColor("#F7F2FA");
    t.surfaceContainer = QColor("#F3EDF7");
    t.surfaceContainerHigh = QColor("#ECE6F0");
    t.surfaceContainerHighest = QColor("#E6E0E5");

    t.outline = QColor("#74727A");
    t.outlineVariant = QColor("#C4C3CD");
    return t;
}

// 暗色主题：背景为暗色基底 #141218，符合 MD3 默认暗色方案
Md3Theme Md3Theme::dark()
{
    Md3Theme t;
    t.primary = QColor("#AAC7FF");
    t.onPrimary = QColor("#003061");
    t.primaryContainer = QColor("#003B6E");
    t.onPrimaryContainer = QColor("#D2E3FC");

    t.secondary = QColor("#9BC6FF");
    t.onSecondary = QColor("#00375E");
    t.secondaryContainer = QColor("#00497A");
    t.onSecondaryContainer = QColor("#CCE5FF");

    t.tertiary = QColor("#FFB96B");
    t.onTertiary = QColor("#3F2D00");
    t.tertiaryContainer = QColor("#5C3F00");
    t.onTertiaryContainer = QColor("#FFDEAD");

    t.error = QColor("#FFB4AB");
    t.onError = QColor("#690005");
    t.errorContainer = QColor("#93000A");
    t.onErrorContainer = QColor("#FFDAD6");

    t.background = QColor("#141218");
    t.onBackground = QColor("#E6E0E9");
    t.surface = QColor("#141218");
    t.onSurface = QColor("#E6E0E9");
    t.surfaceVariant = QColor("#44464F");
    t.onSurfaceVariant = QColor("#E1E0EC");
    t.surfaceContainerLow = QColor("#1D1B20");
    t.surfaceContainer = QColor("#211F26");
    t.surfaceContainerHigh = QColor("#2B2930");
    t.surfaceContainerHighest = QColor("#3C3A41");

    t.outline = QColor("#8E8C94");
    t.outlineVariant = QColor("#5A585F");
    return t;
}

// 状态层混合：在 base 上叠加 tint（透明度 alpha，取值 0~1）
QColor Md3Theme::blend(const QColor &base, const QColor &tint, qreal alpha)
{
    if (alpha <= 0.0) {
        return base;
    }
    if (alpha >= 1.0) {
        return tint;
    }
    return QColor(
        int(base.red() * (1.0 - alpha) + tint.red() * alpha),
        int(base.green() * (1.0 - alpha) + tint.green() * alpha),
        int(base.blue() * (1.0 - alpha) + tint.blue() * alpha),
        255);
}

// 禁用态背景：on-surface 12% 叠于表面
QColor Md3Theme::disabledContainer() const
{
    return blend(surface, onSurface, 0.12);
}

// 禁用态内容：on-surface 38% 叠于表面
QColor Md3Theme::disabledContent() const
{
    return blend(surface, onSurface, 0.38);
}
