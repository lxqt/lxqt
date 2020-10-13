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
#include <QDateTime>
#include <QDebug>

#include "batteryinfoframe.h"
#include "batteryhelper.h"
#include "ui_batteryinfoframe.h"

#include <LXQt/Globals>

BatteryInfoFrame::BatteryInfoFrame(Solid::Battery *battery) :
    QFrame(),
    mBattery(battery),
    mUi(new Ui::BatteryInfoFrame)
{
    mUi->setupUi(this);

    mUi->energyFullDesignValue->setText(QString::fromLatin1("%1 Wh").arg(mBattery->energyFullDesign(), 0, 'f', 2));
    mUi->typeValue->setText(BatteryHelper::typeToString(mBattery->type()));
    mUi->technologyValue->setText(BatteryHelper::technologyToString(mBattery->technology()));

    QString vendor = QString::fromLatin1("%1 %2").arg(battery->recallVendor()).arg(battery->serial());
    if (vendor.trimmed().isEmpty())
        vendor = QSL("Unknown");
    mUi->vendorValue->setText(vendor);

    connect(mBattery, SIGNAL(energyChanged(double, const QString)), this, SLOT(onBatteryChanged()));
    connect(mBattery, SIGNAL(chargeStateChanged(int, const QString)), this, SLOT(onBatteryChanged()));
    onBatteryChanged();
}

BatteryInfoFrame::~BatteryInfoFrame()
{
    delete mUi;
}

void BatteryInfoFrame::onBatteryChanged()
{
    mUi->stateValue->setText(BatteryHelper::stateToString(mBattery->chargeState()));
    mUi->energyFullValue->setText(QString::fromLatin1("%1 Wh (%2 %)").arg(mBattery->energyFull(), 0, 'f', 2).arg(mBattery->capacity()));
    mUi->energyValue->setText(QString::fromLatin1("%1 Wh (%2 %)").arg(mBattery->energy(), 0, 'f', 2).arg(mBattery->chargePercent()));
    mUi->energyRateValue->setText(QString::fromLatin1("%1 W").arg(mBattery->energyRate(), 0, 'f', 2));
    mUi->voltageValue->setText(QString::fromLatin1("%1 V").arg(mBattery->voltage(), 0, 'f', 2));
    mUi->temperatureValue->setText(QString::fromUtf8("%1 ºC").arg(mBattery->temperature()));
}
