/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2010-2011 LXQt team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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

#include "windowmanager.h"

#include <QObject>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <LXQt/Globals>
#include <LXQt/Settings>
#include <QDebug>


bool findProgram(const QString &program)
{
    QFileInfo fi(program);
    if (fi.isExecutable())
        return true;

    const QStringList paths = QFile::decodeName(qgetenv("PATH")).split(QL1C(':'));
    for(const QString &dir : paths)
    {
        QFileInfo fi= QFileInfo(dir + QDir::separator() + program);
        if (fi.isExecutable() )
            return true;
    }
    return false;
}

WindowManagerList getWindowManagerList(bool onlyAvailable)
{
    LXQt::Settings cfg(QSL("windowmanagers"));
    cfg.beginGroup(QSL("KnownManagers"));
    const QStringList names = cfg.childGroups();

    WindowManagerList ret;

    for (const QString &name : names)
    {
        bool exists = findProgram(name);
        if (!onlyAvailable || exists)
        {
            cfg.beginGroup(name);
            WindowManager wm;
            wm.command = name;
            wm.name = cfg.localizedValue(QSL("Name"), wm.command).toString();
            wm.comment = cfg.localizedValue(QSL("Comment")).toString();
            wm.exists = exists;
            ret << wm;
            cfg.endGroup();
        }
    }

    return ret;
}
