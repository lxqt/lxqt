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

#include <QDebug>
#include <QStringList>
#include <QProcess>

#include "batteryhelper.h"
#include "powermanagementd.h"
#include "../config/powermanagementsettings.h"
#include "idlenesswatcher.h"
#include "lidwatcher.h"
#include "batterywatcher.h"
#include "powerbutton.h"

#include <LXQt/Globals>

#define CURRENT_RUNCHECK_LEVEL 1

PowerManagementd::PowerManagementd() :
        mBatterywatcherd(nullptr),
        mLidwatcherd(nullptr),
        mIdlenesswatcherd(nullptr),
        mSettings()
 {
    connect(&mSettings, SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
    settingsChanged();

    if (mSettings.getRunCheckLevel() < CURRENT_RUNCHECK_LEVEL)
    {
        performRunCheck();
        mSettings.setRunCheckLevel(CURRENT_RUNCHECK_LEVEL);
    }

    mPowerButton = new PowerButton(this);
}

PowerManagementd::~PowerManagementd()
{
}

void PowerManagementd::settingsChanged()
{
    if (mSettings.isBatteryWatcherEnabled() && !mBatterywatcherd)
        mBatterywatcherd = new BatteryWatcher(this);
    else if (mBatterywatcherd && ! mSettings.isBatteryWatcherEnabled())
    {
        mBatterywatcherd->deleteLater();
        mBatterywatcherd = nullptr;
    }

    if (mSettings.isLidWatcherEnabled() && !mLidwatcherd)
    {
        mLidwatcherd = new LidWatcher(this);
    }
    else if (mLidwatcherd && ! mSettings.isLidWatcherEnabled())
    {
        mLidwatcherd->deleteLater();
        mLidwatcherd = nullptr;
    }

    if (mSettings.isIdlenessWatcherEnabled() && !mIdlenesswatcherd)
    {
        mIdlenesswatcherd = new IdlenessWatcher(this);
    }
    else if (mIdlenesswatcherd && !mSettings.isIdlenessWatcherEnabled())
    {
        mIdlenesswatcherd->deleteLater();
        mIdlenesswatcherd = nullptr;
    }

}

void PowerManagementd::runConfigure()
{
    mNotification.close();
    QProcess::startDetached(QSL("lxqt-config-powermanagement"));
}

void PowerManagementd::performRunCheck()
{
    mSettings.setLidWatcherEnabled(Lid().haveLid());
    bool hasBattery = false;

    const auto devices = Solid::Device::listFromType(Solid::DeviceInterface::Battery, QString());
    for (const Solid::Device& device : devices)
        if (device.as<Solid::Battery>()->type() == Solid::Battery::PrimaryBattery)
            hasBattery = true;
    mSettings.setBatteryWatcherEnabled(hasBattery);
    mSettings.setIdlenessWatcherEnabled(true);
    mSettings.sync();

    mNotification.setSummary(tr("Power Management"));
    mNotification.setBody(tr("You are running LXQt Power Management for the first time.\nYou can configure it from settings... "));
    mNotification.setActions(QStringList() << tr("Configure..."));
    mNotification.setTimeout(10000);
    connect(&mNotification, SIGNAL(actionActivated(int)), SLOT(runConfigure()));
    mNotification.update();
}
