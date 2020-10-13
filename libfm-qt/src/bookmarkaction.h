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


#ifndef BOOKMARKACTION_H
#define BOOKMARKACTION_H

#include "libfmqtglobals.h"
#include <QAction>
#include "core/bookmarks.h"

namespace Fm {

// action used to create bookmark menu items
class LIBFM_QT_API BookmarkAction : public QAction {
public:
    explicit BookmarkAction(std::shared_ptr<const Fm::BookmarkItem> item, QObject* parent = nullptr);

    const std::shared_ptr<const Fm::BookmarkItem>& bookmark() const {
        return item_;
    }

    const Fm::FilePath& path() const {
        return item_->path();
    }

private:
    std::shared_ptr<const Fm::BookmarkItem> item_;
};

}

#endif // BOOKMARKACTION_H
