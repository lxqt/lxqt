#include "fileinfo.h"
#include "fileinfo_p.h"
#include <gio/gio.h>

#define METADATA_TRUST "metadata::trust"

namespace Fm {

const char defaultGFileInfoQueryAttribs[] = "standard::*,"
                                            "unix::*,"
                                            "time::*,"
                                            "access::*,"
                                            "trash::deletion-date,"
                                            "id::filesystem,"
                                            "metadata::emblems,"
                                            METADATA_TRUST;

FileInfo::FileInfo() {
    // FIXME: initialize numeric data members
}

FileInfo::FileInfo(const GFileInfoPtr& inf, const FilePath& filePath, const FilePath& parentDirPath) {
    setFromGFileInfo(inf, filePath, parentDirPath);
}

FileInfo::~FileInfo() {
}

void FileInfo::setFromGFileInfo(const GObjectPtr<GFileInfo>& inf, const FilePath& filePath, const FilePath& parentDirPath) {
    inf_ = inf;
    filePath_ = filePath;
    if (filePath_ && filePath_.hasParent()) {
        dirPath_ = filePath_.parent();
    }
    else {
        dirPath_ = parentDirPath;
    }
    const char* tmp, *uri;
    GIcon* gicon;
    GFileType type;

    if (const char * name = g_file_info_get_name(inf.get()))
        name_ = name;

    dispName_ = QString::fromUtf8(g_file_info_get_display_name(inf.get()));

    size_ = g_file_info_get_size(inf.get());

    tmp = g_file_info_get_content_type(inf.get());
    if(tmp) {
        mimeType_ = MimeType::fromName(tmp);
    }

    mode_ = g_file_info_get_attribute_uint32(inf.get(), G_FILE_ATTRIBUTE_UNIX_MODE);

    uid_ = gid_ = -1;
    if(g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_UNIX_UID)) {
        uid_ = g_file_info_get_attribute_uint32(inf.get(), G_FILE_ATTRIBUTE_UNIX_UID);
    }
    if(g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_UNIX_GID)) {
        gid_ = g_file_info_get_attribute_uint32(inf.get(), G_FILE_ATTRIBUTE_UNIX_GID);
    }

    type = g_file_info_get_file_type(inf.get());
    if(0 == mode_) { /* if UNIX file mode is not available, compose a fake one. */
        switch(type) {
        case G_FILE_TYPE_REGULAR:
            mode_ |= S_IFREG;
            break;
        case G_FILE_TYPE_DIRECTORY:
            mode_ |= S_IFDIR;
            break;
        case G_FILE_TYPE_SYMBOLIC_LINK:
            mode_ |= S_IFLNK;
            break;
        case G_FILE_TYPE_SHORTCUT:
            break;
        case G_FILE_TYPE_MOUNTABLE:
            break;
        case G_FILE_TYPE_SPECIAL:
            if(mode_) {
                break;
            }
            /* if it's a special file but it doesn't have UNIX mode, compose a fake one. */
            if(strcmp(tmp, "inode/chardevice") == 0) {
                mode_ |= S_IFCHR;
            }
            else if(strcmp(tmp, "inode/blockdevice") == 0) {
                mode_ |= S_IFBLK;
            }
            else if(strcmp(tmp, "inode/fifo") == 0) {
                mode_ |= S_IFIFO;
            }
#ifdef S_IFSOCK
            else if(strcmp(tmp, "inode/socket") == 0) {
                mode_ |= S_IFSOCK;
            }
#endif
            break;
        case G_FILE_TYPE_UNKNOWN:
            ;
        }
    }

    if(g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_ACCESS_CAN_READ)) {
        isAccessible_ = g_file_info_get_attribute_boolean(inf.get(), G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
    }
    else
        /* assume it's accessible */
    {
        isAccessible_ = true;
    }

    if(g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE)) {
        isWritable_ = g_file_info_get_attribute_boolean(inf.get(), G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE);
    }
    else
        /* assume it's writable */
    {
        isWritable_ = true;
    }

    if(g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE)) {
        isDeletable_ = g_file_info_get_attribute_boolean(inf.get(), G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE);
    }
    else
        /* assume it's deletable */
    {
        isDeletable_ = true;
    }

    isShortcut_ = (type == G_FILE_TYPE_SHORTCUT);
    isMountable_ = (type == G_FILE_TYPE_MOUNTABLE);

    /* special handling for symlinks */
    if(g_file_info_get_is_symlink(inf.get())) {
        mode_ &= ~S_IFMT; /* reset type */
        mode_ |= S_IFLNK; /* set type to symlink */
        goto _file_is_symlink;
    }

    switch(type) {
    case G_FILE_TYPE_SHORTCUT:
    /* Falls through. */
    case G_FILE_TYPE_MOUNTABLE:
        uri = g_file_info_get_attribute_string(inf.get(), G_FILE_ATTRIBUTE_STANDARD_TARGET_URI);
        if(uri) {
            if(g_str_has_prefix(uri, "file:///")) {
                auto filename = CStrPtr{g_filename_from_uri(uri, nullptr, nullptr)};
                target_ = filename.get();
            }
            else {
                target_ = uri;
            }
            if(!mimeType_) {
                mimeType_ = MimeType::guessFromFileName(target_.c_str());
            }
        }

        /* if the mime-type is not determined or is unknown */
        if(G_UNLIKELY(!mimeType_ || mimeType_->isUnknownType())) {
            /* FIXME: is this appropriate? */
            if(type == G_FILE_TYPE_SHORTCUT) {
                mimeType_ = MimeType::inodeShortcut();
            }
            else {
                mimeType_ = MimeType::inodeMountPoint();
            }
        }
        break;
    case G_FILE_TYPE_DIRECTORY:
        if(!mimeType_) {
            mimeType_ = MimeType::inodeDirectory();
        }
        isReadOnly_ = false; /* default is R/W */
        if(g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_FILESYSTEM_READONLY)) {
            isReadOnly_ = g_file_info_get_attribute_boolean(inf.get(), G_FILE_ATTRIBUTE_FILESYSTEM_READONLY);
        }
        /* directories should be writable to be deleted by user */
        if(isReadOnly_ || !isWritable_) {
            isDeletable_ = false;
        }
        break;
    case G_FILE_TYPE_SYMBOLIC_LINK:
