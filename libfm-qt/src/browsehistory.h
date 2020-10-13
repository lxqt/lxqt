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


#ifndef FM_BROWSEHISTORY_H
#define FM_BROWSEHISTORY_H

#include "libfmqtglobals.h"
#include <vector>

#include "core/filepath.h"

namespace Fm {

// class used to story browsing history of folder views
// We use this class to replace FmNavHistory provided by libfm since
// the original Libfm API is hard to use and confusing.

class LIBFM_QT_API BrowseHistoryItem {
public:

    explicit BrowseHistoryItem():
        scrollPos_(0) {
    }

    explicit BrowseHistoryItem(Fm::FilePath path, int scrollPos = 0):
        path_(std::move(path)),
        scrollPos_(scrollPos) {
    }

    BrowseHistoryItem(const BrowseHistoryItem& other) = default;

    ~BrowseHistoryItem() {
    }

    BrowseHistoryItem& operator=(const BrowseHistoryItem& other) {
        path_ = other.path_;
        scrollPos_ = other.scrollPos_;
        return *this;
    }

    Fm::FilePath path() const {
        return path_;
    }

    int scrollPos() const {
        return scrollPos_;
    }

    void setScrollPos(int pos) {
        scrollPos_ = pos;
    }

private:
    Fm::FilePath path_;
    int scrollPos_;
    // TODO: we may need to store current selection as well.
};

class LIBFM_QT_API BrowseHistory {

public:
    BrowseHistory();
    virtual ~BrowseHistory();

    int currentIndex() const {
        return currentIndex_;
    }
    void setCurrentIndex(int index);

    Fm::FilePath currentPath() const {
        return items_[currentIndex_].path();
    }

    int currentScrollPos() const {
        return items_[currentIndex_].scrollPos();
    }

    BrowseHistoryItem& currentItem() {
        return items_[currentIndex_];
    }

    size_t size() const {
        return items_.size();
    }

    BrowseHistoryItem& at(int index) {
        return items_[index];
    }

    void add(Fm::FilePath path, int scrollPos = 0);

    bool canForward() const;

    bool canBackward() const;

    int backward();

    int forward();

    int maxCount() const {
        return maxCount_;
    }

    void setMaxCount(int maxCount);

private:
    std::vector<BrowseHistoryItem> items_;
    int currentIndex_;
    int maxCount_;
};

}

#endif // FM_BROWSEHISTORY_H
