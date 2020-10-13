/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "xdgmenu.h"
#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>

#define REBUILD_DELAY 3000

class QDomElement;
class QStringList;
class QString;
class QDomDocument;

class XdgMenuPrivate: QObject
{
Q_OBJECT
public:
    XdgMenuPrivate(XdgMenu* parent);

    void simplify(QDomElement& element);
    void mergeMenus(QDomElement& element);
    void moveMenus(QDomElement& element);
    void deleteDeletedMenus(QDomElement& element);
    void processDirectoryEntries(QDomElement& element, const QStringList& parentDirs);
    void processApps(QDomElement& element);
    void deleteEmpty(QDomElement& element);
    void processLayouts(QDomElement& element);
    void fixSeparators(QDomElement& element);

    bool loadDirectoryFile(const QString& fileName, QDomElement& element);
    void prependChilds(QDomElement& srcElement, QDomElement& destElement);
    void appendChilds(QDomElement& srcElement, QDomElement& destElement);

    void saveLog(const QString& logFileName);
    void load(const QString& fileName);

    void clearWatcher();

    QString mErrorString;
    QStringList mEnvironments;
    QString mMenuFileName;
    QString mLogDir;
    QDomDocument mXml;
    QByteArray mHash;
    QTimer mRebuildDelayTimer;

    QFileSystemWatcher mWatcher;
    bool mOutDated;

public Q_SLOTS:
    void rebuild();

Q_SIGNALS:
    void changed();


private:
    XdgMenu* const q_ptr;
    Q_DECLARE_PUBLIC(XdgMenu)
};
