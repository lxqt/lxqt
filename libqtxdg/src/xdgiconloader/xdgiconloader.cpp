/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// clazy:excludeall=non-pod-global-static

#ifndef QT_NO_ICON
#include "xdgiconloader_p.h"

#include <private/qguiapplication_p.h>
#include <private/qicon_p.h>

#include <QtGui/QIconEnginePlugin>
#include <QtGui/QPixmapCache>
#include <qpa/qplatformtheme.h>
#include <QtGui/QIconEngine>
#include <QtGui/QPalette>
#include <QtCore/qmath.h>
#include <QtCore/QList>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtGui/QPainter>
#include <QImageReader>
#include <QXmlStreamReader>
#include <QFileSystemWatcher>

#include <private/qhexstring_p.h>

//QT_BEGIN_NAMESPACE


Q_GLOBAL_STATIC(XdgIconLoader, iconLoaderInstance)

/* Theme to use in last resort, if the theme does not have the icon, neither the parents  */
static QString fallbackTheme()
{
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        const QVariant themeHint = theme->themeHint(QPlatformTheme::SystemIconFallbackThemeName);
        if (themeHint.isValid()) {
            const QString theme = themeHint.toString();
            if (theme != QLatin1String("hicolor"))
                return theme;
        }
    }
    return QString();
}

#ifdef QT_NO_LIBRARY
static bool gSupportsSvg = false;
#else
static bool gSupportsSvg = true;
#endif //QT_NO_LIBRARY

void XdgIconLoader::setFollowColorScheme(bool enable)
{
    if (m_followColorScheme != enable)
    {
        QIconLoader::instance()->invalidateKey();
        m_followColorScheme = enable;
    }
}

XdgIconLoader *XdgIconLoader::instance()
{
   QIconLoader::instance()->ensureInitialized();
   return iconLoaderInstance();
}

/*!
    \class QIconCacheGtkReader
    \internal
    Helper class that reads and looks up into the icon-theme.cache generated with
    gtk-update-icon-cache. If at any point we detect a corruption in the file
    (because the offsets point at wrong locations for example), the reader
    is marked as invalid.
*/
class QIconCacheGtkReader
{
public:
    explicit QIconCacheGtkReader(const QString &themeDir);
    QVector<const char *> lookup(const QStringRef &);
    bool isValid() const { return m_isValid; }
    bool reValid(bool infoRefresh);
private:
    QFileInfo m_cacheFileInfo;
    QFile m_file;
    const unsigned char *m_data;
    quint64 m_size;
    bool m_isValid;

    quint16 read16(uint offset)
    {
        if (offset > m_size - 2 || (offset & 0x1)) {
            m_isValid = false;
            return 0;
        }
        return m_data[offset+1] | m_data[offset] << 8;
    }
    quint32 read32(uint offset)
    {
        if (offset > m_size - 4 || (offset & 0x3)) {
            m_isValid = false;
            return 0;
        }
        return m_data[offset+3] | m_data[offset+2] << 8
            | m_data[offset+1] << 16 | m_data[offset] << 24;
    }
};
Q_GLOBAL_STATIC(QFileSystemWatcher, gtkCachesWatcher)


QIconCacheGtkReader::QIconCacheGtkReader(const QString &dirName)
    : m_cacheFileInfo{dirName + QLatin1String("/icon-theme.cache")}
    , m_data(nullptr)
    , m_isValid(false)
{
    m_file.setFileName(m_cacheFileInfo.absoluteFilePath());
    // Note: The cache file can be (IS) removed and newly created during the
    // cache update. But we hold open file descriptor for the "old" removed
    // file. So we need to watch the changes and reopen/remap the file.
    QObject::connect(gtkCachesWatcher(), &QFileSystemWatcher::fileChanged, &m_file, [this](const QString & path)
        {
            if (m_file.fileName() == path)
            {
                m_isValid = false;
                // invalidate icons to reload them ...
                QIconLoader::instance()->invalidateKey();
            }
        });
    reValid(false);
}

