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

#ifndef GLOBAL_KEY_SHORTCUT_CLIENT__CLIENT__INCLUDED
#define GLOBAL_KEY_SHORTCUT_CLIENT__CLIENT__INCLUDED

#include <QtGlobal>

#include <QObject>
#include <QString>


namespace GlobalKeyShortcut
{

class Action;
class ClientImpl;

#ifndef SHARED_EXPORT
#define SHARED_EXPORT Q_DECL_IMPORT
#endif

class SHARED_EXPORT Client : public QObject
{
    Q_OBJECT

public:
    static Client *instance();
    ~Client() override;

    Action *addAction(const QString &shortcut, const QString &path, const QString &description, QObject *parent = nullptr);
    bool removeAction(const QString &path);

    bool isDaemonPresent() const;

public slots:
    void grabShortcut(uint timeout);
    void cancelShortcutGrab();

signals:
    void shortcutGrabbed(const QString &);
    void grabShortcutFailed();
    void grabShortcutCancelled();
    void grabShortcutTimedout();

    void daemonDisappeared();
    void daemonAppeared();
    void daemonPresenceChanged(bool);

private:
    Client();

    ClientImpl *impl;
};

}

#endif // GLOBAL_KEY_SHORTCUT_CLIENT__CLIENT__INCLUDED
