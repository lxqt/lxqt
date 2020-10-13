/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright (C) 2013  Alec Moskvin <alecm@gmx.com>
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

#include "idlenesswatcher.h"

#include <QCoreApplication>
#include <KIdleTime>
#include <Solid/Device>
#include <Solid/Battery>
#include <QDebug>
#include <LXQt/lxqtnotification.h>
#include <QObject>

IdlenessWatcher::IdlenessWatcher(QObject* parent):
    Watcher(parent)
{
    qDebug() << "Starting idlenesswatcher";
    
    mIdleWatcher = mIdleBacklightWatcher = mBacklightActualValue = -1;
    mBacklight = nullptr;
    mDischarging = false;

    connect(KIdleTime::instance(),
            static_cast<void (KIdleTime::*)(int)>(&KIdleTime::timeoutReached),
            this,
            &IdlenessWatcher::timeoutReached);

    {
        // Init backlight control
        connect(KIdleTime::instance(),
                &KIdleTime::resumingFromIdle,
                this,
                &IdlenessWatcher::resumingFromIdle);
        const QList<Solid::Device> devices = Solid::Device::listFromType(Solid::DeviceInterface::Battery, QString());
    
        for (Solid::Device device : devices)
        {
            Solid::Battery *battery = device.as<Solid::Battery>();
            if (battery->type() == Solid::Battery::PrimaryBattery) {
                connect(battery, &Solid::Battery::chargeStateChanged, this, &IdlenessWatcher::onBatteryChanged);
                mDischarging |= battery->chargeState() == Solid::Battery::ChargeState::Discharging;
            }
        }
    }
    connect(&mPSettings, &LXQt::Settings::settingsChanged,
            this, &IdlenessWatcher::onSettingsChanged);
    
    setup();
}

IdlenessWatcher::~IdlenessWatcher()
{
    KIdleTime::instance()->removeAllIdleTimeouts();
}

void IdlenessWatcher::setup()
{
    if(mPSettings.isIdlenessWatcherEnabled()) {
        int timeout = 1000 * mPSettings.getIdlenessTimeSecs();
        mIdleWatcher = KIdleTime::instance()->addIdleTimeout(timeout);
        {
            // Enable backlight control:
            if(mPSettings.isIdlenessBacklightWatcherEnabled() &&
                (
                    !mPSettings.isIdlenessBacklightOnBatteryDischargingEnabled()
                      ||
                    (
                        mPSettings.isIdlenessBacklightOnBatteryDischargingEnabled()
                          &&
                        mDischarging
                    )
                )
              ) {
                QTime time = mPSettings.getIdlenessBacklightTime();
                int milliseconds = (time.second() + time.minute() * 60) * 1000;
                if(milliseconds < 1000)
                    milliseconds = 1000;
                mIdleBacklightWatcher = KIdleTime::instance()->addIdleTimeout(milliseconds);
            }
        }
    }
}

void IdlenessWatcher::timeoutReached(int identifier)
{
    if(identifier == mIdleWatcher) {
        doAction(mPSettings.getIdlenessAction());
    } else if(identifier == mIdleBacklightWatcher && mBacklightActualValue < 0) {
        if(mBacklight == nullptr) {
            mBacklight = new LXQt::Backlight();
            connect(mBacklight, &QObject::destroyed, [this](QObject *) {mBacklight = nullptr;} );
        }
            
        //LXQt::Notification::notify(QStringLiteral("IdlenessWatcher::timeoutReached"),
        //    mBacklight->isBacklightAvailable() ?
        //     QStringLiteral("").setNum(mBacklightActualValue):QStringLiteral("Error!!"));

        mBacklightActualValue = mBacklight->getBacklight();
        if(mBacklight->isBacklightAvailable() && !mBacklight->isBacklightOff())
            mBacklight->setBacklight((float)mBacklightActualValue * (float)(mPSettings.getBacklight())/100.0f);
        
        KIdleTime::instance()->removeIdleTimeout(mIdleBacklightWatcher);
        KIdleTime::instance()->catchNextResumeEvent();

        mBacklight->deleteLater();
    }
}

void IdlenessWatcher::resumingFromIdle()
{
    if(mBacklightActualValue > -1) {
        if(mPSettings.isIdlenessBacklightWatcherEnabled()) {
            if(mBacklight == nullptr) {
                mBacklight = new LXQt::Backlight();
                connect(mBacklight, &QObject::destroyed, [this](QObject *) {mBacklight = nullptr;} );
            }

            if(mBacklight->isBacklightAvailable() && !mBacklight->isBacklightOff())
                mBacklight->setBacklight(mBacklightActualValue);
            mBacklightActualValue = -1;
            onSettingsChanged();

            mBacklight->deleteLater();
        }
    }
}

void IdlenessWatcher::onBatteryChanged(int, const QString &)
{
    if( mPSettings.isIdlenessBacklightOnBatteryDischargingEnabled() ) {
        const QList<Solid::Device> devices = Solid::Device::listFromType(Solid::DeviceInterface::Battery, QString());
        
        mDischarging = false;
    
        for (Solid::Device device : devices) {
            Solid::Battery *battery = device.as<Solid::Battery>();
            if (battery->type() == Solid::Battery::PrimaryBattery) {
                mDischarging |= battery->chargeState() == Solid::Battery::ChargeState::Discharging;
            }
        }
    }
    onSettingsChanged();
}


void IdlenessWatcher::onSettingsChanged()
{
    KIdleTime::instance()->removeAllIdleTimeouts();
    mIdleWatcher = mIdleBacklightWatcher = -1;
    setup();
}
