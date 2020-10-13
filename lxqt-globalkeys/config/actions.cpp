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

#include "actions.h"

#include "org.lxqt.global_key_shortcuts.daemon.h"

Actions::Actions(QObject *parent)
    : QObject(parent)
    , mServiceWatcher(new QDBusServiceWatcher(QLatin1String("org.lxqt.global_key_shortcuts"), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this))
    , mMultipleActionsBehaviour(MULTIPLE_ACTIONS_BEHAVIOUR_FIRST)
{
    connect(mServiceWatcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(on_daemonDisappeared(QString)));
    connect(mServiceWatcher, SIGNAL(serviceRegistered(QString)), this, SLOT(on_daemonAppeared(QString)));
    mDaemonProxy = new org::lxqt::global_key_shortcuts::daemon(QLatin1String("org.lxqt.global_key_shortcuts"), QStringLiteral("/daemon"), QDBusConnection::sessionBus(), this);

    connect(mDaemonProxy, SIGNAL(actionAdded(qulonglong)), this, SLOT(on_actionAdded(qulonglong)));
    connect(mDaemonProxy, SIGNAL(actionEnabled(qulonglong, bool)), this, SLOT(on_actionEnabled(qulonglong, bool)));
    connect(mDaemonProxy, SIGNAL(clientActionSenderChanged(qulonglong, QString)), this, SLOT(on_clientActionSenderChanged(qulonglong, QString)));
    connect(mDaemonProxy, SIGNAL(actionModified(qulonglong)), this, SLOT(on_actionModified(qulonglong)));
    connect(mDaemonProxy, SIGNAL(actionRemoved(qulonglong)), this, SLOT(on_actionRemoved(qulonglong)));
    connect(mDaemonProxy, SIGNAL(actionShortcutChanged(qulonglong)), this, SLOT(on_actionShortcutChanged(qulonglong)));
    connect(mDaemonProxy, SIGNAL(actionsSwapped(qulonglong, qulonglong)), this, SLOT(on_actionsSwapped(qulonglong, qulonglong)));
    connect(mDaemonProxy, SIGNAL(multipleActionsBehaviourChanged(uint)), this, SLOT(on_multipleActionsBehaviourChanged(uint)));

    QTimer::singleShot(0, this, SLOT(delayedInit()));
}

Actions::~Actions()
{
}

void Actions::delayedInit()
{
    if (mDaemonProxy->isValid())
    {
        on_daemonAppeared(QString());
    }
}

void Actions::on_daemonDisappeared(const QString &)
{
    clear();
    emit daemonDisappeared();
}

void Actions::on_daemonAppeared(const QString &)
{
    init();
    emit daemonAppeared();
}

void Actions::init()
{
    clear();

    mGeneralActionInfo = getAllActions();
    GeneralActionInfos::const_iterator M = mGeneralActionInfo.constEnd();
    for (GeneralActionInfos::const_iterator I = mGeneralActionInfo.constBegin(); I != M; ++I)
    {
        if (I.value().type == QLatin1String("client"))
        {
            QString shortcut;
            QString description;
            bool enabled = false;
            QDBusObjectPath path;
            if (getClientActionInfoById(I.key(), shortcut, description, enabled, path))
            {
                ClientActionInfo info;
                info.shortcut = shortcut;
                info.description = description;
                info.enabled = enabled;
                info.path = path;
                mClientActionInfo[I.key()] = info;

                updateClientActionSender(I.key());
            }
        }
        else if (I.value().type == QLatin1String("method"))
        {
            QString shortcut;
            QString description;
            bool enabled = false;
            QString service;
            QDBusObjectPath path;
            QString interface;
            QString method;
            if (getMethodActionInfoById(I.key(), shortcut, description, enabled, service, path, interface, method))
            {
                MethodActionInfo info;
                info.shortcut = shortcut;
                info.description = description;
                info.enabled = enabled;
                info.service = service;
                info.path = path;
                info.interface = interface;
                info.method = method;
                mMethodActionInfo[I.key()] = info;
            }
        }
        else if (I.value().type == QLatin1String("command"))
        {
            QString shortcut;
            QString description;
            bool enabled = false;
            QString command;
            QStringList arguments;
            if (getCommandActionInfoById(I.key(), shortcut, description, enabled, command, arguments))
            {
                CommandActionInfo info;
                info.shortcut = shortcut;
                info.description = description;
                info.enabled = enabled;
                info.command = command;
                info.arguments = arguments;
                mCommandActionInfo[I.key()] = info;
            }
        }
    }

    mMultipleActionsBehaviour = static_cast<MultipleActionsBehaviour>(getMultipleActionsBehaviour());
}

