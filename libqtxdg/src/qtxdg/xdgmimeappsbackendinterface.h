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

#ifndef XDGMIMEAPPSBACKENDINTERFACE_H
#define XDGMIMEAPPSBACKENDINTERFACE_H

#include <QObject>

class XdgDesktopFile;

class QString;

class XdgMimeAppsBackendInterface : public QObject {
    Q_OBJECT

public:
    explicit XdgMimeAppsBackendInterface(QObject *parent);
    virtual ~XdgMimeAppsBackendInterface();

    virtual bool addAssociation(const QString &mimeType, const XdgDesktopFile &app) = 0;
    virtual QList<XdgDesktopFile *> allApps() = 0;
    virtual QList<XdgDesktopFile *> apps(const QString &mimeType) = 0;
    virtual XdgDesktopFile *defaultApp(const QString &mimeType) = 0;
    virtual QList<XdgDesktopFile *> fallbackApps(const QString &mimeType) = 0;
    virtual QList<XdgDesktopFile *> recommendedApps(const QString &mimeType) = 0;
    virtual bool reset(const QString &mimeType) = 0;
    virtual bool removeAssociation(const QString &mimeType, const XdgDesktopFile &app) = 0;
    virtual bool setDefaultApp(const QString &mimeType, const XdgDesktopFile &app) = 0;

Q_SIGNALS:
    void changed();
};

#endif // XDGMIMEAPPSBACKENDINTERFACE_H
