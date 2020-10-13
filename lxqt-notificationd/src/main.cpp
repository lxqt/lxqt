/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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

#include <QApplication>
#include <QtDBus/QDBusConnection>
#include <QCommandLineParser>

#include <LXQt/Application>
#include <LXQt/Globals>

#include "notificationsadaptor.h"
#include "notifyd.h"


/*! \mainpage LXQt notification daemon
 *
 * Running in user session; implementing standard as described in:
 *    docs/nodification-spec-latest.html
 *
 * <b>Implementation notes:</b>
 *
 * Class \c Notifyd implements the main "server" part, a DBUS
 * interface. Displaying of notifications is handled by these
 * classes:
 *
 *  - \c NotificationArea: a QScrollArea object with transparency.
 *       It ensures tha no action is unreachable (user can scroll
 *       over notifications)
 *  - \c NotificationLayout: a \c NotificationArea's main widget,
 *       (QWidget instance) holding instances of \c Notification.
 *       Layouting (in real QLayout) is done here.
 *  - \c Notification: a QWidget with one notification. Icon, texts,
 *       user interaction, etc. is handled in it.
 *
 * \c Notification can be extended with widgets located in files
 * notificationwidgets.*. Currently there is only one extension:
 *
 *  - \c NotificationActionsWidget holding user interface for
 *       interactive actions (buttons or combobox).
 *
 * Other extensions for e.g. "x-cannonical-*" can be implemented too.
 *
 */
int main(int argc, char** argv)
{
    LXQt::Application a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("LXQt Notification Daemon"));
    const QString VERINFO = QStringLiteral(LXQT_NOTIFICATIOND_VERSION
                                           "\nliblxqt   " LXQT_VERSION
                                           "\nQt        " QT_VERSION_STR);
    a.setApplicationVersion(VERINFO);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(a);

    // Ensure the helper widgets are hidden
    a.setStyleSheet(a.styleSheet() +
                QSL("NotificationArea {background: transparent;}"
                    "NotificationLayout {background: transparent;}")
                   );

    Notifyd* daemon = new Notifyd();
    new NotificationsAdaptor(daemon);

    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.registerService(QSL("org.freedesktop.Notifications")))
        qDebug() << "registerService failed: another service with 'org.freedesktop.Notifications' runs already";
    if (!connection.registerObject(QSL("/org/freedesktop/Notifications"), daemon))
        qDebug() << "registerObject failed: another object with '/org/freedesktop/Notifications' runs already";

    return a.exec();
}
