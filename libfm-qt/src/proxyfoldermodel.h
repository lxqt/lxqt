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


#ifndef FM_PROXYFOLDERMODEL_H
#define FM_PROXYFOLDERMODEL_H

#include "libfmqtglobals.h"
#include <QSortFilterProxyModel>
#include <QList>
#include <QCollator>

#include "core/fileinfo.h"

namespace Fm {

// a proxy model used to sort and filter FolderModel

class FolderModelItem;
class ProxyFolderModel;

class LIBFM_QT_API ProxyFolderModelFilter {
public:
    virtual bool filterAcceptsRow(const ProxyFolderModel* model, const std::shared_ptr<const Fm::FileInfo>& info) const = 0;
    virtual ~ProxyFolderModelFilter() {}
};


class LIBFM_QT_API ProxyFolderModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit ProxyFolderModel(QObject* parent = nullptr);
    ~ProxyFolderModel() override;

    // only Fm::FolderModel is allowed for being sourceModel
    void setSourceModel(QAbstractItemModel* model) override;

    void setShowHidden(bool show);
    bool showHidden() const {
        return showHidden_;
    }

    void setBackupAsHidden(bool backupAsHidden);
    bool backupAsHidden() const {
        return backupAsHidden_;
    }

    void setFolderFirst(bool folderFirst);
    bool folderFirst() {
        return folderFirst_;
    }

    void setHiddenLast(bool hiddenLast);
    bool hiddenLast() {
        return hiddenLast_;
    }

    void setSortCaseSensitivity(Qt::CaseSensitivity cs);

    bool showThumbnails() {
        return showThumbnails_;
    }
    void setShowThumbnails(bool show);

    int thumbnailSize() {
        return thumbnailSize_;
    }
    void setThumbnailSize(int size);

    std::shared_ptr<const Fm::FileInfo> fileInfoFromIndex(const QModelIndex& index) const;

    std::shared_ptr<const Fm::FileInfo> fileInfoFromPath(const FilePath& path) const;

    QModelIndex indexFromPath(const FilePath& path) const;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void addFilter(ProxyFolderModelFilter* filter);
    void removeFilter(ProxyFolderModelFilter* filter);
    void updateFilters();

Q_SIGNALS:
    void sortFilterChanged();

protected Q_SLOTS:
    void onThumbnailLoaded(const QModelIndex& srcIndex, int size);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
    // void reloadAllThumbnails();

private:
    QCollator collator_;
    bool showHidden_;
    bool backupAsHidden_;
    bool folderFirst_;
    bool hiddenLast_;
    bool showThumbnails_;
    int thumbnailSize_;
    QList<ProxyFolderModelFilter*> filters_;
};

}

#endif // FM_PROXYFOLDERMODEL_H
