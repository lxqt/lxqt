/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs
 * Copyright (C) 2020  Luís Pereira <luis.artur.pereira@gmail.com>
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

#ifndef XDGDEFAULTAPPS_H
#define XDGDEFAULTAPPS_H

#include "xdgmacros.h"

class XdgDesktopFile;

/*!
 * \brief Sets the default app for special case applications categories(web browser, etc)
 */
class QTXDG_API XdgDefaultApps {

public:
    /*!
     * \brief Sets the default email client
     * \return The default email client. nullptr if it's not set or a error ocurred.
     */
    static XdgDesktopFile *emailClient();

    /*!
     * \brief Gets the installed email clients
     * \return A list of installed email clients
     */
    static QList<XdgDesktopFile *> emailClients();

    /*!
     * \brief Sets the default file manager
     * \return The default file manager. nullptr if it's not set or a error ocurred.
     */
    static XdgDesktopFile *fileManager();

    /*!
     * \brief Gets the installed file managers
     * \return A list of installed file managers
     */
    static QList<XdgDesktopFile *> fileManagers();

    /*!
     * \brief Sets the default email client
     * \param The app to be set as the default email client
     * \return True if successful, false otherwise
     */
    static bool setEmailClient(const XdgDesktopFile &app);

    /*!
     * \brief Sets the default file manager
     * \param The app to be set as the default file manager
     * \return True if successful, false otherwise
     */
    static bool setFileManager(const XdgDesktopFile &app);

    /*!
     * \brief Sets the default web browser
     * \param The app to be set as the default web browser
     * \return True if successful, false otherwise
     */
    static bool setWebBrowser(const XdgDesktopFile &app);

    /*!
     * \brief Gets the default web browser
     * \return The default web browser. nullptr if it's not set or a error ocurred.
     */
    static XdgDesktopFile *webBrowser();

    /*!
     * \brief Gets the installed web browsers
     * \return A list of installed web browsers
     */
    static QList<XdgDesktopFile *> webBrowsers();
};

#endif // XDGDEFAULTAPPS_H
