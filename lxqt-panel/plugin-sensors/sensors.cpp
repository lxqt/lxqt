/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Łukasz Twarduś <ltwardus@gmail.com>
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

#include "sensors.h"
#include <QDebug>


QList<Chip> Sensors::mDetectedChips = QList<Chip>();
int Sensors::mInstanceCounter = 0;
bool Sensors::mSensorsInitialized = false;


Sensors::Sensors()
{
    // Increase instance counter
    ++mInstanceCounter;

    if (!mSensorsInitialized && sensors_init(nullptr) == 0)
    {
        // Sensors initialized
        mSensorsInitialized = true;

        sensors_chip_name const * chipName;
        int chipNr = 0;
        while ((chipName = sensors_get_detected_chips(nullptr, &chipNr)) != nullptr)
        {
            mDetectedChips.push_back(chipName);
        }

        qDebug() << "lm_sensors library initialized";
    }
}


Sensors::~Sensors()
{
    // Decrease instance counter
    --mInstanceCounter;

    if (mInstanceCounter == 0 && mSensorsInitialized)
    {
        mDetectedChips.clear();
        mSensorsInitialized = false;
        sensors_cleanup();

        qDebug() << "lm_sensors library cleanup";
    }
}


const QList<Chip>& Sensors::getDetectedChips() const
{
    return mDetectedChips;
}
