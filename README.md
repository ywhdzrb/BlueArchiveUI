# BlueArchiveUI

模仿游戏《Blue Archive》（蔚蓝档案）界面风格的 **Qt6 Widgets 组件库**。

纯 QPainter 手绘 + 官方素材可选加载：斜切平行四边形（skew -16°）按钮/进度条/卡片、白玻顶栏、青蓝主题色（#4EC3F5）、黄色标题下划线（#FFE433）、BA 官方字体/地图背景/货币图标等，并有复刻 BASpark 的粒子特效组件（点击波纹 + 白/粉/蓝星 + 拖尾发光带）。

## 特性

- **24 个组件**：按钮/滑块/复选框/进度条/顶栏/页面页签/任务卡/商店菜单块/对话气泡/基础卡/信息卡/玩家卡/面板/黑晶片/弹窗/背景/底部导航/侧栏/粒子特效等
- **BA 设计语言**：-16° 斜切家族（`BaStyle::skewRectPath`，含防裁剪预压缩）、青蓝渐变按钮、白玻胶囊、黄色下划线、黑晶片
- **官方素材可选**：字体（Blueaka/Mushin/Gyeonggi）、7 张地图背景、货币图标、设置标题条、鼠标指针——加载失败自动回退手绘
- **零外部依赖**：仅 Qt6 Widgets (+OpenGL 仅部分 demo 用)；C++17
- **粒子特效** `BaSpark`：点击波纹（原版 BASpark 参数）、喷星（白/粉/蓝随机）、按住拖动的发光马尾（越旧越细越淡）

## 组件清单

| 分组 | 组件 |
| --- | --- |
| 基础 | `BaButton`、`BaCheckBox`、`BaSlider`、`BaProgressBar`、`BaSectionHeader` |
| 结构 | `BaTopBar`（Page/Hall 双形态）、`BaNavigationBar`、`BaTabColumn`、`BaPageTabs` |
| 卡片 | `BaCard`（斜切+行式布局）、`BaPanel`、`BaChip`（黑晶片）、`BaBountyCard`、`BaMissionCard`、`BaInfoCard`、`BaPlayerCard` |
| 对话 | `BaDialog`、`BaVoiceBubble`、`BaShopMenuBlock` |
| 背景 | `BaBackground`（官方背景图/手绘天空双模式） |
| 特效 | `BaSpark`（点击波纹/喷星/发光拖尾） |
| 工具 | `BaStyle`（色彩/渐变/路径/缓动）、`BaAssets`（素材加载/字体/光标）、`BaIcon`（手绘矢量图标/官方图标） |

## 快速开始

```cmake
# CMakeLists.txt
add_subdirectory(BlueArchiveUI)
target_link_libraries(your_app PRIVATE blue_archive_ui)
```

```cpp
#include <QApplication>
#include "blue_archive_ui.h"   // 一行引入全量

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    BaAssets::enableCursor();  // 可选：注册官方字体 + 装 BA 鼠标

    QWidget win;
    win.resize(1180, 740);

    // 顶栏（Page 形态）
    BaTopBar *top = new BaTopBar(QStringLiteral("任務"), &win);
    top->setWallet(QStringLiteral("152"), QStringLiteral("16,118,958"), QStringLiteral("685"));
    top->setGeometry(0, 0, 1180, 64);

    // 斜切按钮
    auto *btn = new BaButton(QStringLiteral("確定"), ba::SurfaceRole::Sky, &win);
    btn->setGeometry(40, 120, 140, 44);

    // 进度条
    auto *bar = new BaProgressBar(&win);
    bar->setGeometry(40, 180, 500, 22);
    bar->setRange(0, 100);
    bar->setValue(62);

    win.show();
    return app.exec();
}
```

## 演示程序

构建后（见下），仓库存有 9 个演示目标：

```
./build/blue_archive_ui_all_demo           # 全组件总汇（可滚动）
./build/blue_archive_ui_integrated_demo    # 整合页（顶栏+按钮/声音/进度/卡片+粒子装饰层）
./build/blue_archive_ui_topbar_demo        # 顶栏（Page/Hall、自适应宽度）
./build/blue_archive_ui_button_demo        # 按钮色板预览
./build/blue_archive_ui_progress_demo      # 进度条（Daily Login 复刻）
./build/blue_archive_ui_slider_demo        # 滑块（音量设置）
./build/blue_archive_ui_options_demo       # Options 弹窗复刻
./build/blue_archive_ui_info_demo          # 账号信息页复刻
./build/blue_archive_ui_card_demo          # 基础卡片
./build/blue_archive_ui_spark_demo         # 粒子特效（点击/拖动试玩）
```

大多数 demo 支持 `--screenshot <png>` 无头截图（需 `QT_QPA_PLATFORM=xcb` 运行环境）。

## 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

依赖：CMake ≥3.16、Qt6（Widgets、OpenGL）、C++17 编译器。

## 素材

`assets/` 目录为**可选运行素材**（Blue Archive 游戏解包资源，版权归 Nexon 所有，仅作学习参考，请勿二次分发）。`BaAssets` 自动探测目录：`$EXE_DIR/../assets` → `$EXE_DIR/assets` → 当前工作目录 `assets/`，命中 `fonts/` 子目录即加载字体。

- `assets/fonts/`：Blueaka.ttf、mushin.otf、Gyeonggi_Title_{Medium,Light}.ttf
- `assets/img/bg/`：7 张 1920×1080 官方地图背景
- `assets/img/icons/`：官方货币图标（AP 行动体力/金币/青辉石）
- `assets/img/ui/`：设置标题条装饰 `settingTitleBG.png`、官方鼠标指针 `cursor-default.png`

不放置素材时全部组件使用纯 QPainter 手绘回退，功能不受影响。

## 许可

代码（本仓库）MIT 风格开源；Blue Archive 名称与素材版权归 Nexon。素材仅用于私人学习。
