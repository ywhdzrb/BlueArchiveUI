#pragma once

#include <QPainter>

// 公共线性图标库：Md3Glyph 枚举 + 统一绘制函数。
// 所有 MD3 组件（侧栏 / 导航条 / FAB / 复选框 / 芯片等）共用一套 24x24
// 基准线性图标几何，避免各处副本漂移；笔宽默认 2px，圆头圆角。
namespace md3 {

enum class Glyph {
    Home,       // 首页
    Search,     // 放大镜
    Star,       // 五角星
    Person,     // 人像
    Palette,    // 调色板
    Drop,       // 水滴（液态玻璃）
    Plus,       // 加号（FAB）
    Check,      // 对勾（复选框 / 菜单选中）
    Close,      // 叉（清空 / 关闭）
    Info,       // 信息（i 字圆）
    Bell,       // 铃铛（通知 / Badge）
    Heart,      // 心形（收藏）
};

// 在 (cx, cy) 处绘制基准 24x24 线性图标，颜色 / 描边宽由调用方指定。
// 内部坐标系与现有 side_bar::paintGlyph 同源：角度约定 0°=右、90°=上。
void paintGlyph(QPainter &p, Glyph glyph, qreal cx, qreal cy, const QColor &color,
                qreal stroke = 2.0);

} // namespace md3
