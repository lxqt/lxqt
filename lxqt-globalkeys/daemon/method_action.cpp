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

#include "method_action.h"
#include "log_target.h"


MethodAction::MethodAction(LogTarget *logTarget, const QDBusConnection &connection, const QString &service, const QDBusObjectPath &path, const QString &interface, const QString &method, const QString &description)
    : BaseAction(logTarget, description)
    , mConnection(connection)
    , mService(service)
    , mPath(path)
    , mInterface(interface)
    , mMethodName(method)
{
}

bool MethodAction::call()
{
    if (!isEnabled())
    {
        return false;
    }

    bool result = mConnection.call(QDBusMessage::createMethodCall(mService, mPath.path(), mInterface, mMethodName), QDBus::BlockWithGui).type() == QDBusMessage::ReplyMessage;
    if (!result)
    {
        mLogTarget->log(LOG_WARNING, "Failed to call dbus method: service:'%s' path:'%s' interface:'%s' method:'%s'", qPrintable(mService), qPrintable(mPath.path()), qPrintable(mInterface), qPrintable(mMethodName));
    }

    return result;
}
