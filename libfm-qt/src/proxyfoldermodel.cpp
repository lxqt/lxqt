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


#include "proxyfoldermodel.h"
#include "foldermodel.h"
#include <QCollator>

namespace Fm {

ProxyFolderModel::ProxyFolderModel(QObject* parent):
    QSortFilterProxyModel(parent),
    showHidden_(false),
    backupAsHidden_(true),
    folderFirst_(true),
    hiddenLast_(false),
    showThumbnails_(false),
    thumbnailSize_(0) {

    setDynamicSortFilter(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);

    collator_.setNumericMode(true);
}

ProxyFolderModel::~ProxyFolderModel() {
    if(showThumbnails_ && thumbnailSize_ != 0) {
        FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
        // tell the source model that we don't need the thumnails anymore
        if(srcModel) {
            srcModel->releaseThumbnails(thumbnailSize_);
            disconnect(srcModel, SIGNAL(thumbnailLoaded(QModelIndex, int)));
        }
    }
}

void ProxyFolderModel::setSourceModel(QAbstractItemModel* model) {
    if(model == sourceModel()) // avoid setting the same model twice
        return;
    FolderModel* oldSrcModel = static_cast<FolderModel*>(sourceModel());
#if (QT_VERSION == QT_VERSION_CHECK(5,11,0))
    // workaround for Qt-5.11 bug https://bugreports.qt.io/browse/QTBUG-68581
    if(oldSrcModel) {
        disconnect(oldSrcModel, SIGNAL(destroyed()), this, SLOT(_q_sourceModelDestroyed()));
    }
#endif
    if(model) {
        // we only support Fm::FolderModel
        Q_ASSERT(model->inherits("Fm::FolderModel"));

        if(showThumbnails_ && thumbnailSize_ != 0) { // if we're showing thumbnails
            if(oldSrcModel) { // we need to release cached thumbnails for the old source model
                oldSrcModel->releaseThumbnails(thumbnailSize_);
                disconnect(oldSrcModel, SIGNAL(thumbnailLoaded(QModelIndex, int)));
            }
            FolderModel* newSrcModel = static_cast<FolderModel*>(model);
            if(newSrcModel) { // tell the new source model that we want thumbnails of this size
                newSrcModel->cacheThumbnails(thumbnailSize_);
                connect(newSrcModel, &FolderModel::thumbnailLoaded, this, &ProxyFolderModel::onThumbnailLoaded);
            }
        }
    }
    QSortFilterProxyModel::setSourceModel(model);
}

void ProxyFolderModel::sort(int column, Qt::SortOrder order) {
    int oldColumn = sortColumn();
    Qt::SortOrder oldOrder = sortOrder();
    QSortFilterProxyModel::sort(column, order);
    if(column != oldColumn || order != oldOrder) {
        Q_EMIT sortFilterChanged();
    }
}

void ProxyFolderModel::setShowHidden(bool show) {
    if(show != showHidden_) {
        showHidden_ = show;
        invalidateFilter();
        Q_EMIT sortFilterChanged();
    }
}

void ProxyFolderModel::setBackupAsHidden(bool backupAsHidden) {
    if(backupAsHidden != backupAsHidden_) {
        backupAsHidden_ = backupAsHidden;
        invalidateFilter();
        Q_EMIT sortFilterChanged();
    }
}

// need to call invalidateFilter() manually.
void ProxyFolderModel::setFolderFirst(bool folderFirst) {
    if(folderFirst != folderFirst_) {
        folderFirst_ = folderFirst;
        invalidate();
        Q_EMIT sortFilterChanged();
    }
}

void ProxyFolderModel::setHiddenLast(bool hiddenLast) {
    if(hiddenLast != hiddenLast_) {
        hiddenLast_ = hiddenLast;
        invalidate();
        Q_EMIT sortFilterChanged();
    }
}

void ProxyFolderModel::setSortCaseSensitivity(Qt::CaseSensitivity cs) {
    collator_.setCaseSensitivity(cs);
    QSortFilterProxyModel::setSortCaseSensitivity(cs);
    invalidate();
    Q_EMIT sortFilterChanged();
}

bool ProxyFolderModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
    if(!showHidden_) {
        if(FolderModel* srcModel = static_cast<FolderModel*>(sourceModel())) {
            auto info = srcModel->fileInfoFromIndex(srcModel->index(source_row, 0, source_parent));
            if(info && (info->isHidden() || (backupAsHidden_ && info->isBackup()))) {
                return false;
            }
        }
    }
    // apply additional filters if there're any
    for(ProxyFolderModelFilter* const filter : qAsConst(filters_)) {
        if(FolderModel* srcModel = static_cast<FolderModel*>(sourceModel())){
            auto fileInfo = srcModel->fileInfoFromIndex(srcModel->index(source_row, 0, source_parent));
            if(!filter->filterAcceptsRow(this, fileInfo)) {
                return false;
            }
        }
    }
    return true;
}

