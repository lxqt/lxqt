/*
 * Copyright (C) 2014 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef FM_APPMENUVIEW_H
#define FM_APPMENUVIEW_H

#include <QTreeView>
#include "libfmqtglobals.h"
#include <menu-cache/menu-cache.h>

#include "core/gioptrs.h"
#include "core/filepath.h"

class QStandardItemModel;
class QStandardItem;

namespace Fm {

class AppMenuViewItem;

class LIBFM_QT_API AppMenuView : public QTreeView {
    Q_OBJECT
public:
    explicit AppMenuView(QWidget* parent = nullptr);
    ~AppMenuView() override;

    Fm::GAppInfoPtr selectedApp() const;

    const char* selectedAppDesktopId() const;

    QByteArray selectedAppDesktopFilePath() const;

    FilePath selectedAppDesktopPath() const;

    bool isAppSelected() const;

Q_SIGNALS:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

private:
    void addMenuItems(QStandardItem* parentItem, MenuCacheDir* dir);
    void onMenuCacheReload(MenuCache* mc);
    static void _onMenuCacheReload(MenuCache* mc, gpointer user_data) {
        static_cast<AppMenuView*>(user_data)->onMenuCacheReload(mc);
    }

    AppMenuViewItem* selectedItem() const;

private:
    // gboolean fm_app_menu_view_is_item_app(, GtkTreeIter* it);
    QStandardItemModel* model_;
    MenuCache* menu_cache;
    MenuCacheNotifyId menu_cache_reload_notify;
};

}

#endif // FM_APPMENUVIEW_H