_file_is_symlink:
        uri = g_file_info_get_symlink_target(inf.get());
        if(uri) {
            if(g_str_has_prefix(uri, "file:///")) {
                auto filename = CStrPtr{g_filename_from_uri(uri, nullptr, nullptr)};
                target_ = filename.get();
            }
            else {
                target_ = uri;
            }
            if(!mimeType_) {
                mimeType_ = MimeType::guessFromFileName(target_.c_str());
            }
        }
    /* Falls through. */
    /* continue with absent mime type */
    default: /* G_FILE_TYPE_UNKNOWN G_FILE_TYPE_REGULAR G_FILE_TYPE_SPECIAL */
        if(G_UNLIKELY(!mimeType_)) {
            if(!mimeType_) {
                mimeType_ = MimeType::guessFromFileName(name_.c_str());
            }
        }
    }

    if(!mimeType_) {
        mimeType_ = MimeType::fromName("application/octet-stream");
    }

    /* if there is a custom folder icon, use it */
    if(isNative() && type == G_FILE_TYPE_DIRECTORY) {
        auto local_path = path().localPath();
        auto dot_dir = CStrPtr{g_build_filename(local_path.get(), ".directory", nullptr)};
        if(g_file_test(dot_dir.get(), G_FILE_TEST_IS_REGULAR)) {
            GKeyFile* kf = g_key_file_new();
            if(g_key_file_load_from_file(kf, dot_dir.get(), G_KEY_FILE_NONE, nullptr)) {
                CStrPtr icon_name{g_key_file_get_string(kf, "Desktop Entry", "Icon", nullptr)};
                if(icon_name) {
                    auto dot_icon = IconInfo::fromName(icon_name.get());
                    if(dot_icon && dot_icon->isValid()) {
                        icon_ = dot_icon;
                    }
                }
            }
            g_key_file_free(kf);
        }
     }

    if(!icon_) {
        /* try file-specific icon first */
        gicon = g_file_info_get_icon(inf.get());
        if(gicon) {
            icon_ = IconInfo::fromGIcon(gicon);
        }
    }