void Actions::clear()
{
    mGeneralActionInfo.clear();
    mClientActionInfo.clear();
    mMethodActionInfo.clear();
    mCommandActionInfo.clear();
    mMultipleActionsBehaviour = MULTIPLE_ACTIONS_BEHAVIOUR_FIRST;
}

QList<qulonglong> Actions::allActionIds() const
{
    return mGeneralActionInfo.keys();
}

QPair<bool, GeneralActionInfo> Actions::actionById(qulonglong id) const
{
    GeneralActionInfos::const_iterator I = mGeneralActionInfo.constFind(id);
    if (I == mGeneralActionInfo.constEnd())
    {
        return qMakePair(false, GeneralActionInfo());
    }
    return qMakePair(true, I.value());
}

QList<qulonglong> Actions::allClientActionIds() const
{
    return mClientActionInfo.keys();
}

QPair<bool, ClientActionInfo> Actions::clientActionInfoById(qulonglong id) const
{
    ClientActionInfos::const_iterator I = mClientActionInfo.constFind(id);
    if (I == mClientActionInfo.constEnd())
    {
        return qMakePair(false, ClientActionInfo());
    }
    return qMakePair(true, I.value());
}

QList<qulonglong> Actions::allMethodActionIds() const
{
    return mMethodActionInfo.keys();
}

QPair<bool, MethodActionInfo> Actions::methodActionInfoById(qulonglong id) const
{
    MethodActionInfos::const_iterator I = mMethodActionInfo.constFind(id);
    if (I == mMethodActionInfo.constEnd())
    {
        return qMakePair(false, MethodActionInfo());
    }
    return qMakePair(true, I.value());
}

QList<qulonglong> Actions::allCommandActionIds() const
{
    return mCommandActionInfo.keys();
}

QPair<bool, CommandActionInfo> Actions::commandActionInfoById(qulonglong id) const
{
    CommandActionInfos::const_iterator I = mCommandActionInfo.constFind(id);
    if (I == mCommandActionInfo.constEnd())
    {
        return qMakePair(false, CommandActionInfo());
    }
    return qMakePair(true, I.value());
}

MultipleActionsBehaviour Actions::multipleActionsBehaviour() const
{
    return mMultipleActionsBehaviour;
}

void Actions::do_actionAdded(qulonglong id)
{
    QString shortcut;
    QString description;
    bool enabled = false;
    QString type;
    QString info;
    if (getActionById(id, shortcut, description, enabled, type, info))
    {
        GeneralActionInfo generalActionInfo;
        generalActionInfo.shortcut = shortcut;
        generalActionInfo.description = description;
        generalActionInfo.enabled = enabled;
        generalActionInfo.type = type;
        generalActionInfo.info = info;
        mGeneralActionInfo[id] = generalActionInfo;
    }

    if (type == QLatin1String("client"))
    {
        QDBusObjectPath path;
        if (getClientActionInfoById(id, shortcut, description, enabled, path))
        {
            ClientActionInfo clientActionInfo;
            clientActionInfo.shortcut = shortcut;
            clientActionInfo.description = description;
            clientActionInfo.enabled = enabled;
            clientActionInfo.path = path;
            mClientActionInfo[id] = clientActionInfo;
        }
    }
    else if (type == QLatin1String("method"))
    {
        QString service;
        QDBusObjectPath path;
        QString interface;
        QString method;
        if (getMethodActionInfoById(id, shortcut, description, enabled, service, path, interface, method))
        {
            MethodActionInfo methodActionInfo;
            methodActionInfo.shortcut = shortcut;
            methodActionInfo.description = description;
            methodActionInfo.enabled = enabled;
            methodActionInfo.service = service;
            methodActionInfo.path = path;
            methodActionInfo.interface = interface;
            methodActionInfo.method = method;
            mMethodActionInfo[id] = methodActionInfo;
        }
    }
    else if (type == QLatin1String("command"))
    {
        QString command;
        QStringList arguments;
        if (getCommandActionInfoById(id, shortcut, description, enabled, command, arguments))
        {
            CommandActionInfo commandActionInfo;
            commandActionInfo.shortcut = shortcut;
            commandActionInfo.description = description;
            commandActionInfo.enabled = enabled;
            commandActionInfo.command = command;
            commandActionInfo.arguments = arguments;
            mCommandActionInfo[id] = commandActionInfo;
        }
    }
}

