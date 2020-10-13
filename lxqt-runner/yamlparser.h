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

#ifndef YAMLPARSER_H
#define YAMLPARSER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QString>

class QIODevice;

class YamlParser : public QObject
{
    Q_OBJECT
public:
    YamlParser();
    virtual ~YamlParser();

    void consumeLine(QString line);

signals:
    void newListOfMaps(QList<QMap<QString, QString> > maps);

private:

    QList<QMap<QString, QString> > m_ListOfMaps;

    enum
    {
        start,
        atdocumentstart,
        inlist,
        documentdone,
        error
    } state;

    int m_CurrentIndent;
    QString m_LastKey;

    void addEntryToCurrentMap(QString key, QString value);
};

#endif /* YAMLPARSER_H */
