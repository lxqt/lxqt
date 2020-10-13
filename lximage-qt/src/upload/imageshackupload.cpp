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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "imageshackupload.h"

using namespace LxImage;

ImageShackUpload::ImageShackUpload(QNetworkReply *reply)
    : Upload(reply)
{
}

void ImageShackUpload::processReply(const QByteArray &data)
{
    // Obtain the root object from the JSON response
    QJsonObject object(QJsonDocument::fromJson(data).object());

    // Attempt to retrieve the link
    bool success = object.value(QStringLiteral("success")).toBool();
    QString link = object.value(QStringLiteral("result")).toObject().value(QStringLiteral("images")).toArray()
            .at(0).toObject().value(QStringLiteral("direct_link")).toString();

    // Check for success
    if (!success || link.isNull()) {
        QString errorMessage = object.value(QStringLiteral("error")).toObject()
                .value(QStringLiteral("error_message")).toString();
        if (errorMessage.isNull()) {
            errorMessage = tr("unknown error response");
        }
        Q_EMIT error(errorMessage);
    } else {
        Q_EMIT completed(QStringLiteral("https://") + link);
    }
}
