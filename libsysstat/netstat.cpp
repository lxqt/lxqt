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


#include "netstat.h"
#include "netstat_p.h"


namespace SysStat {

NetStatPrivate::NetStatPrivate(NetStat *parent)
    : BaseStatPrivate(parent)
{
    mSource = defaultSource();

    connect(mTimer, SIGNAL(timeout()), SLOT(timeout()));


    QStringList rows(readAllFile("/proc/net/dev").split(QLatin1Char('\n'), QString::SkipEmptyParts));

    rows.erase(rows.begin(), rows.begin() + 2);

    for (const QString &row : qAsConst(rows))
    {
        QStringList tokens = row.split(QLatin1Char(':'), QString::SkipEmptyParts);
        if (tokens.size() != 2)
            continue;

        mSources.append(tokens[0].trimmed());
    }
}

NetStatPrivate::~NetStatPrivate()
{
}

void NetStatPrivate::timeout()
{
    QStringList rows(readAllFile("/proc/net/dev").split(QLatin1Char('\n'), QString::SkipEmptyParts));


    if (rows.size() < 2)
        return;

    QStringList names = rows[1].split(QLatin1Char('|'));
    if (names.size() != 3)
        return;
    QStringList namesR = names[1].split(QLatin1Char(' '), QString::SkipEmptyParts);
    QStringList namesT = names[2].split(QLatin1Char(' '), QString::SkipEmptyParts);
    int receivedIndex    =                 namesR.indexOf(QLatin1String("bytes"));
    int transmittedIndex = namesR.size() + namesT.indexOf(QLatin1String("bytes"));

    rows.erase(rows.begin(), rows.begin() + 2);

    for (const QString &row : qAsConst(rows))
    {
        QStringList tokens = row.split(QLatin1Char(':'), QString::SkipEmptyParts);
        if (tokens.size() != 2)
            continue;

        QString interfaceName = tokens[0].trimmed();

        QStringList data = tokens[1].split(QLatin1Char(' '), QString::SkipEmptyParts);
        if (data.size() < transmittedIndex)
            continue;

        Values current;
        current.received    = data[receivedIndex   ].toULongLong();
        current.transmitted = data[transmittedIndex].toULongLong();

        if (!mPrevious.contains(interfaceName))
            mPrevious.insert(interfaceName, Values());
        const Values &previous = mPrevious[interfaceName];

        if (interfaceName == mSource)
            emit update((( current.received - previous.received ) * 1000 ) / mTimer->interval(), (( current.transmitted - previous.transmitted ) * 1000 ) / mTimer->interval());

        mPrevious[interfaceName] = current;
    }
}

QString NetStatPrivate::defaultSource()
{
    return QLatin1String("lo");
}

NetStatPrivate::Values::Values()
    : received(0)
    , transmitted(0)
{
}

NetStat::NetStat(QObject *parent)
    : BaseStat(parent)
{
    impl = new NetStatPrivate(this);
    baseimpl = impl;

    connect(impl, SIGNAL(update(unsigned,unsigned)), this, SIGNAL(update(unsigned,unsigned)));
}

NetStat::~NetStat()
{
}

}
