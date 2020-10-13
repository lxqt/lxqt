/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
 * Authors:
 *   Christian Surlykke <christian@surlykke.dk>
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

#include <QIODevice>
#include <QRegExp>
#include <QDebug>

#include <LXQt/Globals>

#include "yamlparser.h"

YamlParser::YamlParser()
{
    state = start;
}

YamlParser::~YamlParser()
{
}

void YamlParser::consumeLine(QString line)
{
    static QRegExp documentStart(QSL("---\\s*(\\[\\]\\s*)?"));
    static QRegExp mapStart(QSL("(-\\s*)(\\w*)\\s*:(.*)$"));
    static QRegExp mapEntry(QSL("(\\s*)(\\w*)\\s*:(.*)"));
    static QRegExp continuation(QSL("(\\s*)(.*)"));
    static QRegExp documentEnd(QSL("...\\s*"));
    static QRegExp emptyLine(QSL("\\s*(#.*)?"));

    qDebug() << line;

    if (documentStart.exactMatch(line))
    {
        m_ListOfMaps.clear();
        state = atdocumentstart;
        m_CurrentIndent = -1;
    }
    else if (state == error)
    {
        // Skip
    }
    else if (emptyLine.exactMatch(line))
    {
        // Skip
    }
    else if ((state == atdocumentstart || state == inlist) && mapStart.exactMatch(line))
    {
        m_ListOfMaps << QMap<QString, QString>();
        addEntryToCurrentMap(mapStart.cap(2), mapStart.cap(3));
        m_CurrentIndent = mapStart.cap(1).size();
        state = inlist;
    }
    else if (state == inlist && mapEntry.exactMatch(line) && mapEntry.cap(1).size() == m_CurrentIndent)
    {
        addEntryToCurrentMap(mapEntry.cap(2), mapEntry.cap(3));
    }
    else if (state == inlist && continuation.exactMatch(line) && continuation.cap(1).size() > m_CurrentIndent)
    {
        m_ListOfMaps.last()[m_LastKey].append(continuation.cap(2));
    }
    else if ((state == atdocumentstart || state == inlist) && documentEnd.exactMatch(line))
    {
        qDebug() << "emitting:" << m_ListOfMaps;
        emit newListOfMaps(m_ListOfMaps);
        state = documentdone;
    }
    else
    {
        qWarning() << "Yaml parser could not read:" << line;
        state = error;
    }
}

void YamlParser::addEntryToCurrentMap(QString key, QString value)
{
    m_ListOfMaps.last()[key.trimmed()] = value.trimmed();
    m_LastKey = key.trimmed();
}