bool ProxyFolderModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
    // left and right are indexes of source model, not the proxy model.
    if(srcModel) {
        auto leftInfo = srcModel->fileInfoFromIndex(left);
        auto rightInfo = srcModel->fileInfoFromIndex(right);

        if(folderFirst_) {
            bool leftIsFolder = leftInfo->isDir();
            bool rightIsFolder = rightInfo->isDir();
            if(leftIsFolder != rightIsFolder) {
                return sortOrder() == Qt::AscendingOrder ? leftIsFolder : rightIsFolder;
            }
        }

        if(hiddenLast_) {
            bool leftIsHidden = leftInfo->isHidden();
            bool rightIsHidden = rightInfo->isHidden();
            if(leftIsHidden != rightIsHidden) {
                return sortOrder() == Qt::AscendingOrder ? rightIsHidden : leftIsHidden;
            }
        }

        int comp = 0;
        switch(sortColumn()) {
        case FolderModel::ColumnFileMTime:
            if(leftInfo->mtime() != rightInfo->mtime()) { // quint64
                return leftInfo->mtime() < rightInfo->mtime();
            }
            break;
        case FolderModel::ColumnFileSize:
            if(leftInfo->size() != rightInfo->size()) { // quint64
                return leftInfo->size() < rightInfo->size();
            }
            break;
        default: {
            QString leftText = left.data(Qt::DisplayRole).toString();
            QString rightText = right.data(Qt::DisplayRole).toString();
            comp = collator_.compare(leftText, rightText);
            break;
        }
        }
        // always sort files by their display names when they have the same property
        if(comp == 0) {
            return collator_.compare(leftInfo->displayName(), rightInfo->displayName()) < 0;
        }
        return comp < 0;
    }
    return QSortFilterProxyModel::lessThan(left, right);
}

std::shared_ptr<const Fm::FileInfo> ProxyFolderModel::fileInfoFromIndex(const QModelIndex& index) const {
    if(index.isValid()) {
        FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
        if(srcModel) {
            QModelIndex srcIndex = mapToSource(index);
            return srcModel->fileInfoFromIndex(srcIndex);
        }
    }
    return nullptr;
}

QModelIndex ProxyFolderModel::indexFromPath(const FilePath &path) const {
    QModelIndex ret;
    int n_rows = rowCount();
    for(int row = 0; row < n_rows; ++row) {
        auto idx = index(row, FolderModel::ColumnFileName, QModelIndex());
        auto fi = fileInfoFromIndex(idx);
        if(fi && fi->path() == path) { // found the item
            ret = idx;
            break;
        }
    }
    return ret;
}

std::shared_ptr<const FileInfo> ProxyFolderModel::fileInfoFromPath(const FilePath &path) const {
    return fileInfoFromIndex(indexFromPath(path));
}

