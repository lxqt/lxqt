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

#include "dirtreemodel.h"
#include "dirtreemodelitem.h"
#include "dndactionmenu.h"
#include "fileoperation.h"
#include "utilities.h"
#include <QDebug>
#include "core/fileinfojob.h"

namespace Fm {

DirTreeModel::DirTreeModel(QObject* parent):
    QAbstractItemModel(parent),
    showHidden_(false) {
}

DirTreeModel::~DirTreeModel() {
}

void DirTreeModel::addRoots(Fm::FilePathList rootPaths) {
    auto job = new Fm::FileInfoJob{std::move(rootPaths)};
    job->setAutoDelete(true);
    connect(job, &Fm::FileInfoJob::finished, this, &DirTreeModel::onFileInfoJobFinished, Qt::BlockingQueuedConnection);
    job->runAsync();
}

void DirTreeModel::onFileInfoJobFinished() {
    auto job = static_cast<Fm::FileInfoJob*>(sender());
    for(auto file: job->files()) {
        addRoot(std::move(file));
    }
    Q_EMIT rootsAdded();
}

// QAbstractItemModel implementation

Qt::ItemFlags DirTreeModel::flags(const QModelIndex& index) const {
    DirTreeModelItem* item = itemFromIndex(index);
    if(item) {
        if(item->isPlaceHolder()) {
            return Qt::ItemIsEnabled;
        }
        return Qt::ItemIsDropEnabled | QAbstractItemModel::flags(index);
    }
    return QAbstractItemModel::flags(index);
}

QVariant DirTreeModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid() || index.column() > 1) {
        return QVariant();
    }
    DirTreeModelItem* item = itemFromIndex(index);
    if(item) {
        auto info = item->fileInfo_;
        switch(role) {
        case Qt::ToolTipRole:
            return QVariant(item->displayName_);
        case Qt::DisplayRole:
            return QVariant(item->displayName_);
        case Qt::DecorationRole:
            return QVariant(item->icon_);
        case FileInfoRole: {
            QVariant v;
            v.setValue(info);
            return v;
        }
        }
    }
    return QVariant();
}

bool DirTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction /*action*/, int /*row*/, int /*column*/, const QModelIndex& parent) {
    if(auto destPath = filePath(parent)) {
        if(data->hasUrls()) { // files uris are dropped
            Qt::DropAction action = DndActionMenu::askUser(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction, QCursor::pos());
            auto paths = pathListFromQUrls(data->urls());
            if(!paths.empty()) {
                switch(action) {
                case Qt::CopyAction:
                    FileOperation::copyFiles(paths, destPath);
                    break;
                case Qt::MoveAction:
                    FileOperation::moveFiles(paths, destPath);
                    break;
                case Qt::LinkAction:
                    FileOperation::symlinkFiles(paths, destPath);
                /* Falls through. */
                default:
                    return false;
                }
                return true;
            }
        }
    }
    return false;
}

bool DirTreeModel::canDropMimeData(const QMimeData* /*data*/, Qt::DropAction /*action*/, int /*row*/, int /*column*/, const QModelIndex& parent) const {
    return parent.isValid();
}

int DirTreeModel::columnCount(const QModelIndex& /*parent*/) const {
    return 1;
}

int DirTreeModel::rowCount(const QModelIndex& parent) const {
    if(!parent.isValid()) {
        return rootItems_.size();
    }
    DirTreeModelItem* item = itemFromIndex(parent);
    if(item) {
        return item->children_.size();
    }
    return 0;
}

QModelIndex DirTreeModel::parent(const QModelIndex& child) const {
    DirTreeModelItem* item = itemFromIndex(child);
    if(item && item->parent_) {
        item = item->parent_; // go to parent item
        if(item) {
            const auto& items = item->parent_ ? item->parent_->children_ : rootItems_;
            auto it = std::find(items.cbegin(), items.cend(), item);
            if(it != items.cend()) {
                int row = it - items.cbegin();
                return createIndex(row, 0, (void*)item);
            }
        }
    }
    return QModelIndex();
}

