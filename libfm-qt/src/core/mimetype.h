/*
 *      fm-mime-type.h
 *
 *      Copyright 2009 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 *      This file is a part of the Libfm library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _FM2_MIME_TYPE_H_
#define _FM2_MIME_TYPE_H_

#include "../libfmqtglobals.h"
#include <glib.h>
#include <gio/gio.h>

#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <cstring>
#include <forward_list>
#include <functional>

#include "cstrptr.h"
#include "gobjectptr.h"
#include "iconinfo.h"
#include "thumbnailer.h"

namespace Fm {

class LIBFM_QT_API MimeType {
public:
    friend class Thumbnailer;

    explicit MimeType(const char* typeName);

    MimeType() = delete;

    ~MimeType();

    std::shared_ptr<const Thumbnailer> firstThumbnailer() const {
        std::lock_guard<std::mutex> lock{mutex_};
        return thumbnailers_.empty() ? nullptr : thumbnailers_.front();
    }

    void forEachThumbnailer(std::function<bool(const std::shared_ptr<const Thumbnailer>&)> func) const {
        std::lock_guard<std::mutex> lock{mutex_};
        for(auto& thumbnailer: thumbnailers_) {
            if(func(thumbnailer)) {
                break;
            }
        }
    }

    const std::shared_ptr<const IconInfo>& icon() const {
        return icon_;
    }

    const char* name() const {
        return name_.get();
    }

    const char* desc() const {
        if(!desc_) {
            desc_ = CStrPtr{g_content_type_get_description(name_.get())};
        }
        return desc_.get();
    }

    static std::shared_ptr<const MimeType> fromName(const char* typeName);

    static std::shared_ptr<const MimeType> guessFromFileName(const char* fileName);

    bool isUnknownType() const {
        return g_content_type_is_unknown(name_.get());
    }

    bool isDesktopEntry() const {
        return this == desktopEntry().get();
    }

    bool isText() const {
        return g_content_type_is_a(name_.get(), "text/plain");
    }

    bool isImage() const {
        return !std::strncmp("image/", name_.get(), 6);
    }

    bool isMountable() const {
        return this == inodeMountPoint().get();
    }

    bool isShortcut() const {
        return this == inodeShortcut().get();
    }

    bool isDir() const {
        return this == inodeDirectory().get();
    }

    bool canBeExecutable() const {
        return g_content_type_can_be_executable(name_.get());
    }

    static std::shared_ptr<const MimeType> inodeDirectory() {   // inode/directory
        if(!inodeDirectory_)
            inodeDirectory_ = fromName("inode/directory");
        return inodeDirectory_;
    }

    static std::shared_ptr<const MimeType> inodeShortcut() {  // inode/x-shortcut
        if(!inodeShortcut_)
            inodeShortcut_ = fromName("inode/x-shortcut");
        return inodeShortcut_;
    }

    static std::shared_ptr<const MimeType> inodeMountPoint() {  // inode/mount-point
        if(!inodeMountPoint_)
            inodeMountPoint_ = fromName("inode/mount-point");
        return inodeMountPoint_;
    }

    static std::shared_ptr<const MimeType> desktopEntry() { // application/x-desktop
        if(!desktopEntry_)
            desktopEntry_ = fromName("application/x-desktop");
        return desktopEntry_;
    }

private:
    void removeThumbnailer(std::shared_ptr<const Thumbnailer>& thumbnailer) {
        std::lock_guard<std::mutex> lock{mutex_};
        thumbnailers_.remove(thumbnailer);
    }

    void addThumbnailer(std::shared_ptr<const Thumbnailer> thumbnailer) {
        std::lock_guard<std::mutex> lock{mutex_};
        thumbnailers_.push_front(std::move(thumbnailer));
    }

private:
    std::shared_ptr<const IconInfo> icon_;
    CStrPtr name_;
    mutable CStrPtr desc_;
    std::forward_list<std::shared_ptr<const Thumbnailer>> thumbnailers_;
    static std::unordered_map<const char*, std::shared_ptr<const MimeType>, CStrHash, CStrEqual> cache_;
    static std::mutex mutex_;

    static std::shared_ptr<const MimeType> inodeDirectory_;  // inode/directory
    static std::shared_ptr<const MimeType> inodeShortcut_;  // inode/x-shortcut
    static std::shared_ptr<const MimeType> inodeMountPoint_;  // inode/mount-point
    static std::shared_ptr<const MimeType> desktopEntry_; // application/x-desktop
};


} // namespace Fm

#endif
