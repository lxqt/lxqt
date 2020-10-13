#ifndef ARCHIVERITEM_H
#define ARCHIVERITEM_H

#include "core/file-data.h"

#include <QMetaType>
#include <vector>


class ArchiverItem {
public:
    ArchiverItem();

    ArchiverItem(const FileData* data, bool ownData);

    ArchiverItem(const ArchiverItem& other);

    ~ArchiverItem();

    const char* name() const;

    const char* contentType() const;

    const char* originalPath() const;

    const char* fullPath() const;

    qint64 modifiedTime() const;

    size_t size() const;

    bool isEncrypted() const;

    bool isDir() const;

    bool hasChildren() const;

    const std::vector<const ArchiverItem*>& children() const;

    const FileData *data() const;

    void setData(const FileData *data, bool ownData);

    void addChild(ArchiverItem* child);

    // recursively get all children of this item
    std::vector<const ArchiverItem*>& allChildren() const;

    void allChildren(std::vector<const ArchiverItem*>& results) const;

private:
    std::vector<const ArchiverItem*> children_;
    const FileData* data_;  // data from FrArchiver (optional)
    bool ownData_;
};

Q_DECLARE_METATYPE(const ArchiverItem*)


#endif // ARCHIVERITEM_H