bool QIconCacheGtkReader::reValid(bool infoRefresh)
{
    if (m_data)
        m_file.unmap(const_cast<unsigned char *>(m_data));
    m_file.close();

    if (infoRefresh)
        m_cacheFileInfo.refresh();

    const QDir dir = m_cacheFileInfo.absoluteDir();

    if (!m_cacheFileInfo.exists() || m_cacheFileInfo.lastModified() < QFileInfo{dir.absolutePath()}.lastModified())
        return m_isValid;

    // Note: If the file is removed, it is also silently removed from watched
    // paths in QFileSystemWatcher.
    if (!gtkCachesWatcher->files().contains(m_cacheFileInfo.absoluteFilePath()))
        gtkCachesWatcher->addPath(m_cacheFileInfo.absoluteFilePath());

    if (!m_file.open(QFile::ReadOnly))
        return m_isValid;
    m_size = m_file.size();
    m_data = m_file.map(0, m_size);
    if (!m_data)
        return m_isValid;
    if (read16(0) != 1) // VERSION_MAJOR
        return m_isValid;

    m_isValid = true;

    // Check that all the directories are older than the cache
    auto lastModified = m_cacheFileInfo.lastModified();
    quint32 dirListOffset = read32(8);
    quint32 dirListLen = read32(dirListOffset);
    for (uint i = 0; i < dirListLen; ++i) {
        quint32 offset = read32(dirListOffset + 4 + 4 * i);
        if (!m_isValid || offset >= m_size || lastModified < QFileInfo(dir
                    , QString::fromUtf8(reinterpret_cast<const char*>(m_data + offset))).lastModified()) {
            m_isValid = false;
            return m_isValid;
        }
    }
    return m_isValid;
}

static quint32 icon_name_hash(const char *p)
{
    quint32 h = static_cast<signed char>(*p);
    for (p += 1; *p != '\0'; p++)
        h = (h << 5) - h + *p;
    return h;
}

/*! \internal
    lookup the icon name and return the list of subdirectories in which an icon
    with this name is present. The char* are pointers to the mapped data.
    For example, this would return { "32x32/apps", "24x24/apps" , ... }
 */
QVector<const char *> QIconCacheGtkReader::lookup(const QStringRef &name)
{
    QVector<const char *> ret;
    if (!isValid() || name.isEmpty())
        return ret;

    QByteArray nameUtf8 = name.toUtf8();
    quint32 hash = icon_name_hash(nameUtf8.data());

    quint32 hashOffset = read32(4);
    quint32 hashBucketCount = read32(hashOffset);

    if (!isValid() || hashBucketCount == 0) {
        m_isValid = false;
        return ret;
    }

    quint32 bucketIndex = hash % hashBucketCount;
    quint32 bucketOffset = read32(hashOffset + 4 + bucketIndex * 4);
    while (bucketOffset > 0 && bucketOffset <= m_size - 12) {
        quint32 nameOff = read32(bucketOffset + 4);
        if (nameOff < m_size && strcmp(reinterpret_cast<const char*>(m_data + nameOff), nameUtf8.constData()) == 0) {
            quint32 dirListOffset = read32(8);
            quint32 dirListLen = read32(dirListOffset);

            quint32 listOffset = read32(bucketOffset+8);
            quint32 listLen = read32(listOffset);

            if (!m_isValid || listOffset + 4 + 8 * listLen > m_size) {
                m_isValid = false;
                return ret;
            }

            ret.reserve(listLen);
            for (uint j = 0; j < listLen && m_isValid; ++j) {
                quint32 dirIndex = read16(listOffset + 4 + 8 * j);
                quint32 o = read32(dirListOffset + 4 + dirIndex*4);
                if (!m_isValid || dirIndex >= dirListLen || o >= m_size) {
                    m_isValid = false;
                    return ret;
                }
                ret.append(reinterpret_cast<const char*>(m_data) + o);
            }
            return ret;
        }
        bucketOffset = read32(bucketOffset);
    }
    return ret;
}

