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

#include "daemon_adaptor.h"

#include "org.lxqt.global_key_shortcuts.daemon.h"


DaemonAdaptor::DaemonAdaptor(QObject *parent)
    : QObject(parent)
    , QDBusContext()
{
    new OrgLxqtGlobalActionDaemonAdaptor(this);
}

QString DaemonAdaptor::addMethodAction(const QString &shortcut, const QString &service, const QDBusObjectPath &path, const QString &interface, const QString &method, const QString &description, qulonglong &id)
{
    QPair<QString, qulonglong> result;
    emit onAddMethodAction(result, shortcut, service, path, interface, method, description);
    QString usedShortcut = result.first;
    id = result.second;
    if (id)
    {
        emit actionAdded(id);
    }
    return usedShortcut;
}

QString DaemonAdaptor::addCommandAction(const QString &shortcut, const QString &command, const QStringList &arguments, const QString &description, qulonglong &id)
{
    QPair<QString, qulonglong> result;
    emit onAddCommandAction(result, shortcut, command, arguments, description);
    QString usedShortcut = result.first;
    id = result.second;
    if (id)
    {
        emit actionAdded(id);
    }
    return usedShortcut;
}

bool DaemonAdaptor::modifyActionDescription(qulonglong id, const QString &description)
{
    bool result;
    emit onModifyActionDescription(result, id, description);
    if (result)
    {
        emit actionModified(id);
    }
    return result;
}

bool DaemonAdaptor::modifyMethodAction(qulonglong id, const QString &service, const QDBusObjectPath &path, const QString &interface, const QString &method, const QString &description)
{
    bool result;
    emit onModifyMethodAction(result, id, service, path, interface, method, description);
    if (result)
    {
        emit actionModified(id);
    }
    return result;
}

bool DaemonAdaptor::modifyCommandAction(qulonglong id, const QString &command, const QStringList &arguments, const QString &description)
{
    bool result;
    emit onModifyCommandAction(result, id, command, arguments, description);
    if (result)
    {
        emit actionModified(id);
    }
    return result;
}

bool DaemonAdaptor::enableAction(qulonglong id, bool enabled)
{
    bool result;
    emit onEnableAction(result, id, enabled);
    if (result)
    {
        emit actionEnabled(id, enabled);
    }
    return result;
}

bool DaemonAdaptor::isActionEnabled(qulonglong id)
{
    bool enabled;
    emit onIsActionEnabled(enabled, id);
    return enabled;
}

QString DaemonAdaptor::getClientActionSender(qulonglong id)
{
    QString sender;
    emit onGetClientActionSender(sender, id);
    return sender;
}

QString DaemonAdaptor::changeShortcut(qulonglong id, const QString &shortcut)
{
    QString result;
    emit onChangeShortcut(result, id, shortcut);
    if (!result.isEmpty())
    {
        emit actionShortcutChanged(id);
    }
    return result;
}

bool DaemonAdaptor::swapActions(qulonglong id1, qulonglong id2)
{
    bool result;
    emit onSwapActions(result, id1, id2);
    if (result)
    {
        emit actionsSwapped(id1, id2);
    }
    return result;
}

bool DaemonAdaptor::removeAction(qulonglong id)
{
    bool result;
    emit onRemoveAction(result, id);
    if (result)
    {
        emit actionRemoved(id);
    }
    return result;
}

bool DaemonAdaptor::setMultipleActionsBehaviour(uint behaviour)
{
    if (behaviour >= MULTIPLE_ACTIONS_BEHAVIOUR__COUNT)
    {
        return false;
    }
    emit onSetMultipleActionsBehaviour(static_cast<MultipleActionsBehaviour>(behaviour));
    emit multipleActionsBehaviourChanged(behaviour);
    return true;
}

uint DaemonAdaptor::getMultipleActionsBehaviour()
{
    MultipleActionsBehaviour result;
    emit onGetMultipleActionsBehaviour(result);
    return result;
}

QList<qulonglong> DaemonAdaptor::getAllActionIds()
{
    QList<qulonglong> result;
    emit onGetAllActionIds(result);
    return result;
}

bool DaemonAdaptor::getActionById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &type, QString &info)
{
    QPair<bool, GeneralActionInfo> result;
    emit onGetActionById(result, id);
    bool success = result.first;
    if (success)
    {
        shortcut = result.second.shortcut;
        description = result.second.description;
        enabled = result.second.enabled;
        type = result.second.type;
        info = result.second.info;
    }
    return success;
}

QMap<qulonglong, GeneralActionInfo> DaemonAdaptor::getAllActions()
{
    QMap<qulonglong, GeneralActionInfo> result;
    emit onGetAllActions(result);
    return result;
}

bool DaemonAdaptor::getClientActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QDBusObjectPath &path)
{
    QPair<bool, ClientActionInfo> result;
    emit onGetClientActionInfoById(result, id);
    bool success = result.first;
    if (success)
    {
        shortcut = result.second.shortcut;
        description = result.second.description;
        enabled = result.second.enabled;
        path = result.second.path;
    }
    return success;
}

bool DaemonAdaptor::getMethodActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &service, QDBusObjectPath &path, QString &interface, QString &method)
{
    QPair<bool, MethodActionInfo> result;
    emit onGetMethodActionInfoById(result, id);
    bool success = result.first;
    if (success)
    {
        shortcut = result.second.shortcut;
        description = result.second.description;
        enabled = result.second.enabled;
        service = result.second.service;
        path = result.second.path;
        interface = result.second.interface;
        method = result.second.method;
    }
    return success;
}

bool DaemonAdaptor::getCommandActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &command, QStringList &arguments)
{
    QPair<bool, CommandActionInfo> result;
    emit onGetCommandActionInfoById(result, id);
    bool success = result.first;
    if (success)
    {
        shortcut = result.second.shortcut;
        description = result.second.description;
        enabled = result.second.enabled;
        command = result.second.command;
        arguments = result.second.arguments;
    }
    return success;
}

QString DaemonAdaptor::grabShortcut(uint timeout, bool &failed, bool &cancelled, bool &timedout)
{
    QString shortcut;
    emit onGrabShortcut(timeout, shortcut, failed, cancelled, timedout, message());
    return shortcut;
}

void DaemonAdaptor::cancelShortcutGrab()
{
    emit onCancelShortcutGrab();
}

void DaemonAdaptor::quit()
{
    emit onQuit();
}

void DaemonAdaptor::emit_actionAdded(qulonglong id)
{
    emit actionAdded(id);
}

void DaemonAdaptor::emit_actionModified(qulonglong id)
{
    emit actionModified(id);
}

void DaemonAdaptor::emit_actionRemoved(qulonglong id)
{
    emit actionRemoved(id);
}

void DaemonAdaptor::emit_actionShortcutChanged(qulonglong id)
{
    emit actionShortcutChanged(id);
}

void DaemonAdaptor::emit_actionEnabled(qulonglong id, bool enabled)
{
    emit actionEnabled(id, enabled);
}

void DaemonAdaptor::emit_clientActionSenderChanged(qulonglong id, const QString &sender)
{
    emit clientActionSenderChanged(id, sender);
}
