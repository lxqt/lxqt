/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
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

#include <QDBusConnection>
#include <QDBusReply>
#include <QObject>
#include <QDebug>

#include "lid.h"

#include <LXQt/Globals>

Lid::Lid()
{
    mUPowerInterface = new QDBusInterface(QL1S("org.freedesktop.UPower"),
                                          QL1S("/org/freedesktop/UPower"),
                                          QL1S("org.freedesktop.UPower"),
                                          QDBusConnection::systemBus(), this);

    mUPowerPropertiesInterface = new QDBusInterface(QL1S("org.freedesktop.UPower"),
                                                    QL1S("/org/freedesktop/UPower"),
                                                    QL1S("org.freedesktop.DBus.Properties"),
                                                    QDBusConnection::systemBus(), this);

    if (!QDBusConnection::systemBus().connect(QL1S("org.freedesktop.UPower"),
                                        QL1S("/org/freedesktop/UPower"),
                                        QL1S("org.freedesktop.DBus.Properties"),
                                        QL1S("PropertiesChanged"),
                                        this,
                                        SLOT(uPowerChange())))
        qDebug() << "Could not connect to org.freedesktop.DBus.Properties.PropertiesChanged()";

    mIsClosed = mUPowerPropertiesInterface->property("LidIsClosed").toBool();
}

bool Lid::haveLid()
{
    return mUPowerInterface->property("LidIsPresent").toBool();
}

bool Lid::onBattery()
{
    return mUPowerInterface->property("OnBattery").toBool();
}

void Lid::uPowerChange()
{
    bool newIsClosed = mUPowerInterface->property("LidIsClosed").toBool();
    if (newIsClosed != mIsClosed)
    {
        mIsClosed = newIsClosed;
        emit changed(mIsClosed);
    }
}
