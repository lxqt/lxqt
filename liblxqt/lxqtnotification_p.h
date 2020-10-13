/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright (C) 2012  Alec Moskvin <alecm@gmx.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef LXQTNOTIFICATION_P_H
#define LXQTNOTIFICATION_P_H

#include "lxqtnotification.h"
#include "notifications_interface.h"

namespace LXQt
{

class NotificationPrivate : public QObject
{
    Q_OBJECT
public:
    NotificationPrivate(const QString& summary, Notification* parent);
    ~NotificationPrivate() override;

    void update();
    void close();
    void setActions(QStringList actions, int defaultAction);
    const Notification::ServerInfo serverInfo();
    void queryServerInfo(bool async=1);

public Q_SLOTS:
    void handleAction(uint id, QString key);
    void notificationClosed(uint, uint);

private:
    OrgFreedesktopNotificationsInterface* mInterface;
    uint mId;

    QString mSummary;
    QString mBody;
    QString mIconName;
    QStringList mActions;
    QVariantMap mHints;
    int mDefaultAction;
    int mTimeout;

    static Notification::ServerInfo sServerInfo;
    static bool sIsServerInfoQuried;

    Notification* const q_ptr;
    Q_DECLARE_PUBLIC(Notification)
};

} // namespace LXQt
#endif // LXQTNOTIFICATION_P_H
