/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
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

#include "string_utils.h"


QString joinToString(const QStringList &list, const QString &prefix, const QString &joiner, const QString &postfix)
{
    QString result = list.join(joiner);
    if (!result.isEmpty())
    {
        result = prefix + result + postfix;
    }
    return result;
}

QString joinCommandLine(const QString &command, QStringList arguments)
{
    arguments.prepend(command);
    int m = arguments.length();
    for (int i = 0; i < m; ++i)
    {
        QString &item = arguments[i];
        if (item.contains(QRegExp(QStringLiteral("[ \r\n\t\"']"))))
        {
            item.prepend(QLatin1Char('\'')).append(QLatin1Char('\''));
        }
        else if (item.isEmpty())
        {
            item = QString(QStringLiteral("''"));
        }
    }
    return arguments.join(QLatin1Char(' '));
}
