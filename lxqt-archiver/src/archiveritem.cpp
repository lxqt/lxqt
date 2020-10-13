#include "archiveritem.h"

ArchiverItem::ArchiverItem(): data_{nullptr}, ownData_{false} {
}

ArchiverItem::ArchiverItem(const FileData* data, bool ownData):
    data_{data}, ownData_{ownData} {
}

ArchiverItem::ArchiverItem(const ArchiverItem &other):
    data_{other.data_ ? file_data_copy(const_cast<FileData*>(other.data_)) : nullptr},
    ownData_{true} {
}

ArchiverItem::~ArchiverItem() {
    if(ownData_ && data_) {
        file_data_free(const_cast<FileData*>(data_));
    }
}

const char* ArchiverItem::name() const {
    return data_ ? data_->name : nullptr;
}

const char* ArchiverItem::contentType() const {
    if(data_) {
        return isDir() ? "inode/directory" : data_->content_type;
    }
    return nullptr;
}

const char *ArchiverItem::originalPath() const {
    return data_ ? data_->original_path : nullptr;
}

const char *ArchiverItem::fullPath() const {
    return data_ ? data_->full_path : nullptr;
}


qint64 ArchiverItem::modifiedTime() const {
    return data_ ? static_cast<quint64>(data_->modified) : 0LL; // data_->modified is time_t
}

size_t ArchiverItem::size() const {
    return data_ ? data_->size : 0;
}

bool ArchiverItem::isEncrypted() const {
    return data_ ? data_->encrypted : 0;
}

bool ArchiverItem::isDir() const {
    return data_ ? file_data_is_dir(const_cast<FileData*>(data_)) : false;
}

bool ArchiverItem::hasChildren() const {
    return !children_.empty();
}

const std::vector<const ArchiverItem*>& ArchiverItem::children() const {
    return children_;
}

const FileData *ArchiverItem::data() const {
    return data_;
}

void ArchiverItem::setData(const FileData *data, bool ownData) {
    if(data_ && ownData_) {
        file_data_free(const_cast<FileData*>(data_));
    }
    data_ = data;
    ownData_ = ownData;
}

void ArchiverItem::addChild(ArchiverItem *child) {
  children_.emplace_back(child);
}

std::vector<const ArchiverItem *> &ArchiverItem::allChildren() const {
    std::vector<const ArchiverItem *> results;
    allChildren(results);
    return results;
}

void ArchiverItem::allChildren(std::vector<const ArchiverItem*>& results) const {
    for(auto& child: children()) {
        results.emplace_back(child);
        if(child->isDir()) {
            child->allChildren(results);
        }
    }
}
