/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2014 LXQt team
 * Authors:
 *   Luís Pereira <luis.artur.pereira.gmail.com>
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


#ifndef MIMETYPEDATA_H
#define MIMETYPEDATA_H

#include <XdgMimeType>
#include <QMetaType>


class MimeTypeData {
public:
    MimeTypeData();
    MimeTypeData(const XdgMimeType& mime);

    QString inline name() const { return mName; };
    QString inline patterns() const { return mPatterns; };
    QString inline comment() const { return mComment; };

    void setName(const QString name) {
        mName = name;
    }

    bool matches(const QString& filter) const;

private:
    QString mName;
    QString mPatterns;
    QString mComment;
};

Q_DECLARE_METATYPE(MimeTypeData)
#endif // MIMETYPEDATA_H