XdgIconTheme::XdgIconTheme(const QString &themeName)
        : m_valid(false)
        , m_followsColorScheme(false)
{
    QFile themeIndex;

    const QStringList iconDirs = QIcon::themeSearchPaths();
    for ( int i = 0 ; i < iconDirs.size() ; ++i) {
        QDir iconDir(iconDirs[i]);
        QString themeDir = iconDir.path() + QLatin1Char('/') + themeName;
        QFileInfo themeDirInfo(themeDir);

        if (themeDirInfo.isDir()) {
            m_contentDirs << themeDir;
            m_gtkCaches << QSharedPointer<QIconCacheGtkReader>::create(themeDir);
        }

        if (!m_valid) {
            themeIndex.setFileName(themeDir + QLatin1String("/index.theme"));
            if (themeIndex.exists())
                m_valid = true;
        }
    }
#ifndef QT_NO_SETTINGS
    if (themeIndex.exists()) {
        const QSettings indexReader(themeIndex.fileName(), QSettings::IniFormat);
        m_followsColorScheme = indexReader.value(QStringLiteral("Icon Theme/FollowsColorScheme"), false).toBool();
        const QStringList keys = indexReader.allKeys();
        for (auto const &key : keys) {
            if (key.endsWith(QLatin1String("/Size"))) {
                // Note the QSettings ini-format does not accept
                // slashes in key names, hence we have to cheat
                if (int size = indexReader.value(key).toInt()) {
                    QString directoryKey = key.left(key.size() - 5);
                    QIconDirInfo dirInfo(directoryKey);
                    dirInfo.size = size;
                    QString type = indexReader.value(directoryKey +
                                                     QLatin1String("/Type")
                                                     ).toString();

                    if (type == QLatin1String("Fixed"))
                        dirInfo.type = QIconDirInfo::Fixed;
                    else if (type == QLatin1String("Scalable"))
                        dirInfo.type = QIconDirInfo::Scalable;
                    else
                        dirInfo.type = QIconDirInfo::Threshold;

                    dirInfo.threshold = indexReader.value(directoryKey +
                                                        QLatin1String("/Threshold"),
                                                        2).toInt();

                    dirInfo.minSize = indexReader.value(directoryKey +
                                                         QLatin1String("/MinSize"),
                                                         size).toInt();

                    dirInfo.maxSize = indexReader.value(directoryKey +
                                                        QLatin1String("/MaxSize"),
                                                        size).toInt();
                    dirInfo.scale = indexReader.value(directoryKey +
                                                      QLatin1String("/Scale"),
                                                      1).toInt();
                    m_keyList.append(dirInfo);
                }
            }
        }

        // Parent themes provide fallbacks for missing icons
        m_parents = indexReader.value(
                QLatin1String("Icon Theme/Inherits")).toStringList();
        m_parents.removeAll(QString());
        m_parents.removeAll(QLatin1String("hicolor"));

        // Ensure a default platform fallback for all themes
        if (m_parents.isEmpty()) {
            const QString fallback = fallbackTheme();
            if (!fallback.isEmpty())
                m_parents.append(fallback);
        }
    }
#endif //QT_NO_SETTINGS
}

/* WARNING:
 *
 * https://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
 *
 * <cite>
 * The dash “-” character is used to separate levels of specificity in icon
 * names, for all contexts other than MimeTypes. For instance, we use
 * “input-mouse” as the generic item for all mouse devices, and we use
 * “input-mouse-usb” for a USB mouse device. However, if the more specific
 * item does not exist in the current theme, and does exist in a parent
 * theme, the generic icon from the current theme is preferred, in order
 * to keep consistent style.
 * </cite>
 *
 * But we believe, that using the more specific icon (even from parents)
 * is better for user experience. So we are violating the standard
 * intentionally.
 *
 * Ref.
 * https://github.com/lxqt/lxqt/issues/1252
 * https://github.com/lxqt/libqtxdg/pull/116
 */
