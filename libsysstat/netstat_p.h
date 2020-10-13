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


#ifndef LIBSYSSTAT__NET_STAT__PRIVATE__INCLUDED
#define LIBSYSSTAT__NET_STAT__PRIVATE__INCLUDED


#include <QtCore/QObject>
#include <QtCore/QtGlobal>
#include <QtCore/QMap>

#include "basestat_p.h"
#include "netstat.h"


namespace SysStat {

class NetStatPrivate : public BaseStatPrivate
{
    Q_OBJECT

public:
    NetStatPrivate(NetStat *parent = nullptr);
    ~NetStatPrivate() override;

signals:
    void update(unsigned received, unsigned transmitted);

private slots:
    void timeout();

private:
    QString defaultSource() override;

    typedef struct Values
    {
        Values();

        qulonglong received;
        qulonglong transmitted;
    } Values;
    typedef QMap<QString, Values> NamedValues;
    NamedValues mPrevious;
};

}

#endif //LIBSYSSTAT__NET_STAT__PRIVATE__INCLUDED
