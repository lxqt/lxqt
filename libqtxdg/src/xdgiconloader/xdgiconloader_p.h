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

#ifndef XDGICONLOADER_P_H
#define XDGICONLOADER_P_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_ICON
//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <xdgiconloader_export.h>

#include <QtGui/QIcon>
#include <QtGui/QIconEngine>
#include <private/qicon_p.h>
#include <private/qiconloader_p.h>
#include <QtCore/QHash>
#include <QtCore/QVector>

//QT_BEGIN_NAMESPACE

class XdgIconLoader;

struct ScalableFollowsColorEntry : public ScalableEntry
{
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE;
};

//class QIconLoaderEngine : public QIconEngine
class XDGICONLOADER_EXPORT XdgIconLoaderEngine : public QIconEngine
{
public:
    XdgIconLoaderEngine(const QString& iconName = QString());
    ~XdgIconLoaderEngine() override;

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE;
    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE;
    QIconEngine *clone() const Q_DECL_OVERRIDE;
    bool read(QDataStream &in) Q_DECL_OVERRIDE;
    bool write(QDataStream &out) const Q_DECL_OVERRIDE;

private:
    QString key() const Q_DECL_OVERRIDE;
    bool hasIcon() const;
    void ensureLoaded();
    void virtual_hook(int id, void *data) Q_DECL_OVERRIDE;
    QIconLoaderEngineEntry *entryForSize(const QSize &size, int scale = 1);
    XdgIconLoaderEngine(const XdgIconLoaderEngine &other);
    QThemeIconInfo m_info;
    QString m_iconName;
    uint m_key;

    friend class XdgIconLoader;
};

class QIconCacheGtkReader;

// Note: We can't simply reuse the QIconTheme from Qt > 5.7 because
// the QIconTheme constructor symbol isn't exported.
class XdgIconTheme
{
public:
    XdgIconTheme(const QString &name);
    XdgIconTheme() = default;
    QStringList parents() { return m_parents; }
    QVector <QIconDirInfo> keyList() { return m_keyList; }
    QStringList contentDirs() { return m_contentDirs; }
    bool isValid() const { return m_valid; }
    bool followsColorScheme() const { return m_followsColorScheme; }
private:
    QStringList m_contentDirs;
    QVector <QIconDirInfo> m_keyList;
    QStringList m_parents;
    bool m_valid = false;
    bool m_followsColorScheme = false;
public:
    QVector<QSharedPointer<QIconCacheGtkReader>> m_gtkCaches;
};

class XDGICONLOADER_EXPORT XdgIconLoader
{
public:
    QThemeIconInfo loadIcon(const QString &iconName) const;

    /* TODO: deprecate & remove all QIconLoader wrappers */
    inline uint themeKey() const { return QIconLoader::instance()->themeKey(); }
    inline QString themeName() const { return QIconLoader::instance()->themeName(); }
    inline void setThemeName(const QString &themeName) { QIconLoader::instance()->setThemeName(themeName); }
    inline void setThemeSearchPath(const QStringList &searchPaths) { QIconLoader::instance()->setThemeSearchPath(searchPaths); }
    inline QIconDirInfo dirInfo(int dirindex) { return QIconLoader::instance()->dirInfo(dirindex); }
    inline QStringList themeSearchPaths() const { return QIconLoader::instance()->themeSearchPaths(); }
    inline void updateSystemTheme() { QIconLoader::instance()->updateSystemTheme(); }
    inline void invalidateKey() { QIconLoader::instance()->invalidateKey(); }
    inline void ensureInitialized() { QIconLoader::instance()->ensureInitialized(); }
    inline bool hasUserTheme() const { return QIconLoader::instance()->hasUserTheme(); }
    /*!
     * Flag if the "FollowsColorScheme" hint (the KDE extension to XDG
     * themes) should be honored.
     */
    inline bool followColorScheme() const { return m_followColorScheme; }
    void setFollowColorScheme(bool enable);

    XdgIconTheme theme() { return themeList.value(QIconLoader::instance()->themeName()); }
    static XdgIconLoader *instance();

private:
    QThemeIconInfo findIconHelper(const QString &themeName,
                                  const QString &iconName,
                                  QStringList &visited,
                                  bool dashFallback = false) const;
    QThemeIconInfo unthemedFallback(const QString &iconName, const QStringList &searchPaths) const;
    mutable QHash <QString, XdgIconTheme> themeList;
    bool m_followColorScheme = true;
};

#endif // QT_NO_ICON

#endif // XDGICONLOADER_P_H
