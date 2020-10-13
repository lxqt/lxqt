/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *  Balázs Béla <balazsbela[at]gmail.com>
 *  Paulo Lieuthier <paulolieuthier@gmail.com>
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
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef STATUSNOTIFIERWATCHER_H
#define STATUSNOTIFIERWATCHER_H

#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusServiceWatcher>

#include "dbustypes.h"

class StatusNotifierWatcher : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.StatusNotifierWatcher")
    Q_SCRIPTABLE Q_PROPERTY(bool IsStatusNotifierHostRegistered READ isStatusNotifierHostRegistered)
    Q_SCRIPTABLE Q_PROPERTY(int ProtocolVersion READ protocolVersion)
    Q_SCRIPTABLE Q_PROPERTY(QStringList RegisteredStatusNotifierItems READ RegisteredStatusNotifierItems)

public:
    explicit StatusNotifierWatcher(QObject *parent = nullptr);
    ~StatusNotifierWatcher();

    bool isStatusNotifierHostRegistered() { return mHosts.count() > 0; }
    int protocolVersion() const { return 0; }
    QStringList RegisteredStatusNotifierItems() const { return mServices; }

signals:
    Q_SCRIPTABLE void StatusNotifierItemRegistered(const QString &service);
    Q_SCRIPTABLE void StatusNotifierItemUnregistered(const QString &service);
    Q_SCRIPTABLE void StatusNotifierHostRegistered();

public slots:
    Q_SCRIPTABLE void RegisterStatusNotifierItem(const QString &serviceOrPath);
    Q_SCRIPTABLE void RegisterStatusNotifierHost(const QString &service);

    void serviceUnregistered(const QString &service);

private:
    QStringList mServices;
    QStringList mHosts;
    QDBusServiceWatcher *mWatcher;
};

#endif // STATUSNOTIFIERWATCHER_H
