// BA 点击粒子组件（按用户规格重写）
// 效果：
//   点击    —— ① 主题蓝半透明圆：原地渐入渐出（sin 曲线）；② 一堆彩色半透明圆角等腰三角从中心
//              向四周散开、渐出消失；③ 若干纯色月牙形波纹从中心向四周移动消失
//   长按拖动 —— 轨迹线跟在鼠标后，时间越久的部分越细并慢慢消失；
//              拖动途中随机生成彩色圆角三角，从轨迹线上散向四周
//
// 实现要点：普通半透明 SourceOver 绘制（无需离屏加性缓冲），QPainter 直接画于本 widget，
// 与下层背景正常合成；无粒子时停表零开销。
// API：setColor/setScale/setFxOpacity/setAlwaysTrail/clickAt/moveTo/releaseEffect/clearEffects

#ifndef BA_SPARK_H
#define BA_SPARK_H

#include <QWidget>
#include <QColor>
#include <QPointF>
#include <QTimer>
#include <QVector>

class BaSpark : public QWidget
{
    Q_OBJECT

public:
    explicit BaSpark(QWidget *parent = nullptr);
    ~BaSpark() override = default;

    // 主题色（默认青蓝 #4EC3F5）；particle 颜色从固定色板随机取
    void setColor(const QString &rgb);
    void setColor(const QColor &color);
    QColor color() const { return color_; }

    void setScale(qreal scale);       // 粒子整体尺寸乘子
    void setFxOpacity(qreal opacity); // 全部粒子的 alpha 总乘子
    void setAlwaysTrail(bool on);     // 无按下也跟随鼠标轨迹

    // 装饰层模式：false = 鼠标事件穿透（WA_TransparentForMouseEvents），
    // 并安装全局事件过滤器监听任意位置按下/移动/释放 —— 适合整合进表单页且不抢交互
    void setInteractive(bool on);

    // 手动驱动接口（宿主转发触摸/键盘时用）
    void clickAt(const QPointF &pos);
    void moveTo(const QPointF &pos);
    void releaseEffect();
    void clearEffects();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // 单一粒子：kind 0=主题蓝圆 1=彩三角 2=月牙 3=白色弧环 4=白星；
    // c 颜色；s 尺寸比例；rot/vr 旋角与角速度；v 速度
    struct Particle {
        QPointF p, v;
        QColor c;
        qreal t = 0, T = 1;
        qreal s = 1;
        qreal rot = 0, vr = 0;
        qreal rr = 0; // 环型粒子半径偏移（原版 rRoundRate）
        int kind = 0;
    };
    struct TrailPt { QPointF p; qreal age = 0; bool gap = false; }; // 轨迹点（age 秒越久越细越淡；gap=与上一段断线）

    // 点击：蓝球（渐入渐出+白芯）+ 白色双弧环 + 白色小星簇
    void spawnClick(const QPointF &pos);
    void spawnTri(const QPointF &pos, qreal speedBase, qreal sizeMul);
    QColor randomParticleColor() const;
    void tick();   // 粒子演进 + 重绘（无粒子时停表）
    void ensureTimer();

    QVector<Particle> parts_;  // 粒子池（上限 64，超了弹掉最老）
    QVector<TrailPt> trail_;   // 轨迹点列（上限 24）
    QTimer *timer_ = nullptr;
    bool down_ = false;
    bool alwaysTrail_ = false;
    bool interactive_ = true; // 装饰层模式（false=透传+全局监听）
    bool trailQuota_ = false;  // 本帧拖尾喷三角配额（防高速移动爆炸）

    QColor color_;
    qreal scale_ = 1.0;
    qreal opacity_ = 1.0;
    QPointF lastPos_;
    bool hasLast_ = false;
};

#endif // BA_SPARK_H
