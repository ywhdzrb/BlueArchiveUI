#include "monet_theme.h"

#include "cpp/cam/hct.h"
#include "cpp/dynamiccolor/dynamic_scheme.h"
#include "cpp/palettes/core.h"
#include "cpp/quantize/celebi.h"
#include "cpp/score/score.h"
#include "cpp/scheme/scheme_tonal_spot.h"
#include "cpp/utils/utils.h"

// QColor(0~255, 不透明) → ARGB 32 位整数（A=0xFF）
static material_color_utilities::Argb argbFromQColor(const QColor &c)
{
    return material_color_utilities::ArgbFromRgb(c.red(), c.green(), c.blue());
}

// ARGB 32 位整数 → QColor（取 RGB，忽略 Alpha）
static QColor qcolorFromArgb(material_color_utilities::Argb argb)
{
    return QColor(material_color_utilities::RedFromInt(argb),
                  material_color_utilities::GreenFromInt(argb),
                  material_color_utilities::BlueFromInt(argb));
}

// 用 Material You 的暖汤方案，把动态方案里的色彩角色填进 Md3Theme
static Md3Theme themeFromScheme(const material_color_utilities::SchemeTonalSpot &s)
{
    using material_color_utilities::Argb;

    Md3Theme t;
    // 主色系
    t.primary = qcolorFromArgb(s.GetPrimary());
    t.onPrimary = qcolorFromArgb(s.GetOnPrimary());
    t.primaryContainer = qcolorFromArgb(s.GetPrimaryContainer());
    t.onPrimaryContainer = qcolorFromArgb(s.GetOnPrimaryContainer());
    // 辅助色系
    t.secondary = qcolorFromArgb(s.GetSecondary());
    t.onSecondary = qcolorFromArgb(s.GetOnSecondary());
    t.secondaryContainer = qcolorFromArgb(s.GetSecondaryContainer());
    t.onSecondaryContainer = qcolorFromArgb(s.GetOnSecondaryContainer());
    // 第三色系
    t.tertiary = qcolorFromArgb(s.GetTertiary());
    t.onTertiary = qcolorFromArgb(s.GetOnTertiary());
    t.tertiaryContainer = qcolorFromArgb(s.GetTertiaryContainer());
    t.onTertiaryContainer = qcolorFromArgb(s.GetOnTertiaryContainer());
    // 错误色系（固定红色，不随种子变化）
    t.error = qcolorFromArgb(s.GetError());
    t.onError = qcolorFromArgb(s.GetOnError());
    t.errorContainer = qcolorFromArgb(s.GetErrorContainer());
    t.onErrorContainer = qcolorFromArgb(s.GetOnErrorContainer());
    // 表面与背景
    t.background = qcolorFromArgb(s.GetBackground());
    t.onBackground = qcolorFromArgb(s.GetOnBackground());
    t.surface = qcolorFromArgb(s.GetSurface());
    t.onSurface = qcolorFromArgb(s.GetOnSurface());
    t.surfaceVariant = qcolorFromArgb(s.GetSurfaceVariant());
    t.onSurfaceVariant = qcolorFromArgb(s.GetOnSurfaceVariant());
    t.surfaceContainerLow = qcolorFromArgb(s.GetSurfaceContainerLow());
    t.surfaceContainer = qcolorFromArgb(s.GetSurfaceContainer());
    t.surfaceContainerHigh = qcolorFromArgb(s.GetSurfaceContainerHigh());
    t.surfaceContainerHighest = qcolorFromArgb(s.GetSurfaceContainerHighest());
    // 轮廓
    t.outline = qcolorFromArgb(s.GetOutline());
    t.outlineVariant = qcolorFromArgb(s.GetOutlineVariant());
    return t;
}

Md3Theme makeMonetTheme(const QColor &seed, bool dark)
{
    // 种子色 → HCT 色彩空间，再由暖汤方案派生出整套主题
    const material_color_utilities::Hct hct(argbFromQColor(seed));
    const material_color_utilities::SchemeTonalSpot scheme(hct, dark);
    return themeFromScheme(scheme);
}

std::optional<QColor> extractMonetSeed(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        return std::nullopt;
    }

    // 图片缩放到最长边不超过 128，降低量化成本
    const QPixmap scaled = pixmap.scaled(QSize(128, 128), Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
    const QImage img = scaled.toImage().convertToFormat(QImage::Format_RGB32);

    std::vector<material_color_utilities::Argb> pixels;
    pixels.reserve(img.width() * img.height());
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            pixels.push_back(argbFromQColor(img.pixelColor(x, y)));
        }
    }

    // CELEBI 量化：压缩为最多 16 种代表色（Monet 的默认上限）
    const auto result =
        material_color_utilities::QuantizeCelebi(pixels, 16);
    if (result.color_to_count.empty()) {
        return std::nullopt;
    }

    // Score 评分：按色度/色调分布/占比打分排序，取第一名做种子色
    const auto scored =
        material_color_utilities::RankedSuggestions(result.color_to_count);
    if (scored.empty()) {
        return std::nullopt;
    }
    return qcolorFromArgb(scored.front());
}
