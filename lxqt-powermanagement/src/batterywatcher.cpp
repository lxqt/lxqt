/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
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
#include <QTime>
#include <QTimer>

#include <LXQt/Notification>
#include <LXQt/AutostartEntry>
#include <LXQt/Globals>
#include <LXQt/Power>
#include <Solid/Device>
#include <Solid/Battery>

#include "batterywatcher.h"
#include "../config/powermanagementsettings.h"

BatteryWatcher::BatteryWatcher(QObject *parent) : Watcher(parent)
{
    const QList<Solid::Device> devices = Solid::Device::listFromType(Solid::DeviceInterface::Battery, QString());

    if (devices.isEmpty())
    {
        LXQt::Notification::notify(tr("No battery!"),
                tr("LXQt could not find data about any battery - monitoring disabled"),
                QSL("lxqt-powermanagement"));
    }

    for (Solid::Device device : devices)
    {
        Solid::Battery *battery = device.as<Solid::Battery>();
        if (battery->type() != Solid::Battery::PrimaryBattery)
        {
            continue;
        }
        mBatteries << battery;
        connect(battery, &Solid::Battery::energyChanged, this, &BatteryWatcher::batteryChanged);
        connect(battery, &Solid::Battery::chargeStateChanged, this, &BatteryWatcher::batteryChanged);
    }

    mBatteryInfoDialog = new BatteryInfoDialog(mBatteries);

    connect(&mSettings, SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
    connect(LXQt::Settings::globalSettings(), SIGNAL(iconThemeChanged()), this, SLOT(settingsChanged()));

    settingsChanged();
    batteryChanged();
}

BatteryWatcher::~BatteryWatcher()
{
}

void BatteryWatcher::batteryChanged()
{
    static QTime actionTime;
    static LXQt::Notification *notification = nullptr;

    double totalEnergyFull = 0;
    double totalEnergyNow = 0;
    bool discharging = true;
    double chargeLevel;

    for (const Solid::Battery *battery : qAsConst(mBatteries))
    {
        totalEnergyFull += battery->energyFull();
        totalEnergyNow += battery->energy();
        discharging &= (battery->chargeState() == Solid::Battery::Discharging);
    }

    chargeLevel = 100 * totalEnergyNow / totalEnergyFull;

    qDebug() <<  "BatteryChanged"
             <<  "discharging:" << discharging
             <<  "chargeLevel:" << chargeLevel
             <<  "actionTime:"  << actionTime;

    bool powerLowActionRequired =
            discharging &&
            chargeLevel < mSettings.getPowerLowLevel() &&
            mSettings.getPowerLowAction() > 0;

    if (powerLowActionRequired)
    {
        if (actionTime.isNull())
            actionTime = QTime::currentTime().addMSecs(mSettings.getPowerLowWarningTime() * 1000);

        if (!notification)
        {
            notification = new LXQt::Notification(tr("Power low!"), this);
            notification->setTimeout(2000);
        }

        int milliSecondsToAction = QTime::currentTime().msecsTo(actionTime);

        if (milliSecondsToAction > 0)
        {
            int secondsToAction = milliSecondsToAction / 1000;
            switch (mSettings.getPowerLowAction())
            {
            case LXQt::Power::PowerSuspend:
                notification->setBody(tr("Suspending in %n second(s)", nullptr, secondsToAction).arg(secondsToAction));
                break;
            case LXQt::Power::PowerHibernate:
                notification->setBody(tr("Hibernating in %n second(s)", nullptr, secondsToAction).arg(secondsToAction));
                break;
            case LXQt::Power::PowerShutdown:
                notification->setBody(tr("Shutting down in %n second(s)", nullptr, secondsToAction).arg(secondsToAction));
                break;
            }

            notification->update();

            QTimer::singleShot(200, this, SLOT(batteryChanged()));
        }
        else
        {
            doAction(mSettings.getPowerLowAction());
        }
    }
    else
    {
        if (!actionTime.isNull())
            actionTime = QTime();

        if (notification)
        {
            delete notification;
            notification = nullptr;
        }
    }
}

void BatteryWatcher::settingsChanged()
{
    if (!mSettings.isShowIcon())
    {
        while (!mTrayIcons.isEmpty())
        {
            mTrayIcons.first()->hide();
            mTrayIcons.takeFirst()->deleteLater();
        }
    }
    else if (mTrayIcons.isEmpty())
    {
        for (Solid::Battery *battery : qAsConst(mBatteries))
        {
            mTrayIcons.append(new TrayIcon(battery, this));
            connect(mTrayIcons.last(), SIGNAL(toggleShowInfo()), mBatteryInfoDialog, SLOT(toggleShow()));
            mTrayIcons.last()->show();
        }
    }
}

