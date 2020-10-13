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


#include "foldermodel.h"
#include <iostream>
#include <algorithm>
#include <QtAlgorithms>
#include <QVector>
#include <qmimedata.h>
#include <QMimeData>
#include <QByteArray>
#include <QPixmap>
#include <QPainter>
#include <QTimer>
#include <QDateTime>
#include <QString>
#include <QApplication>
#include <QClipboard>
#include "utilities.h"
#include "fileoperation.h"

namespace Fm {

FolderModel::FolderModel():
    hasPendingThumbnailHandler_{false},
    showFullNames_{false},
    isLoaded_{false},
    hasCutfile_{false} {
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &FolderModel::onClipboardDataChange);
}

FolderModel::~FolderModel() {
    // if the thumbnail requests list is not empty, cancel them
    for(auto job: pendingThumbnailJobs_) {
        job->cancel();
    }
}

void FolderModel::setFolder(const std::shared_ptr<Fm::Folder>& new_folder) {
    if(folder_) {
        removeAll();        // remove old items
    }
    if(new_folder) {
        folder_ = new_folder;
        connect(folder_.get(), &Fm::Folder::startLoading, this, &FolderModel::onStartLoading);
        connect(folder_.get(), &Fm::Folder::finishLoading, this, &FolderModel::onFinishLoading);
        connect(folder_.get(), &Fm::Folder::filesAdded, this, &FolderModel::onFilesAdded);
        connect(folder_.get(), &Fm::Folder::filesChanged, this, &FolderModel::onFilesChanged);
        connect(folder_.get(), &Fm::Folder::filesRemoved, this, &FolderModel::onFilesRemoved);
        // handle the case if the folder is already loaded
        if(folder_->isLoaded()) {
            isLoaded_ = true;
            insertFiles(0, folder_->files());
        }
    }
}

void FolderModel::onStartLoading() {
    isLoaded_ = false;
    hasCutfile_ = false;
    // remove all items
    removeAll();
}

void FolderModel::onFinishLoading() {
    isLoaded_ = true;
    // files may have been cut before (e.g., by another app)
    onClipboardDataChange();
}

void FolderModel::onFilesAdded(const Fm::FileInfoList& files) {
    int n_files = files.size();
    beginInsertRows(QModelIndex(), items.count(), items.count() + n_files - 1);
    for(auto& info : files) {
        FolderModelItem item(info);

        // cut files may be removed and added again
        if(isLoaded_ && cutFilesHashSet_.count(info->path().hash()) != 0) {
            item.isCut = true;
            hasCutfile_ = true;
        }

        items.append(item);
    }
    endInsertRows();

    if(isLoaded_) {
        Q_EMIT filesAdded(files);
    }
}

void FolderModel::onFilesChanged(std::vector<Fm::FileInfoPair>& files) {
    for(auto& change : files) {
        int row;
        auto& oldInfo = change.first;
        auto& newInfo = change.second;
        QList<FolderModelItem>::iterator it = findItemByFileInfo(oldInfo.get(), &row);
        if(it != items.end()) {
            FolderModelItem& item = *it;
            // try to update the item
            item.info = newInfo;
            item.thumbnails.clear();
            QModelIndex index = createIndex(row, 0, &item);
            Q_EMIT dataChanged(index, index);
            if(oldInfo->size() != newInfo->size()) {
                Q_EMIT fileSizeChanged(index);
            }
        }
    }
}

void FolderModel::onFilesRemoved(const Fm::FileInfoList& files) {
    for(auto& info : files) {
        int row;
        QList<FolderModelItem>::iterator it = findItemByName(info->name().c_str(), &row);
        if(it != items.end()) {
            beginRemoveRows(QModelIndex(), row, row);
            items.erase(it);
            endRemoveRows();
        }
    }
}

