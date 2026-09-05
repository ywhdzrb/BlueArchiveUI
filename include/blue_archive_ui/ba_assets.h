#pragma once

#include <QCursor>
#include <QPixmap>
#include <QString>

// BA 素材资产访问：从 assets/ 目录加载官方提取素材（字体 / 背景 / UI 元件）。
// 素材版权归 Nexon（Blue Archive），本库仅以"用户本地运行时可选资源"形式交付，
// 未打包进库文件；找不到素材时全部 API 优雅回退，不破坏功能。
class BaAssets
{
public:
    // 定位 assets 根目录：依次探测可执行文件旁 ../assets、可执行文件旁 assets、运行目录 assets。
    // 找不到时返回合理默认路径（../assets），调用方按 QFileInfo::exists 判断。
    static QString rootDir();

    // 加载素材图片并缓存（惰性加载、幂等）。加载失败返回空 QPixmap。
    static QPixmap image(const QString &relPath);

    // 注册官方字体（一次性）：优先 Blueaka（主界面字体）、Mushin、Gyeonggi Title。
    // 返回可用的 UI 字体家族名；失败返回空字符串。
    static QString uiFontFamily();

    // 安装 BA 官方鼠标指针（白箭头）：幂等；素材缺失则不安装。
    static void enableCursor();

private:
    static void ensureInit();   // 惰性初始化（字体注册 + 素材目录定位）
};
