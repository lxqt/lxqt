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


#ifndef LIBSYSSTAT__BASE_STAT__PRIVATE__INCLUDED
#define LIBSYSSTAT__BASE_STAT__PRIVATE__INCLUDED


#include <QtCore/QObject>
#include <QtCore/QtGlobal>
#include <QtCore/QTimer>
#include <QtCore/QTime>
#include <QtCore/QStringList>

#include "cpustat.h"


namespace SysStat {

class BaseStatPrivate : public QObject
{
    Q_OBJECT

public:
    BaseStatPrivate(BaseStat *parent = nullptr);
    ~BaseStatPrivate() override;

    QStringList sources() const;

    bool timerIsActive() const;
    int updateInterval() const;
    void setUpdateInterval(int msec);
    void stopUpdating();

    QString monitoredSource() const;
    void setMonitoredSource(const QString &Source);
    void monitorDefaultSource();

private slots:
    void synchroTimeout();

protected:
    virtual QString defaultSource() = 0;

    QString readAllFile(const char *filename);

    QTimer *mTimer;
    QTimer *mSynchroTimer;
    QString mSource;
    QStringList mSources;

    int mLastSynchro;

    virtual void intervalChanged();
    virtual void sourceChanged();
};

}

#endif //LIBSYSSTAT__BASE_STAT__PRIVATE__INCLUDED
