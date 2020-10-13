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

#ifndef GLOBAL_ACTION_DAEMON__DBUS_ACTION__INCLUDED
#define GLOBAL_ACTION_DAEMON__DBUS_ACTION__INCLUDED


#include "base_action.h"

#include <QString>
#include <QDBusObjectPath>
#include <QDBusConnection>


class ClientProxy;

class ClientAction : public BaseAction
{
public:
    ClientAction(LogTarget *logTarget, const QDBusObjectPath &path, const QString &description);
    ClientAction(LogTarget *logTarget, const QDBusConnection &connection, const QString &service, const QDBusObjectPath &path, const QString &description);
    ~ClientAction() override;

    static const char *id() { return "client"; }

    const char *type() const override { return id(); }

    bool call() override;

    void shortcutChanged(const QString &oldShortcut, const QString &newShortcut);

    const QString &service() const { return mService; }
    const QDBusObjectPath &path() const { return mPath; }

    void appeared(const QDBusConnection &connection, const QString &service);
    void disappeared();

    bool isPresent() const { return mProxy; }

private:
    ClientProxy *mProxy;

    QString mService;
    QDBusObjectPath mPath;
};

#endif // GLOBAL_ACTION_DAEMON__DBUS_ACTION__INCLUDED
