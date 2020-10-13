/*
 * Copyright (C) 2016 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef __LIBFM_QT_FM2_FILE_INFO_H__
#define __LIBFM_QT_FM2_FILE_INFO_H__

#include <QObject>
#include <QtGlobal>
#include "../libfmqtglobals.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <vector>
#include <set>
#include <utility>
#include <string>
#include <forward_list>

#include "gioptrs.h"
#include "filepath.h"
#include "iconinfo.h"
#include "mimetype.h"


namespace Fm {

class FileInfoList;
typedef std::set<unsigned int> HashSet;

class LIBFM_QT_API FileInfo {
public:

    explicit FileInfo();

    explicit FileInfo(const GFileInfoPtr& inf, const FilePath& filePath, const FilePath& parentDirPath = FilePath());

    virtual ~FileInfo();

    bool canSetHidden() const {
        return isHiddenChangeable_;
    }

    bool canSetIcon() const {
        return isIconChangeable_;
    }

    bool canSetName() const {
        return isNameChangeable_;
    }

    bool canThumbnail() const;

    gid_t gid() const {
        return gid_;
    }

    uid_t uid() const {
        return uid_;
    }

    const char* filesystemId() const {
        return filesystemId_;
    }

    const std::shared_ptr<const IconInfo>& icon() const {
        return icon_;
    }

    const std::shared_ptr<const MimeType>& mimeType() const {
        return mimeType_;
    }

    quint64 ctime() const {
        return ctime_;
    }


    quint64 atime() const {
        return atime_;
    }

    quint64 mtime() const {
        return mtime_;
    }

    quint64 dtime() const {
        return dtime_;
    }
    const std::string& target() const {
        return target_;
    }

    bool isWritableDirectory() const {
        return (!isReadOnly_ && isDir());
    }

    bool isAccessible() const {
        return isAccessible_;
    }

    bool isWritable() const {
        return isWritable_;
    }

    bool isDeletable() const {
        return isDeletable_;
    }

    bool isExecutableType() const;

    bool isBackup() const {
        return isBackup_;
    }

    bool isHidden() const {
        // FIXME: we might treat backup files as hidden
        return isHidden_;
    }

    bool isUnknownType() const {
        return mimeType_->isUnknownType();
    }

    bool isDesktopEntry() const {
        return mimeType_->isDesktopEntry();
    }

    bool isText() const {
        return mimeType_->isText();
    }

    bool isImage() const {
        return mimeType_->isImage();
    }

    bool isMountable() const {
        return isMountable_;
    }

    bool isShortcut() const {
        return isShortcut_;
    }

    bool isSymlink() const {
        return S_ISLNK(mode_) ? true : false;
    }

    bool isDir() const {
        return S_ISDIR(mode_) || mimeType_->isDir();
    }

    bool isNative() const {
        return dirPath_ ? dirPath_.isNative() : path().isNative();
    }

    mode_t mode() const {
        return mode_;
    }

    uint64_t realSize() const {
        return blksize_ *blocks_;
    }

    uint64_t size() const {
        return size_;
    }

    const std::string& name() const {
        return name_;
    }

    const QString& displayName() const {
        return dispName_;
    }

    QString description() const {
        return QString::fromUtf8(mimeType_ ? mimeType_->desc() : "");
    }

    FilePath path() const {
        return filePath_ ? filePath_ : dirPath_ ? dirPath_.child(name_.c_str()) : FilePath::fromPathStr(name_.c_str());
    }

    const FilePath& dirPath() const {
        return dirPath_;
    }

    void setFromGFileInfo(const GFileInfoPtr& inf, const FilePath& filePath, const FilePath& parentDirPath);

    const std::forward_list<std::shared_ptr<const IconInfo>>& emblems() const {
        return emblems_;
    }

    bool isTrustable() const;

    void setTrustable(bool trust) const;

    GObjectPtr<GFileInfo> gFileInfo() const {
        return inf_;
    }

private:
    GObjectPtr<GFileInfo> inf_;
    std::string name_;
    QString dispName_;

    FilePath filePath_;
    FilePath dirPath_;

    mode_t mode_;
    const char* filesystemId_;
    uid_t uid_;
    gid_t gid_;
    uint64_t size_;
    quint64 mtime_;
    quint64 atime_;
    quint64 ctime_;
    quint64 dtime_;

    uint64_t blksize_;
    uint64_t blocks_;

    std::shared_ptr<const MimeType> mimeType_;
    std::shared_ptr<const IconInfo> icon_;
    std::forward_list<std::shared_ptr<const IconInfo>> emblems_;

    std::string target_; /* target of shortcut or mountable. */

    bool isShortcut_ : 1; /* TRUE if file is shortcut type */
    bool isMountable_ : 1; /* TRUE if file is mountable type */
    bool isAccessible_ : 1; /* TRUE if can be read by user */
    bool isWritable_ : 1; /* TRUE if can be written to by user */
    bool isDeletable_ : 1; /* TRUE if can be deleted by user */
    bool isHidden_ : 1; /* TRUE if file is hidden */
    bool isBackup_ : 1; /* TRUE if file is backup */
    bool isNameChangeable_ : 1; /* TRUE if name can be changed */
    bool isIconChangeable_ : 1; /* TRUE if icon can be changed */
    bool isHiddenChangeable_ : 1; /* TRUE if hidden can be changed */
    bool isReadOnly_ : 1; /* TRUE if host FS is R/O */
};


class LIBFM_QT_API FileInfoList: public std::vector<std::shared_ptr<const FileInfo>> {
public:

    bool isSameType() const;

    bool isSameFilesystem() const;

    FilePathList paths() const {
        FilePathList ret;
        for(auto& file: *this) {
            ret.push_back(file->path());
        }
        return ret;
    }
};

// smart pointer to FileInfo objects (once created, FileInfo objects should stay immutable so const is needed here)
typedef std::shared_ptr<const FileInfo> FileInfoPtr;

typedef std::pair<FileInfoPtr, FileInfoPtr> FileInfoPair;

}

Q_DECLARE_METATYPE(std::shared_ptr<const Fm::FileInfo>)


#endif // __LIBFM_QT_FM2_FILE_INFO_H__
