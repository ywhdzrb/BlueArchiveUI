#pragma once

#include <QColor>
#include <QFont>
#include <QLinearGradient>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QWidget>

#include <utility>

namespace ba {

// 表面角色色：BA 顶栏 / 按钮常用的六种底色。
// 取色参考 BA-AD 解包素材与社区还原项目，均为原游戏 UI 高频出现色。
enum class SurfaceRole {
    Sky,     // 亮青：主操作按钮
    Green,   // 亮绿：任务 / 成就类按钮
    Purple,  // 亮紫：教学 / 学园选单类按钮
    Yellow,  // 亮黄：强调 / 装饰高亮
    Red,     // 亮红：危险 / 关闭
    Deep,    // 深藏蓝：主色文字 / 圆形返回按钮
    Ghost    // 透明描边：次级操作
};

} // namespace ba

// BA（Blue Archive）统一样式：色板、几何形状、动画时序与字体。
// 该类为无状态工具集：所有颜色 / 形状 / 缓动以静态方式暴露。
class BaStyle
{
public:
    // ---------- 色板（BA 官方 UI 取色） ----------
    static QColor accent();      // #4EC3F5 青蓝高亮：滑块填充 / 图标 / 选中态
    static QColor sky();         // #77DEFF 亮青：主按钮底色
    static QColor deep();        // #003153 深藏蓝：标题 / 正文主色
    static QColor yellow();      // #FFE433 装饰黄：标题底划线
    static QColor lightBlue();   // #CDE8FD 浅蓝：侧栏 tab 背景
    static QColor muted();       // #6B7F8D 灰蓝：次级文字
    static QColor dash();        // #C9D8E2 虚线分隔
    static QColor track();       // #DFE6EA 滑块轨道 / 进度条底
    static QColor panelBorder(); // rgba(107,127,141,0.40) 面板描边
    static QColor white();       // #FFFFFF 主体白
    static QColor dim();         // 弹窗背后的遮罩黑

    // ---------- 几何与绘图 ----------
    // 左倾平行四边形 + 圆角。skewDeg 为 -10 时即经典 BA 按钮形状。
    // 传入 painter 会被 translate 到 rect 左上角，可直接 fill 后画文字。
    static QPainterPath skewRectPath(const QRectF &rect, qreal skewDeg = -10.0, qreal radius = 6.0);
    static QPainterPath baButtonPath(const QRectF &rect, qreal cut = 20.0, qreal radius = 6.0);

    // 右侧斜切路径：右上角 45° 切角（BA「お仕事」按钮形态）
    static QPainterPath rightCutPath(const QRectF &rect, qreal cut = 16.0, qreal radius = 6.0);

    // ---------- 渐变材质（v2：对照游戏截图升级） ----------
    // 青蓝亮渐变：一般按钮 / 滑块/ 弹窗动作钮（#9FE7FB → #5FC6F2）
    static QLinearGradient accentGradient(const QRectF &r);
    // 深藏蓝渐变：返回钮 / 深蓝卡片 / お仕事（#2E6CA8 → #0F3761）
    static QLinearGradient deepGradient(const QRectF &r);
    // 面板白底微蓝渐变（弹窗 / 菜单面板，顶部略蓝）
    static QLinearGradient panelGradient(const QRectF &r);
    // 卡片顶部浅蓝信息区（#FFFFFF → #EDF3F9）
    static QLinearGradient cardInfoGradient(const QRectF &r);

    // 角色色映射：返回 SurfaceRole 对应的填充底色 / 前景色
    static QColor roleColor(ba::SurfaceRole role);
    static QColor onRoleColor(ba::SurfaceRole role);
    // 角色渐变对（亮段 → 深段）：角色色按钮的立体渐变底
    static std::pair<QColor, QColor> roleGradientPair(ba::SurfaceRole role);

    // 橙色系：任务页「成就」标签 / 页签选中块 / 商店菜单角标（#FFC95C → #F5821F）
    static QLinearGradient orangeGradient(const QRectF &r);
    // 大进度条轨道：任务「次数 0/1」下方深黑灰圆头条（#40454F → #23272E）
    static QColor progressTrackDark();
    // 大进度条填充：亮青蓝渐变（#C7F0FF → #96DCFA）
    static QColor progressFill();

    // 黑晶片色：数值消耗片（×10000 / 30）的深蓝黑底
    static QColor chipBlack() { return QColor("#252F41"); }

    // 丝带渐变：账号信息页卡片标题条（稱呼/簡介設定，顶部亮 #4E86BE → 底部深 #12396B）
    static QLinearGradient ribbonGradient(const QRectF &r);
    // 信息卡边缘浅蓝描边色（卡片白底圆角 8 的外描边）
    static QColor infoCardEdge() { return QColor("#A9C6E4"); }

    // ---------- 动画 ----------
    // 近似 cubic-bezier(0.3, 1.3, 0.3, 1)：带少许过冲的弹性回弹。
    // BA 面板从底部滑入的标准时序。
    static qreal easeOutBack(qreal t);

    // 让 widget 从父容器底部滑入（translateY(100%) → 0，默认 0.4s）。
    // 动画随 widget 父对象析构自动回收；多次调用会连缀动画。
    static void slideInFromBottom(QWidget *widget, int durationMs = 400);

    // ---------- 字体 ----------
    // BA 图内为圆润无衬线字，此处按优先级取系统可用字体。
    static QFont font(int pointSize = 10, QFont::Weight weight = QFont::Normal);
};
