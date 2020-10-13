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


#ifndef FM_PLACESMODEL_H
#define FM_PLACESMODEL_H

#include "libfmqtglobals.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include <QList>
#include <QAction>

#include <memory>

#include "core/filepath.h"
#include "core/bookmarks.h"

namespace Fm {

class PlacesModelItem;
class PlacesModelVolumeItem;
class PlacesModelMountItem;
class PlacesModelBookmarkItem;

class LIBFM_QT_API PlacesModel : public QStandardItemModel {
    Q_OBJECT
    friend class PlacesView;
public:

    enum {
        FileInfoRole = Qt::UserRole,
        FmIconRole
    };

    // QAction used for popup menus
    class ItemAction : public QAction {
    public:
        explicit ItemAction(const QModelIndex& index, QString text, QObject* parent = nullptr):
            QAction(text, parent),
            index_(index) {
        }

        QPersistentModelIndex& index() {
            return index_;
        }
    private:
        QPersistentModelIndex index_;
    };

public:
    explicit PlacesModel(QObject* parent = nullptr);
    ~PlacesModel() override;

    bool showTrash() {
        return trashItem_ != nullptr;
    }
    void setShowTrash(bool show);

    bool showApplications() {
        return showApplications_;
    }
    void setShowApplications(bool show);

    bool showDesktop() {
        return showDesktop_;
    }
    void setShowDesktop(bool show);

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    static std::shared_ptr<PlacesModel> globalInstance();

public Q_SLOTS:
    void updateTrash();
    void onBookmarksChanged();

protected:

    PlacesModelItem* itemFromPath(const Fm::FilePath& path);
    PlacesModelItem* itemFromPath(QStandardItem* rootItem, const Fm::FilePath & path);
    PlacesModelVolumeItem* itemFromVolume(GVolume* volume);
    PlacesModelMountItem* itemFromMount(GMount* mount);
    PlacesModelBookmarkItem* itemFromBookmark(std::shared_ptr<const Fm::BookmarkItem> bkitem);

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
    Qt::DropActions supportedDropActions() const override;

    void createTrashItem();

private:
    void loadBookmarks();

    static void onVolumeAdded(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis);
    static void onVolumeRemoved(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis);
    static void onVolumeChanged(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis);
    static void onMountAdded(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis);
    static void onMountRemoved(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis);
    static void onMountChanged(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis);

    static void onTrashChanged(GFileMonitor* monitor, GFile* gf, GFile* other, GFileMonitorEvent evt, PlacesModel* pThis);

private:
    std::shared_ptr<Fm::Bookmarks> bookmarks;
    GVolumeMonitor* volumeMonitor;
    bool showApplications_;
    bool showDesktop_;
    QStandardItem* placesRoot;
    QStandardItem* devicesRoot;
    QStandardItem* bookmarksRoot;
    PlacesModelItem* trashItem_;
    GFileMonitor* trashMonitor_;
    QTimer* trashUpdateTimer_;
    PlacesModelItem* desktopItem;
    PlacesModelItem* homeItem;
    PlacesModelItem* computerItem;
    PlacesModelItem* networkItem;
    PlacesModelItem* applicationsItem;
    QIcon ejectIcon_;
    QList<GMount*> shadowedMounts_;

    static std::weak_ptr<PlacesModel> globalInstance_;
};

}

#endif // FM_PLACESMODEL_H
