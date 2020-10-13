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

#ifndef FM_DIRTREEMODELITEM_H
#define FM_DIRTREEMODELITEM_H

#include "libfmqtglobals.h"
#include <vector>
#include <QIcon>
#include <QModelIndex>

#include "core/fileinfo.h"
#include "core/folder.h"

namespace Fm {

class DirTreeModel;
class DirTreeView;

class LIBFM_QT_API DirTreeModelItem {
public:
    friend class DirTreeModel; // allow direct access of private members in DirTreeModel
    friend class DirTreeView; // allow direct access of private members in DirTreeView

    explicit DirTreeModelItem();
    explicit DirTreeModelItem(std::shared_ptr<const Fm::FileInfo> info, DirTreeModel* model, DirTreeModelItem* parent = nullptr);
    ~DirTreeModelItem();

    void loadFolder();
    void unloadFolder();

    inline bool isPlaceHolder() const {
        return (fileInfo_ == nullptr);
    }

    void setShowHidden(bool show);

    bool isQueuedForDeletion() const {
        return queuedForDeletion_;
    }
    

private:
    void freeFolder();
    void addPlaceHolderChild();
    DirTreeModelItem* childFromName(const char* utf8_name, int* pos);
    DirTreeModelItem* childFromPath(Fm::FilePath path, bool recursive) const;

    DirTreeModelItem* insertFile(std::shared_ptr<const Fm::FileInfo> fi);
    void insertFiles(Fm::FileInfoList files);
    int insertItem(Fm::DirTreeModelItem* newItem);
    QModelIndex index();

    void onFolderFinishLoading();
    void onFolderFilesAdded(Fm::FileInfoList &files);
    void onFolderFilesRemoved(Fm::FileInfoList &files);
    void onFolderFilesChanged(std::vector<Fm::FileInfoPair>& changes);

private:
    std::shared_ptr<const Fm::FileInfo> fileInfo_;
    std::shared_ptr<Fm::Folder> folder_;
    QString displayName_ ;
    QIcon icon_;
    bool expanded_;
    bool loaded_;
    DirTreeModelItem* parent_;
    DirTreeModelItem* placeHolderChild_;
    std::vector<DirTreeModelItem*> children_;
    std::vector<DirTreeModelItem*> hiddenChildren_;
    DirTreeModel* model_;
    bool queuedForDeletion_;
    // signal connections
    QMetaObject::Connection onFolderFinishLoadingConn_;
    QMetaObject::Connection onFolderFilesAddedConn_;
    QMetaObject::Connection onFolderFilesRemovedConn_;
    QMetaObject::Connection onFolderFilesChangedConn_;
};

}

#endif // FM_DIRTREEMODELITEM_H