QThemeIconInfo XdgIconLoader::findIconHelper(const QString &themeName,
                                 const QString &iconName,
                                 QStringList &visited,
                                 bool dashFallback) const
{
    QThemeIconInfo info;
    Q_ASSERT(!themeName.isEmpty());

    // Used to protect against potential recursions
    visited << themeName;

    XdgIconTheme &theme = themeList[themeName];
    if (!theme.isValid()) {
        theme = XdgIconTheme(themeName);
        if (!theme.isValid()) {
            const QString fallback = fallbackTheme();
            if (!fallback.isEmpty())
                theme = XdgIconTheme(fallback);
        }
    }

    const QStringList contentDirs = theme.contentDirs();

    const QString svgext(QLatin1String(".svg"));
    const QString pngext(QLatin1String(".png"));
    const QString xpmext(QLatin1String(".xpm"));


    QStringRef iconNameFallback(&iconName);

    // Iterate through all icon's fallbacks in current theme
    if (info.entries.isEmpty()) {
        const QString svgIconName = iconNameFallback + svgext;
        const QString pngIconName = iconNameFallback + pngext;
        const QString xpmIconName = iconNameFallback + xpmext;

        // Add all relevant files
        for (int i = 0; i < contentDirs.size(); ++i) {
            QVector<QIconDirInfo> subDirs = theme.keyList();

            // Try to reduce the amount of subDirs by looking in the GTK+ cache in order to save
            // a massive amount of file stat (especially if the icon is not there)
            auto cache = theme.m_gtkCaches.at(i);
            if (cache->isValid() || cache->reValid(true)) {
                const auto result = cache->lookup(iconNameFallback);
                if (cache->isValid()) {
                    const QVector<QIconDirInfo> subDirsCopy = subDirs;
                    subDirs.clear();
                    subDirs.reserve(result.count());
                    for (const char *s : result) {
                        QString path = QString::fromUtf8(s);
                        auto it = std::find_if(subDirsCopy.cbegin(), subDirsCopy.cend(),
                                               [&](const QIconDirInfo &info) {
                                                   return info.path == path; } );
                        if (it != subDirsCopy.cend()) {
                            subDirs.append(*it);
                        }
                    }
                }
            }

            QString contentDir = contentDirs.at(i) + QLatin1Char('/');
            for (int j = 0; j < subDirs.size() ; ++j) {
                const QIconDirInfo &dirInfo = subDirs.at(j);
                const QString subDir = contentDir + dirInfo.path + QLatin1Char('/');
                const QString pngPath = subDir + pngIconName;
                if (QFile::exists(pngPath)) {
                    PixmapEntry *iconEntry = new PixmapEntry;
                    iconEntry->dir = dirInfo;
                    iconEntry->filename = pngPath;
                    // Notice we ensure that pixmap entries always come before
                    // scalable to preserve search order afterwards
                    info.entries.prepend(iconEntry);
                } else {
                    const QString svgPath = subDir + svgIconName;
                    if (gSupportsSvg && QFile::exists(svgPath)) {
                        ScalableEntry *iconEntry = (followColorScheme() && theme.followsColorScheme()) ? new ScalableFollowsColorEntry : new ScalableEntry;
                        iconEntry->dir = dirInfo;
                        iconEntry->filename = svgPath;
                        info.entries.append(iconEntry);
                    }
                }
                const QString xpmPath = subDir + xpmIconName;
                if (QFile::exists(xpmPath)) {
                    PixmapEntry *iconEntry = new PixmapEntry;
                    iconEntry->dir = dirInfo;
                    iconEntry->filename = xpmPath;
                    // Notice we ensure that pixmap entries always come before
                    // scalable to preserve search order afterwards
                    info.entries.append(iconEntry);
                }
            }
        }

        if (!info.entries.isEmpty())
            info.iconName = iconNameFallback.toString();
    }

    if (info.entries.isEmpty()) {
        const QStringList parents = theme.parents();
        // Search recursively through inherited themes
        for (int i = 0 ; i < parents.size() ; ++i) {

            const QString parentTheme = parents.at(i).trimmed();

            if (!visited.contains(parentTheme)) // guard against recursion
                info = findIconHelper(parentTheme, iconName, visited);

            if (!info.entries.isEmpty()) // success
                break;
        }
    }

    if (dashFallback && info.entries.isEmpty()) {
        // If it's possible - find next fallback for the icon
        const int indexOfDash = iconNameFallback.lastIndexOf(QLatin1Char('-'));
        if (indexOfDash != -1) {
            iconNameFallback.truncate(indexOfDash);
            QStringList _visited;
            info = findIconHelper(themeName, iconNameFallback.toString(), _visited, true);
        }
    }

    return info;
}

QThemeIconInfo XdgIconLoader::unthemedFallback(const QString &iconName, const QStringList &searchPaths) const
{
    QThemeIconInfo info;

    const QString svgext(QLatin1String(".svg"));
    const QString pngext(QLatin1String(".png"));
    const QString xpmext(QLatin1String(".xpm"));

    for (const auto &contentDir : searchPaths)  {
        QDir currentDir(contentDir);

        if (currentDir.exists(iconName + pngext)) {
            PixmapEntry *iconEntry = new PixmapEntry;
            iconEntry->filename = currentDir.filePath(iconName + pngext);
            // Notice we ensure that pixmap entries always come before
            // scalable to preserve search order afterwards
            info.entries.prepend(iconEntry);
        } else if (gSupportsSvg &&
            currentDir.exists(iconName + svgext)) {
            ScalableEntry *iconEntry = new ScalableEntry;
            iconEntry->filename = currentDir.filePath(iconName + svgext);
            info.entries.append(iconEntry);
        } else if (currentDir.exists(iconName + xpmext)) {
            PixmapEntry *iconEntry = new PixmapEntry;
            iconEntry->filename = currentDir.filePath(iconName + xpmext);
            // Notice we ensure that pixmap entries always come before
            // scalable to preserve search order afterwards
            info.entries.append(iconEntry);
        }
    }
    return info;
}

