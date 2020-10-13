/***************************************************************************
 *   Copyright (C) 2010 - 2013 by Artem 'DOOMer' Galichkin                 *
 *   doomer3d@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef X11UTILS_H
#define X11UTILS_H

#include <QPixmap>
#include <QScopedPointer>
#include <QX11Info>

#include <X11/Xlib-xcb.h>
#include <fixx11h.h>
#include <xcb/xcb.h>

namespace X11Utils {
    void compositePointer(int offsetX, int offsetY, QPixmap *snapshot);
}


namespace Xcb {
    template <typename T>
    class ScopedCPointer : public QScopedPointer<T, QScopedPointerPodDeleter>
    {
    public:
        ScopedCPointer(T *p = 0) : QScopedPointer<T, QScopedPointerPodDeleter>(p) {}
    };

    inline xcb_connection_t *connection()
    {
        return XGetXCBConnection(QX11Info::display());
    }
} // namespace Xcb

#endif // X11UTILS_H

