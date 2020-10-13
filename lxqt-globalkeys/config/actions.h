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

#ifndef GLOBAL_ACTION_CONFIG__ACTIONS__INCLUDED
#define GLOBAL_ACTION_CONFIG__ACTIONS__INCLUDED

#include <QObject>
#include <QtGlobal>
#include <QString>
#include <QList>
#include <QStringList>
#include <QMap>
#include <QPair>
#include <QDBusObjectPath>

#include "../daemon/meta_types.h"


class OrgLxqtGlobal_key_shortcutsDaemonInterface;
namespace org
{
namespace lxqt
{
namespace global_key_shortcuts
{
typedef ::OrgLxqtGlobal_key_shortcutsDaemonInterface daemon;
}
}
}

class QDBusServiceWatcher;
class QDBusPendingCallWatcher;

class Actions : public QObject
{
    Q_OBJECT
public:
    Actions(QObject *parent = nullptr);
    ~Actions() override;


    QList<qulonglong> allActionIds() const;
    QPair<bool, GeneralActionInfo> actionById(qulonglong id) const;

    QList<qulonglong> allClientActionIds() const;
    QPair<bool, ClientActionInfo> clientActionInfoById(qulonglong id) const;

    QList<qulonglong> allMethodActionIds() const;
    QPair<bool, MethodActionInfo> methodActionInfoById(qulonglong id) const;

    QList<qulonglong> allCommandActionIds() const;
    QPair<bool, CommandActionInfo> commandActionInfoById(qulonglong id) const;


    QPair<QString, qulonglong> addMethodAction(const QString &shortcut, const QString &service, const QDBusObjectPath &path, const QString &interface, const QString &method, const QString &description);
    QPair<QString, qulonglong> addCommandAction(const QString &shortcut, const QString &command, const QStringList &arguments, const QString &description);

    bool modifyActionDescription(const qulonglong &id, const QString &description);
    bool modifyMethodAction(const qulonglong &id, const QString &service, const QDBusObjectPath &path, const QString &interface, const QString &method, const QString &description);
    bool modifyCommandAction(const qulonglong &id, const QString &command, const QStringList &arguments, const QString &description);

    bool enableAction(qulonglong id, bool enabled);
    bool isActionEnabled(qulonglong id);

    QString getClientActionSender(qulonglong id);
    QString updateClientActionSender(qulonglong id);

    QString changeShortcut(const qulonglong &id, const QString &shortcut);

    bool swapActions(const qulonglong &id1, const qulonglong &id2);

    bool removeAction(const qulonglong &id);


    MultipleActionsBehaviour multipleActionsBehaviour() const;

    void setMultipleActionsBehaviour(const MultipleActionsBehaviour &behaviour);


    void grabShortcut(uint timeout);
    void cancelShortcutGrab();

signals:
    void daemonDisappeared();
    void daemonAppeared();

    void actionAdded(qulonglong id);
    void actionEnabled(qulonglong id, bool enabled);
    void actionModified(qulonglong id);
    void actionsSwapped(qulonglong id1, qulonglong id2);
    void actionRemoved(qulonglong id);

    void multipleActionsBehaviourChanged(MultipleActionsBehaviour behaviour);

    void shortcutGrabbed(const QString &);
    void grabShortcutFailed();
    void grabShortcutCancelled();
    void grabShortcutTimedout();

private:
    void init();
    void clear();

    QList<qulonglong> getAllActionIds();

    bool getActionById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &type, QString &info);
    QMap<qulonglong, GeneralActionInfo> getAllActions();

    bool getClientActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QDBusObjectPath &path);
    bool getMethodActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &service, QDBusObjectPath &path, QString &interface, QString &method);
    bool getCommandActionInfoById(qulonglong id, QString &shortcut, QString &description, bool &enabled, QString &command, QStringList &arguments);

    uint getMultipleActionsBehaviour();

private slots:
    void delayedInit();

    void on_daemonDisappeared(const QString &);
    void on_daemonAppeared(const QString &);

    void on_actionAdded(qulonglong id);
    void on_actionEnabled(qulonglong id, bool enabled);
    void on_clientActionSenderChanged(qulonglong id, const QString &sender);
    void on_actionModified(qulonglong id);
    void on_actionShortcutChanged(qulonglong id);
    void on_actionsSwapped(qulonglong id1, qulonglong id2);
    void on_actionRemoved(qulonglong id);
    void on_multipleActionsBehaviourChanged(uint behaviour);

    void grabShortcutFinished(QDBusPendingCallWatcher *call);

private:
    void do_actionAdded(qulonglong id);
    void do_actionRemoved(qulonglong id);

private:
    org::lxqt::global_key_shortcuts::daemon *mDaemonProxy;
    QDBusServiceWatcher *mServiceWatcher;

    typedef QMap<qulonglong, GeneralActionInfo> GeneralActionInfos;
    GeneralActionInfos mGeneralActionInfo;

    typedef QMap<qulonglong, ClientActionInfo> ClientActionInfos;
    ClientActionInfos mClientActionInfo;

    typedef QMap<qulonglong, QString> ClientActionSenders;
    ClientActionSenders mClientActionSenders;

    typedef QMap<qulonglong, MethodActionInfo> MethodActionInfos;
    MethodActionInfos mMethodActionInfo;

    typedef QMap<qulonglong, CommandActionInfo> CommandActionInfos;
    CommandActionInfos mCommandActionInfo;

    MultipleActionsBehaviour mMultipleActionsBehaviour;
};

#endif // GLOBAL_ACTION_CONFIG__ACTIONS__INCLUDED