QThemeIconInfo XdgIconLoader::loadIcon(const QString &name) const
{
    const QString theme_name = QIconLoader::instance()->themeName();
    if (!theme_name.isEmpty()) {
        QStringList visited;
        auto info = findIconHelper(theme_name, name, visited, true);
        if (info.entries.isEmpty()) {
            const auto hicolorInfo = findIconHelper(QLatin1String("hicolor"), name, visited, true);
            if (hicolorInfo.entries.isEmpty()) {
                const auto unthemedInfo = unthemedFallback(name, QIcon::themeSearchPaths());
                if (unthemedInfo.entries.isEmpty()) {
                    /* Freedesktop standard says to look in /usr/share/pixmaps last */
                    const QStringList pixmapPath = (QStringList() << QString::fromLatin1("/usr/share/pixmaps"));
                    const auto pixmapInfo = unthemedFallback(name, pixmapPath);
                    if (pixmapInfo.entries.isEmpty()) {
                        return QThemeIconInfo();
                    } else {
                        return pixmapInfo;
                    }
                } else {
                    return unthemedInfo;
                }
            } else {
                return hicolorInfo;
            }
        } else {
            return info;
        }
    }

    return QThemeIconInfo();
}


// -------- Icon Loader Engine -------- //


XdgIconLoaderEngine::XdgIconLoaderEngine(const QString& iconName)
        : m_iconName(iconName), m_key(0)
{
}

XdgIconLoaderEngine::~XdgIconLoaderEngine()
{
    qDeleteAll(m_info.entries);
}

XdgIconLoaderEngine::XdgIconLoaderEngine(const XdgIconLoaderEngine &other)
        : QIconEngine(other),
        m_iconName(other.m_iconName),
        m_key(0)
{
}

QIconEngine *XdgIconLoaderEngine::clone() const
{
    return new XdgIconLoaderEngine(*this);
}

bool XdgIconLoaderEngine::read(QDataStream &in) {
    in >> m_iconName;
    return true;
}

bool XdgIconLoaderEngine::write(QDataStream &out) const
{
    out << m_iconName;
    return true;
}

bool XdgIconLoaderEngine::hasIcon() const
{
    return !(m_info.entries.isEmpty());
}

// Lazily load the icon
void XdgIconLoaderEngine::ensureLoaded()
{
    if (!(QIconLoader::instance()->themeKey() == m_key)) {
        qDeleteAll(m_info.entries);
        m_info.entries.clear();
        m_info.iconName.clear();

        Q_ASSERT(m_info.entries.size() == 0);
        m_info = XdgIconLoader::instance()->loadIcon(m_iconName);
        m_key = QIconLoader::instance()->themeKey();
    }
}

void XdgIconLoaderEngine::paint(QPainter *painter, const QRect &rect,
                             QIcon::Mode mode, QIcon::State state)
{
    QSize pixmapSize = rect.size();
    const qreal dpr = painter->device()->devicePixelRatioF();
    painter->drawPixmap(rect, pixmap(QSizeF(pixmapSize * dpr).toSize(), mode, state));
}

/*
 * This algorithm is defined by the freedesktop spec:
 * http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html
 */
static bool directoryMatchesSize(const QIconDirInfo &dir, int iconsize, int iconscale)
{
    if (dir.scale != iconscale)
        return false;
    if (dir.type == QIconDirInfo::Fixed) {
        return dir.size == iconsize;

    } else if (dir.type == QIconDirInfo::Scalable) {
        return iconsize <= dir.maxSize &&
                iconsize >= dir.minSize;

    } else if (dir.type == QIconDirInfo::Threshold) {
        return iconsize >= dir.size - dir.threshold &&
                iconsize <= dir.size + dir.threshold;
    }

    Q_ASSERT(1); // Not a valid value
    return false;
}

/*
 * This algorithm is defined by the freedesktop spec:
 * http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html
 */
