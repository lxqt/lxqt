/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2014 LXQt team
 * Authors:
 *  Luís Pereira <luis.artur.pereira@gmail.com>
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

#include "lxqtsingleapplication.h"
#include "singleapplicationadaptor.h"
#include <KWindowSystem/KWindowSystem>
#include <QDBusMessage>
#include <QWidget>
#include <QDebug>
#include <QTimer>

using namespace LXQt;

SingleApplication::SingleApplication(int &argc, char **argv, StartOptions options)
    : Application(argc, argv),
    mActivationWindow(nullptr)
{
    QString service =
        QString::fromLatin1("org.lxqt.%1").arg(QApplication::applicationName());

    QDBusConnection bus = QDBusConnection::sessionBus();

    if (!bus.isConnected()) {
        QLatin1String errorMessage("Can't connect to the D-Bus session bus\n"
                                   "Make sure the D-Bus daemon is running");

        /* ExitOnDBusFailure is the default. Any value other than
           NoExitOnDBusFailure will be taken as ExitOnDBusFailure (the default).
         */
        if (options == NoExitOnDBusFailure) {
            qDebug() << Q_FUNC_INFO << errorMessage;
            return;
        } else {
            qCritical() << Q_FUNC_INFO << errorMessage;
            QTimer::singleShot(0, [this] { exit(1); });
        }
    }

    bool registered = (bus.registerService(service) ==
                       QDBusConnectionInterface::ServiceRegistered);
    if (registered) { // We are the primary instance
        SingleApplicationAdaptor *mAdaptor = new SingleApplicationAdaptor(this);
        QLatin1String objectPath("/");
        bus.registerObject(objectPath, mAdaptor,
            QDBusConnection::ExportAllSlots);
    } else { // We are the second outstance
        QDBusMessage msg = QDBusMessage::createMethodCall(service,
            QStringLiteral("/"),
            QStringLiteral("org.lxqt.SingleApplication"),
            QStringLiteral("activateWindow"));
        QDBusConnection::sessionBus().send(msg);

        QTimer::singleShot(0, [this] { exit(0); });
    }
}

SingleApplication::~SingleApplication()
{
}

void SingleApplication::setActivationWindow(QWidget *w)
{
    mActivationWindow = w;
}

QWidget *SingleApplication::activationWindow() const
{
    return mActivationWindow;
}

void SingleApplication::activateWindow()
{
    if (mActivationWindow) {
        mActivationWindow->show();
        WId window = mActivationWindow->effectiveWinId();

        KWindowInfo info(window, KWindowSystem::WMDesktop);
        int windowDesktop = info.desktop();

        if (windowDesktop != KWindowSystem::currentDesktop())
            KWindowSystem::setCurrentDesktop(windowDesktop);
        KWindowSystem::activateWindow(window);
    } else {
        qDebug() << Q_FUNC_INFO << "activationWindow not set or null";
    }
}
