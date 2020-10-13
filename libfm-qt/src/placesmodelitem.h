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


#ifndef FM_PLACESMODELITEM_H
#define FM_PLACESMODELITEM_H

#include "libfmqtglobals.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include <QList>
#include <QAction>

#include "core/fileinfo.h"
#include "core/filepath.h"
#include "core/bookmarks.h"

namespace Fm {

// model item
class LIBFM_QT_API PlacesModelItem : public QStandardItem {
public:
    enum Type {
        Places = QStandardItem::UserType + 1,
        Volume,
        Mount,
        Bookmark
    };

public:
    explicit PlacesModelItem();
    explicit PlacesModelItem(QIcon icon, QString title, Fm::FilePath path = Fm::FilePath{});
    explicit PlacesModelItem(const char* iconName, QString title, Fm::FilePath path = Fm::FilePath{});
    explicit PlacesModelItem(std::shared_ptr<const Fm::IconInfo> icon, QString title, Fm::FilePath path = Fm::FilePath{});
    ~PlacesModelItem() override;

    const std::shared_ptr<const Fm::FileInfo>& fileInfo() const {
        return fileInfo_;
    }
    void setFileInfo(std::shared_ptr<const Fm::FileInfo> fileInfo) {
        fileInfo_ = std::move(fileInfo);
    }

    const Fm::FilePath& path() const {
        return path_;
    }
    void setPath(Fm::FilePath path) {
        path_ = std::move(path);
    }

    const std::shared_ptr<const Fm::IconInfo>& icon() const {
        return icon_;
    }
    void setIcon(std::shared_ptr<const Fm::IconInfo> icon);
    void setIcon(GIcon* gicon);
    void updateIcon();

    QVariant data(int role = Qt::UserRole + 1) const override;

    int type() const override {
        return Places;
    }

private:
    Fm::FilePath path_;
    std::shared_ptr<const Fm::FileInfo> fileInfo_;
    std::shared_ptr<const Fm::IconInfo> icon_;
};

class LIBFM_QT_API PlacesModelVolumeItem : public PlacesModelItem {
public:
    PlacesModelVolumeItem(GVolume* volume);
    bool isMounted();
    bool canEject() {
        return g_volume_can_eject(volume_);
    }
    int type() const override {
        return Volume;
    }
    GVolume* volume() {
        return volume_;
    }
    void update();
private:
    GVolume* volume_;
};

class LIBFM_QT_API PlacesModelMountItem : public PlacesModelItem {
public:
    PlacesModelMountItem(GMount* mount);
    int type() const override {
        return Mount;
    }
    GMount* mount() const {
        return mount_;
    }
    void update();
private:
    GMount* mount_;
};

class LIBFM_QT_API PlacesModelBookmarkItem : public PlacesModelItem {
public:
    int type() const override {
        return Bookmark;
    }
    PlacesModelBookmarkItem(std::shared_ptr<const Fm::BookmarkItem> bm_item);
    const std::shared_ptr<const Fm::BookmarkItem>& bookmark() const {
        return bookmarkItem_;
    }
private:
    std::shared_ptr<const Fm::BookmarkItem> bookmarkItem_;
};

}

#endif // FM_PLACESMODELITEM_H
