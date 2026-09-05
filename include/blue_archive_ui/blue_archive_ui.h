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

//
// BA 风格层（Blue Archive 游戏风：青蓝斜切、白顶栏、黄下划线、底部滑入弹窗）
//
#include "ba_style.h"             // BaStyle：色板 / 斜切矩形 / 弹性缓动 / 字体
#include "ba_icon.h"              // ba::Glyph / pixmap：手绘图标集（闪电/金币/青辉石等）
#include "ba_button.h"            // BaButton：平行四边形按钮（七种角色色，按压缩放）
#include "ba_slider.h"            // BaSlider：青蓝填充滑块（白色圆 thumb 蓝描边）
#include "ba_check_box.h"         // BaCheckBox：白方青勾复选框（勾画生长动画）
#include "ba_progress_bar.h"      // BaProgressBar：青蓝渐变进度条（右上格位标记）
#include "ba_section_header.h"    // BaSectionHeader：蓝竖条小标题 + 下虚线
#include "ba_top_bar.h"           // BaTopBar：白底顶栏（黄下划线标题 / 钱包 / 主页钮）
#include "ba_tab_column.h"        // BaTabColumn：浅蓝设置侧栏 tab 列
#include "ba_dialog.h"            // BaDialog：遮罩 + 白面板弹窗（底部滑入）
#include "ba_background.h"        // BaBackground：蓝天云朵 + 远山渐变背景
#include "ba_panel.h"             // BaPanel：白卡 + 深蓝描边 + 顶部浅蓝信息区
#include "ba_chip.h"              // BaChip：深蓝黑斜切数值晶片（×10000 样式）
#include "ba_assets.h"            // BaAssets：官方素材访问（字体/背景/鼠标，运行时可选）
#include "ba_info_card.h"         // BaInfoCard：账号信息页卡片（丝带标题 + 行模型）
#include "ba_player_card.h"       // BaPlayerCard：玩家卡（Lv 大数字 + 进度条 + 左端大斜切）
#include "ba_card.h"  // 基础卡片（白-16°平行四边形+深蓝描边+内容区）

#include "ba_spark.h"             // BaSpark：BA 鼠标/点击粒子特效（波纹+喷星+拖尾，复刻 BASpark）

#include "ba_navigation_bar.h"    // BaNavigationBar：底部白色胶囊导航条 + 深蓝选中块

#include "ba_page_tabs.h"         // BaPageTabs：页签白药丸条 + 橙渐变选中块 + 红旗

#include "ba_bounty_card.h"       // BaBountyCard：悬赏/任务卡（红标签+深蓝标题片+跳战券胶囊）

#include "ba_shop_menu_block.h"   // BaShopMenuBlock：商店深蓝渐变菜单块 + 橙角标

#include "ba_voice_bubble.h"
#include "ba_mission_card.h"   // 成就/任务卡（橙标签+进度条+奖励盒+立即前往）      // BaVoiceBubble：对话白气泡 + 名字条 + 尾巴
