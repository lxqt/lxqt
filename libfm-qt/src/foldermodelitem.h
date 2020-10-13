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


#ifndef FM_FOLDERMODELITEM_H
#define FM_FOLDERMODELITEM_H

#include "libfmqtglobals.h"
#include <QImage>
#include <QString>
#include <QIcon>
#include <QVector>

#include "core/folder.h"

namespace Fm {

class LIBFM_QT_API FolderModelItem {
public:

    enum ThumbnailStatus {
        ThumbnailNotChecked,
        ThumbnailLoading,
        ThumbnailLoaded,
        ThumbnailFailed
    };

    struct Thumbnail {
        int size;
        ThumbnailStatus status;
        QImage image;
    };

public:
    explicit FolderModelItem(const std::shared_ptr<const Fm::FileInfo>& _info);
    FolderModelItem(const FolderModelItem& other);
    virtual ~FolderModelItem();

    const QString& displayName() const {
        return info->displayName();
    }

    const std::string& name() const {
        return info->name();
    }

    QIcon icon() const {
        const auto i = info->icon();
        return i ? i->qicon() : QIcon{};
    }

    QString ownerName() const;

    QString ownerGroup() const;

    const QString& displayMtime() const;

    const QString& displayDtime() const;

    const QString &displaySize() const;

    Thumbnail* findThumbnail(int size);

    void removeThumbnail(int size);

    std::shared_ptr<const Fm::FileInfo> info;
    mutable QString dispMtime_;
    mutable QString dispDtime_;
    mutable QString dispSize_;
    QVector<Thumbnail> thumbnails;
    bool isCut;
};

}

#endif // FM_FOLDERMODELITEM_H