#if 0
    /* set "locked" icon on unaccesible folder */
    else if(!accessible && type == G_FILE_TYPE_DIRECTORY) {
        icon = g_object_ref(icon_locked_folder);
    }
    else {
        icon = g_object_ref(fm_mime_type_get_icon(mime_type));
    }
#endif

    /* if the file has emblems, add them to the icon */
    auto emblem_names = g_file_info_get_attribute_stringv(inf.get(), "metadata::emblems");
    if(emblem_names) {
        auto n_emblems = g_strv_length(emblem_names);
        for(int i = n_emblems - 1; i >= 0; --i) {
            emblems_.emplace_front(Fm::IconInfo::fromName(emblem_names[i]));
        }
    }

    tmp = g_file_info_get_attribute_string(inf.get(), G_FILE_ATTRIBUTE_ID_FILESYSTEM);
    filesystemId_ = g_intern_string(tmp);

    mtime_ = g_file_info_get_attribute_uint64(inf.get(), G_FILE_ATTRIBUTE_TIME_MODIFIED);
    atime_ = g_file_info_get_attribute_uint64(inf.get(), G_FILE_ATTRIBUTE_TIME_ACCESS);
    ctime_ = g_file_info_get_attribute_uint64(inf.get(), G_FILE_ATTRIBUTE_TIME_CHANGED);
    if(auto dt = g_file_info_get_deletion_date(inf.get())){
        dtime_ = g_date_time_to_unix(dt);
    }
    else {
        dtime_ = 0;
    }
    isHidden_ = g_file_info_get_is_hidden(inf.get());
    // g_file_info_get_is_backup() does not cover ".bak" and ".old".
    // NOTE: Here, dispName_ is not modified for desktop entries yet.
    isBackup_ = g_file_info_get_is_backup(inf.get())
                || dispName_.endsWith(QLatin1String(".bak"))
                || dispName_.endsWith(QLatin1String(".old"));
    isNameChangeable_ = true; /* GVFS tends to ignore this attribute */
    isIconChangeable_ = isHiddenChangeable_ = false;
    if(g_file_info_has_attribute(inf.get(), G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME)) {
        isNameChangeable_ = g_file_info_get_attribute_boolean(inf.get(), G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME);
    }

    // special handling for desktop entry files (show the name and icon defined in the desktop entry instead)
    if(isNative() && G_UNLIKELY(isDesktopEntry())) {
        auto local_path = path().localPath();
        GKeyFile* kf = g_key_file_new();
        if(g_key_file_load_from_file(kf, local_path.get(), G_KEY_FILE_NONE, nullptr)) {
            /* check if type is correct and supported */
            CStrPtr type{g_key_file_get_string(kf, "Desktop Entry", "Type", nullptr)};
            if(type) {
                // Type == "Link"
                if(strcmp(type.get(), G_KEY_FILE_DESKTOP_TYPE_LINK) == 0) {
                    CStrPtr uri{g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_URL, nullptr)};
                    if(uri) {
                        isShortcut_ = true;
                        target_ = uri.get();
                    }
                }
            }
            CStrPtr icon_name{g_key_file_get_string(kf, "Desktop Entry", "Icon", nullptr)};
            if(icon_name) {
                icon_ = IconInfo::fromName(icon_name.get());
            }
            /* Use title of the desktop entry for display */
            CStrPtr displayName{g_key_file_get_locale_string(kf, "Desktop Entry", "Name", nullptr, nullptr)};
            if(displayName) {
                dispName_ = QString::fromUtf8(displayName.get());
            }
            /* handle 'Hidden' key to set hidden attribute */
            if(!isHidden_) {
                isHidden_ = g_key_file_get_boolean(kf, "Desktop Entry", "Hidden", nullptr);
            }
        }
        g_key_file_free(kf);
    }

    if(!icon_ && mimeType_)
        icon_ = mimeType_->icon();

#if 0
    GFile* _gf = nullptr;
    GFileAttributeInfoList* list;
    auto list = g_file_query_settable_attributes(gf, nullptr, nullptr);
    if(G_LIKELY(list)) {
        if(g_file_attribute_info_list_lookup(list, G_FILE_ATTRIBUTE_STANDARD_ICON)) {
            icon_is_changeable = true;
        }
        if(g_file_attribute_info_list_lookup(list, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN)) {
            hidden_is_changeable = true;
        }
        g_file_attribute_info_list_unref(list);
    }
    if(G_UNLIKELY(_gf)) {
        g_object_unref(_gf);
    }
