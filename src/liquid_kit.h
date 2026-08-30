#pragma once

// liquid_kit：MD3 + 液态玻璃 UI 框架统一聚合头。
// 引用方式：只需 #include "liquid_kit.h" 即可使用全部组件，
// 再链接 liquid_kit 静态库（见 README.md 的 CMake 接入说明）。

//
// 主题层
//
#include "md3_theme.h"        // Md3Theme：Material Design 3 色彩角色（亮/暗两套）

#include "monet_theme.h"      // 莫奈主题：种子色/图片提取 → 整套 Md3Theme

//
// MD3 基础控件（纯色 Material 风格）
//
#include "md3_button.h"       // Md3Button：填充/描边/文本按钮
#include "md3_card.h"         // Md3Card：圆角卡片（悬停状态层）
#include "md3_switch.h"       // Md3Switch：52x32 开关（200ms 动画）
#include "md3_text_field.h"   // Md3TextField：填充式输入框
#include "md3_progress_bar.h" // Md3ProgressBar：4px 进度条（支持不确定动画）
#include "md3_slider.h"       // Md3Slider：滑块（thumb 44px 竖条）
#include "md3_dropdown.h"     // Md3Dropdown：下拉选择（弹出列表）
#include "md3_side_bar.h"     // Md3SideBar：MD3 侧边导航栏（Navigation Rail）

//
// 液态玻璃控件（玻璃材质：背景折射 + 半透明着色 + 中心透光）
//
#include "liquid_glass.h"           // LiquidGlassPanel 玻璃面板 / LiquidGlassButton 玻璃按钮
#include "liquid_glass_widgets.h"   // 玻璃开关/滑块/进度条/卡片/导航条 + 抓帧/渲染公共函数
