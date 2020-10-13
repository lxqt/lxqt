/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs
 * Copyright (C) 2014  Luís Pereira <luis.artur.pereira@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "xdgmimetype.h"

#include "xdgicon.h"

#include <algorithm>

class XdgMimeTypePrivate : public QSharedData {
public:
    XdgMimeTypePrivate();
    XdgMimeTypePrivate(const XdgMimeType& other);

    void computeIconName();

    QString iconName;
    bool computed;
};


XdgMimeTypePrivate::XdgMimeTypePrivate()
    : computed(false)
{
}


XdgMimeTypePrivate::XdgMimeTypePrivate(const XdgMimeType& other)
    : iconName(other.dx->iconName),
      computed(other.dx->computed)
{
}


XdgMimeType::XdgMimeType()
    : QMimeType(),
      dx(new XdgMimeTypePrivate())
{
}


XdgMimeType::XdgMimeType(const QMimeType& mime)
    : QMimeType(mime),
      dx(new XdgMimeTypePrivate())
{
}


XdgMimeType::XdgMimeType(const XdgMimeType& mime)
    : QMimeType(mime),
      dx(mime.dx)
{
}


XdgMimeType &XdgMimeType::operator=(const XdgMimeType &other)
{
    QMimeType::operator =(other);

    if (dx != other.dx)
        dx = other.dx;

    return *this;
}

void XdgMimeType::swap(XdgMimeType &other) noexcept
{
    QMimeType::swap(other);
    std::swap(dx, other.dx);
}

XdgMimeType::~XdgMimeType()
{
}


QString XdgMimeType::iconName() const
{
    if (dx->computed) {
        return dx->iconName;
    } else {
        dx->iconName.clear();
        QStringList names;

        names.append(QMimeType::iconName());
        names.append(QMimeType::genericIconName());

        for (const QString &s : qAsConst(names)) {
            if (!XdgIcon::fromTheme(s).isNull()) {
                dx->iconName = s;
                break;
            }
        }
        dx->computed = true;
        return dx->iconName;
    }
}


QIcon XdgMimeType::icon() const
{
    return XdgIcon::fromTheme((iconName()));
}