static int directorySizeDistance(const QIconDirInfo &dir, int iconsize, int iconscale)
{
    const int scaledIconSize = iconsize * iconscale;
    if (dir.type == QIconDirInfo::Fixed) {
        return qAbs(dir.size * dir.scale - scaledIconSize);

    } else if (dir.type == QIconDirInfo::Scalable) {
        if (scaledIconSize < dir.minSize * dir.scale)
            return dir.minSize * dir.scale - scaledIconSize;
        else if (scaledIconSize > dir.maxSize * dir.scale)
            return scaledIconSize - dir.maxSize * dir.scale;
        else
            return 0;

    } else if (dir.type == QIconDirInfo::Threshold) {
        if (scaledIconSize < (dir.size - dir.threshold) * dir.scale)
            return dir.minSize * dir.scale - scaledIconSize;
        else if (scaledIconSize > (dir.size + dir.threshold) * dir.scale)
            return scaledIconSize - dir.maxSize * dir.scale;
        else return 0;
    }

    Q_ASSERT(1); // Not a valid value
    return INT_MAX;
}

QIconLoaderEngineEntry *XdgIconLoaderEngine::entryForSize(const QSize &size, int scale)
{
    int iconsize = qMin(size.width(), size.height());

    // Note that m_info.entries are sorted so that png-files
    // come first

    const int numEntries = m_info.entries.size();

    // Search for exact matches first
    for (int i = 0; i < numEntries; ++i) {
        QIconLoaderEngineEntry *entry = m_info.entries.at(i);
        if (directoryMatchesSize(entry->dir, iconsize, scale)) {
            return entry;
        }
    }

    // Find the minimum distance icon
    int minimalSize = INT_MAX;
    QIconLoaderEngineEntry *closestMatch = nullptr;
    for (int i = 0; i < numEntries; ++i) {
        QIconLoaderEngineEntry *entry = m_info.entries.at(i);
        int distance = directorySizeDistance(entry->dir, iconsize, scale);
        if (distance < minimalSize) {
            minimalSize  = distance;
            closestMatch = entry;
        }
    }
    return closestMatch;
}

/*
 * Returns the actual icon size. For scalable svg's this is equivalent
 * to the requested size. Otherwise the closest match is returned but
 * we can never return a bigger size than the requested size.
 *
 */
QSize XdgIconLoaderEngine::actualSize(const QSize &size, QIcon::Mode mode,
                                   QIcon::State state)
{
    Q_UNUSED(mode);
    Q_UNUSED(state);

    ensureLoaded();

    QIconLoaderEngineEntry *entry = entryForSize(size);
    if (entry) {
        const QIconDirInfo &dir = entry->dir;
        if (dir.type == QIconDirInfo::Scalable || dynamic_cast<ScalableEntry *>(entry))
            return size;
        else {
            int dir_size = dir.size;
            //Note: fallback for directories that don't have its content size defined
            //  -> get the actual size based on the image if possible
            PixmapEntry * pix_e;
            if (0 == dir_size && nullptr != (pix_e = dynamic_cast<PixmapEntry *>(entry)))
            {
                QSize pix_size = pix_e->basePixmap.size();
                dir_size = qMin(pix_size.width(), pix_size.height());
            }
            int result = qMin(dir_size, qMin(size.width(), size.height()));
            return {result, result};
        }
    }
    return {0, 0};
}

// XXX: duplicated from qiconloader.cpp, because this symbol isn't exported :(
QPixmap PixmapEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(state);

    // Ensure that basePixmap is lazily initialized before generating the
    // key, otherwise the cache key is not unique
    if (basePixmap.isNull())
        basePixmap.load(filename);

    QSize actualSize = basePixmap.size();
    // If the size of the best match we have (basePixmap) is larger than the
    // requested size, we downscale it to match.
    if (!actualSize.isNull() && (actualSize.width() > size.width() || actualSize.height() > size.height()))
        actualSize.scale(size, Qt::KeepAspectRatio);

    QString key = QLatin1String("$qt_theme_")
                  % HexString<qint64>(basePixmap.cacheKey())
                  % HexString<int>(mode)
                  % HexString<qint64>(QGuiApplication::palette().cacheKey())
                  % HexString<int>(actualSize.width())
                  % HexString<int>(actualSize.height());

    QPixmap cachedPixmap;
    if (QPixmapCache::find(key, &cachedPixmap)) {
        return cachedPixmap;
    } else {
        if (basePixmap.size() != actualSize)
            cachedPixmap = basePixmap.scaled(actualSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        else
            cachedPixmap = basePixmap;
        if (QGuiApplication *guiApp = qobject_cast<QGuiApplication *>(qApp))
            cachedPixmap = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(guiApp))->applyQIconStyleHelper(mode, cachedPixmap);
        QPixmapCache::insert(key, cachedPixmap);
    }
    return cachedPixmap;
}

