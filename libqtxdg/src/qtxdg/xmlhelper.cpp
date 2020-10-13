/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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


#include "xmlhelper.h"

#include <QDebug>
#include <QtXml/QDomElement>
#include <QtXml/QDomNode>


QDebug operator<<(QDebug dbg, const QDomElement &el)
{
    QDomNamedNodeMap map = el.attributes();

    QString args;
    for (int i=0; i<map.count(); ++i)
        args += QLatin1Char(' ') + map.item(i).nodeName() + QLatin1Char('=') + map.item(i).nodeValue() + QLatin1Char('\'');

    dbg.nospace() << QString::fromLatin1("<%1%2>%3</%1>").arg(el.tagName(), args, el.text());
    return dbg.space();
}
