/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2010-2011 LXQt team
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

#ifndef SESSIONDBUS_H
#define SESSIONDBUS_H

#include <QtDBus>
#include <LXQt/Power>

#include "lxqtmodman.h"


/*! \brief Simple DBus adaptor for LXQtSession.
It allows 3rd party apps/lxqt modules to logout from session.
It's a part of "LXQt Power Management" - see liblxqt.
\author Petr Vanek <petr@scribus.info>
*/
class SessionDBusAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.lxqt.session")

public:
    SessionDBusAdaptor(LXQtModuleManager * manager)
        : QDBusAbstractAdaptor(manager),
          m_manager(manager),
          m_power(false/*don't use ourself, just all other power providers*/)
    {
        connect(m_manager, SIGNAL(moduleStateChanged(QString,bool)), SIGNAL(moduleStateChanged(QString,bool)));
    }

signals:
    void moduleStateChanged(QString moduleName, bool state);

public slots:

    // there can be a situation when is the session asked for availability.
    // And the lxqt-session is not always required to be started...
    bool canLogout()
    {
        return true;
    }

    bool canReboot()
    {
        return m_power.canReboot();
    }

    bool canPowerOff()
    {
        return m_power.canShutdown();
    }

    Q_NOREPLY void logout()
    {
        m_manager->logout(true);
    }

    Q_NOREPLY void reboot()
    {
        m_manager->logout(false);
        m_power.reboot();
        QCoreApplication::exit(0);
    }

    Q_NOREPLY void powerOff()
    {
        m_manager->logout(false);
        m_power.shutdown();
        QCoreApplication::exit(0);
    }

    QDBusVariant listModules()
    {
        return QDBusVariant(m_manager->listModules());
    }

    Q_NOREPLY void startModule(const QString& name)
    {
        m_manager->startProcess(name);
    }

    Q_NOREPLY void stopModule(const QString& name)
    {
        m_manager->stopProcess(name);
    }

private:
    LXQtModuleManager * m_manager;
    LXQt::Power m_power;
};

#endif
