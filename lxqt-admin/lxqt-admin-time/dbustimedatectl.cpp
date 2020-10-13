/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2016 LXQt team
 * Authors:
 *   Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#include "dbustimedatectl.h"
#include <LXQt/Globals>
#include <QProcess>
#include <QDebug>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QMessageBox>


DbusTimeDateCtl::DbusTimeDateCtl()
{
    mIface = new QDBusInterface(QStringLiteral("org.freedesktop.timedate1"),
                                QStringLiteral("/org/freedesktop/timedate1"),
                                QStringLiteral("org.freedesktop.timedate1"),
                                QDBusConnection::systemBus());
}

DbusTimeDateCtl::~DbusTimeDateCtl()
{
    delete mIface;
}

QString DbusTimeDateCtl::timeZone() const
{
    return mIface->property("Timezone").toString();
}

bool DbusTimeDateCtl::setTimeZone(QString timeZone, QString& errorMessage)
{
    mIface->call(QSL("SetTimezone"), timeZone, true);
    QDBusError err = mIface->lastError();
    if(err.isValid())
    {
        errorMessage = err.message();
        return false;
    }
    return true;
}

bool DbusTimeDateCtl::setDateTime(QDateTime dateTime, QString& errorMessage)
{
    // the timedatectl dbus service accepts "usec" input.
    // Qt can only get "msec"  => convert to usec here.
    mIface->call(QSL("SetTime"), dateTime.toMSecsSinceEpoch() * 1000, false, true);
    QDBusError err = mIface->lastError();
    if(err.isValid())
    {
        errorMessage = err.message();
        return false;
    }
    return true;
}

bool DbusTimeDateCtl::useNtp() const
{
    return mIface->property("NTP").toBool();
}

bool DbusTimeDateCtl::setUseNtp(bool value, QString& errorMessage)
{
    mIface->call(QSL("SetNTP"), value, true);
    QDBusError err = mIface->lastError();
    if(err.isValid())
    {
        errorMessage = err.message();
        return false;
    }
    return true;
}

bool DbusTimeDateCtl::localRtc() const
{
    return mIface->property("LocalRTC").toBool();
}

bool DbusTimeDateCtl::setLocalRtc(bool value, QString& errorMessage)
{
    mIface->call(QSL("SetLocalRTC"), value, false, true);
    QDBusError err = mIface->lastError();
    if(err.isValid())
    {
        errorMessage = err.message();
        return false;
    }
    return true;
}
