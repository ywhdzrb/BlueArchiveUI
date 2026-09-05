#pragma once

#include <QColor>
#include <QPixmap>
#include <QSize>

// BA 手绘图标集：以 QPainter 矢量绘制代替图片资源，
// 避免嵌入游戏解包素材（版权归 Nexon），同时适配任意尺寸。
namespace ba {

enum class Glyph {
    Lightning,      // AP：绿渐变圆 + 白闪电
    Coin,           // 金币：金色圆 + 白五角星
    Pyroxene,       // 青辉石：蓝紫多面体 + 高光
    Return,         // 深蓝圆 + 白色左箭头
    Home,           // 深蓝圆 + 白色房子
    Close,          // 红色圆 + 白叉
    Check,          // 青蓝勾
    Plus,           // 青蓝加号
    Speaker,        // 喇叭 + 声波（右端）
    SpeakerMini,    // 喇叭本体无波环（左端）
    Mute,           // 喇叭 + 斜杠
    Search,         // 放大镜
    Settings,       // 齿轮
    Ticket,         // 证件 / 票卡（功能钮）
    Mail,           // 信封（功能钮）
    Grid,           // 棋盘格（功能钮）
    Star,           // 白色五角星（お仕事装饰）
    Arrow,          // 白色大三角（大厅翻页 / 返回）
    Music,          // 音符（BGM 行行头）—— 供滑块行头使用
    Pencil,         // 铅笔（账号信息页编辑钮）
    Copy,           // 复制（UID 胶囊）
    MarkRing        // 十字圆环徽章（称號输入框装饰）
};

// 生成矢量图标位图。tint 为可忽略的绘制基准色（多数图标自带固定配色）。
QPixmap pixmap(Glyph glyph, const QSize &size, const QColor &tint = {});

} // namespace ba