void FolderModel::loadPendingThumbnails() {
    hasPendingThumbnailHandler_ = false;
    for(auto& item: thumbnailData_) {
        if(!item.pendingThumbnails_.empty()) {
            auto job = new Fm::ThumbnailJob(std::move(item.pendingThumbnails_), item.size_);
            pendingThumbnailJobs_.push_back(job);
            job->setAutoDelete(true);
            connect(job, &Fm::ThumbnailJob::thumbnailLoaded, this, &FolderModel::onThumbnailLoaded, Qt::BlockingQueuedConnection);
            connect(job, &Fm::ThumbnailJob::finished, this, &FolderModel::onThumbnailJobFinished, Qt::BlockingQueuedConnection);
            Fm::ThumbnailJob::threadPool()->start(job);
        }
    }
}

void FolderModel::queueLoadThumbnail(const std::shared_ptr<const Fm::FileInfo>& file, int size) {
    auto it = std::find_if(thumbnailData_.begin(), thumbnailData_.end(), [size](ThumbnailData& item){return item.size_ == size;});
    if(it != thumbnailData_.end()) {
        it->pendingThumbnails_.push_back(file);
        if(!hasPendingThumbnailHandler_) {
            QTimer::singleShot(0, this, &FolderModel::loadPendingThumbnails);
            hasPendingThumbnailHandler_ = true;
        }
    }
}

void FolderModel::insertFiles(int row, const Fm::FileInfoList& files) {
    int n_files = files.size();
    beginInsertRows(QModelIndex(), row, row + n_files - 1);
    for(auto& info : files) {
        FolderModelItem item(info);
        items.append(item);
    }
    endInsertRows();
}

void FolderModel::updateCutFilesSet() {
    // NOTE: To process large numbers of files as fast as possible,
    // we analyze the clipboard data directly, without using "utilities.h",
    // because otherwise, we should iterate through path lists multiple times.

    if(folder_ == nullptr) {
        return;
    }
    cutFilesHashSet_.clear();

    bool cutPathFound = false;
    const QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* data = clipboard->mimeData();

    // Gnome, LXDE, XFCE (see utilities.cpp -> pasteFilesFromClipboard)
    if(data->hasFormat(QStringLiteral("x-special/gnome-copied-files"))) {
        QByteArray gnomeData = data->data(QStringLiteral("x-special/gnome-copied-files"));
        char* pdata = gnomeData.data();
        char* eol = strchr(pdata, '\n');
        if(eol) {
            *eol = '\0';
            if(strcmp(pdata, "cut") == 0) {
                char** uris = g_strsplit_set(eol + 1, "\r\n", -1);
                for(char** uri = uris; *uri; ++uri) {
                    if(**uri != '\0') {
                        cutPathFound = true; // although it may not be in this folder
                        auto path = Fm::FilePath::fromUri(*uri);
                        if(path.parent() == folder_->path()) {
                            cutFilesHashSet_.insert(path.hash());
                        }
                    }
                }
                g_strfreev(uris);
            }
        }
    }
    // KDE
    if(!cutPathFound && data->hasUrls()) {
        QByteArray cut = data->data(QStringLiteral("application/x-kde-cutselection"));
        if(!cut.isEmpty() && QChar::fromLatin1(cut.at(0)) == QLatin1Char('1')) {
            QList<QUrl> urls = data->urls();
            for(auto it = urls.cbegin(); it != urls.cend(); ++it) {
                auto path = Fm::FilePath::fromUri(it->toString().toUtf8().constData());
                if(path.parent() == folder_->path()) {
                    cutFilesHashSet_.insert(path.hash());
                }
            }
        }
    }
}

