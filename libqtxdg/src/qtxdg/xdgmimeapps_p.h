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

#ifndef XDGMIMEAPPS_P_H
#define XDGMIMEAPPS_P_H

#include <private/qobject_p.h>
#include <QMutex>

class XdgMimeApps;
class XdgMimeAppsBackendInterface;

class Q_DECL_HIDDEN XdgMimeAppsPrivate : public QObjectPrivate {
    Q_DECLARE_PUBLIC(XdgMimeApps)

public:
    XdgMimeAppsPrivate();
    ~XdgMimeAppsPrivate();

    void init();
    static XdgMimeAppsPrivate *instance();

    QMutex mutex;
    XdgMimeAppsBackendInterface *mBackend;
};

#endif // XDGMIMEAPPS_P_H
