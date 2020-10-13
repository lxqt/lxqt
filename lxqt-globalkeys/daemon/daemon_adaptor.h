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

#ifndef GLOBAL_ACTION_DAEMON__DAEMON_ADAPTOR__INCLUDED
#define GLOBAL_ACTION_DAEMON__DAEMON_ADAPTOR__INCLUDED


#include <QObject>
#include <QDBusContext>
#include <QString>
#include <QStringList>
#include <QDBusObjectPath>
#include <QPair>

#include "meta_types.h"


class DaemonAdaptor : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    DaemonAdaptor(QObject *parent = nullptr);

public slots:
    QString addMethodAction(const QString &shortcut, const QString &service, const QDBusObjectPath &path, const QString &interface, const QString &method, const QString &description, qulonglong &id);
    QString addCommandAction(const QString &shortcut, const QString &command, const QStringList &arguments, const QString &description, qulonglong &id);

    bool modifyActionDescription(qulonglong id, const QString &description);
    bool modifyMethodAction(qulonglong id, const QString &service, const QDBusObjectPath &path, const QString &interface, const QString &method, const QString &description);
    bool modifyCommandAction(qulonglong id, const QString &command, const QStringList &arguments, const QString &description);

    bool enableAction(qulonglong id, bool enabled);
    bool isActionEnabled(qulonglong id);

    QString getClientActionSender(qulonglong id);

    QString changeShortcut(qulonglong id, const QString &shortcut);

    bool swapActions(qulonglong id1, qulonglong id2);

    bool removeAction(qulonglong id);

    bool setMultipleActionsBehaviour(uint behaviour);
    uint getMultipleActionsBehaviour();

    QList<qulonglong> getAllActionIds();
    QMap<qulonglong, GeneralActionInfo> getAllActions();
    bool getActionById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &type, QString &info);
    bool getClientActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QDBusObjectPath &path);
    bool getMethodActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &service, QDBusObjectPath &path, QString &interface, QString &method);
    bool getCommandActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &command, QStringList &arguments);

    QString grabShortcut(uint timeout, bool &failed, bool &cancelled, bool &timedout);
    void cancelShortcutGrab();

    void quit();

    void emit_actionAdded(qulonglong id);
    void emit_actionModified(qulonglong id);
    void emit_actionRemoved(qulonglong id);
    void emit_actionShortcutChanged(qulonglong id);
    void emit_actionEnabled(qulonglong id, bool enabled);
    void emit_clientActionSenderChanged(qulonglong id, const QString &sender);

signals:
    void actionAdded(qulonglong id);
    void actionModified(qulonglong id);
    void actionRemoved(qulonglong id);
    void actionShortcutChanged(qulonglong id);
    void actionEnabled(qulonglong id, bool enabled);
    void clientActionSenderChanged(qulonglong id, const QString &sender);
    void actionsSwapped(qulonglong id1, qulonglong id2);
    void multipleActionsBehaviourChanged(uint behaviour);

signals:
    void onAddMethodAction(QPair<QString, qulonglong> &, const QString &, const QString &, const QDBusObjectPath &, const QString &, const QString &, const QString &);
    void onAddCommandAction(QPair<QString, qulonglong> &, const QString &, const QString &, const QStringList &, const QString &);

    void onModifyActionDescription(bool &, qulonglong, const QString &);
    void onModifyMethodAction(bool &, qulonglong, const QString &, const QDBusObjectPath &, const QString &, const QString &, const QString &);
    void onModifyCommandAction(bool &, qulonglong, const QString &, const QStringList &, const QString &);

    void onEnableAction(bool &, qulonglong, bool);
    void onIsActionEnabled(bool &, qulonglong);

    void onGetClientActionSender(QString &, qulonglong);

    void onChangeShortcut(QString &, qulonglong, const QString &);

    void onSwapActions(bool &, qulonglong, qulonglong);

    void onRemoveAction(bool &, qulonglong);

    void onSetMultipleActionsBehaviour(const MultipleActionsBehaviour &);
    void onGetMultipleActionsBehaviour(MultipleActionsBehaviour &);

    void onGetAllActionIds(QList<qulonglong> &);
    void onGetActionById(QPair<bool, GeneralActionInfo> &, qulonglong);
    void onGetAllActions(QMap<qulonglong, GeneralActionInfo> &);

    void onGetClientActionInfoById(QPair<bool, ClientActionInfo> &, qulonglong);
    void onGetMethodActionInfoById(QPair<bool, MethodActionInfo> &, qulonglong);
    void onGetCommandActionInfoById(QPair<bool, CommandActionInfo> &, qulonglong);

    void onGrabShortcut(uint, QString &, bool &, bool &, bool &, const QDBusMessage &);
    void onCancelShortcutGrab();

    void onQuit();
};

#endif // GLOBAL_ACTION_DAEMON__DAEMON_ADAPTOR__INCLUDED
