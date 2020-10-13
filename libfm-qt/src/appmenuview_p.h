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

#ifndef FM_APPMENUVIEW_P_H
#define FM_APPMENUVIEW_P_H

#include <QStandardItem>
#include <menu-cache/menu-cache.h>
#include "core/iconinfo.h"

namespace Fm {

class AppMenuViewItem : public QStandardItem {
public:
    explicit AppMenuViewItem(MenuCacheItem* item):
        item_(menu_cache_item_ref(item)) {
        std::shared_ptr<const Fm::IconInfo> icon;
        if(menu_cache_item_get_icon(item)) {
            icon = Fm::IconInfo::fromName(menu_cache_item_get_icon(item));
        }
        setText(QString::fromUtf8(menu_cache_item_get_name(item)));
        setEditable(false);
        setDragEnabled(false);
        if(icon) {
            setIcon(icon->qicon());
        }
    }

    ~AppMenuViewItem() override {
        menu_cache_item_unref(item_);
    }

    MenuCacheItem* item() {
        return item_;
    }

    int type() const override {
        return menu_cache_item_get_type(item_);
    }

    bool isApp() {
        return type() == MENU_CACHE_TYPE_APP;
    }

    bool isDir() {
        return type() == MENU_CACHE_TYPE_DIR;
    }

private:
    MenuCacheItem* item_;
};

}

#endif // FM_APPMENUVIEW_P_H
