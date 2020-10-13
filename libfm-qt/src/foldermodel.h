/*
 * Copyright (C) 2012 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#ifndef FM_FOLDERMODEL_H
#define FM_FOLDERMODEL_H

#include "libfmqtglobals.h"
#include <QAbstractListModel>
#include <QItemSelection>
#include <QIcon>
#include <QImage>
#include <QList>
#include <vector>
#include <utility>
#include <forward_list>
#include "foldermodelitem.h"

#include "core/folder.h"
#include "core/thumbnailjob.h"

namespace Fm {

class LIBFM_QT_API FolderModel : public QAbstractListModel {
    Q_OBJECT
public:

    enum Role {
        FileInfoRole = Qt::UserRole,
        FileIsDirRole,
        FileIsCutRole
    };

    enum ColumnId {
        ColumnFileName,
        ColumnFileType,
        ColumnFileSize,
        ColumnFileMTime,
        ColumnFileDTime,
        ColumnFileOwner,
        ColumnFileGroup,
        NumOfColumns
    };

public:
    explicit FolderModel();
    ~FolderModel() override;

    const std::shared_ptr<Fm::Folder>& folder() const {
        return folder_;
    }

    void setFolder(const std::shared_ptr<Fm::Folder>& new_folder);

    Fm::FilePath path() {
        return folder_ ? folder_->path() : Fm::FilePath();
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    // void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    std::shared_ptr<const Fm::FileInfo> fileInfoFromIndex(const QModelIndex& index) const;
    std::shared_ptr<const Fm::FileInfo> fileInfoFromPath(const Fm::FilePath& path) const;
    FolderModelItem* itemFromIndex(const QModelIndex& index) const;
    QImage thumbnailFromIndex(const QModelIndex& index, int size);

    void cacheThumbnails(int size);
    void releaseThumbnails(int size);

    void setShowFullName(bool fullName) {
        showFullNames_ = fullName;
    }

Q_SIGNALS:
    void thumbnailLoaded(const QModelIndex& index, int size);
    void fileSizeChanged(const QModelIndex& index);
    void filesAdded(FileInfoList infoList);

protected Q_SLOTS:

    void onStartLoading();
    void onFinishLoading();
    void onFilesAdded(const Fm::FileInfoList& files);
    void onFilesChanged(std::vector<Fm::FileInfoPair>& files);
    void onFilesRemoved(const Fm::FileInfoList& files);

    void onThumbnailLoaded(const std::shared_ptr<const Fm::FileInfo>& file, int size, const QImage& image);
    void onThumbnailJobFinished();
    void loadPendingThumbnails();

    void onClipboardDataChange();

protected:
    void queueLoadThumbnail(const std::shared_ptr<const Fm::FileInfo>& file, int size);
    void insertFiles(int row, const Fm::FileInfoList& files);
    void removeAll();
    QList<FolderModelItem>::iterator findItemByName(const char* name, int* row);
    QList<FolderModelItem>::iterator findItemByFileInfo(const Fm::FileInfo* info, int* row);

private:
    QString makeTooltip(FolderModelItem* item) const;
    void updateCutFilesSet();

private:

    struct ThumbnailData {
        ThumbnailData(int size):
            size_{size},
            refCount_{1} {
        }

        int size_;
        int refCount_;
        Fm::FileInfoList pendingThumbnails_;
    };

    std::shared_ptr<Fm::Folder> folder_;
    QList<FolderModelItem> items;

    bool hasPendingThumbnailHandler_;
    std::vector<Fm::ThumbnailJob*> pendingThumbnailJobs_;
    std::forward_list<ThumbnailData> thumbnailData_;

    bool showFullNames_;

    bool isLoaded_;

    bool hasCutfile_;
    HashSet cutFilesHashSet_;
};

}

#endif // FM_FOLDERMODEL_H
