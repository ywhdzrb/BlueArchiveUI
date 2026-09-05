#include "ba_assets.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHash>

namespace {

bool g_init = false;                 // 惰性初始化标记
QString g_root;                      // 素材根目录
QString g_family;                    // 已选定的 UI 字体家族名
QHash<QString, QPixmap> g_images;    // 图片缓存（按相对路径）

// 从 assets/fonts/ 注册一个字体文件，返回其家族名（未命中返回空）
QString registerFont(const QString &fileName)
{
    const QString path = g_root + QLatin1String("/fonts/") + fileName;
    if (!QFileInfo::exists(path))
        return {};
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    const QByteArray data = f.readAll();
    const int id = QFontDatabase::addApplicationFont(QString::fromUtf8(data));
    if (id < 0)
        return {};
    const QStringList fams = QFontDatabase::applicationFontFamilies(id);
    return fams.isEmpty() ? QString() : fams.first();
}

} // namespace

QString BaAssets::rootDir()
{
    ensureInit();
    return g_root;
}

QPixmap BaAssets::image(const QString &relPath)
{
    ensureInit();
    if (const auto it = g_images.constFind(relPath); it != g_images.constEnd())
        return it.value();
    QPixmap pm(g_root + QLatin1Char('/') + relPath);
    if (!pm.isNull())
        g_images.insert(relPath, pm);
    return pm;
}

QString BaAssets::uiFontFamily()
{
    ensureInit();
    return g_family;
}

void BaAssets::enableCursor()
{
    ensureInit();
    const QPixmap pm = image(QStringLiteral("img/ui/cursor-default.png"));
    if (pm.isNull())
        return;
    QCursor cur(pm, 4, 4);
    if (QApplication *app = qobject_cast<QApplication *>(QCoreApplication::instance()))
        app->setOverrideCursor(cur);
}

void BaAssets::ensureInit()
{
    if (g_init)
        return;
    g_init = true;

    // 从可执行文件位置向上找 assets/（开发构建时 build/x 与仓库同层）
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(exeDir).absoluteFilePath(QStringLiteral("../assets")),
        QDir(exeDir).absoluteFilePath(QStringLiteral("assets")),
        QDir::current().absoluteFilePath(QStringLiteral("assets")),
    };
    for (const QString &c : candidates) {
        if (QFileInfo::exists(c + QLatin1String("/fonts"))) {
            g_root = QDir(c).absolutePath();
            break;
        }
    }
    if (g_root.isEmpty())
        g_root = QDir(exeDir).absoluteFilePath(QStringLiteral("../assets"));

    // 官方字体注册：Blueaka（主界面）→ Mushin → Gyeonggi Title，全部注册后按优先级挑选
    const QStringList files = {
        QStringLiteral("Blueaka.ttf"),
        QStringLiteral("mushin.otf"),
        QStringLiteral("Gyeonggi_Title_Medium.ttf"),
        QStringLiteral("Gyeonggi_Title_Light.ttf"),
    };
    QStringList registered;
    for (const QString &file : files) {
        const QString fam = registerFont(file);
        if (!fam.isEmpty())
            registered.append(fam);
    }
    for (const char *hint : {"BLUEAKA", "mushin", "gyeonggi"}) {
        for (const QString &fam : registered) {
            if (fam.toLower().contains(QLatin1String(hint))) {
                g_family = fam;
                break;
            }
        }
        if (!g_family.isEmpty())
            break;
    }
}
