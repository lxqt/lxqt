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

#include "chip.h"
#include <QDebug>


Chip::Chip(const sensors_chip_name* sensorsChipName)
    : mSensorsChipName(sensorsChipName)
{
    const int BUF_SIZE = 256;
    char buf[BUF_SIZE];
    if (sensors_snprintf_chip_name(buf, BUF_SIZE, mSensorsChipName) > 0)
    {
        mName = QString::fromLatin1(buf);
    }

    qDebug() << "Detected chip:" << mName;

    const sensors_feature* feature;
    int featureNr = 0;

    while ((feature = sensors_get_features(mSensorsChipName, &featureNr)))
    {
        mFeatures.push_back(Feature(mSensorsChipName, feature));
    }
}


const QString& Chip::getName() const
{
    return mName;
}


const QList<Feature>& Chip::getFeatures() const
{
    return mFeatures;
}
