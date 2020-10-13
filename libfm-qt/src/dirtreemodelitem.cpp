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

#include "dirtreemodelitem.h"
#include "dirtreemodel.h"
#include <QDebug>

namespace Fm {

DirTreeModelItem::DirTreeModelItem():
    fileInfo_(nullptr),
    folder_(nullptr),
    expanded_(false),
    loaded_(false),
    parent_(nullptr),
    placeHolderChild_(nullptr),
    model_(nullptr),
    queuedForDeletion_(false) {
}

DirTreeModelItem::DirTreeModelItem(std::shared_ptr<const Fm::FileInfo> info, DirTreeModel* model, DirTreeModelItem* parent):
    fileInfo_{std::move(info)},
    expanded_(false),
    loaded_(false),
    parent_(parent),
    placeHolderChild_(nullptr),
    model_(model),
    queuedForDeletion_(false) {

    if(fileInfo_) {
        displayName_ = fileInfo_->displayName();
        icon_ = fileInfo_->icon()->qicon();
        addPlaceHolderChild();
    }
}

DirTreeModelItem::~DirTreeModelItem() {
    freeFolder();
    // delete child items if needed
    if(!children_.empty()) {
        for(DirTreeModelItem* const item : qAsConst(children_)) {
            delete item;
        }
    }
    if(!hiddenChildren_.empty()) {
        for(DirTreeModelItem* const item : qAsConst(hiddenChildren_)) {
            delete item;
        }
    }
    /*if(queuedForDeletion_)
        qDebug() << "queued deletion done";*/
}

void DirTreeModelItem::freeFolder() {
    if(folder_) {
        QObject::disconnect(onFolderFinishLoadingConn_);
        QObject::disconnect(onFolderFilesAddedConn_);
        QObject::disconnect(onFolderFilesRemovedConn_);
        QObject::disconnect(onFolderFilesChangedConn_);
        folder_.reset();
    }
}

void DirTreeModelItem::addPlaceHolderChild() {
    placeHolderChild_ = new DirTreeModelItem();
    placeHolderChild_->parent_ = this;
    placeHolderChild_->model_ = model_;
    placeHolderChild_->displayName_ = DirTreeModel::tr("Loading...");
    children_.push_back(placeHolderChild_);
}

void DirTreeModelItem::loadFolder() {
    if(!expanded_) {
        /* dynamically load content of the folder. */
        folder_ =  Fm::Folder::fromPath(fileInfo_->path());
        /* g_debug("fm_dir_tree_model_load_row()"); */
        /* associate the data with loaded handler */

        onFolderFinishLoadingConn_ = QObject::connect(folder_.get(), &Fm::Folder::finishLoading, model_, [=]() {
            onFolderFinishLoading();
        });
        onFolderFilesAddedConn_ = QObject::connect(folder_.get(), &Fm::Folder::filesAdded, model_, [=](Fm::FileInfoList files) {
            onFolderFilesAdded(files);
        });
        onFolderFilesRemovedConn_ = QObject::connect(folder_.get(), &Fm::Folder::filesRemoved, model_, [=](Fm::FileInfoList files) {
            onFolderFilesRemoved(files);
        });
        onFolderFilesChangedConn_ = QObject::connect(folder_.get(), &Fm::Folder::filesChanged, model_, [=](std::vector<Fm::FileInfoPair>& changes) {
            onFolderFilesChanged(changes);
        });

        /* set 'expanded' flag beforehand as callback may check it */
        expanded_ = true;
        /* if the folder is already loaded, call "loaded" handler ourselves */
        if(folder_->isLoaded()) { // already loaded
            insertFiles(folder_->files());
            onFolderFinishLoading();
        }
    }
}

void DirTreeModelItem::unloadFolder() {
    if(expanded_) { /* do some cleanup */
        /* remove all children, and replace them with a dummy child
          * item to keep expander in the tree view around. */

        // delete all visible child items
        model_->beginRemoveRows(index(), 0, children_.size() - 1);
        if(!children_.empty()) {
            for(DirTreeModelItem* const item : qAsConst(children_)) {
                delete item;
            }
            children_.clear();
        }
        model_->endRemoveRows();

        // remove hidden children
        if(!hiddenChildren_.empty()) {
            for(DirTreeModelItem* const item : qAsConst(hiddenChildren_)) {
                delete item;
            }
            hiddenChildren_.clear();
        }

        /* now, we have no child since all child items are removed.
         * So we add a place holder child item to keep the expander around. */
        addPlaceHolderChild();
        /* deactivate folder since it will be reactivated on expand */
        freeFolder();
        expanded_ = false;
        loaded_ = false;
    }
}

QModelIndex DirTreeModelItem::index() {
    Q_ASSERT(model_);
    return model_->indexFromItem(this);
}

/* Add file info to parent node to proper position. */
DirTreeModelItem* DirTreeModelItem::insertFile(std::shared_ptr<const Fm::FileInfo> fi) {
    // qDebug() << "insertFileInfo: " << fm_file_info_get_disp_name(fi);
    DirTreeModelItem* item = new DirTreeModelItem(std::move(fi), model_);
    insertItem(item);
    return item;
}

/* Add file info to parent node to proper position. */
void DirTreeModelItem::insertFiles(Fm::FileInfoList files) {
    if(children_.size() == 1 && placeHolderChild_) {
        // the list is empty, add them all at once and do sort
        if(!model_->showHidden()) { // need to separate visible and hidden items
            // insert hidden files into the "hiddenChildren_" list and remove them from "files" list
            // WARNING: "std::remove_if" shouldn't be used to work on the "removed" items because, as
            // docs say, the elements between the returned and the end iterators are in an unspecified
            // state and, as far as I (@tsujan) have tested, some of them announce themselves as null.
            for(auto it = files.begin(); it != files.end();) {
                auto file = *it;
                if(file->isHidden()) {
                    hiddenChildren_.push_back(new DirTreeModelItem{std::move(file), model_});
                    it = files.erase(it);
                }
                else {
                    ++it;
                }
            }

        }
        // sort the remaining visible files by name
        std::sort(files.begin(), files.end(), [](const std::shared_ptr<const Fm::FileInfo>& a, const std::shared_ptr<const Fm::FileInfo>& b) {
            return QString::localeAwareCompare(a->displayName(), b->displayName()) < 0;
        });
        // insert the files into the visible children list at once
        model_->beginInsertRows(index(), 1, files.size() + 1); // the first item is the placeholder item, so we start from row 1
        for(auto& file: files) {
            if(file->isDir()) {
                DirTreeModelItem* newItem = new DirTreeModelItem(std::move(file), model_);
                newItem->parent_ = this;
                children_.push_back(newItem);
            }
        }
        model_->endInsertRows();

        // remove the place holder if a folder is added
        if(children_.size() > 1) {
            auto it = std::find(children_.cbegin(), children_.cend(), placeHolderChild_);
            if(it != children_.cend()) {
                auto pos = it - children_.cbegin();
                model_->beginRemoveRows(index(), pos, pos);
                children_.erase(it);
                delete placeHolderChild_;
                model_->endRemoveRows();
                placeHolderChild_ = nullptr;
            }
        }
    }
    else {
        // the list already contain some items, insert new items one by one so they can be sorted.
        for(auto& file: files) {
            if(file->isDir()) {
                insertFile(std::move(file));
            }
        }
    }
}

// find a good position to insert the new item
// FIXME: insert one item at a time is slow. Insert multiple items at once and then sort is faster.
int DirTreeModelItem::insertItem(DirTreeModelItem* newItem) {
    if(!newItem->fileInfo_ || !newItem->fileInfo_->isDir()) {
        // don't insert placeholders or non-directory files 
        return -1;
    }
    if(model_->showHidden() || !newItem->fileInfo_ || !newItem->fileInfo_->isHidden()) {
        auto it = std::lower_bound(children_.cbegin(), children_.cend(), newItem, [=](const DirTreeModelItem* a, const DirTreeModelItem* b) {
            if(Q_UNLIKELY(!a->fileInfo_)) {
                return true;  // this is a placeholder item which will be removed so the order doesn't matter.
            }
            if(Q_UNLIKELY(!b->fileInfo_)) {
                return false;
            }
            return QString::localeAwareCompare(a->fileInfo_->displayName(), b->fileInfo_->displayName()) < 0;
        });
        // inform the world that we're about to insert the item
        auto position = it - children_.begin();
        model_->beginInsertRows(index(), position, position);
        newItem->parent_ = this;
        children_.insert(it, newItem);
        model_->endInsertRows();
        return position;
    }
    else { // hidden folder
        hiddenChildren_.push_back(newItem);
    }
    return -1;
}


// FmFolder signal handlers

void DirTreeModelItem::onFolderFinishLoading() {
    DirTreeModel* model = model_;
    /* set 'loaded' flag beforehand as callback may check it */
    loaded_ = true;
    QModelIndex idx = index();
    //qDebug() << "folder loaded";
    // remove the placeholder child if needed
    // (a check for its existence is necessary; see insertItem)
    if(placeHolderChild_) {
        if(children_.size() == 1) { // we have no other child other than the place holder item, leave it
            placeHolderChild_->displayName_ = DirTreeModel::tr("<No sub folders>");
            QModelIndex placeHolderIndex = placeHolderChild_->index();
            // qDebug() << "placeHolderIndex: "<<placeHolderIndex;
            Q_EMIT model->dataChanged(placeHolderIndex, placeHolderIndex);
        }
        else {
            auto it = std::find(children_.cbegin(), children_.cend(), placeHolderChild_);
            if(it != children_.cend()) {
                auto pos = it - children_.cbegin();
                model->beginRemoveRows(idx, pos, pos);
                children_.erase(it);
                delete placeHolderChild_;
                model->endRemoveRows();
                placeHolderChild_ = nullptr;
            }
        }
    }

    Q_EMIT model->rowLoaded(idx);
}

void DirTreeModelItem::onFolderFilesAdded(Fm::FileInfoList& files) {
    insertFiles(files);
}

void DirTreeModelItem::onFolderFilesRemoved(Fm::FileInfoList& files) {
    DirTreeModel* model = model_;

    for(auto& fi: files) {
        int pos;
        DirTreeModelItem* child  = childFromName(fi->name().c_str(), &pos);
        if(child) {
            // The item shouldn't be deleted now but after its row is removed from QTreeView;
            // otherwise a freeze will happen when it has a child item (its row is expanded).
            child->queuedForDeletion_ = true;
            model->beginRemoveRows(index(), pos, pos);
            children_.erase(children_.cbegin() + pos);
            model->endRemoveRows();
            
        }
    }

    if(children_.empty()) { // no visible children, add a placeholder item to keep the row expanded
        addPlaceHolderChild();
        placeHolderChild_->displayName_ = DirTreeModel::tr("<No sub folders>");
    }
}

void DirTreeModelItem::onFolderFilesChanged(std::vector<Fm::FileInfoPair> &changes) {
    DirTreeModel* model = model_;
    for(auto& changePair: changes) {
        int pos;
        auto& changedFile = changePair.first;
        DirTreeModelItem* child = childFromName(changedFile->name().c_str(), &pos);
        if(child) {
            QModelIndex childIndex = child->index();
            Q_EMIT model->dataChanged(childIndex, childIndex);
        }
    }
}

DirTreeModelItem* DirTreeModelItem::childFromName(const char* utf8_name, int* pos) {
    int i = 0;
    for(const auto item : children_) {
        if(item->fileInfo_ && item->fileInfo_->name() == utf8_name) {
            if(pos) {
                *pos = i;
            }
            return item;
        }
        ++i;
    }
    return nullptr;
}

DirTreeModelItem* DirTreeModelItem::childFromPath(Fm::FilePath path, bool recursive) const {
    Q_ASSERT(path != nullptr);

    for(DirTreeModelItem* const item : qAsConst(children_)) {
        // if(item->fileInfo_)
        //  qDebug() << "child: " << QString::fromUtf8(fm_file_info_get_disp_name(item->fileInfo_));
        if(item->fileInfo_ && item->fileInfo_->path() == path) {
            return item;
        }
        else if(recursive) {
            DirTreeModelItem* child = item->childFromPath(std::move(path), true);
            if(child) {
                return child;
            }
        }
    }
    return nullptr;
}

void DirTreeModelItem::setShowHidden(bool show) {
    if(show) {
        // move all hidden children to visible list
        for(auto item: hiddenChildren_) {
            insertItem(item);
        }
        hiddenChildren_.clear();
        // remove the placeholder if needed
        if(children_.size() > 1) {
            auto it = std::find(children_.cbegin(), children_.cend(), placeHolderChild_);
            if(it != children_.cend()) {
                auto pos = it - children_.cbegin();
                model_->beginRemoveRows(index(), pos, pos);
                children_.erase(it);
                delete placeHolderChild_;
                model_->endRemoveRows();
                placeHolderChild_ = nullptr;
            }
        }
        // recursively show children of children, etc.
        for(auto item: children_) {
            item->setShowHidden(true);
        }
    }
    else { // hide hidden folders
        QModelIndex _index = index();
        int pos = 0;
        for(auto it = children_.begin(); it != children_.end(); ++pos) {
            DirTreeModelItem* item = *it;
            if(item->fileInfo_) {
                if(item->fileInfo_->isHidden()) { // hidden folder
                    // remove from the model and add to the hiddenChildren_ list
                    model_->beginRemoveRows(_index, pos, pos);
                    it = children_.erase(it);
                    hiddenChildren_.push_back(item);
                    model_->endRemoveRows();
                }
                else { // visible folder, recursively filter its children
                    item->setShowHidden(show);
                    ++it;
                }
            }
            else {
                ++it;
            }
        }
        if(children_.empty()) { // no visible children, add a placeholder item to keep the row expanded
            addPlaceHolderChild();
            placeHolderChild_->displayName_ = DirTreeModel::tr("<No sub folders>");
        }
    }
}



} // namespace Fm
