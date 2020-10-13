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


#include "placesmodelitem.h"
#include <gio/gio.h>
#include <QPainter>

namespace Fm {

PlacesModelItem::PlacesModelItem():
    QStandardItem(),
    fileInfo_(nullptr),
    icon_(nullptr) {
}

PlacesModelItem::PlacesModelItem(const char* iconName, QString title, Fm::FilePath path):
    QStandardItem(title),
    path_{std::move(path)},
    icon_(Fm::IconInfo::fromName(iconName)) {
    if(icon_) {
        QStandardItem::setIcon(icon_->qicon());
    }
    setEditable(false);
}

PlacesModelItem::PlacesModelItem(std::shared_ptr<const Fm::IconInfo> icon, QString title, Fm::FilePath path):
    QStandardItem(title),
    path_{std::move(path)},
    icon_{std::move(icon)} {
    if(icon_) {
        QStandardItem::setIcon(icon_->qicon());
    }
    setEditable(false);
}

PlacesModelItem::PlacesModelItem(QIcon icon, QString title, Fm::FilePath path):
    QStandardItem(icon, title),
    path_{std::move(path)} {
    setEditable(false);
}

PlacesModelItem::~PlacesModelItem() {
}


void PlacesModelItem::setIcon(std::shared_ptr<const Fm::IconInfo> icon) {
    icon_= std::move(icon);
    if(icon_) {
        QStandardItem::setIcon(icon_->qicon());
    }
    else {
        QStandardItem::setIcon(QIcon());
    }
}

void PlacesModelItem::setIcon(GIcon* gicon) {
    setIcon(Fm::IconInfo::fromGIcon(Fm::GIconPtr{gicon, true}));
}

void PlacesModelItem::updateIcon() {
    if(icon_) {
        QStandardItem::setIcon(icon_->qicon());
    }
}

QVariant PlacesModelItem::data(int role) const {
    // we use a QPixmap from FmIcon cache rather than QIcon object for decoration role.
    return QStandardItem::data(role);
}

PlacesModelBookmarkItem::PlacesModelBookmarkItem(std::shared_ptr<const Fm::BookmarkItem> bm_item):
    PlacesModelItem{bm_item->icon(), bm_item->name(), bm_item->path()},
    bookmarkItem_{std::move(bm_item)} {
    setEditable(true);
}

PlacesModelVolumeItem::PlacesModelVolumeItem(GVolume* volume):
    PlacesModelItem(),
    volume_(reinterpret_cast<GVolume*>(g_object_ref(volume))) {
    update();
    setEditable(false);
}

void PlacesModelVolumeItem::update() {
    // set title
    char* volumeName = g_volume_get_name(volume_);
    setText(QString::fromUtf8(volumeName));
    g_free(volumeName);

    // set icon
    Fm::GIconPtr gicon{g_volume_get_icon(volume_), false};
    setIcon(gicon.get());

    QString toolTip;

    // set dir path and tooltip
    Fm::GMountPtr mount{g_volume_get_mount(volume_), false};
    if(mount) {
        Fm::FilePath mount_root{g_mount_get_root(mount.get()), false};
        setPath(mount_root);
        toolTip = QString::fromUtf8(mount_root.toString().get());
    }
    else {
        setPath(Fm::FilePath{});
        if(CStrPtr identifier{g_volume_get_identifier(volume_, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE)}) {
            toolTip = QObject::tr("Identifier: ");
            toolTip += QLatin1String(identifier.get());
        }
        if(CStrPtr uuid{g_volume_get_uuid(volume_)}) {
            if(toolTip.isEmpty()) {
                toolTip = QLatin1String("UUID: ");
            }
            else {
                toolTip += QLatin1String("\nUUID: ");
            }
            toolTip += QLatin1String(uuid.get());
        }
    }

    setToolTip(toolTip);
}


bool PlacesModelVolumeItem::isMounted() {
    GMount* mount = g_volume_get_mount(volume_);
    if(mount) {
        g_object_unref(mount);
    }
    return mount != nullptr ? true : false;
}


PlacesModelMountItem::PlacesModelMountItem(GMount* mount):
    PlacesModelItem(),
    mount_(reinterpret_cast<GMount*>(mount)) {
    update();
    setEditable(false);
}

void PlacesModelMountItem::update() {
    // set title
    setText(QString::fromUtf8(g_mount_get_name(mount_)));

    // set path
    Fm::FilePath mount_root{g_mount_get_root(mount_), false};
    setPath(mount_root);
    setToolTip(QString::fromUtf8(mount_root.toString().get()));

    // set icon
    Fm::GIconPtr gicon{g_mount_get_icon(mount_), false};
    setIcon(gicon.get());
}

}