#endif
}

bool FileInfo::canThumbnail() const {
    /* We cannot use S_ISREG here as this exclude all symlinks */
    if(size_ == 0 ||  /* don't generate thumbnails for empty files */
            !(mode_ & S_IFREG) ||
            isDesktopEntry() ||
            isUnknownType()) {
        return false;
    }
    return true;
}

/* full path of the file is required by this function */
bool FileInfo::isExecutableType() const {
    if(isDesktopEntry()) {
        /* treat desktop entries as executables if
         they are native and have read permission */
        if(isNative() && (mode_ & (S_IRUSR|S_IRGRP|S_IROTH))) {
            if(isShortcut() && !target_.empty()) {
                /* handle shortcuts from desktop to menu entries:
                   first check for entries in /usr/share/applications and such
                   which may be considered as a safe desktop entry path
                   then check if that is a shortcut to a native file
                   otherwise it is a link to a file under menu:// */
                if (!g_str_has_prefix(target_.c_str(), "/usr/share/")) {
                    auto target = FilePath::fromPathStr(target_.c_str());
                    bool is_native = target.isNative();
                    if (is_native) {
                        return true;
                    }
                }
            }
            else {
                return true;
            }
        }
        return false;
    }
    else if(isText()) { /* g_content_type_can_be_executable reports text files as executables too */
        /* We don't execute remote files nor files in trash */
        if(isNative() && (mode_ & (S_IXOTH | S_IXGRP | S_IXUSR))) {
            /* it has executable bits so lets check shell-bang */
            auto pathStr = path().toString();
            int fd = open(pathStr.get(), O_RDONLY);
            if(fd >= 0) {
                char buf[2];
                ssize_t rdlen = read(fd, &buf, 2);
                close(fd);
                if(rdlen == 2 && buf[0] == '#' && buf[1] == '!') {
                    return true;
                }
            }
        }
        return false;
    }
    return mimeType_->canBeExecutable();
}

bool FileInfo::isTrustable() const {
    if(isExecutableType()) {
        /* to avoid GIO assertion warning: */
        if(g_file_info_get_attribute_type(inf_.get(), METADATA_TRUST) == G_FILE_ATTRIBUTE_TYPE_STRING) {
            if(const auto data = g_file_info_get_attribute_string(inf_.get(), METADATA_TRUST)) {
                return (strcmp(data, "true") == 0);
            }
        }
        return false;
    }
    return true;
}

void FileInfo::setTrustable(bool trust) const {
    if(!isExecutableType()) {
        return; // METADATA_TRUST is only for executables
    }
    GObjectPtr<GFileInfo> info {g_file_info_new()}; // used to set only this attribute
    if(trust) {
        g_file_info_set_attribute_string(info.get(), METADATA_TRUST, "true");
        g_file_info_set_attribute_string(inf_.get(), METADATA_TRUST, "true");
    }
    else {
        g_file_info_set_attribute(info.get(), METADATA_TRUST, G_FILE_ATTRIBUTE_TYPE_INVALID, nullptr);
        g_file_info_set_attribute(inf_.get(), METADATA_TRUST, G_FILE_ATTRIBUTE_TYPE_INVALID, nullptr);
    }
    g_file_set_attributes_from_info(path().gfile().get(),
                                    info.get(),
                                    G_FILE_QUERY_INFO_NONE,
                                    nullptr, nullptr);
}


bool FileInfoList::isSameType() const {
    if(!empty()) {
        auto& item = front();
        for(auto it = cbegin() + 1; it != cend(); ++it) {
            auto& item2 = *it;
            if(item->mimeType() != item2->mimeType()) {
                return false;
            }
        }
    }
    return true;
}

bool FileInfoList::isSameFilesystem() const {
    if(!empty()) {
        auto& item = front();
        for(auto it = cbegin() + 1; it != cend(); ++it) {
            auto& item2 = *it;
            if(item->filesystemId() != item2->filesystemId()) {
                return false;
            }
        }
    }
    return true;
}



} // namespace Fm
