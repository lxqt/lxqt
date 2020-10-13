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

#include <QHttpMultiPart>
#include <QHttpPart>
#include <QNetworkRequest>
#include <QUrl>

#include "imgbbprovider.h"
#include "imgbbupload.h"

using namespace LxImage;

const QUrl gUploadURL(QStringLiteral("https://imgbb.com/json"));

Upload *ImgBBProvider::upload(QIODevice *device)
{
    // Create the file part of the multipart request
    QHttpPart filePart;
    filePart.setBodyDevice(device);
    filePart.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QStringLiteral(R"(form-data; name="source"; filename="upload.jpg")")
    );

    // Create the parts for the two parameters
    QHttpPart typePart;
    typePart.setBody(QByteArrayLiteral("file"));
    typePart.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QStringLiteral("form-data; name=\"type\"")
    );
    QHttpPart actionPart;
    actionPart.setBody(QByteArrayLiteral("upload"));
    actionPart.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QStringLiteral("form-data; name=\"action\"")
    );

    // Create the multipart and append the parts
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType, device);
    multiPart->append(typePart);
    multiPart->append(actionPart);
    multiPart->append(filePart);

    // Start the request and wrap it in an ImgBBUpload
    return new ImgBBUpload(sManager.post(QNetworkRequest(gUploadURL), multiPart));
}
