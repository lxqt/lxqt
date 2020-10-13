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

#ifndef FEATURE_H
#define FEATURE_H

#include <QList>
#include <QString>
#include <sensors/sensors.h>


/**
 * @brief Feature class is providing RAII-style for lm_sensors library
 */

class Feature
{
public:
    Feature(const sensors_chip_name*, const sensors_feature*);
    const QString& getName() const;
    const QString& getLabel() const;
    double getValue(sensors_subfeature_type) const;
    sensors_feature_type getType() const;
private:
    // Do not try to change these chip names, as they point to internal structures of lm_sensors!
    const sensors_chip_name* mSensorsChipName;

    const sensors_feature* mSensorsFeature;

    // "Printable" feature label
    QString mLabel;

    QList<const sensors_subfeature*> mSubFeatures;
};

#endif // CHIP_H
