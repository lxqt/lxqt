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

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "imgurprovider.h"
#include "imgurupload.h"

using namespace LxImage;

const QUrl gUploadURL(QStringLiteral("https://api.imgur.com/3/upload.json"));
const QByteArray gAuthHeader = "Client-ID 63ff047cd8bcf9e";
const QByteArray gTypeHeader = "application/x-www-form-urlencoded";

Upload *ImgurProvider::upload(QIODevice *device)
{
    // Create the request with the correct HTTP headers
    QNetworkRequest request(gUploadURL);
    request.setHeader(QNetworkRequest::ContentTypeHeader, gTypeHeader);
    request.setRawHeader("Authorization", gAuthHeader);

    // Start the request and wrap it in an ImgurUpload
    return new ImgurUpload(sManager.post(request, device));
}
