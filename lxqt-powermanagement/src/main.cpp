/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 *
 * Authors:
 *   Christian Surlykke <christian@surlykke.dk>
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

#include <QDBusConnection>
#include <QDebug>
#include <QCommandLineParser>

#include <LXQt/Application>

#include "powermanagementd.h"

int main(int argc, char *argv[])
{

    LXQt::Application a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("LXQt Powermanagement Daemon"));
    const QString VERINFO = QSL(LXQT_POWERMANAGEMENT_VERSION \
                            "\nliblxqt   " LXQT_VERSION \
                            "\nQt        " QT_VERSION_STR);
    a.setApplicationVersion(VERINFO);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(a);

    // To ensure only one instance of lxqt-powermanagement is running we register as a DBus service and refuse to run
    // if not able to do so.
    // We do not register any object as we don't have any dbus-operations to expose.
    if (! QDBusConnection::sessionBus().registerService(QSL("org.lxqt.lxqt-powermanagement")))
    {
        qWarning() << "Unable to register 'org.lxqt.lxqt-powermanagement' service - is another instance of lxqt-powermanagement running?";
        return 1;
    }
    else
    {
        PowerManagementd powerManagementd;
        return a.exec();
    }
}
