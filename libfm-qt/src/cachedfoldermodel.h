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


#ifndef FM_CACHEDFOLDERMODEL_H
#define FM_CACHEDFOLDERMODEL_H

#include "libfmqtglobals.h"
#include "foldermodel.h"

#include "core/folder.h"

namespace Fm {

// FIXME: deprecate CachedFolderModel later (ugly API design with manual ref()/unref())
class LIBFM_QT_API CachedFolderModel : public FolderModel {
    Q_OBJECT
public:
    explicit CachedFolderModel(const std::shared_ptr<Fm::Folder>& folder);
    void ref() {
        ++refCount;
    }
    void unref();

    static CachedFolderModel* modelFromFolder(const std::shared_ptr<Fm::Folder>& folder);
    static CachedFolderModel* modelFromPath(const Fm::FilePath& path);

private:
    ~CachedFolderModel() override;

private:
    int refCount;
    constexpr static const char* cacheKey = "CachedFolderModel";
};


}

#endif // FM_CACHEDFOLDERMODEL_H
