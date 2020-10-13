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

#include "native_adaptor.h"

#include "org.lxqt.global_key_shortcuts.native.h"


NativeAdaptor::NativeAdaptor(QObject *parent)
    : QObject(parent)
    , QDBusContext()
{
    new OrgLxqtGlobalActionNativeAdaptor(this);
}

QString NativeAdaptor::addClientAction(const QString &shortcut, const QDBusObjectPath &path, const QString &description, qulonglong &id)
{
    QPair<QString, qulonglong> result;
    emit onAddClientAction(result, shortcut, path, description, calledFromDBus() ? message().service() : QString());
    QString usedShortcut = result.first;
    id = result.second;
    return usedShortcut;
}

bool NativeAdaptor::modifyClientAction(const QDBusObjectPath &path, const QString &description)
{
    qulonglong result;
    emit onModifyClientAction(result, path, description, calledFromDBus() ? message().service() : QString());
    return result;
}

QString NativeAdaptor::changeClientActionShortcut(const QDBusObjectPath &path, const QString &shortcut)
{
    QPair<QString, qulonglong> result;
    emit onChangeClientActionShortcut(result, path, shortcut, calledFromDBus() ? message().service() : QString());
    QString usedShortcut = result.first;
    return usedShortcut;
}

bool NativeAdaptor::removeClientAction(const QDBusObjectPath &path)
{
    bool result;
    emit onRemoveClientAction(result, path, calledFromDBus() ? message().service() : QString());
    return result;
}

bool NativeAdaptor::deactivateClientAction(const QDBusObjectPath &path)
{
    bool result;
    emit onDeactivateClientAction(result, path, calledFromDBus() ? message().service() : QString());
    return result;
}

bool NativeAdaptor::enableClientAction(const QDBusObjectPath &path, bool enabled)
{
    bool result;
    emit onEnableClientAction(result, path, enabled, calledFromDBus() ? message().service() : QString());
    return result;
}

bool NativeAdaptor::isClientActionEnabled(const QDBusObjectPath &path)
{
    bool enabled;
    emit onIsClientActionEnabled(enabled, path, calledFromDBus() ? message().service() : QString());
    return enabled;
}

QString NativeAdaptor::grabShortcut(uint timeout, bool &failed, bool &cancelled, bool &timedout)
{
    QString shortcut;
    emit onGrabShortcut(timeout, shortcut, failed, cancelled, timedout, message());
    return shortcut;
}

void NativeAdaptor::cancelShortcutGrab()
{
    emit onCancelShortcutGrab();
}
