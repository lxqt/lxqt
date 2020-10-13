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

#ifndef LXIMAGE_UPLOAD_H
#define LXIMAGE_UPLOAD_H

#include <QNetworkReply>
#include <QObject>

namespace LxImage {

/**
 * @brief Base class for uploads
 */
class Upload : public QObject
{
    Q_OBJECT

public:

    /**
     * @brief Create an upload
     * @param reply network reply
     *
     * The upload will assume ownership of the network reply and connect to its
     * signals, emitting uploadError() when something goes wrong.
     */
    explicit Upload(QNetworkReply *reply);

    /**
     * @brief Abort the upload
     */
    void abort();

Q_SIGNALS:

    /**
     * @brief Indicate that upload progress has changed
     * @param value new progress value
     */
    void progress(int value);

    /**
     * @brief Indicate that the upload completed
     * @param url new URL of the upload
     */
    void completed(const QString &url);

    /**
     * @brief Indicate that an error occurred
     * @param message description of the error
     */
    void error(const QString &message);

    /**
     * @brief Indicate that the upload finished
     *
     * This signal is emitted after either completed() or error().
     */
    void finished();

protected:

    /**
     * @brief Process the data from the reply
     * @param data content from the reply
     *
     * This method should parse the data and either emit the completed() or
     * error() signal.
     */
    virtual void processReply(const QByteArray &data) = 0;

private:

    QNetworkReply *mReply;
};

}

#endif // LXIMAGE_UPLOAD_H
