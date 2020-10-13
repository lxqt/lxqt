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

#include "client_action.h"
#include "client_proxy.h"
#include "log_target.h"


ClientAction::ClientAction(LogTarget *logTarget, const QDBusObjectPath &path, const QString &description)
    : BaseAction(logTarget, description)
    , mProxy(nullptr)
    , mPath(path)
{
}

ClientAction::ClientAction(LogTarget *logTarget, const QDBusConnection &connection, const QString &service, const QDBusObjectPath &path, const QString &description)
    : BaseAction(logTarget, description)
    , mProxy(nullptr)
    , mPath(path)
{
    appeared(connection, service);
}

ClientAction::~ClientAction()
{
    delete mProxy;
}

bool ClientAction::call()
{
    if (!isEnabled())
    {
        return false;
    }

    if (!mProxy)
    {
        mLogTarget->log(LOG_WARNING, "No native client: \"%s\"", qPrintable(mService));
        return false;
    }

    mProxy->emitActivated();

    return true;
}

void ClientAction::appeared(const QDBusConnection &connection, const QString &service)
{
    if (mProxy) // should never happen
    {
        return;
    }
    mService = service;
    mProxy = new ClientProxy(mService, QDBusObjectPath(QStringLiteral("/global_key_shortcuts") + mPath.path()), connection);
}

void ClientAction::disappeared()
{
    mService.clear();
    delete mProxy;
    mProxy = nullptr;
}

void ClientAction::shortcutChanged(const QString &oldShortcut, const QString &newShortcut)
{
    if (mProxy)
    {
        mProxy->emitShortcutChanged(oldShortcut, newShortcut);
    }
}
