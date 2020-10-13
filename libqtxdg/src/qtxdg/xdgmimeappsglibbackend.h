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

#ifndef XDGMIMEAPPSGLIBBACKEND_H
#define XDGMIMEAPPSGLIBBACKEND_H

#include "xdgmimeappsbackendinterface.h"

class XdgDesktopFile;

class QString;

typedef struct _GAppInfoMonitor GAppInfoMonitor;

class Q_DECL_HIDDEN XdgMimeAppsGLibBackend : public XdgMimeAppsBackendInterface {
public:
    XdgMimeAppsGLibBackend(QObject *parent);
    ~XdgMimeAppsGLibBackend() override;

    bool addAssociation(const QString &mimeType, const XdgDesktopFile &app) override;
    QList<XdgDesktopFile *> allApps() override;
    QList<XdgDesktopFile *> apps(const QString &mimeType) override;
    XdgDesktopFile *defaultApp(const QString &mimeType) override;
    QList<XdgDesktopFile *> fallbackApps(const QString &mimeType) override;
    QList<XdgDesktopFile *> recommendedApps(const QString &mimeType) override;
    bool removeAssociation(const QString &mimeType, const XdgDesktopFile &app) override;
    bool reset(const QString &mimeType) override;
    bool setDefaultApp(const QString &mimeType, const XdgDesktopFile &app) override;

private:
    GAppInfoMonitor *mWatcher;
    static void _changed(GAppInfoMonitor *monitor, XdgMimeAppsGLibBackend *_this);
};

#endif // XDGMIMEAPPSGLIBBACKEND_H
