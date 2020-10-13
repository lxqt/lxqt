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

#ifndef GLOBAL_KEY_SHORTCUT_CLIENT__CLIENT__IMPL__INCLUDED
#define GLOBAL_KEY_SHORTCUT_CLIENT__CLIENT__IMPL__INCLUDED

#include <QObject>
#include <QString>
#include <QMap>
#include <QDBusPendingCallWatcher>

#include "action.h"


class OrgLxqtGlobal_key_shortcutsNativeInterface;
namespace org
{
namespace lxqt
{
namespace global_key_shortcuts
{
typedef ::OrgLxqtGlobal_key_shortcutsNativeInterface native;
}
}
}

class QDBusServiceWatcher;

namespace GlobalKeyShortcut
{
class Client;

class ClientAdaptor;

class ClientImpl : public QObject
{
    Q_OBJECT

public:
    ClientImpl(Client *interface, QObject *parent = nullptr);
    ~ClientImpl() override;

    Action *addClientAction(const QString &shortcut, const QString &path, const QString &description, QObject *parent);

    QString changeClientActionShortcut(const QString &path, const QString &shortcut);
    bool modifyClientAction(const QString &path, const QString &description);
    bool removeClientAction(const QString &path);

    void removeAction(ActionImpl *action);

    void grabShortcut(uint timeout);
    void cancelShortcutGrab();

    bool isDaemonPresent() const;

public slots:
    void grabShortcutFinished(QDBusPendingCallWatcher *call);
    void daemonDisappeared(const QString &);
    void daemonAppeared(const QString &);
    void registrationFinished(QDBusPendingCallWatcher *call);

signals:
    void emitShortcutGrabbed(const QString &);
    void emitGrabShortcutFailed();
    void emitGrabShortcutCancelled();
    void emitGrabShortcutTimedout();

    void emitDaemonDisappeared();
    void emitDaemonAppeared();
    void emitDaemonPresenceChanged(bool);

private:
    Client *mInterface;
    org::lxqt::global_key_shortcuts::native *mProxy;
    QMap<QString, Action*> mActions;
    QDBusServiceWatcher *mServiceWatcher;
    bool mDaemonPresent;

    QMap<QDBusPendingCallWatcher*, ActionImpl*> mPendingRegistrationsActions;
    QMap<ActionImpl*, QDBusPendingCallWatcher*> mPendingRegistrationsWatchers;
};

}

#endif // GLOBAL_KEY_SHORTCUT_CLIENT__CLIENT__IMPL__INCLUDED
