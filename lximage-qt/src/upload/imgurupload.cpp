/*
 * LXImage-Qt - a simple and fast image viewer
 * Copyright (C) 2013  PCMan <pcman.tw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <QJsonDocument>
#include <QJsonObject>

#include "imgurupload.h"

using namespace LxImage;

ImgurUpload::ImgurUpload(QNetworkReply *reply)
    : Upload(reply)
{
}

void ImgurUpload::processReply(const QByteArray &data)
{
    // Obtain the root object from the JSON response
    QJsonObject object(QJsonDocument::fromJson(data).object());

    // Attempt to retrieve the value for "success" and "data->link"
    bool success = object.value(QStringLiteral("success")).toBool();
    QJsonObject dataObject = object.value(QStringLiteral("data")).toObject();
    QString dataLink = dataObject.value(QStringLiteral("link")).toString();
    QString dataError = dataObject.value(QStringLiteral("error")).toString();

    // Ensure that "success" is true & link is valid, otherwise throw an error
    if (!success || dataLink.isNull()) {
        if (dataError.isNull()) {
            dataError = tr("unknown error response");
        }
        Q_EMIT error(dataError);
    } else {
        Q_EMIT completed(dataLink);
    }
}
