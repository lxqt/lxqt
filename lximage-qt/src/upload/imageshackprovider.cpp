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

#include <QHttpMultiPart>
#include <QHttpPart>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

#include "imageshackprovider.h"
#include "imageshackupload.h"

using namespace LxImage;

const QString gUploadURL = QStringLiteral("https://api.imageshack.com/v2/images");
const QByteArray gAPIKey = "4DINORVXbcbda9ac64b424a0e6b37caed4cf3b8b";

Upload *ImageShackProvider::upload(QIODevice *device)
{
    // Construct the URL that will be used for the upload
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("api_key"), QString::fromUtf8(gAPIKey));
    QUrl url(gUploadURL);
    url.setQuery(query);

    // The first (and only) part is the file upload
    QHttpPart filePart;
    filePart.setBodyDevice(device);
    filePart.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QString::fromLatin1(R"(form-data; name="file"; filename="upload.jpg")")
    );

    // Create the multipart and append the file part
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType, device);
    multiPart->append(filePart);

    // Start the request and wrap it in an ImageShackUpload
    return new ImageShackUpload(sManager.post(QNetworkRequest(url), multiPart));
}
