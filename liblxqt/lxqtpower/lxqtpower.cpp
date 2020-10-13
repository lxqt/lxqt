/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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


#include "lxqtpower.h"
#include "lxqtpowerproviders.h"
#include <QtAlgorithms>
#include <QDebug>

using namespace LXQt;

Power::Power(bool useLxqtSessionProvider, QObject * parent /*= nullptr*/) :
    QObject(parent)
{
    mProviders.append(new CustomProvider(this));
    if (useLxqtSessionProvider)
        mProviders.append(new LXQtProvider(this));
    mProviders.append(new SystemdProvider(this));
    mProviders.append(new UPowerProvider(this));
    mProviders.append(new ConsoleKitProvider(this));
    mProviders.append(new LxSessionProvider(this));
}

Power::Power(QObject * parent /*= nullptr*/)
    : Power(true, parent)
{
}


Power::~Power()
{
}


bool Power::canAction(Power::Action action) const
{
    for(const PowerProvider* provider : qAsConst(mProviders))
    {
        if (provider->canAction(action))
            return true;
    }

    return false;
}


bool Power::doAction(Power::Action action)
{
    for(PowerProvider* provider : qAsConst(mProviders))
    {
        if (provider->canAction(action) &&
            provider->doAction(action)
           )
        {
            return true;
        }
    }
    return false;
}


bool Power::canLogout()    const { return canAction(PowerLogout);    }
bool Power::canHibernate() const { return canAction(PowerHibernate); }
bool Power::canReboot()    const { return canAction(PowerReboot);    }
bool Power::canShutdown()  const { return canAction(PowerShutdown);  }
bool Power::canSuspend()   const { return canAction(PowerSuspend);   }
bool Power::canMonitorOff() const { return canAction(PowerMonitorOff);   }
bool Power::canShowLeaveDialog() const { return canAction(PowerShowLeaveDialog);   }

bool Power::logout()       { return doAction(PowerLogout);    }
bool Power::hibernate()    { return doAction(PowerHibernate); }
bool Power::reboot()       { return doAction(PowerReboot);    }
bool Power::shutdown()     { return doAction(PowerShutdown);  }
bool Power::suspend()      { return doAction(PowerSuspend);   }
bool Power::monitorOff()   { return doAction(PowerMonitorOff);   }
bool Power::showLeaveDialog()   { return doAction(PowerShowLeaveDialog);   }
