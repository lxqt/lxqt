/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#include <QRegExp>
#include <qmath.h>

#include "lxqtsysstatutils.h"


namespace PluginSysStat
{

QString netSpeedToString(int value)
{
    QString prefix;
    static const char prefixes[] = "kMG";
    if (value / 10)
        prefix = QLatin1Char(prefixes[value / 10 - 1]);

    return QStringLiteral("%1 %2B/s").arg(1 << (value % 10)).arg(prefix);
}

int netSpeedFromString(QString value)
{
    QRegExp re(QStringLiteral("^(\\d+) ([kMG])B/s$"));
    if (re.exactMatch(value))
    {
        int shift = 0;
        switch (re.cap(2).at(0).toLatin1())
        {
        case 'k':
            shift = 10;
            break;

        case 'M':
            shift = 20;
            break;

        case 'G':
            shift = 30;
            break;
        }

        return qCeil(qLn(re.cap(1).toInt()) / qLn(2.)) + shift;
    }

    return 0;
}

}
