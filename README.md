# BlueArchiveUI

<img src="assets/logo/BlueArchive-UI_symbolon.png" width="360" alt="BlueArchiveUI logo">

一个用 Qt6 Widgets 模仿《蔚蓝档案》界面风格的组件库。

> 学生党，更新随缘，目前还没完全还原。

## 现在有的

目前有 24 个组件，基本覆盖了常见 UI 元素：

· 按钮、复选框、滑块、进度条、标题栏
· 顶栏（Page/Hall 两种）、底部导航、侧栏、页面页签
· 各种卡片（任务卡、信息卡、玩家卡、商店菜单块）
· 对话框、对话气泡、面板、黑晶片
· 背景（支持官方地图图或手绘回退）
· 粒子特效：点击波纹、喷星、拖尾光效（接近原版点击特效）


## 怎么用

### CMake 引入：

```cmake
add_subdirectory(BlueArchiveUI)
target_link_libraries(your_app PRIVATE blue_archive_ui)
```

### 代码里：

```cpp
#include <QApplication>
#include "blue_archive_ui.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    // 可选：加载官方字体和鼠标指针
    BaAssets::enableCursor();

    QWidget win;
    win.resize(1180, 740);

    BaTopBar *top = new BaTopBar("任務", &win);
    top->setWallet("152", "16,118,958", "685");
    top->setGeometry(0, 0, 1180, 64);

    BaButton *btn = new BaButton("確定", ba::SurfaceRole::Sky, &win);
    btn->setGeometry(40, 120, 140, 44);

    BaProgressBar *bar = new BaProgressBar(&win);
    bar->setGeometry(40, 180, 500, 22);
    bar->setRange(0, 100);
    bar->setValue(62);

    win.show();
    return app.exec();
}
```

### 编译

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

需要 CMake ≥3.16、Qt6（Widgets + OpenGL）、C++17 编译器。

---

### 附带的 demo

构建后会有这些可执行文件（在 build/ 下）：

· blue_archive_ui_all_demo —— 全组件总览（可滚动）
· blue_archive_ui_integrated_demo —— 整合页示例
· blue_archive_ui_topbar_demo —— 顶栏两种形态
· blue_archive_ui_button_demo —— 按钮色板
· blue_archive_ui_progress_demo —— 每日登录进度条复刻
· blue_archive_ui_slider_demo —— 音量滑块
· blue_archive_ui_options_demo —— 设置弹窗
· blue_archive_ui_info_demo —— 账号信息页
· blue_archive_ui_card_demo —— 基础卡片
· blue_archive_ui_spark_demo —— 粒子特效试玩

多数 demo 支持 --screenshot <文件名>.png 无头截图（需要 xcb 环境）。

## 素材

assets/ 目录下放的是游戏解包资源（字体、背景图、图标、光标），版权归 Nexon，仅供个人学习，请勿二次分发。
BaAssets 会自动在 ../assets、./assets 等路径查找，找不到就用手绘回退，不影响功能。

##许可

代码部分 MIT 协议；素材和“蔚蓝档案”名称版权归 Nexon。

