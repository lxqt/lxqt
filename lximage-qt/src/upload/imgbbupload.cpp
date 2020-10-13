/*
    LxImage - image viewer and screenshot tool for lxqt
    Copyright (C) 2017  Nathan Osman <nathan@quickmediasolutions.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <QJsonDocument>
#include <QJsonObject>

#include "imgbbupload.h"

using namespace LxImage;

ImgBBUpload::ImgBBUpload(QNetworkReply *reply)
    : Upload(reply)
{
}

void ImgBBUpload::processReply(const QByteArray &data)
{
    // Obtain the root object from the JSON response
    QJsonObject object(QJsonDocument::fromJson(data).object());

    // Attempt to retrieve the image->url value
    QString url = object.value(QStringLiteral("image")).toObject().value(QStringLiteral("url")).toString();

    // Check for success
    if (url.isNull()) {
        QString message = object.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString();
        if (message.isNull()) {
            message = tr("unknown error response");
        }
        Q_EMIT error(message);
    } else {
        Q_EMIT completed(url);
    }
}