QModelIndex DirTreeModel::index(int row, int column, const QModelIndex& parent) const {
    if(row >= 0 && column >= 0 && column == 0) {
        if(!parent.isValid()) { // root items
            if(static_cast<size_t>(row) < rootItems_.size()) {
                const DirTreeModelItem* item = rootItems_.at(row);
                return createIndex(row, column, (void*)item);
            }
        }
        else { // child items
            DirTreeModelItem* parentItem = itemFromIndex(parent);
            if(static_cast<size_t>(row) < parentItem->children_.size()) {
                const DirTreeModelItem* item = parentItem->children_.at(row);
                return createIndex(row, column, (void*)item);
            }
        }
    }
    return QModelIndex(); // invalid index
}

bool DirTreeModel::hasChildren(const QModelIndex& parent) const {
    DirTreeModelItem* item = itemFromIndex(parent);
    return item ? !item->isPlaceHolder() : true;
}

QModelIndex DirTreeModel::indexFromItem(DirTreeModelItem* item) const {
    Q_ASSERT(item);
    const auto& items = item->parent_ ? item->parent_->children_ : rootItems_;
    auto it = std::find(items.cbegin(), items.cend(), item);
    if(it != items.cend()) {
        int row = it - items.cbegin();
        return createIndex(row, 0, (void*)item);
    }
    return QModelIndex();
}

// public APIs
QModelIndex DirTreeModel::addRoot(std::shared_ptr<const Fm::FileInfo> root) {
    DirTreeModelItem* item = new DirTreeModelItem(std::move(root), this);
    int row = rootItems_.size();
    beginInsertRows(QModelIndex(), row, row);
    rootItems_.push_back(item);
    // add_place_holder_child_item(model, item_l, nullptr, FALSE);
    endInsertRows();
    return createIndex(row, 0, (void*)item);
}

DirTreeModelItem* DirTreeModel::itemFromIndex(const QModelIndex& index) const {
    return reinterpret_cast<DirTreeModelItem*>(index.internalPointer());
}

QModelIndex DirTreeModel::indexFromPath(const Fm::FilePath &path) const {
    DirTreeModelItem* item = itemFromPath(path);
    return item ? item->index() : QModelIndex();
}

DirTreeModelItem* DirTreeModel::itemFromPath(const Fm::FilePath &path) const {
    for(DirTreeModelItem* const item : qAsConst(rootItems_)) {
        if(item->fileInfo_ && path == item->fileInfo_->path()) {
            return item;
        }
        else {
            DirTreeModelItem* child = item->childFromPath(path, true);
            if(child) {
                return child;
            }
        }
    }
    return nullptr;
}


void DirTreeModel::loadRow(const QModelIndex& index) {
    DirTreeModelItem* item = itemFromIndex(index);
    Q_ASSERT(item);
    if(item && !item->isPlaceHolder()) {
        item->loadFolder();
    }
}

void DirTreeModel::unloadRow(const QModelIndex& index) {
    DirTreeModelItem* item = itemFromIndex(index);
    if(item && !item->isPlaceHolder()) {
        item->unloadFolder();
    }
}

bool DirTreeModel::isLoaded(const QModelIndex& index) {
    DirTreeModelItem* item = itemFromIndex(index);
    return item ? item->loaded_ : false;
}

QIcon DirTreeModel::icon(const QModelIndex& index) {
    DirTreeModelItem* item = itemFromIndex(index);
    return item ? item->icon_ : QIcon();
}

std::shared_ptr<const Fm::FileInfo> DirTreeModel::fileInfo(const QModelIndex& index) {
    DirTreeModelItem* item = itemFromIndex(index);
    return item ? item->fileInfo_ : nullptr;
}

Fm::FilePath DirTreeModel::filePath(const QModelIndex& index) {
    DirTreeModelItem* item = itemFromIndex(index);
    return (item && item->fileInfo_) ? item->fileInfo_->path() : Fm::FilePath{};
}

QString DirTreeModel::dispName(const QModelIndex& index) {
    DirTreeModelItem* item = itemFromIndex(index);
    return item ? item->displayName_ : QString();
}

void DirTreeModel::setShowHidden(bool show_hidden) {
    showHidden_ = show_hidden;
    for(DirTreeModelItem* const item : qAsConst(rootItems_)) {
        item->setShowHidden(show_hidden);
    }
}


} // namespace Fm
