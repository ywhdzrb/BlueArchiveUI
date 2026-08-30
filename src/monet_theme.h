#pragma once

#include <optional>

#include <QColor>
#include <QPixmap>

#include "md3_theme.h"

// 莫奈主题生成：基于 Google material-color-utilities（C++ 版）实现 Material You
// 动态取色。核心流程：种子色 → CAM16/HCT 色彩空间 → 主/次/三/中性调色板 →
// 依据暖汤（tonal spot）方案生成整套 MD3 色彩角色（亮色 / 暗色两套）。

// 从种子色生成完整 MD3 主题。dark 决定亮色 / 暗色方案
Md3Theme makeMonetTheme(const QColor &seed, bool dark);

// 从整张图片提取主题种子色：图片缩放到 128x128 内 → CELEBI 量化 16 色 →
// Score 评分排序取第一名。图片为空或无法量化时返回 nullopt
std::optional<QColor> extractMonetSeed(const QPixmap &pixmap);
