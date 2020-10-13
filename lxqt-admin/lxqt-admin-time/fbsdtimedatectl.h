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

#ifndef FBSDTIMEDATECTL_H
#define FBSDTIMEDATECTL_H
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include "itimedatectl.h"
class QDBusInterface;

class FBSDTimeDateCtl: public ITimeDateCtl
{
public:
    explicit FBSDTimeDateCtl();
    ~FBSDTimeDateCtl();

    bool useNtp() const;
    bool setUseNtp(bool value, QString& errorMessage);

    bool localRtc() const;
    bool setLocalRtc(bool value, QString& errorMessage);

    QString timeZone() const;
    bool setTimeZone(QString timeZone, QString& errorMessage);

    bool setDateTime(QDateTime dateTime, QString& errorMessage);
    bool pkexec();
private:
    QStringList mHelperArgs;

};
#endif // FSBDITEMDATECTL
