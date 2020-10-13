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

#include "appmenuview.h"
#include <QStandardItemModel>
#include "appmenuview_p.h"
#include "core/filepath.h"

#include <gio/gdesktopappinfo.h>

namespace Fm {

AppMenuView::AppMenuView(QWidget* parent):
    QTreeView(parent),
    model_(new QStandardItemModel()),
    menu_cache(nullptr),
    menu_cache_reload_notify(nullptr) {

    setHeaderHidden(true);
    setSelectionMode(SingleSelection);

    // initialize model
    // TODO: share one model among all app menu view widgets
    // ensure that we're using lxmenu-data (FIXME: should we do this?)
    QByteArray oldenv = qgetenv("XDG_MENU_PREFIX");
    qputenv("XDG_MENU_PREFIX", "lxde-");
    menu_cache = menu_cache_lookup("applications.menu");
    // if(!oldenv.isEmpty())
    qputenv("XDG_MENU_PREFIX", oldenv); // restore the original value if needed

    if(menu_cache) {
        MenuCacheDir* dir = menu_cache_dup_root_dir(menu_cache);
        menu_cache_reload_notify = menu_cache_add_reload_notify(menu_cache, _onMenuCacheReload, this);
        if(dir) { /* content of menu is already loaded */
            addMenuItems(nullptr, dir);
            menu_cache_item_unref(MENU_CACHE_ITEM(dir));
        }
    }
    setModel(model_);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &AppMenuView::selectionChanged);
    selectionModel()->select(model_->index(0, 0), QItemSelectionModel::SelectCurrent);
}

AppMenuView::~AppMenuView() {
    delete model_;
    if(menu_cache) {
        if(menu_cache_reload_notify) {
            menu_cache_remove_reload_notify(menu_cache, menu_cache_reload_notify);
        }
        menu_cache_unref(menu_cache);
    }
}

void AppMenuView::addMenuItems(QStandardItem* parentItem, MenuCacheDir* dir) {
    GSList* l;
    GSList* list;
    /* Iterate over all menu items in this directory. */
    for(l = list = menu_cache_dir_list_children(dir); l != nullptr; l = l->next) {
        /* Get the menu item. */
        MenuCacheItem* menuItem = MENU_CACHE_ITEM(l->data);
        switch(menu_cache_item_get_type(menuItem)) {
        case MENU_CACHE_TYPE_NONE:
        case MENU_CACHE_TYPE_SEP:
            break;
        case MENU_CACHE_TYPE_APP:
        case MENU_CACHE_TYPE_DIR: {
            AppMenuViewItem* newItem = new AppMenuViewItem(menuItem);
            if(parentItem) {
                parentItem->insertRow(parentItem->rowCount(), newItem);
            }
            else {
                model_->insertRow(model_->rowCount(), newItem);
            }

            if(menu_cache_item_get_type(menuItem) == MENU_CACHE_TYPE_DIR) {
                addMenuItems(newItem, MENU_CACHE_DIR(menuItem));
            }
            break;
        }
        }
    }
    g_slist_free_full(list, (GDestroyNotify)menu_cache_item_unref);
}

void AppMenuView::onMenuCacheReload(MenuCache* mc) {
    MenuCacheDir* dir = menu_cache_dup_root_dir(mc);
    model_->clear();
    /* FIXME: preserve original selection */
    if(dir) {
        addMenuItems(nullptr, dir);
        menu_cache_item_unref(MENU_CACHE_ITEM(dir));
        selectionModel()->select(model_->index(0, 0), QItemSelectionModel::SelectCurrent);
    }
}

bool AppMenuView::isAppSelected() const {
    AppMenuViewItem* item = selectedItem();
    return (item && item->isApp());
}

AppMenuViewItem* AppMenuView::selectedItem() const {
    QModelIndexList selected = selectedIndexes();
    if(!selected.isEmpty()) {
        AppMenuViewItem* item = static_cast<AppMenuViewItem*>(model_->itemFromIndex(selected.first()
                                                                                   ));
        return item;
    }
    return nullptr;
}

Fm::GAppInfoPtr AppMenuView::selectedApp() const {
    const char* id = selectedAppDesktopId();
    return Fm::GAppInfoPtr{id ? G_APP_INFO(g_desktop_app_info_new(id)) : nullptr, false};
}

QByteArray AppMenuView::selectedAppDesktopFilePath() const {
    AppMenuViewItem* item = selectedItem();
    if(item && item->isApp()) {
        char* path = menu_cache_item_get_file_path(item->item());
        QByteArray ret(path);
        g_free(path);
        return ret;
    }
    return QByteArray();
}

const char* AppMenuView::selectedAppDesktopId() const {
    AppMenuViewItem* item = selectedItem();
    if(item && item->isApp()) {
        return menu_cache_item_get_id(item->item());
    }
    return nullptr;
}

FilePath AppMenuView::selectedAppDesktopPath() const {
    AppMenuViewItem* item = selectedItem();
    FilePath path;
    if(item && item->isApp()) {
        char* mpath = menu_cache_dir_make_path(MENU_CACHE_DIR(item));
        path = FilePath::fromUri("menu://applications/").relativePath(mpath + 13 /* skip "/Applications" */);
        g_free(mpath);
    }
    return path;
}

} // namespace Fm