void Actions::on_actionAdded(qulonglong id)
{
    do_actionAdded(id);
    emit actionAdded(id);
}

void Actions::on_actionEnabled(qulonglong id, bool enabled)
{
    GeneralActionInfos::iterator GI = mGeneralActionInfo.find(id);
    if (GI != mGeneralActionInfo.end())
    {
        GI.value().enabled = enabled;

        if (GI.value().type == QLatin1String("client"))
        {
            ClientActionInfos::iterator DI = mClientActionInfo.find(id);
            if (DI != mClientActionInfo.end())
            {
                DI.value().enabled = enabled;
            }
        }
        else if (GI.value().type == QLatin1String("method"))
        {
            MethodActionInfos::iterator MI = mMethodActionInfo.find(id);
            if (MI != mMethodActionInfo.end())
            {
                MI.value().enabled = enabled;
            }
        }
        else if (GI.value().type == QLatin1String("command"))
        {
            CommandActionInfos::iterator CI = mCommandActionInfo.find(id);
            if (CI != mCommandActionInfo.end())
            {
                CI.value().enabled = enabled;
            }
        }
    }
    emit actionEnabled(id, enabled);
}

void Actions::on_clientActionSenderChanged(qulonglong id, const QString &sender)
{
    mClientActionSenders[id] = sender;
    emit actionModified(id);
}

void Actions::on_actionModified(qulonglong id)
{
    do_actionAdded(id);
    emit actionModified(id);
}

void Actions::on_actionShortcutChanged(qulonglong id)
{
    do_actionAdded(id);
    emit actionModified(id);
}

void Actions::on_actionsSwapped(qulonglong id1, qulonglong id2)
{
    GeneralActionInfos::iterator GI1 = mGeneralActionInfo.find(id1);
    GeneralActionInfos::iterator GI2 = mGeneralActionInfo.find(id2);
    if ((GI1 != mGeneralActionInfo.end()) && (GI2 != mGeneralActionInfo.end()))
    {
        bool swapped = false;

        if (GI1.value().type == GI2.value().type)
        {
            if (GI1.value().type == QLatin1String("client"))
            {
                ClientActionInfos::iterator DI1 = mClientActionInfo.find(id1);
                ClientActionInfos::iterator DI2 = mClientActionInfo.find(id2);
                if ((DI1 != mClientActionInfo.end()) && (DI2 != mClientActionInfo.end()))
                {
                    ClientActionInfo clientActionInfo = DI1.value();
                    DI1.value() = DI2.value();
                    DI2.value() = clientActionInfo;
                    swapped = true;
                }
            }
            else if (GI1.value().type == QLatin1String("method"))
            {
                MethodActionInfos::iterator MI1 = mMethodActionInfo.find(id1);
                MethodActionInfos::iterator MI2 = mMethodActionInfo.find(id2);
                if ((MI1 != mMethodActionInfo.end()) && (MI2 != mMethodActionInfo.end()))
                {
                    MethodActionInfo methodActionInfo = MI1.value();
                    MI1.value() = MI2.value();
                    MI2.value() = methodActionInfo;
                    swapped = true;
                }
            }
            else if (GI1.value().type == QLatin1String("command"))
            {
                CommandActionInfos::iterator CI1 = mCommandActionInfo.find(id1);
                CommandActionInfos::iterator CI2 = mCommandActionInfo.find(id2);
                if ((CI1 != mCommandActionInfo.end()) && (CI2 != mCommandActionInfo.end()))
                {
                    CommandActionInfo commandActionInfo = CI1.value();
                    CI1.value() = CI2.value();
                    CI2.value() = commandActionInfo;
                    swapped = true;
                }
            }
        }

        if (swapped)
        {
            GeneralActionInfo generalActionInfo = GI1.value();
            GI1.value() = GI2.value();
            GI2.value() = generalActionInfo;
        }
        else
        {
            do_actionRemoved(id1);
            do_actionRemoved(id2);
            do_actionAdded(id1);
            do_actionAdded(id2);
        }
    }
    emit actionsSwapped(id1, id2);
}

