#ifndef DIRTREEITEM_H
#define DIRTREEITEM_H

#include "core/file-data.h"
#include <memory>
#include <forward_list>

class DirTreeItem {
public:
    DirTreeItem();

private:
    FileData* data_;
    char* name_;

};

#endif // DIRTREEITEM_H