// XXX: duplicated from qiconloader.cpp, because this symbol isn't exported :(
QPixmap ScalableEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    if (svgIcon.isNull())
        svgIcon = QIcon(filename);

    // Bypass QIcon API, as that will scale by device pixel ratio of the
    // highest DPR screen since we're not passing on any QWindow.
    if (QIconEngine *engine = svgIcon.data_ptr() ? svgIcon.data_ptr()->engine : nullptr)
        return engine->pixmap(size, mode, state);

    return QPixmap();
}

static const QString STYLE = QStringLiteral("\n.ColorScheme-Text, .ColorScheme-NeutralText {color:%1;}\
\n.ColorScheme-Background {color:%2;}\
\n.ColorScheme-Highlight {color:%3;}");
// Note: Qt palette does not have any colors for positive/negative text
// .ColorScheme-PositiveText,ColorScheme-NegativeText {color:%4;}

QPixmap ScalableFollowsColorEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QPixmap pm;
    // see ScalableEntry::pixmap() for the reason
    if (QIconEngine *engine = svgIcon.data_ptr() ? svgIcon.data_ptr()->engine : nullptr)
        pm = engine->pixmap(size, mode, state);

    // Note: not checking the QIcon::isNull(), because in Qt5.10 the isNull() is not reliable
    // for svg icons desierialized from stream (see https://codereview.qt-project.org/#/c/216086/)
    if (pm.isNull())
    {
        // The following lines are adapted and updated from KDE's "kiconloader.cpp" ->
        // KIconLoaderPrivate::processSvg() and KIconLoaderPrivate::createIconImage().
        // They read the SVG color scheme of SVG icons and give images based on the icon mode.
        QHash<int, QByteArray> svg_buffers;
        QFile device{filename};
        if (device.open(QIODevice::ReadOnly))
        {
            const QPalette pal = qApp->palette();
            // Note: indexes are assembled as in qtsvg (QSvgIconEnginePrivate::hashKey())
            QMap<int, QString> style_sheets;
            style_sheets[(QIcon::Normal<<4)|QIcon::Off] = STYLE.arg(pal.windowText().color().name(), pal.window().color().name(), pal.highlight().color().name());
            style_sheets[(QIcon::Selected<<4)|QIcon::Off] = STYLE.arg(pal.highlightedText().color().name(), pal.highlight().color().name(), pal.highlightedText().color().name());
            QMap<int, QSharedPointer<QXmlStreamWriter> > writers;
            for (auto i = style_sheets.cbegin(); i != style_sheets.cend(); ++i)
            {
                writers[i.key()].reset(new QXmlStreamWriter{&svg_buffers[i.key()]});
            }

            QXmlStreamReader xmlReader(&device);
            while (!xmlReader.atEnd())
            {
                if (xmlReader.readNext() == QXmlStreamReader::StartElement
                        && xmlReader.qualifiedName() == QLatin1String("style")
                        && xmlReader.attributes().value(QLatin1String("id")) == QLatin1String("current-color-scheme"))
                {
                    const auto attribs = xmlReader.attributes();
                    // store original data/text of the <style> element
                    QString original_data;
                    while (xmlReader.tokenType() != QXmlStreamReader::EndElement)
                    {
                        if (xmlReader.tokenType() == QXmlStreamReader::Characters)
                            original_data += xmlReader.text();
                        xmlReader.readNext();
                    }
                    for (auto i = style_sheets.cbegin(); i != style_sheets.cend(); ++i)
                    {
                        QXmlStreamWriter & writer = *writers[i.key()];
                        writer.writeStartElement(QLatin1String("style"));
                        writer.writeAttributes(attribs);
                        // Note: We're writting the original style text to leave
                        // there "defaults" for unknown/unsupported classes.
                        // Then appending our "overrides"
                        writer.writeCharacters(original_data);
                        writer.writeCharacters(*i);
                        writer.writeEndElement();
                    }
                } else if (xmlReader.tokenType() != QXmlStreamReader::Invalid)
                {
                    for (auto i = style_sheets.cbegin(); i != style_sheets.cend(); ++i)
                    {
                        writers[i.key()]->writeCurrentToken(xmlReader);
                    }
                }
            }
            // duplicate the contets also for opposite state
            svg_buffers[(QIcon::Normal<<4)|QIcon::On] = svg_buffers[(QIcon::Normal<<4)|QIcon::Off];
            svg_buffers[(QIcon::Selected<<4)|QIcon::On] = svg_buffers[(QIcon::Selected<<4)|QIcon::Off];
        }
        // use the QSvgIconEngine
        //  - assemble the content as it is done by the operator <<(QDataStream &s, const QIcon &icon)
        //  (the QSvgIconEngine::key() + QSvgIconEngine::write())
        //  - create the QIcon from the content by usage of the QIcon::operator >>(QDataStream &s, const QIcon &icon)
        //  (icon with the (QSvgIconEngine) will be used)
        QByteArray icon_arr;
        QDataStream str{&icon_arr, QIODevice::WriteOnly};
        str.setVersion(QDataStream::Qt_4_4);
        QHash<int, QString> filenames;
        filenames[0] = filename; // Note: filenames are ignored in the QSvgIconEngine::read()
        str << QStringLiteral("svg") << filenames << static_cast<int>(0)/*isCompressed*/ << svg_buffers << static_cast<int>(0)/*hasAddedPimaps*/;

        QDataStream str_read{&icon_arr, QIODevice::ReadOnly};
        str_read.setVersion(QDataStream::Qt_4_4);

        str_read >> svgIcon;
        if (QIconEngine *engine = svgIcon.data_ptr() ? svgIcon.data_ptr()->engine : nullptr)
            pm = engine->pixmap(size, mode, state);

        // load the icon directly from file, if still null
        if (pm.isNull())
        {
            svgIcon = QIcon(filename);
            if (QIconEngine *engine = svgIcon.data_ptr() ? svgIcon.data_ptr()->engine : nullptr)
                pm = engine->pixmap(size, mode, state);
        }
    }

    return pm;
}

