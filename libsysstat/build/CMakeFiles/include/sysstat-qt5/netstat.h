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


#ifndef LIBSYSSTAT__NET_STAT__INCLUDED
#define LIBSYSSTAT__NET_STAT__INCLUDED


#include <QtCore/QObject>

#include "basestat.h"


namespace SysStat {

class NetStatPrivate;

class SYSSTATSHARED_EXPORT NetStat : public BaseStat
{
    Q_OBJECT

public:
    NetStat(QObject *parent = nullptr);
    ~NetStat() override;

signals:
    void update(unsigned received, unsigned transmitted);

protected:
    NetStatPrivate* impl;
};

}

#endif //LIBSYSSTAT__NET_STAT__INCLUDED
