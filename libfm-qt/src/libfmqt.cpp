/*
 * Copyright (C) 2013 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "libfmqt.h"
#include <QLocale>
#include <QPixmapCache>
#include "core/thumbnailer.h"
#include "xdndworkaround.h"
#include "core/vfs/fm-file.h"
#include "core/legacy/fm-config.h"

namespace Fm {

struct LibFmQtData {
    LibFmQtData();
    ~LibFmQtData();

    QTranslator translator;
    XdndWorkaround workaround;
    int refCount;
    Q_DISABLE_COPY(LibFmQtData)
};

static LibFmQtData* theLibFmData = nullptr;

extern "C" {

GFile *_fm_vfs_search_new_for_uri(const char *uri);  // defined in vfs-search.c
GFile *_fm_vfs_menu_new_for_uri(const char *uri);  // defined in vfs-menu.c

}

static GFile* lookupSearchUri(GVfs * /*vfs*/, const char *identifier, gpointer /*user_data*/) {
    return _fm_vfs_search_new_for_uri(identifier);
}

static GFile* lookupMenuUri(GVfs * /*vfs*/, const char *identifier, gpointer /*user_data*/) {
    return _fm_vfs_menu_new_for_uri(identifier);
}

LibFmQtData::LibFmQtData(): refCount(1) {
#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init();
#endif
    // turn on glib debug message
    // g_setenv("G_MESSAGES_DEBUG", "all", true);
    Fm::Thumbnailer::loadAll();
    translator.load(QLatin1String("libfm-qt_") + QLocale::system().name(), QLatin1String(LIBFM_QT_DATA_DIR) + QLatin1String("/translations"));

    // FIXME: we keep the FmConfig data structure here to keep compatibility with legacy libfm API.
    fm_config_init();

    // register some URI schemes implemented by libfm
    GVfs* vfs = g_vfs_get_default();
    g_vfs_register_uri_scheme(vfs, "menu", lookupMenuUri, nullptr, nullptr, lookupMenuUri, nullptr, nullptr);
    g_vfs_register_uri_scheme(vfs, "search", lookupSearchUri, nullptr, nullptr, lookupSearchUri, nullptr, nullptr);
}

LibFmQtData::~LibFmQtData() {
    // _fm_file_finalize();

    GVfs* vfs = g_vfs_get_default();
    g_vfs_unregister_uri_scheme(vfs, "menu");
    g_vfs_unregister_uri_scheme(vfs, "search");
}

LibFmQt::LibFmQt() {
    if(!theLibFmData) {
        theLibFmData = new LibFmQtData();
    }
    else {
        ++theLibFmData->refCount;
    }
    d = theLibFmData;
}

LibFmQt::~LibFmQt() {
    if(--d->refCount == 0) {
        delete d;
        theLibFmData = nullptr;
    }
}

QTranslator* LibFmQt::translator() {
    return &d->translator;
}

} // namespace Fm