void FolderModel::onClipboardDataChange() {
    if(folder_ && isLoaded_) {
        updateCutFilesSet();
        if(!cutFilesHashSet_.empty()) {
             // (some) files are cut here; so the items need to be updated
            hasCutfile_ = false;
            QList<FolderModelItem>::iterator it = items.begin();
            while(it != items.end()) {
                FolderModelItem& item = *it;
                if(cutFilesHashSet_.count(it->info->path().hash()) != 0) {
                    item.isCut = true;
                    hasCutfile_ = true;
                }
                else {
                    item.isCut = false;
                }
                ++it;
            }
            Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
        }
        else if(hasCutfile_) {
            // this folder contained a cut file before but not anymore;
            // so its items need to be updated
            hasCutfile_ = false;
            QList<FolderModelItem>::iterator it = items.begin();
            while(it != items.end()) {
                FolderModelItem& item = *it;
                item.isCut = false;
                ++it;
            }
            Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
        }
    }
}

void FolderModel::removeAll() {
    if(items.empty()) {
        return;
    }
    beginRemoveRows(QModelIndex(), 0, items.size() - 1);
    items.clear();
    endRemoveRows();
}

int FolderModel::rowCount(const QModelIndex& parent) const {
    if(parent.isValid()) {
        return 0;
    }
    return items.size();
}

int FolderModel::columnCount(const QModelIndex& parent = QModelIndex()) const {
    if(parent.isValid()) {
        return 0;
    }
    return NumOfColumns;
}

FolderModelItem* FolderModel::itemFromIndex(const QModelIndex& index) const {
    return reinterpret_cast<FolderModelItem*>(index.internalPointer());
}

std::shared_ptr<const Fm::FileInfo> FolderModel::fileInfoFromIndex(const QModelIndex& index) const {
    FolderModelItem* item = itemFromIndex(index);
    return item ? item->info : nullptr;
}

QString FolderModel::makeTooltip(FolderModelItem* item) const {
    // name (bold)
    QString tip = QStringLiteral("<p><b>") + (showFullNames_ ? QString::fromStdString(item->name()) : item->displayName()).toHtmlEscaped() + QStringLiteral("</b></p>");
    // parent dir
    auto info = item->info;
    auto parent_path = info->path().parent();
    auto parent_str = parent_path ? parent_path.displayName() : nullptr;
    if(parent_str) {
        tip += QStringLiteral("<p><i>") + tr("Location:") + QStringLiteral("</i> ") + QString::fromUtf8(parent_str.get()).toHtmlEscaped() + QStringLiteral("</p>");
    }
    // file type
    tip += QStringLiteral("<i>") + tr("File type:") + QStringLiteral("</i> ") + QString::fromUtf8(info->mimeType()->desc());
    // file size
    const QString dSize = item->displaySize();
    if(!dSize.isEmpty()) { // it's empty for directories
        tip += QStringLiteral("<br><i>") + tr("File size:") + QStringLiteral("</i> ") + dSize;
    }
    // times
    auto atime = QDateTime::fromMSecsSinceEpoch(info->atime() * 1000);
    tip += QStringLiteral("<br><i>") + tr("Last modified:") + QStringLiteral("</i> ") + item->displayMtime()
           + QStringLiteral("<br><i>") + tr("Last accessed:") + QStringLiteral("</i> ") + atime.toString(Qt::SystemLocaleShortDate);
    // owner
    const QString owner = item->ownerName();
    if(!owner.isEmpty()) { // can be empty
        tip += QStringLiteral("<br><i>") + tr("Owner:") + QStringLiteral("</i> ") + owner
               + QStringLiteral("<br><i>") + tr("Group:") + QStringLiteral("</i> ") + item->ownerGroup();
    }
    return tip;
}