QPixmap XdgIconLoaderEngine::pixmap(const QSize &size, QIcon::Mode mode,
                                 QIcon::State state)
{
    ensureLoaded();

    QIconLoaderEngineEntry *entry = entryForSize(size);
    if (entry)
        return entry->pixmap(size, mode, state);

    return QPixmap();
}

QString XdgIconLoaderEngine::key() const
{
    return QLatin1String("XdgIconLoaderEngine");
}

void XdgIconLoaderEngine::virtual_hook(int id, void *data)
{
    ensureLoaded();

    switch (id) {
    case QIconEngine::AvailableSizesHook:
        {
            QIconEngine::AvailableSizesArgument &arg
                    = *reinterpret_cast<QIconEngine::AvailableSizesArgument*>(data);
            const int N = m_info.entries.size();
            QList<QSize> sizes;
            sizes.reserve(N);

            // Gets all sizes from the DirectoryInfo entries
            for (int i = 0; i < N; ++i) {
                int size = m_info.entries.at(i)->dir.size;
                sizes.append(QSize(size, size));
            }
            arg.sizes.swap(sizes); // commit
        }
        break;
    case QIconEngine::IconNameHook:
        {
            QString &name = *reinterpret_cast<QString*>(data);
            name = m_info.iconName;
        }
        break;
    case QIconEngine::IsNullHook:
        {
            *reinterpret_cast<bool*>(data) = m_info.entries.isEmpty();
        }
        break;
    case QIconEngine::ScaledPixmapHook:
        {
            QIconEngine::ScaledPixmapArgument &arg = *reinterpret_cast<QIconEngine::ScaledPixmapArgument*>(data);
            // QIcon::pixmap() multiplies size by the device pixel ratio.
            const int integerScale = qCeil(arg.scale);
            QIconLoaderEngineEntry *entry = entryForSize(arg.size / integerScale, integerScale);
            arg.pixmap = entry ? entry->pixmap(arg.size, arg.mode, arg.state) : QPixmap();
        }
        break;
    default:
        QIconEngine::virtual_hook(id, data);
    }
}


//QT_END_NAMESPACE

#endif //QT_NO_ICON
