/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
**
**  SysStat is a Qt-based interface to system statistics
**
**  Authors:
**       Copyright (c) 2009 - 2012 Kuzma Shapran <Kuzma.Shapran@gmail.com>
**
**  This library is free software; you can redistribute it and/or
**  modify it under the terms of the GNU Lesser General Public
**  License as published by the Free Software Foundation; either
**  version 2.1 of the License, or (at your option) any later version.
**
**  This library is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**  Lesser General Public License for more details.
**
**  You should have received a copy of the GNU Lesser General Public
**  License along with this library;
**  if not, write to the Free Software Foundation, Inc.,
**  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** END_COMMON_COPYRIGHT_HEADER */


#ifndef LIBSYSSTAT__BASE_STAT__INCLUDED
#define LIBSYSSTAT__BASE_STAT__INCLUDED


#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "sysstat_global.h"


namespace SysStat {

class BaseStatPrivate;

class SYSSTATSHARED_EXPORT BaseStat : public QObject
{
    Q_OBJECT

public:
    BaseStat(QObject *parent = nullptr);
    ~BaseStat() override;

    QStringList sources() const;

signals:
    void updateIntervalChanged(int);
    void monitoredSourceChanged(QString);

public:
    Q_PROPERTY(int updateInterval READ updateInterval WRITE setUpdateInterval RESET stopUpdating NOTIFY updateIntervalChanged)
    Q_PROPERTY(QString monitoredSource READ monitoredSource WRITE setMonitoredSource RESET monitorDefaultSource NOTIFY monitoredSourceChanged)

public slots:
    int updateInterval() const;
    void setUpdateInterval(int msec);
    void stopUpdating();

    QString monitoredSource() const;
    void setMonitoredSource(const QString &Source);
    void monitorDefaultSource();

protected:
    BaseStatPrivate* baseimpl;
};

}

#endif //LIBSYSSTAT__BASE_STAT__INCLUDED