QVariant FolderModel::data(const QModelIndex& index, int role/* = Qt::DisplayRole*/) const {
    if(!index.isValid() || index.row() > items.size() || index.column() >= NumOfColumns) {
        return QVariant();
    }
    FolderModelItem* item = itemFromIndex(index);
    auto info = item->info;
    bool isCut = folder_ && item->isCut;

    switch(role) {
    case Qt::ToolTipRole:
        return QVariant(makeTooltip(item));
    case Qt::DisplayRole:  {
        switch(index.column()) {
        case ColumnFileName:
            return (showFullNames_ && !item->name().empty() ? QString::fromStdString(item->name())
                                                            : item->displayName());
        case ColumnFileType:
            return QString::fromUtf8(info->mimeType()->desc());
        case ColumnFileMTime:
            return item->displayMtime();
        case ColumnFileDTime:
            return item->displayDtime();
        case ColumnFileSize:
            return item->displaySize();
        case ColumnFileOwner:
            return item->ownerName();
        case ColumnFileGroup:
            return item->ownerGroup();
        }
        break;
    }
    case Qt::DecorationRole: {
        if(index.column() == 0) {
            return QVariant(item->icon());
        }
        break;
    }
    case Qt::EditRole: {
        if(index.column() == 0) {
            return QString::fromStdString(info->name());
        }
        break;
    }
    case FileInfoRole:
        return QVariant::fromValue(info);
    case FileIsDirRole:
        return QVariant(info->isDir());
    case FileIsCutRole:
        return isCut;
    }
    return QVariant();
}

QVariant FolderModel::headerData(int section, Qt::Orientation orientation, int role/* = Qt::DisplayRole*/) const {
    if(role == Qt::DisplayRole) {
        if(orientation == Qt::Horizontal) {
            QString title;
            switch(section) {
            case ColumnFileName:
                title = tr("Name");
                break;
            case ColumnFileType:
                title = tr("Type");
                break;
            case ColumnFileSize:
                title = tr("Size");
                break;
            case ColumnFileMTime:
                title = tr("Modified");
                break;
            case ColumnFileDTime:
                title = tr("Deleted");
                break;
            case ColumnFileOwner:
                title = tr("Owner");
                break;
            case ColumnFileGroup:
                title = tr("Group");
                break;
            }
            return QVariant(title);
        }
    }
    return QVariant();
}

QModelIndex FolderModel::index(int row, int column, const QModelIndex& /*parent*/) const {
    if(row < 0 || row >= items.size() || column < 0 || column >= NumOfColumns) {
        return QModelIndex();
    }
    const FolderModelItem& item = items.at(row);
    return createIndex(row, column, (void*)&item);
}

QModelIndex FolderModel::parent(const QModelIndex& /*index*/) const {
    return QModelIndex();
}

Qt::ItemFlags FolderModel::flags(const QModelIndex& index) const {
    // FIXME: should not return same flags unconditionally for all columns
    Qt::ItemFlags flags;
    if(index.isValid()) {
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if(index.column() == ColumnFileName) {
            flags |= (Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled
                      | Qt::ItemIsEditable); // inline renaming);
        }
    }
    else {
        flags = Qt::ItemIsDropEnabled;
    }
    return flags;
}

std::shared_ptr<const Fm::FileInfo> FolderModel::fileInfoFromPath(const Fm::FilePath& path) const {
    QList<FolderModelItem>::const_iterator it = items.begin();
    while(it != items.end()) {
        const FolderModelItem& item = *it;
        if(item.info->path() == path) {
            return item.info;
        }
        ++it;
    }
    return nullptr;
}

// FIXME: this is very inefficient and should be replaced with a
// more reasonable implementation later.
QList<FolderModelItem>::iterator FolderModel::findItemByName(const char* name, int* row) {
    QList<FolderModelItem>::iterator it = items.begin();
    int i = 0;
    while(it != items.end()) {
        FolderModelItem& item = *it;
        if(item.info->name() == name) {
            *row = i;
            return it;
        }
        ++it;
        ++i;
    }
    return items.end();
}

QList< FolderModelItem >::iterator FolderModel::findItemByFileInfo(const Fm::FileInfo* info, int* row) {
    QList<FolderModelItem>::iterator it = items.begin();
    int i = 0;
    while(it != items.end()) {
        FolderModelItem& item = *it;
        if(item.info.get() == info) {
            *row = i;
            return it;
        }
        ++it;
        ++i;
    }
    return items.end();
}

