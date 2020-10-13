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


#ifndef LIBSYSSTAT__CPU_STAT__INCLUDED
#define LIBSYSSTAT__CPU_STAT__INCLUDED


#include <QtCore/QObject>

#include "basestat.h"


namespace SysStat {

class CpuStatPrivate;

class SYSSTATSHARED_EXPORT CpuStat : public BaseStat
{
    Q_OBJECT

    Q_ENUMS(Monitoring)

public:
    enum Monitoring { LoadAndFrequency, LoadOnly, FrequencyOnly };

public:
    CpuStat(QObject *parent = nullptr);
    ~CpuStat() override;

    void updateSources();

    uint minFreq(const QString &source) const;
    uint maxFreq(const QString &source) const;

signals:
    void update(float user, float nice, float system, float other, float frequencyRate, uint frequency);
    void update(float user, float nice, float system, float other);
    void update(uint frequency);

    void monitoringChanged(Monitoring);

public:
    Q_PROPERTY(Monitoring monitoring READ monitoring WRITE setMonitoring NOTIFY monitoringChanged)

public slots:
    Monitoring monitoring() const;
    void setMonitoring(Monitoring value);

protected:
    CpuStatPrivate* impl;
};

}

#endif //LIBSYSSTAT__CPU_STAT__INCLUDED
