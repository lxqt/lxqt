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

#include "xdgmimeapps.h"
#include "xdgmimeapps_p.h"

#include "xdgdesktopfile.h"
#include "xdgmacros.h"
#include "xdgmimeappsglibbackend.h"

#include <QMutexLocker>
#include <QDebug>

XdgMimeAppsPrivate::XdgMimeAppsPrivate()
    : mBackend(nullptr)
{
}

void XdgMimeAppsPrivate::init()
{
    Q_Q(XdgMimeApps);
    mBackend = new XdgMimeAppsGLibBackend(q);
    QObject::connect(mBackend, &XdgMimeAppsBackendInterface::changed, q, [=] () {
        Q_EMIT q->changed();
    });
}

XdgMimeAppsPrivate::~XdgMimeAppsPrivate()
{
}

XdgMimeApps::XdgMimeApps(QObject *parent)
    : QObject(*new XdgMimeAppsPrivate, parent)
{
    d_func()->init();
}

XdgMimeApps::~XdgMimeApps()
{
}

bool XdgMimeApps::addSupport(const QString &mimeType, const XdgDesktopFile &app)
{
    Q_D(XdgMimeApps);
    if (mimeType.isEmpty() || !app.isValid())
        return false;

    QMutexLocker locker(&d->mutex);
    return d->mBackend->addAssociation(mimeType, app);
}

QList<XdgDesktopFile *> XdgMimeApps::allApps()
{
    Q_D(XdgMimeApps);
    QMutexLocker locker(&d->mutex);
    return d->mBackend->allApps();
}

QList<XdgDesktopFile *> XdgMimeApps::apps(const QString &mimeType)
{
    Q_D(XdgMimeApps);
    if (mimeType.isEmpty())
        return QList<XdgDesktopFile *>();

    QMutexLocker locker(&d->mutex);
    return d->mBackend->apps(mimeType);
}

QList<XdgDesktopFile *> XdgMimeApps::categoryApps(const QString &category)
{
    if (category.isEmpty())
        return QList<XdgDesktopFile *>();

    const QString cat = category.toUpper();
    const QList <XdgDesktopFile *> apps = allApps();
    QList <XdgDesktopFile *> dl;
    for (XdgDesktopFile * const df : apps) {
        const QStringList categories = df->value(QL1S("Categories")).toString().toUpper().split(QL1C(';'));
        if (!categories.isEmpty() && (categories.contains(cat) || categories.contains(QL1S("X-") + cat)))
            dl.append(df);
        else
            delete df;
    }
    return dl;
}

QList<XdgDesktopFile *> XdgMimeApps::fallbackApps(const QString &mimeType)
{
    Q_D(XdgMimeApps);
    if (mimeType.isEmpty())
        return QList<XdgDesktopFile *>();

    QMutexLocker locker(&d->mutex);
    return d->mBackend->fallbackApps(mimeType);
}

QList<XdgDesktopFile *> XdgMimeApps::recommendedApps(const QString &mimeType)
{
    Q_D(XdgMimeApps);
    if (mimeType.isEmpty())
        return QList<XdgDesktopFile *>();

    QMutexLocker locker(&d->mutex);
    return d->mBackend->recommendedApps(mimeType);
}

XdgDesktopFile *XdgMimeApps::defaultApp(const QString &mimeType)
{
    Q_D(XdgMimeApps);
    if (mimeType.isEmpty())
        return nullptr;

    QMutexLocker locker(&d->mutex);
    return d->mBackend->defaultApp(mimeType);
}

bool XdgMimeApps::removeSupport(const QString &mimeType, const XdgDesktopFile &app)
{
    Q_D(XdgMimeApps);
    if (mimeType.isEmpty() || !app.isValid())
        return false;

    QMutexLocker locker(&d->mutex);
    return d->mBackend->removeAssociation(mimeType, app);
}

bool XdgMimeApps::reset(const QString &mimeType)
{
    Q_D(XdgMimeApps);
    if (mimeType.isEmpty())
        return false;

    QMutexLocker locker(&d->mutex);
    return d->mBackend->reset(mimeType);
}

bool XdgMimeApps::setDefaultApp(const QString &mimeType, const XdgDesktopFile &app)
{
    Q_D(XdgMimeApps);
    if (mimeType.isEmpty() || !app.isValid())
        return false;

    if (XdgDesktopFile::id(app.fileName()).isEmpty())
        return false;

    QMutexLocker locker(&d->mutex);
    return d->mBackend->setDefaultApp(mimeType, app);
}

#include "moc_xdgmimeapps.cpp"