QStringList FolderModel::mimeTypes() const {
    //qDebug("FolderModel::mimeTypes");
    QStringList types = QAbstractItemModel::mimeTypes();
    // now types contains "application/x-qabstractitemmodeldatalist"

    // add support for freedesktop Xdnd direct save (XDS) protocol.
    // https://www.freedesktop.org/wiki/Specifications/XDS/#index4h2
    // the real implementation is in FolderView::childDropEvent().
    types << QStringLiteral("XdndDirectSave0");
    types << QStringLiteral("text/uri-list");
    types << QStringLiteral("libfm/files"); // see FolderModel::mimeData() below
    return types;
}

QMimeData* FolderModel::mimeData(const QModelIndexList& indexes) const {
    QMimeData* data = QAbstractItemModel::mimeData(indexes);
    //qDebug("FolderModel::mimeData");
    // build two uri lists, one for internal DND
    // and the other for DNDing to external apps
    QByteArray urilist, libfmUrilist;
    urilist.reserve(4096);
    libfmUrilist.reserve(4096);

    for(const auto& index : indexes) {
        FolderModelItem* item = itemFromIndex(index);
        if(item && item->info) {
            auto path = item->info->path();
            if(path.isValid()) {
                // get the list that will be used by internal DND
                auto uri = path.uri();
                libfmUrilist.append(uri.get());
                libfmUrilist.append('\n');

                // also, get the list that will be used when DNDing to external apps,
                // using local paths as far as possible (for DNDing from remote folders)
                if(auto localPath = path.localPath()) {
                    QUrl url = QUrl::fromLocalFile(QString::fromUtf8(localPath.get()));
                    urilist.append(url.toEncoded());
                }
                else {
                    urilist.append(uri.get());
                }
                urilist.append('\n');
            }
        }
    }
    data->setData(QStringLiteral("text/uri-list"), urilist);
    // NOTE: The mimetype "text/uri-list" changes the list in QMimeData::setData() to get URLs
    // but some protocols (like MTP) may need the original list to query file info.
    data->setData(QStringLiteral("libfm/files"), libfmUrilist);

    return data;
}

bool FolderModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
    //qDebug("FolderModel::dropMimeData");
    if(!folder_ || !data) {
        return false;
    }
    Fm::FilePath destPath;
    if(parent.isValid()) { // drop on an item
        std::shared_ptr<const Fm::FileInfo> info;
        if(row == -1 && column == -1) {
            info = fileInfoFromIndex(parent);
        }
        else {
            QModelIndex itemIndex = index(row, column, parent);
            info = fileInfoFromIndex(itemIndex);
        }
        if(info) {
            if (info->isDir()) {
                destPath = info->path();
            }
            else {
                destPath = path(); // don't drop on file
            }
        }
        else {
            return false;
        }
    }
    else { // drop on blank area of the folder
        destPath = path();
    }

    Fm::FilePathList srcPaths;
    // try to get paths from the original data
    if(data->hasFormat(QStringLiteral("libfm/files"))) {
        QByteArray _data = data->data(QStringLiteral("libfm/files"));
        srcPaths = pathListFromUriList(_data.data());
    }
    if(srcPaths.empty() && data->hasUrls()) {
        srcPaths = Fm::pathListFromQUrls(data->urls());
    }

    // FIXME: should we put this in dropEvent handler of FolderView instead?
    if(!srcPaths.empty()) {
        //qDebug("drop action: %d", action);
        switch(action) {
        case Qt::CopyAction:
            FileOperation::copyFiles(srcPaths, destPath);
            break;
        case Qt::MoveAction:
            FileOperation::moveFiles(srcPaths, destPath);
            break;
        case Qt::LinkAction:
            FileOperation::symlinkFiles(srcPaths, destPath);
        /* Falls through. */
        default:
            return false;
        }
        return true;
    }
    else if(data->hasFormat(QStringLiteral("application/x-qabstractitemmodeldatalist"))) {
        return true;
    }
    return QAbstractListModel::dropMimeData(data, action, row, column, parent);
}

