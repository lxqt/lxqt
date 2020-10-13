/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs
 * Copyright (C) 2018  Luís Pereira <luis.artur.pereira@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef XDGMIMEAPPS_H
#define XDGMIMEAPPS_H

#include <QObject>

#include "xdgmacros.h"

class XdgDesktopFile;
class XdgMimeAppsPrivate;

class QString;

/*!
 * \brief The XdgMimeApps class
 */
class QTXDG_API XdgMimeApps : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(XdgMimeApps)
    Q_DECLARE_PRIVATE(XdgMimeApps)

public:
    /*!
     * \brief XdgMimeApps constructor
     */
    explicit XdgMimeApps(QObject *parent = nullptr);

    /*!
      * \brief XdgMimeApps destructor
      */
    ~XdgMimeApps() override;

    /*!
     * \brief addSupport
     * \param mimeType
     * \param app
     * \return
     */
    bool addSupport(const QString &mimeType, const XdgDesktopFile &app);

    /*!
     * \brief allApps
     * \return
     */
    QList<XdgDesktopFile *> allApps();

    /*!
     * \brief apps
     * \param mimeType
     * \return
     */
    QList<XdgDesktopFile *> apps(const QString &mimeType);

    /*!
     * \brief categoryApps
     * \param category
     * \return
     */
    QList<XdgDesktopFile *> categoryApps(const QString &category);

    /*!
     * \brief fallbackApps
     * \param mimeType
     * \return
     */
    QList<XdgDesktopFile *> fallbackApps(const QString &mimeType);

    /*!
     * \brief recommendedApps
     * \param mimeType
     * \return
     */
    QList<XdgDesktopFile *> recommendedApps(const QString &mimeType);

    /*!
     * \brief defaultApp
     * \param mimeType
     * \return
     */
    XdgDesktopFile *defaultApp(const QString &mimeType);

    /*!
     * \brief removeSupport
     * \param mimeType
     * \param app
     * \return
     */
    bool removeSupport(const QString &mimeType, const XdgDesktopFile &app);

    /*!
     * \brief reset
     * \param mimeType
     * \return
     */
    bool reset(const QString &mimeType);

    /*!
     * \brief setDefaultApp
     * \param mimeType
     * \param app
     * \return
     */
    bool setDefaultApp(const QString &mimeType, const XdgDesktopFile &app);

Q_SIGNALS:
    void changed();
};

#endif // XDGMIMEAPPS_H
