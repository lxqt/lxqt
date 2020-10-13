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
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef SENSORS_H
#define SENSORS_H

#include "chip.h"
#include <QList>
#include <sensors/sensors.h>


class Chip;

/**
 * @brief Sensors class is providing RAII-style for lm_sensors library
 */

class Sensors
{
public:
    Sensors();
    ~Sensors();
    const QList<Chip>& getDetectedChips() const;

private:
    static QList<Chip> mDetectedChips;

    /**
     * lm_sensors library can be initialized only once so this will tell us when to init
     * and when to clean up.
     */
    static int mInstanceCounter;
    static bool mSensorsInitialized;
};

#endif // SENSORS_H