Qt::DropActions FolderModel::supportedDropActions() const {
    //qDebug("FolderModel::supportedDropActions");
    return Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
}

// ask the model to load thumbnails of the specified size
void FolderModel::cacheThumbnails(const int size) {
    auto it = std::find_if(thumbnailData_.begin(), thumbnailData_.end(), [size](ThumbnailData& item){return item.size_ == size;});
    if(it != thumbnailData_.cend()) {
        ++it->refCount_;
    }
    else {
        thumbnailData_.push_front(ThumbnailData(size));
    }
}

// ask the model to free cached thumbnails of the specified size
void FolderModel::releaseThumbnails(int size) {
    auto prev = thumbnailData_.before_begin();
    for(auto it = thumbnailData_.begin(); it != thumbnailData_.end(); ++it) {
        if(it->size_ == size) {
            --it->refCount_;
            if(it->refCount_ == 0) {
                thumbnailData_.erase_after(prev);
            }

            // remove all cached thumbnails of the specified size
            QList<FolderModelItem>::iterator itemIt;
            for(itemIt = items.begin(); itemIt != items.end(); ++itemIt) {
                FolderModelItem& item = *itemIt;
                item.removeThumbnail(size);
            }
            break;
        }
        prev = it;
    }
}

void FolderModel::onThumbnailJobFinished() {
    Fm::ThumbnailJob* job = static_cast<Fm::ThumbnailJob*>(sender());
    auto it = std::find(pendingThumbnailJobs_.cbegin(), pendingThumbnailJobs_.cend(), job);
    if(it != pendingThumbnailJobs_.end()) {
        pendingThumbnailJobs_.erase(it);
    }
}

void FolderModel::onThumbnailLoaded(const std::shared_ptr<const Fm::FileInfo>& file, int size, const QImage& image) {
    // find the model item this thumbnail belongs to
    int row;
    QList<FolderModelItem>::iterator it = findItemByFileInfo(file.get(), &row);
    if(it != items.end()) {
        // the file is found in our model
        FolderModelItem& item = *it;
        QModelIndex index = createIndex(row, 0, (void*)&item);
        // store the image in the folder model item.
        FolderModelItem::Thumbnail* thumbnail = item.findThumbnail(size);
        thumbnail->image = image;
        // qDebug("thumbnail loaded for: %s, size: %d", item.displayName.toUtf8().constData(), size);
        if(image.isNull()) {
            thumbnail->status = FolderModelItem::ThumbnailFailed;
        }
        else {
            thumbnail->status = FolderModelItem::ThumbnailLoaded;
            thumbnail->image = image;

            // tell the world that we have the thumbnail loaded
            Q_EMIT thumbnailLoaded(index, size);
        }
    }
}

// get a thumbnail of size at the index
// if a thumbnail is not yet loaded, this will initiate loading of the thumbnail.
QImage FolderModel::thumbnailFromIndex(const QModelIndex& index, int size) {
    FolderModelItem* item = itemFromIndex(index);
    if(item) {
        FolderModelItem::Thumbnail* thumbnail = item->findThumbnail(size);
        // qDebug("FolderModel::thumbnailFromIndex: %d, %s", thumbnail->status, item->displayName.toUtf8().data());
        switch(thumbnail->status) {
        case FolderModelItem::ThumbnailNotChecked: {
            // load the thumbnail
            queueLoadThumbnail(item->info, size);
            thumbnail->status = FolderModelItem::ThumbnailLoading;
            break;
        }
        case FolderModelItem::ThumbnailLoaded:
            return thumbnail->image;
        default:
            ;
        }
    }
    return QImage();
}


} // namespace Fm
