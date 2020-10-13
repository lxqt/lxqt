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


#include <fcntl.h>
#include <unistd.h>

#include <cstddef>

#include "basestat.h"
#include "basestat_p.h"


namespace SysStat {

BaseStatPrivate::BaseStatPrivate(BaseStat* parent)
    : QObject(parent)
    , mTimer(new QTimer(this))
    , mSynchroTimer(new QTimer(this))
{
    mTimer->setSingleShot(false);

    mSynchroTimer->setSingleShot(false);
    connect(mSynchroTimer, SIGNAL(timeout()), SLOT(synchroTimeout()));
}

BaseStatPrivate::~BaseStatPrivate()
{
}

bool BaseStatPrivate::timerIsActive() const
{
    return mTimer->isActive();
}

int BaseStatPrivate::updateInterval() const
{
    return mTimer->interval();
}

void BaseStatPrivate::intervalChanged()
{
}

void BaseStatPrivate::sourceChanged()
{
}

void BaseStatPrivate::setUpdateInterval(int msec)
{
    mTimer->stop();
    mTimer->setInterval(msec);
    this->intervalChanged();
    mSynchroTimer->setInterval(msec / 10);
    if (msec > 0)
    {
        mLastSynchro = 0;
        mSynchroTimer->start();
    }
}

void BaseStatPrivate::synchroTimeout()
{
    QTime now(QTime::currentTime());
    int synchro = ((now.minute() * 60 + now.second()) * 1000 + now.msec() ) / mTimer->interval();
    if ((mLastSynchro != 0) && (mLastSynchro != synchro))
    {
        mSynchroTimer->stop();
        mTimer->start();
    }
    mLastSynchro = synchro;
}

void BaseStatPrivate::stopUpdating()
{
    mTimer->stop();
}

QString BaseStatPrivate::monitoredSource() const
{
    return mSource;
}

void BaseStatPrivate::setMonitoredSource(const QString &Source)
{
    mSource = Source;
    this->sourceChanged();
}

void BaseStatPrivate::monitorDefaultSource()
{
    mSource = defaultSource();
}

QString BaseStatPrivate::readAllFile(const char *filename)
{
    QString result;

    static const size_t bufferSize = 1 << 12; // 4096
    static char buffer[bufferSize];

    int fd = ::open(filename, O_RDONLY);
    if (fd > -1)
    {
        ssize_t size = ::read(fd, buffer, bufferSize);
        ::close(fd);
        if (size > 0)
            result = QString::fromLatin1(buffer, size);
    }

    return result;
}

QStringList BaseStatPrivate::sources() const
{
    return mSources;
}

BaseStat::BaseStat(QObject *parent)
    : QObject(parent)
{
}

BaseStat::~BaseStat()
{
}

QStringList BaseStat::sources() const
{
    return baseimpl->sources();
}

int BaseStat::updateInterval() const
{
    return baseimpl->updateInterval();
}

void BaseStat::setUpdateInterval(int msec)
{
    if ((updateInterval() != msec) || (!baseimpl->timerIsActive()))
    {
        baseimpl->setUpdateInterval(msec);
        emit updateIntervalChanged(msec);
    }
}

void BaseStat::stopUpdating()
{
    if (updateInterval() != 0)
    {
        baseimpl->stopUpdating();
        emit updateIntervalChanged(0);
    }
}

QString BaseStat::monitoredSource() const
{
    return baseimpl->monitoredSource();
}

void BaseStat::setMonitoredSource(const QString &source)
{
    if (monitoredSource() != source)
    {
        baseimpl->setMonitoredSource(source);
        emit monitoredSourceChanged(source);
    }
}

void BaseStat::monitorDefaultSource()
{
    QString oldSource = monitoredSource();
    baseimpl->monitorDefaultSource();
    if (monitoredSource() != oldSource)
        emit monitoredSourceChanged(monitoredSource());
}

}
