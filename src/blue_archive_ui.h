#pragma once

// blue_archive_ui：MD3 + 液态玻璃 UI 框架统一聚合头。
// 引用方式：只需 #include "blue_archive_ui.h" 即可使用全部组件，
// 再链接 blue_archive_ui 静态库（见 README.md 的 CMake 接入说明）。

//
// 主题层
//
#include "md3_theme.h"        // Md3Theme：Material Design 3 色彩角色（亮/暗两套）

#include "monet_theme.h"      // 莫奈主题：种子色/图片提取 → 整套 Md3Theme

//
// 公共图标
//
#include "md3_icon.h"         // md3::Glyph / paintGlyph：24x24 线性图标库

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
#include "md3_fab.h"          // Md3Fab：浮动按钮（Regular/Small/Large）
#include "md3_badge.h"        // Md3Badge：角标（按数字/小圆点）
#include "md3_chip.h"         // Md3Chip：芯片（Assist/Filter/Input/Suggestion）
#include "md3_check_box.h"    // Md3CheckBox：复选框（支持部分选中）
#include "md3_radio_button.h" // Md3RadioButton：单选按钮
#include "md3_segmented_button.h" // Md3SegmentedButton：分段按钮组
#include "md3_dialog.h"       // Md3Dialog：MD3 对话框（圆角浮层）

//
// 液态玻璃控件（玻璃材质：背景折射 + 半透明着色 + 中心透光）
//
#include "liquid_glass.h"           // LiquidGlassPanel 玻璃面板 / LiquidGlassButton 玻璃按钮
#include "liquid_glass_widgets.h"   // 玻璃开关/滑块/进度条/卡片/导航条/输入框/FAB/下拉 + 抓帧/渲染公共函数