void Actions::do_actionRemoved(qulonglong id)
{
    mGeneralActionInfo.remove(id);
    mClientActionInfo.remove(id);
    mMethodActionInfo.remove(id);
    mCommandActionInfo.remove(id);
}

void Actions::on_actionRemoved(qulonglong id)
{
    do_actionRemoved(id);
    emit actionRemoved(id);
}

void Actions::on_multipleActionsBehaviourChanged(uint behaviour)
{
    mMultipleActionsBehaviour = static_cast<MultipleActionsBehaviour>(behaviour);
    emit multipleActionsBehaviourChanged(mMultipleActionsBehaviour);
}

bool Actions::getClientActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QDBusObjectPath &path)
{
    return mDaemonProxy->getClientActionInfoById(id, shortcut, description, enabled, path);
}

bool Actions::getMethodActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &service, QDBusObjectPath &path, QString &interface, QString &method)
{
    return mDaemonProxy->getMethodActionInfoById(id, shortcut, description, enabled, service, path, interface, method);
}

bool Actions::getCommandActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &command, QStringList &arguments)
{
    return mDaemonProxy->getCommandActionInfoById(id, shortcut, description, enabled, command, arguments);
}

QList<qulonglong> Actions::getAllActionIds()
{
    QDBusPendingReply<QList<qulonglong> > reply = mDaemonProxy->getAllActionIds();
    reply.waitForFinished();
    if (reply.isError())
    {
        return QList<qulonglong>();
    }

    return reply.argumentAt<0>();
}

bool Actions::getActionById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &type, QString &info)
{
    return mDaemonProxy->getActionById(id, shortcut, description, enabled, type, info);
}

QMap<qulonglong, GeneralActionInfo> Actions::getAllActions()
{
    QDBusPendingReply<QMap<qulonglong, GeneralActionInfo> > reply = mDaemonProxy->getAllActions();
    reply.waitForFinished();
    if (reply.isError())
    {
        return QMap<qulonglong, GeneralActionInfo>();
    }

    return reply.argumentAt<0>();
}

uint Actions::getMultipleActionsBehaviour()
{
    QDBusPendingReply<uint> reply = mDaemonProxy->getMultipleActionsBehaviour();
    reply.waitForFinished();
    if (reply.isError())
    {
        return 0;
    }

    return reply.argumentAt<0>();
}

QPair<QString, qulonglong> Actions::addMethodAction(const QString &shortcut, const QString &service, const QDBusObjectPath &path, const QString &interface, const QString &method, const QString &description)
{
    QDBusPendingReply<QString, qulonglong> reply = mDaemonProxy->addMethodAction(shortcut, service, path, interface, method, description);
    reply.waitForFinished();
    if (reply.isError())
    {
        return qMakePair<QString, qulonglong>(QString(), 0ull);
    }

    return qMakePair<QString, qulonglong>(reply.argumentAt<0>(), reply.argumentAt<1>());
}

QPair<QString, qulonglong> Actions::addCommandAction(const QString &shortcut, const QString &command, const QStringList &arguments, const QString &description)
{
    QDBusPendingReply<QString, qulonglong> reply = mDaemonProxy->addCommandAction(shortcut, command, arguments, description);
    reply.waitForFinished();
    if (reply.isError())
    {
        return qMakePair<QString, qulonglong>(QString(), 0ull);
    }

    return qMakePair<QString, qulonglong>(reply.argumentAt<0>(), reply.argumentAt<1>());
}

