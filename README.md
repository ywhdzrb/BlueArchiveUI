# liquid_kit

**Material Design 3 + 液态玻璃（Liquid Glass / iOS 26 风格）Qt6 Widgets 控件框架**。

- **主题层**：Material Design 3 色彩系统（亮/暗两套）+ Material You「莫奈」动态取色（Google material-color-utilities 官方实现）
- **MD3 基础控件**：按钮 / 卡片 / 开关 / 输入框 / 进度条 / 滑块 / 下拉 / 侧边栏，纯色 Material 风格
- **液态玻璃控件**：同几何、同交互的玻璃版本——背景折射 + 半透明着色 + 中心透光（自建离屏 GL 渲染，失败自动降级 CPU，永不黑屏）

全部控件基于 `Md3Theme` 取色，主题切换走 `setTheme()` / `applyGlassStyle()`，深浅色界面可一键切换。

## 特性

- **MD3 动态主题**：`makeMonetTheme(seed, dark)` 一键生成整套 Material You 角色色；`extractMonetSeed(pixmap)` 从图片量化提取种子色
- **双渲染路径**：`LiquidGlassPanel` 优先自建 QOpenGLContext + FBO 离屏渲染；GL 不可用（如 NVIDIA+Wayland EGL 3009）自动降级同名 CPU 管线，截图可加 `MD3_GL_DISABLE=1` 强制对比
- **抓帧稳定**：全玻璃控件统一经 `grabGlassBackdrop` 抓窗快照做材质源（隐藏玻璃自身 + FadeOverlay 遮罩），带布局落定 / 限流补抓 / 防重入，Hyprland 等异步 resize 平台下不会残留旧几何
- **真实苹果质感**：桶形倒角折射（中央平坦、边缘倒角弧面 Snell 近似）、菲涅耳边缘光、中心透光、白描边

## 目录结构

```text
liquid_kit/
├── CMakeLists.txt            # 构建静态库 liquid_kit（+ 可选示例）
├── src/
│   ├── liquid_kit.h          # 聚合头：一行 include 全量组件
│   ├── md3_theme.h/.cpp      # Md3Theme 主题
│   ├── monet_theme.h/.cpp    # 莫奈主题生成（种子色/图片提取）
│   ├── md3_button.h/.cpp     # Md3Button
│   ├── md3_card.h/.cpp       # Md3Card
│   ├── md3_switch.h/.cpp     # Md3Switch
│   ├── md3_text_field.h/.cpp # Md3TextField
│   ├── md3_progress_bar.h/.cpp
│   ├── md3_slider.h/.cpp
│   ├── md3_dropdown.h/.cpp
│   ├── md3_side_bar.h/.cpp   # Md3SideBar（含 Glyph 图标枚举）
│   ├── liquid_glass.h/.cpp       # LiquidGlassPanel / LiquidGlassButton + GL 探测
│   └── liquid_glass_widgets.h/.cpp # 玻璃开关/滑块/进度条/卡片/导航栏 + 抓帧基点
├── third_party/material_color_utilities/   # Google 官方 C++ 实现（Apache-2.0）
└── examples/minimal.cpp      # 最小用法示例
```

## 构建与运行示例

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/liquid_kit_minimal
```

## 在项目中使用

```cmake
add_subdirectory(../liquid_kit liquid_kit)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE liquid_kit)
```

头文件（内含全部组件）：

```cpp
#include "liquid_kit.h"
```

## 快速上手

```cpp
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include "liquid_kit.h"

class Demo : public QWidget {
public:
    Demo() {
        theme_ = Md3Theme::dark();

        auto *lay = new QVBoxLayout(this);
        auto *btn = new Md3Button("切换主题");
        connect(btn, &QAbstractButton::clicked, this, [this] {
            dark_ = !dark_;
            theme_ = dark_ ? Md3Theme::dark() : Md3Theme::light();
            applyTheme();
        });

        auto *glass = new LiquidGlassPanel;
        glass->setMinimumSize(360, 240);
        auto *gl = new QVBoxLayout(glass);
        gl->setContentsMargins(32, 28, 32, 28);
        gl->addWidget(new QLabel("玻璃面板：自动折射窗口背景"));
        gl->addWidget(new Md3Button("普通按钮"));

        auto *sw = new LiquidGlassSwitch;
        auto *bar = new LiquidGlassProgressBar;
        bar->setRange(0, 100);
        bar->setValue(60);

        lay->addWidget(btn);
        lay->addWidget(glass, 1);
        lay->addWidget(sw);
        lay->addWidget(bar);
        applyTheme();
    }

private:
    void applyTheme() {
        // 主题角色色传给所有绘制控件
        for (auto *b : findChildren<Md3Button *>()) b->setTheme(theme_);
        for (auto *k : findChildren<LiquidGlassThemeKeeper *>()) {
            k->setTheme(theme_);
            k->applyGlassStyle(dark_);
            k->refreshBackdrop();   // 背景颜色变了要重新抓帧
        }
    }

