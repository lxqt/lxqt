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


#include <QtCore/QString>
#include "version_p.h"


namespace SysStat {

namespace version {

QString verbose()
{
    return QString::fromLatin1("%1.%2.%3").arg(QLatin1String(MAJOR_VERSION_STR)).arg(QLatin1String(MINOR_VERSION_STR)).arg(QLatin1String(PATCH_VERSION_STR));
}

int majorNumber()
{
    return MAJOR_VERSION;
}

int minorNumber()
{
    return MINOR_VERSION;
}

int patchNumber()
{
    return PATCH_VERSION;
}

}

}