bool Actions::modifyActionDescription(const qulonglong &id, const QString &description)
{
    QDBusPendingReply<bool> reply = mDaemonProxy->modifyActionDescription(id, description);
    reply.waitForFinished();
    if (reply.isError())
    {
        return false;
    }

    return reply.argumentAt<0>();
}

bool Actions::modifyMethodAction(const qulonglong &id, const QString &service, const QDBusObjectPath &path, const QString &interface, const QString &method, const QString &description)
{
    QDBusPendingReply<bool> reply = mDaemonProxy->modifyMethodAction(id, service, path, interface, method, description);
    reply.waitForFinished();
    if (reply.isError())
    {
        return false;
    }

    return reply.argumentAt<0>();
}

bool Actions::modifyCommandAction(const qulonglong &id, const QString &command, const QStringList &arguments, const QString &description)
{
    QDBusPendingReply<bool> reply = mDaemonProxy->modifyCommandAction(id, command, arguments, description);
    reply.waitForFinished();
    if (reply.isError())
    {
        return false;
    }

    return reply.argumentAt<0>();
}

bool Actions::enableAction(qulonglong id, bool enabled)
{
    QDBusPendingReply<bool> reply = mDaemonProxy->enableAction(id, enabled);
    reply.waitForFinished();
    if (reply.isError())
    {
        return false;
    }

    return reply.argumentAt<0>();
}

bool Actions::isActionEnabled(qulonglong id)
{
    QDBusPendingReply<bool> reply = mDaemonProxy->isActionEnabled(id);
    reply.waitForFinished();
    if (reply.isError())
    {
        return false;
    }

    return reply.argumentAt<0>();
}

QString Actions::getClientActionSender(qulonglong id)
{
    return mClientActionSenders[id];
}

QString Actions::updateClientActionSender(qulonglong id)
{
    QDBusPendingReply<QString> reply = mDaemonProxy->getClientActionSender(id);
    reply.waitForFinished();
    if (reply.isError())
    {
        return QString();
    }

    QString sender = reply.argumentAt<0>();
    mClientActionSenders[id] = sender;
    return sender;
}

QString Actions::changeShortcut(const qulonglong &id, const QString &shortcut)
{
    QDBusPendingReply<QString> reply = mDaemonProxy->changeShortcut(id, shortcut);
    reply.waitForFinished();
    if (reply.isError())
    {
        return QString();
    }

    return reply.argumentAt<0>();
}

bool Actions::swapActions(const qulonglong &id1, const qulonglong &id2)
{
    QDBusPendingReply<bool> reply = mDaemonProxy->swapActions(id1, id2);
    reply.waitForFinished();
    if (reply.isError())
    {
        return false;
    }

    return reply.argumentAt<0>();
}

bool Actions::removeAction(const qulonglong &id)
{
    QDBusPendingReply<bool> reply = mDaemonProxy->removeAction(id);
    reply.waitForFinished();
    if (reply.isError())
    {
        return false;
    }

    return reply.argumentAt<0>();
}

void Actions::setMultipleActionsBehaviour(const MultipleActionsBehaviour &behaviour)
{
    QDBusPendingReply<bool> reply = mDaemonProxy->setMultipleActionsBehaviour(behaviour);
    reply.waitForFinished();
}

void Actions::grabShortcut(uint timeout)
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(mDaemonProxy->grabShortcut(timeout), this);

    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher *)), this, SLOT(grabShortcutFinished(QDBusPendingCallWatcher *)));
}

void Actions::cancelShortcutGrab()
{
    mDaemonProxy->cancelShortcutGrab();
}

void Actions::grabShortcutFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QString, bool, bool, bool> reply = *call;
    if (reply.isError())
    {
        emit grabShortcutFailed();
    }
    else
    {
        if (reply.argumentAt<1>())
        {
            emit grabShortcutFailed();
        }
        else
        {
            if (reply.argumentAt<2>())
            {
                emit grabShortcutCancelled();
            }
            else
            {
                if (reply.argumentAt<3>())
                {
                    emit grabShortcutTimedout();
                }
                else
                {
                    emit shortcutGrabbed(reply.argumentAt<0>());
                }
            }
        }
    }

    call->deleteLater();
}
