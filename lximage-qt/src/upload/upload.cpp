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

#include "upload.h"

using namespace LxImage;

Upload::Upload(QNetworkReply *reply)
    : mReply(reply)
{
    // Reparent the reply to this object
    mReply->setParent(this);

    // Emit progress() when upload progress changes
    connect(mReply, &QNetworkReply::uploadProgress, [this](qint64 bytesSent, qint64 bytesTotal) {
        Q_EMIT progress(static_cast<int>(
            static_cast<double>(bytesSent) / static_cast<double>(bytesTotal) * 100.0
        ));
    });

    // Emit error() when a socket error occurs
    connect(mReply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [this](QNetworkReply::NetworkError) {
        Q_EMIT error(mReply->errorString());
    });

    // Process the request when it finishes
    connect(mReply, &QNetworkReply::finished, [this]() {
        if (mReply->error() == QNetworkReply::NoError) {
            processReply(mReply->readAll());
        }
    });

    // Emit finished() when completed() or error() is emitted
    connect(this, &Upload::completed, this, &Upload::finished);
    connect(this, &Upload::error, this, &Upload::finished);
}

void Upload::abort()
{
    mReply->abort();
}
