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

#ifndef GLOBAL_ACTION_DAEMON__NATIVE_ADAPTOR__INCLUDED
#define GLOBAL_ACTION_DAEMON__NATIVE_ADAPTOR__INCLUDED


#include <QObject>
#include <QDBusObjectPath>
#include <QDBusContext>
#include <QPair>


class NativeAdaptor : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    NativeAdaptor(QObject *parent = nullptr);

    QString addClientAction(const QString &shortcut, const QDBusObjectPath &path, const QString &description, qulonglong &id);
    bool modifyClientAction(const QDBusObjectPath &path, const QString &description);
    QString changeClientActionShortcut(const QDBusObjectPath &path, const QString &shortcut);
    bool removeClientAction(const QDBusObjectPath &path);
    bool deactivateClientAction(const QDBusObjectPath &path);
    bool enableClientAction(const QDBusObjectPath &path, bool enabled);
    bool isClientActionEnabled(const QDBusObjectPath &path);

    QString grabShortcut(uint timeout, bool &failed, bool &cancelled, bool &timedout);
    void cancelShortcutGrab();

signals:
    void onAddClientAction(QPair<QString, qulonglong> &, const QString &, const QDBusObjectPath &, const QString &, const QString &);
    void onModifyClientAction(qulonglong &, const QDBusObjectPath &, const QString &, const QString &);
    void onChangeClientActionShortcut(QPair<QString, qulonglong> &, const QDBusObjectPath &, const QString &, const QString &);
    void onRemoveClientAction(bool &, const QDBusObjectPath &, const QString &);
    void onDeactivateClientAction(bool &, const QDBusObjectPath &, const QString &);
    void onEnableClientAction(bool &, const QDBusObjectPath &, bool, const QString &);
    void onIsClientActionEnabled(bool &, const QDBusObjectPath &, const QString &);

    void onGrabShortcut(uint, QString &, bool &, bool &, bool &, const QDBusMessage &);
    void onCancelShortcutGrab();

};

#endif // GLOBAL_ACTION_DAEMON__NATIVE_ADAPTOR__INCLUDED
