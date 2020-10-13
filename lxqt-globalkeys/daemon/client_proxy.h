/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
 * Authors:
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#ifndef GLOBAL_ACTION_DAEMON__CLIENT_PROXY__INCLUDED
#define GLOBAL_ACTION_DAEMON__CLIENT_PROXY__INCLUDED


#include <QObject>
#include <QDBusObjectPath>
#include <QDBusConnection>


class ClientAction;

class ClientProxy : public QObject
{
    Q_OBJECT

    friend class ClientAction;

public:
    ClientProxy(const QString &service, const QDBusObjectPath &path, const QDBusConnection &connection, QObject *parent = nullptr);

signals:
    void activated();
    void shortcutChanged(const QString &oldShortcut, const QString &newShortcut);

protected:
    void emitActivated();
    void emitShortcutChanged(const QString &oldShortcut, const QString &newShortcut);
};

#endif // GLOBAL_ACTION_DAEMON__CLIENT_PROXY__INCLUDED
