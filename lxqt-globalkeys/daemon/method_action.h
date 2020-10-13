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

#ifndef GLOBAL_ACTION_DAEMON__METHOD_ACTION__INCLUDED
#define GLOBAL_ACTION_DAEMON__METHOD_ACTION__INCLUDED


#include "base_action.h"

#include <QString>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusMessage>


class MethodAction : public BaseAction
{
public:
    MethodAction(LogTarget *logTarget, const QDBusConnection &connection, const QString &service, const QDBusObjectPath &path, const QString &interface, const QString &method, const QString &description);

    static const char *id() { return "method"; }

    const char *type() const override { return id(); }

    bool call() override;

    QString service() const { return mService; }

    QDBusObjectPath path() const { return mPath; }

    QString interface() const { return mInterface; }

    QString method() const { return mMethodName; }

private:
    QDBusConnection mConnection;
    QString mService;
    QDBusObjectPath mPath;
    QString mInterface;
    QString mMethodName;
};

#endif // GLOBAL_ACTION_DAEMON__METHOD_ACTION__INCLUDED