    Md3Theme theme_;
    bool dark_ = true;
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    Demo w;
    w.resize(760, 480);
    w.show();
    return app.exec();
}
```

完整可编译示例见 `examples/minimal.cpp`（`cmake -B build && cmake --build build` 后运行 `./build/liquid_kit_minimal`）。

## 组件速查

| 组件 | 头文件 | 关键 API | 说明 |
| --- | --- | --- | --- |
| `Md3Theme` | md3_theme.h | `light()` `dark()` `blend(a,b,t)` | 全部色彩角色 + 状态层混合 |
| `makeMonetTheme(seed, dark)` | monet_theme.h | `Md3Theme` 返回 | 种子色 → 整套 MD3 主题 |
| `extractMonetSeed(pixmap)` | monet_theme.h | `std::optional<QColor>` | 图片量化提取主题种子色 |
| `Md3Button` | md3_button.h | `setStyle(Style)` `setTheme()` | Filled/Tonal/Outlined/Text，40 高 |
| `Md3Card` | md3_card.h | `setContent()` | 12px 圆角 + hover 状态层 |
| `Md3Switch` | md3_switch.h | 继承 QAbstractButton | 52x32，200ms 动画 |
| `Md3TextField` | md3_text_field.h | 继承 QLineEdit | 填充式输入框 |
| `Md3ProgressBar` | md3_progress_bar.h | `setRange/setValue/setIndeterminate` | 4px 进度 + 不确定动画 |
| `Md3Slider` | md3_slider.h | `setRange/setValue` `valueChanged` | 48 高，44px 竖条 thumb |
| `Md3Dropdown` | md3_dropdown.h | `addItem` `currentIndexChanged` | 下拉选择 |
| `Md3SideBar` | md3_side_bar.h | `Glyph{}` `addItem` `currentIndexChanged` | MD3 Navigation Rail |
| `LiquidGlassPanel` | liquid_glass.h | `applyGlassStyle(dark)` `refreshBackdrop()` `setRefraction(k)` `setCornerRadius(r)` | 玻璃面板（折射窗口背景） |
| `LiquidGlassButton` | liquid_glass.h | 继承 QAbstractButton | 玻璃按钮（纯着色） |
| `LiquidGlassSwitch` | liquid_glass_widgets.h | 同 Md3Switch | 玻璃开关 |
| `LiquidGlassSlider` | liquid_glass_widgets.h | `setRange/setValue` `valueChanged` | 玻璃滑块 |
| `LiquidGlassProgressBar` | liquid_glass_widgets.h | `setRange/setValue` | 玻璃进度条 |
| `LiquidGlassCard` | liquid_glass_widgets.h | `setContent()` | 玻璃卡片 |
| `LiquidGlassNavigationBar` | liquid_glass_widgets.h | `addItem(label, Glyph)` `setCurrentIndex` | iOS 26 悬浮玻璃导航条 |
| `LiquidGlassThemeKeeper` | liquid_glass_widgets.h | `setTheme()` `applyGlassStyle()` `refreshBackdrop()` | 玻璃控件公共基类（抓帧/主题） |
| `grabGlassBackdrop` | liquid_glass.h | `(self, w)` | 全窗快照（隐藏玻璃控件/遮罩） |
| `renderGlassPlateCPU` | liquid_glass.h | `(backdrop, panelRect, size, ...)` | 纯 CPU 玻璃板渲染 |

## 设计约定

- **明暗主题**：`Md3Theme::light()/dark()` 生成两套角色色，控件持有副本（`setTheme`），无全局单例污染。
- **玻璃控件材质**：`LiquidGlassThemeKeeper` 家族自动抓取窗口快照作为材质源；`refreshBackdrop()`（强制模式）用于页面/主题切换后重抓。抓帧会被零时延调度 + 300ms 实例限流，避免布局未稳抓帧与重绘风暴。
- **GL 与 CPU 双路径**：`LiquidGlassPanel` 用自建离屏 QOpenGLContext + FBO 渲染玻璃帧；上下文创建失败（NVIDIA+Wayland 3009 BAD_MATCH 等平台坑）自动降级 CPU 软渲染，两者视觉一致、永不黑屏。可用 `isGpuRendering()` 查询通道。
- **小控件取景**：LiquidGlassSlider / ProgressBar 按轨道矩形取景（`renderGlassPlate` 的 `viewRect` 参数），保证小件上倒角折射带占比足够。

## 许可证

- **liquid_kit 本体**：MIT（见 `LICENSE`）
- **third_party/material_color_utilities**：Google 官方 C++ 实现，Apache-2.0（见 `third_party/material_color_utilities/LICENSE`），许可证没有授权它被 embedding 后覆盖本项目的许可，遵循 Apache-2.0 再分发条款
