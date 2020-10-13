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

#include "xdgmimeappsglibbackend.h"
#include "xdgmimeapps.h"

#include "qtxdglogging.h"
#include "xdgdesktopfile.h"
#include "xdgdirs.h"

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include <QDebug>
#include <QLoggingCategory>
#include <QMimeDatabase>

static QList<XdgDesktopFile *> GAppInfoGListToXdgDesktopQList(GList *list)
{
    QList<XdgDesktopFile *> dl;
    for (GList *l = list; l != nullptr; l = l->next) {
        if (l->data) {
            const QString file = QString::fromUtf8(g_desktop_app_info_get_filename(G_DESKTOP_APP_INFO(l->data)));
            if (!file.isEmpty()) {
                XdgDesktopFile *df = new XdgDesktopFile;
                if (df->load(file) && df->isValid()) {
                    dl.append(df);
                } else {
                    delete df;
                }
            }
        }
    }
    return dl;
}

static GDesktopAppInfo *XdgDesktopFileToGDesktopAppinfo(const XdgDesktopFile &app)
{
    GDesktopAppInfo *gApp = g_desktop_app_info_new_from_filename(app.fileName().toUtf8().constData());
    if (gApp == nullptr) {
        qCWarning(QtXdgMimeAppsGLib, "Failed to load GDesktopAppInfo for '%s'",
                qPrintable(app.fileName()));
        return nullptr;
    }
    return gApp;
}

XdgMimeAppsGLibBackend::XdgMimeAppsGLibBackend(QObject *parent)
    : XdgMimeAppsBackendInterface(parent),
      mWatcher(nullptr)
{
    // Make sure that we have glib support enabled.
    qunsetenv("QT_NO_GLIB");

    // This is a trick to init the database. Without it, the changed signal
    // functionality doesn't work properly. Also make sure optimizaters can't
    // make it go away.
    volatile GAppInfo *fooApp = g_app_info_get_default_for_type("foo", FALSE);
    if (fooApp)
        g_object_unref(G_APP_INFO(fooApp));

    mWatcher = g_app_info_monitor_get();
    if (mWatcher != nullptr) {
        g_signal_connect (mWatcher, "changed", G_CALLBACK (_changed), this);
    }
}

XdgMimeAppsGLibBackend::~XdgMimeAppsGLibBackend()
{
    g_object_unref(mWatcher);
}

void XdgMimeAppsGLibBackend::_changed(GAppInfoMonitor *monitor, XdgMimeAppsGLibBackend *_this)
{
    Q_UNUSED(monitor);
    Q_EMIT _this->changed();
}

bool XdgMimeAppsGLibBackend::addAssociation(const QString &mimeType, const XdgDesktopFile &app)
{
    GDesktopAppInfo *gApp = XdgDesktopFileToGDesktopAppinfo(app);
    if (gApp == nullptr)
        return false;

    GError *error = nullptr;
    if (g_app_info_add_supports_type(G_APP_INFO(gApp),
                                           mimeType.toUtf8().constData(), &error) == FALSE) {
        qCWarning(QtXdgMimeAppsGLib, "Failed to associate '%s' with '%s'. %s",
                  qPrintable(mimeType), g_desktop_app_info_get_filename(gApp), error->message);

        g_error_free(error);
        g_object_unref(gApp);
        return false;
    }
    g_object_unref(gApp);
    return true;
}

QList<XdgDesktopFile *> XdgMimeAppsGLibBackend::allApps()
{
    GList *list = g_app_info_get_all();
    QList<XdgDesktopFile *> dl = GAppInfoGListToXdgDesktopQList(list);
    g_list_free_full(list, g_object_unref);
    return dl;
}

QList<XdgDesktopFile *> XdgMimeAppsGLibBackend::apps(const QString &mimeType)
{
    GList *list = g_app_info_get_all_for_type(mimeType.toUtf8().constData());
    QList<XdgDesktopFile *> dl = GAppInfoGListToXdgDesktopQList(list);
    g_list_free_full(list, g_object_unref);
    return dl;
}

QList<XdgDesktopFile *> XdgMimeAppsGLibBackend::fallbackApps(const QString &mimeType)
{
    // g_app_info_get_fallback_for_type() doesn't returns the ones in the
    // recommended list
    GList *list = g_app_info_get_fallback_for_type(mimeType.toUtf8().constData());
    QList<XdgDesktopFile *> dl = GAppInfoGListToXdgDesktopQList(list);
    g_list_free_full(list, g_object_unref);
    return dl;
}

