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

#include "x11utils.h"

#include <xcb/xfixes.h>

#include <QDebug>
#include <QPainter>

void X11Utils::compositePointer(int offsetX, int offsetY, QPixmap *snapshot)
{
    Xcb::ScopedCPointer<xcb_xfixes_get_cursor_image_reply_t> cursor(
                xcb_xfixes_get_cursor_image_reply(Xcb::connection(),
                                                  xcb_xfixes_get_cursor_image_unchecked(Xcb::connection()),
                                                  nullptr));

    if (cursor.isNull()) {
        return;
    }

    QImage qcursorimg((uchar *) xcb_xfixes_get_cursor_image_cursor_image(cursor.data()),
                          cursor->width, cursor->height,
                          QImage::Format_ARGB32_Premultiplied);

    QPainter painter(snapshot);

    // NOTE: The device pixel ratio is not considered for the cursor automatically.
    qreal pixelRatio = snapshot->devicePixelRatio();
    qcursorimg.setDevicePixelRatio(pixelRatio);
    painter.drawImage(QPointF((cursor->x - cursor->xhot) / pixelRatio - offsetX, (cursor->y - cursor ->yhot) / pixelRatio - offsetY), qcursorimg);
}
