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


#include "browsehistory.h"

namespace Fm {

BrowseHistory::BrowseHistory():
    currentIndex_(0),
    maxCount_(10) {
}

BrowseHistory::~BrowseHistory() {
}

void BrowseHistory::add(Fm::FilePath path, int scrollPos) {
    int lastIndex = items_.size() - 1;
    if(currentIndex_ < lastIndex) {
        // if we're not at the last item, remove items after the current one.
        items_.erase(items_.cbegin() + currentIndex_ + 1, items_.cend());
    }

    if(items_.size() + 1 > static_cast<size_t>(maxCount_)) {
        // if there are too many items, remove the oldest one.
        // FIXME: what if currentIndex_ == 0? remove the last item instead?
        if(currentIndex_ == 0) {
            items_.erase(items_.cbegin() + lastIndex);
        }
        else {
            items_.erase(items_.cbegin());
            --currentIndex_;
        }
    }
    // add a path and current scroll position to browse history
    items_.push_back(BrowseHistoryItem(path, scrollPos));
    currentIndex_ = items_.size() - 1;
}

void BrowseHistory::setCurrentIndex(int index) {
    if(index >= 0 && static_cast<size_t>(index) < items_.size()) {
        currentIndex_ = index;
        // FIXME: should we emit a signal for the change?
    }
}

bool BrowseHistory::canBackward() const {
    return (currentIndex_ > 0);
}

int BrowseHistory::backward() {
    if(canBackward()) {
        --currentIndex_;
    }
    return currentIndex_;
}

bool BrowseHistory::canForward() const {
    return (static_cast<size_t>(currentIndex_) + 1 < items_.size());
}

int BrowseHistory::forward() {
    if(canForward()) {
        ++currentIndex_;
    }
    return currentIndex_;
}

void BrowseHistory::setMaxCount(int maxCount) {
    maxCount_ = maxCount;
    if(items_.size() > static_cast<size_t>(maxCount)) {
        // TODO: remove some items
    }
}


} // namespace Fm
