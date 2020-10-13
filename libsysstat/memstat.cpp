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


#include "memstat.h"
#include "memstat_p.h"


namespace SysStat {

MemStatPrivate::MemStatPrivate(MemStat *parent)
    : BaseStatPrivate(parent)
{
    mSource = defaultSource();

    connect(mTimer, SIGNAL(timeout()), SLOT(timeout()));

    mSources << QLatin1String("memory") << QLatin1String("swap");
}

MemStatPrivate::~MemStatPrivate()
{
}

void MemStatPrivate::timeout()
{
    qulonglong memTotal = 0;
    qulonglong memFree = 0;
    qulonglong memBuffers = 0;
    qulonglong memCached = 0;
    qulonglong swapTotal = 0;
    qulonglong swapFree = 0;

    const QStringList rows = readAllFile("/proc/meminfo").split(QLatin1Char('\n'), QString::SkipEmptyParts);
    for (const QString &row : rows)
    {
        QStringList tokens = row.split(QLatin1Char(' '), QString::SkipEmptyParts);
        if (tokens.size() != 3)
            continue;

        if (tokens[0] == QLatin1String("MemTotal:"))
            memTotal = tokens[1].toULong();
        else if(tokens[0] == QLatin1String("MemFree:"))
            memFree = tokens[1].toULong();
        else if(tokens[0] == QLatin1String("Buffers:"))
            memBuffers = tokens[1].toULong();
        else if(tokens[0] == QLatin1String("Cached:"))
            memCached = tokens[1].toULong();
        else if(tokens[0] == QLatin1String("SwapTotal:"))
            swapTotal = tokens[1].toULong();
        else if(tokens[0] == QLatin1String("SwapFree:"))
            swapFree = tokens[1].toULong();
    }

    if (mSource == QLatin1String("memory"))
    {
        if (memTotal)
        {
            float memTotal_d     = static_cast<float>(memTotal);
            float applications_d = static_cast<float>(memTotal - memFree - memBuffers - memCached) / memTotal_d;
            float buffers_d      = static_cast<float>(memBuffers) / memTotal_d;
            float cached_d       = static_cast<float>(memCached) / memTotal_d;

            emit memoryUpdate(applications_d, buffers_d, cached_d);
        }
    }
    else if (mSource == QLatin1String("swap"))
    {
        if (swapTotal)
        {
            float swapUsed_d = static_cast<float>(swapTotal - swapFree) / static_cast<float>(swapTotal);

            emit swapUpdate(swapUsed_d);
        }
    }
}

QString MemStatPrivate::defaultSource()
{
    return QLatin1String("memory");
}

MemStat::MemStat(QObject *parent)
    : BaseStat(parent)
{
    impl = new MemStatPrivate(this);
    baseimpl = impl;

    connect(impl, SIGNAL(memoryUpdate(float,float,float)), this, SIGNAL(memoryUpdate(float,float,float)));
    connect(impl, SIGNAL(swapUpdate(float)), this, SIGNAL(swapUpdate(float)));
}

MemStat::~MemStat()
{
}

}