void ProxyFolderModel::setShowThumbnails(bool show) {
    if(show != showThumbnails_) {
        showThumbnails_ = show;
        FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
        if(srcModel && thumbnailSize_ != 0) {
            if(show) {
                // ask for cache of thumbnails of the new size in source model
                srcModel->cacheThumbnails(thumbnailSize_);
                // connect to the srcModel so we can be notified when a thumbnail is loaded.
                connect(srcModel, &FolderModel::thumbnailLoaded, this, &ProxyFolderModel::onThumbnailLoaded);
            }
            else { // turn off thumbnails
                // free cached old thumbnails in souce model
                srcModel->releaseThumbnails(thumbnailSize_);
                disconnect(srcModel, SIGNAL(thumbnailLoaded(QModelIndex, int)));
            }
            // reload all items, FIXME: can we only update items previously having thumbnails
            Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
        }
    }
}

void ProxyFolderModel::setThumbnailSize(int size) {
    if(size != thumbnailSize_) {
        FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
        if(showThumbnails_ && srcModel) {
            // free cached thumbnails of the old size
            if(thumbnailSize_ != 0) {
                srcModel->releaseThumbnails(thumbnailSize_);
            }
            else {
                // if the old thumbnail size is 0, we did not turn on thumbnail initially
                connect(srcModel, &FolderModel::thumbnailLoaded, this, &ProxyFolderModel::onThumbnailLoaded);
            }
            // ask for cache of thumbnails of the new size in source model
            srcModel->cacheThumbnails(size);
            // reload all items, FIXME: can we only update items previously having thumbnails
            Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
        }

        thumbnailSize_ = size;
    }
}

QVariant ProxyFolderModel::data(const QModelIndex& index, int role) const {
    if(index.column() == 0) { // only show the decoration role for the first column
        if(role == Qt::DecorationRole && showThumbnails_ && thumbnailSize_) {
            // we need to show thumbnails instead of icons
            FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
            QModelIndex srcIndex = mapToSource(index);
            QImage image = srcModel->thumbnailFromIndex(srcIndex, thumbnailSize_);
            if(!image.isNull()) { // if we got a thumbnail of the desired size, use it
                return QVariant(image);
            }
        }
    }
    // fallback to icons if thumbnails are not available
    return QSortFilterProxyModel::data(index, role);
}

void ProxyFolderModel::onThumbnailLoaded(const QModelIndex& srcIndex, int size) {
    // FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
    // FolderModelItem* item = srcModel->itemFromIndex(srcIndex);
    // qDebug("ProxyFolderModel::onThumbnailLoaded: %d, %s", size, item->displayName.toUtf8().data());

    if(size == thumbnailSize_ // if a thumbnail of the size we want is loaded
       && srcIndex.model() == sourceModel()) { // check if the sourse model contains the index item
        QModelIndex index = mapFromSource(srcIndex);
        Q_EMIT dataChanged(index, index);
    }
}

void ProxyFolderModel::addFilter(ProxyFolderModelFilter* filter) {
    filters_.append(filter);
    invalidateFilter();
    Q_EMIT sortFilterChanged();
}

void ProxyFolderModel::removeFilter(ProxyFolderModelFilter* filter) {
    filters_.removeOne(filter);
    invalidateFilter();
    Q_EMIT sortFilterChanged();
}

void ProxyFolderModel::updateFilters() {
    invalidate();
    Q_EMIT sortFilterChanged();
}

#if 0
void ProxyFolderModel::reloadAllThumbnails() {
    // reload all thumbnails and update UI
    FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
    if(srcModel) {
        int rows = rowCount();
        for(int row = 0; row < rows; ++row) {
            QModelIndex index = this->index(row, 0);
            QModelIndex srcIndex = mapToSource(index);
            QImage image = srcModel->thumbnailFromIndex(srcIndex, size);
            // tell the world that the item is changed to trigger a UI update
            if(!image.isNull()) {
                Q_EMIT dataChanged(index, index);
            }
        }
    }
}
#endif


} // namespace Fm