QList<XdgDesktopFile *> XdgMimeAppsGLibBackend::recommendedApps(const QString &mimeType)
{
    GList *list = g_app_info_get_recommended_for_type(mimeType.toUtf8().constData());
    QList<XdgDesktopFile *> dl = GAppInfoGListToXdgDesktopQList(list);
    g_list_free_full(list, g_object_unref);
    return dl;
}

bool XdgMimeAppsGLibBackend::removeAssociation(const QString &mimeType, const XdgDesktopFile &app)
{
    GDesktopAppInfo *gApp = XdgDesktopFileToGDesktopAppinfo(app);
    if (gApp == nullptr)
        return false;

    GError *error = nullptr;
    if (g_app_info_remove_supports_type(G_APP_INFO(gApp),
                                           mimeType.toUtf8().constData(), &error) == FALSE) {
        qCWarning(QtXdgMimeAppsGLib, "Failed to remove association between '%s' and '%s'. %s",
                  qPrintable(mimeType), g_desktop_app_info_get_filename(gApp), error->message);

        g_error_free(error);
        g_object_unref(gApp);
        return false;
    }
    g_object_unref(gApp);
    return true;
}

bool XdgMimeAppsGLibBackend::reset(const QString &mimeType)
{
    g_app_info_reset_type_associations(mimeType.toUtf8().constData());
    return true;
}

XdgDesktopFile *XdgMimeAppsGLibBackend::defaultApp(const QString &mimeType)
{
    GAppInfo *appinfo = g_app_info_get_default_for_type(mimeType.toUtf8().constData(), false);
    if (appinfo == nullptr || !G_IS_DESKTOP_APP_INFO(appinfo)) {
        return nullptr;
    }

    const char *file = g_desktop_app_info_get_filename(G_DESKTOP_APP_INFO(appinfo));

    if (file == nullptr) {
        g_object_unref(appinfo);
        return nullptr;
    }

    const QString s = QString::fromUtf8(file);
    g_object_unref(appinfo);

    XdgDesktopFile *f = new XdgDesktopFile;
    if (f->load(s) && f->isValid())
        return f;

    delete f;
    return nullptr;
}

bool XdgMimeAppsGLibBackend::setDefaultApp(const QString &mimeType, const XdgDesktopFile &app)
{
    // NOTE: "g_app_info_set_as_default_for_type()" writes to "~/.config/mimeapps.list"
    // but we want to set the default app only for the DE (e.g., LXQt).

    if (!addAssociation(mimeType, app))
        return false;

    GDesktopAppInfo *gApp = XdgDesktopFileToGDesktopAppinfo(app);
    if (gApp == nullptr)
        return false;

    // first find the DE's mimeapps list file
    QByteArray mimeappsList = "mimeapps.list";
    QList<QByteArray> desktopsList = qgetenv("XDG_CURRENT_DESKTOP").toLower().split(':');
    if (!desktopsList.isEmpty()) {
        mimeappsList = desktopsList.at(0) + "-" + mimeappsList;
    }
    char *mimeappsListPath = g_build_filename(XdgDirs::configHome(true).toUtf8().constData(),
                                              mimeappsList.constData(),
                                              nullptr);

    const char *desktop_id = g_app_info_get_id(G_APP_INFO(gApp));
    GKeyFile *kf = g_key_file_new();
    g_key_file_load_from_file(kf, mimeappsListPath, G_KEY_FILE_NONE, nullptr);
    g_key_file_set_string(kf, "Default Applications", mimeType.toUtf8().constData(), desktop_id);
    GError *error = nullptr;
    if (g_key_file_save_to_file(kf, mimeappsListPath, &error) == false) {
        qCWarning(QtXdgMimeAppsGLib, "Failed to set '%s' as the default for '%s'. %s",
                  g_desktop_app_info_get_filename(gApp), qPrintable(mimeType), error->message);
        g_error_free(error);
        g_key_file_free(kf);
        g_free(mimeappsListPath);
        g_object_unref(gApp);
        return false;
    }
    g_key_file_free(kf);
    g_free(mimeappsListPath);

    qCDebug(QtXdgMimeAppsGLib, "Set '%s' as the default for '%s'",
            g_desktop_app_info_get_filename(gApp), qPrintable(mimeType));

    g_object_unref(gApp);
    return true;
}
