#include "filechangeattrjob.h"
#include "totalsizejob.h"

#include <sys/stat.h>

namespace Fm {

static const char query[] =  G_FILE_ATTRIBUTE_STANDARD_TYPE","
                             G_FILE_ATTRIBUTE_STANDARD_NAME","
                             G_FILE_ATTRIBUTE_UNIX_GID","
                             G_FILE_ATTRIBUTE_UNIX_UID","
                             G_FILE_ATTRIBUTE_UNIX_MODE","
                             G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME;

FileChangeAttrJob::FileChangeAttrJob(FilePathList paths):
    paths_{std::move(paths)},
    recursive_{false},
    // chmod
    fileModeEnabled_{false},
    newMode_{0},
    newModeMask_{0},
    // chown
    ownerEnabled_{false},
    uid_{0},
    groupEnabled_{false},
    gid_{0},
    // Display name
    displayNameEnabled_{false},
    // icon
    iconEnabled_{false},
    // hidden
    hiddenEnabled_{false},
    hidden_{false},
    // target uri
    targetUriEnabled_{false} {

    // the progress of chmod/chown is not related to file size
    setCalcProgressUsingSize(false);
}

void FileChangeAttrJob::exec() {
    // count total amount of the work
    if(recursive_) {
        TotalSizeJob totalSizeJob{paths_};
        connect(&totalSizeJob, &TotalSizeJob::error, this, &FileChangeAttrJob::error);
        connect(this, &FileChangeAttrJob::cancelled, &totalSizeJob, &TotalSizeJob::cancel);
        totalSizeJob.run();
        std::uint64_t totalSize, totalCount;
        totalSizeJob.totalAmount(totalSize, totalCount);
        setTotalAmount(totalSize, totalCount);
    }
    else {
        setTotalAmount(paths_.size(), paths_.size());
    }

    // ready to start
    Q_EMIT preparedToRun();

    // do the actual change attrs job
    for(auto& path : paths_) {
        if(isCancelled()) {
            break;
        }
        GErrorPtr err;
        GFileInfoPtr info{
            g_file_query_info(path.gfile().get(), query,
                              G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                              cancellable().get(), &err),
            false
        };
        if(info) {
            processFile(path, info);
        }
        else {
            handleError(err, path, info);
        }
    }
}

bool FileChangeAttrJob::processFile(const FilePath& path, const GFileInfoPtr& info) {
    setCurrentFile(path);
    bool ret = true;

    if(ownerEnabled_) {
        changeFileOwner(path, info, uid_);
    }
    if(groupEnabled_) {
        changeFileGroup(path, info, gid_);
    }
    if(fileModeEnabled_) {
        changeFileMode(path, info, newMode_, newModeMask_);
    }
    /* change display name, icon, hidden, target */
    if(displayNameEnabled_ && !displayName().empty()) {
        changeFileDisplayName(path, info, displayName_.c_str());
    }
    if(iconEnabled_ && icon_) {
        changeFileIcon(path, info, icon_);
    }
    if(hiddenEnabled_) {
        changeFileHidden(path, info, hidden_);
    }
    if(targetUriEnabled_ && !targetUri_.empty()) {
        changeFileTargetUri(path, info, targetUri_.c_str());
    }

    // FIXME: do not use size 1 here.
    addFinishedAmount(1, 1);

    // recursively apply to subfolders
    auto type = g_file_info_get_file_type(info.get());
    if(!isCancelled() && recursive_ && type == G_FILE_TYPE_DIRECTORY) {
        bool retry;
        do {
            retry = false;
            GErrorPtr err;
            GFileEnumeratorPtr enu{
                g_file_enumerate_children(path.gfile().get(), query,
                                            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                            cancellable().get(), &err),
                false
            };
            if(enu) {
                while(!isCancelled()) {
                    err.reset();
                    GFileInfoPtr childInfo{g_file_enumerator_next_file(enu.get(), cancellable().get(), &err), false};
                    if(childInfo) {
                        auto childPath = path.child(g_file_info_get_name(childInfo.get()));
                        ret = processFile(childPath, childInfo);
                        if(!ret) { /* _fm_file_ops_job_change_attr_file() failed */
                            break;
                        }
                    }
                    else {
                        if(err) {
                            handleError(err, path, info, ErrorSeverity::MILD);
                            retry = false;
                            /* FM_JOB_RETRY is not supported here */
                        }
                        else { /* EOF */
                            break;
                        }
                    }
                }
                g_file_enumerator_close(enu.get(), cancellable().get(), nullptr);
            }
            else {
                retry = handleError(err, path, info);
            }
        } while(!isCancelled() && retry);
    }
    return ret;
}

bool FileChangeAttrJob::handleError(GErrorPtr &err, const FilePath & /*path*/, const GFileInfoPtr & /*info*/, ErrorSeverity severity) {
    auto act = emitError(err, severity);
    if (act == ErrorAction::RETRY) {
        err.reset();
        return true;
    }
    return false;
}

bool FileChangeAttrJob::changeFileOwner(const FilePath& path, const GFileInfoPtr& info, uid_t uid) {
    /* change owner */
    bool ret = false;
    bool retry;
    do {
        GErrorPtr err;
        if(!g_file_set_attribute_uint32(path.gfile().get(), G_FILE_ATTRIBUTE_UNIX_UID,
                                        uid, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                        cancellable().get(), &err)) {
            retry = handleError(err, path, info, ErrorSeverity::MILD);
            err.reset();
        }
        else {
            ret = true;
            retry = false;
        }
    } while(retry && !isCancelled());
    return ret;
}

bool FileChangeAttrJob::changeFileGroup(const FilePath& path, const GFileInfoPtr& info, gid_t gid) {
    /* change group */
    bool ret = false;
    bool retry;
    do {
        GErrorPtr err;
        if(!g_file_set_attribute_uint32(path.gfile().get(), G_FILE_ATTRIBUTE_UNIX_GID,
                                        gid, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                        cancellable().get(), &err)) {
            retry = handleError(err, path, info, ErrorSeverity::MILD);
            err.reset();
        }
        else {
            ret = true;
            retry = false;
        }
    } while(retry && !isCancelled());
    return ret;
}

bool FileChangeAttrJob::changeFileMode(const FilePath& path, const GFileInfoPtr& info, mode_t newMode, mode_t newModeMask) {
    bool ret = false;
    /* change mode */
    if(newModeMask) {
        guint32 mode = g_file_info_get_attribute_uint32(info.get(), G_FILE_ATTRIBUTE_UNIX_MODE);
        mode &= ~newModeMask;
        mode |= (newMode & newModeMask);

        auto type = g_file_info_get_file_type(info.get());
        /* FIXME: this behavior should be optional. */
        /* treat dirs with 'r' as 'rx' */
        if(type == G_FILE_TYPE_DIRECTORY) {
            if((newModeMask & S_IRUSR) && (mode & S_IRUSR)) {
                mode |= S_IXUSR;
            }
            if((newModeMask & S_IRGRP) && (mode & S_IRGRP)) {
                mode |= S_IXGRP;
            }
            if((newModeMask & S_IROTH) && (mode & S_IROTH)) {
                mode |= S_IXOTH;
            }
        }

        /* new mode */
        bool retry;
        do {
            GErrorPtr err;
            if(!g_file_set_attribute_uint32(path.gfile().get(), G_FILE_ATTRIBUTE_UNIX_MODE,
                                            mode, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                            cancellable().get(), &err)) {
                retry = handleError(err, path, info, ErrorSeverity::MILD);
                err.reset();
            }
            else {
                ret = true;
                retry = false;
            }
        } while(retry && !isCancelled());
    }
    return ret;

}

bool FileChangeAttrJob::changeFileDisplayName(const FilePath& path, const GFileInfoPtr& info, const char* displayName) {
    bool ret = false;
    bool retry;
    do {
        GErrorPtr err;
        if(!g_file_set_display_name(path.gfile().get(), displayName, cancellable().get(), &err)) {
            retry = handleError(err, path, info, ErrorSeverity::MILD);
            err.reset();
        }
        else {
            ret = true;
            retry = false;
        }
    } while(retry && !isCancelled());
    return ret;
}

bool FileChangeAttrJob::changeFileIcon(const FilePath& path, const GFileInfoPtr& info, GIconPtr& icon) {
    bool ret = false;
    bool retry;
    do {
        GErrorPtr err;
        if(!g_file_set_attribute(path.gfile().get(), G_FILE_ATTRIBUTE_STANDARD_ICON,
                                 G_FILE_ATTRIBUTE_TYPE_OBJECT, icon.get(),
                                 G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                 cancellable().get(), &err)) {
            retry = handleError(err, path, info, ErrorSeverity::MILD);
            err.reset();
        }
        else {
            ret = true;
            retry = false;
        }
    } while(retry && !isCancelled());
    return ret;
}

bool FileChangeAttrJob::changeFileHidden(const FilePath& path, const GFileInfoPtr& info, bool hidden) {
    bool ret = false;
    bool retry;
    do {
        GErrorPtr err;
        gboolean g_hidden = hidden;
        if(!g_file_set_attribute(path.gfile().get(), G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN,
                                 G_FILE_ATTRIBUTE_TYPE_BOOLEAN, &g_hidden,
                                 G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                 cancellable().get(), &err)) {
            retry = handleError(err, path, info, ErrorSeverity::MILD);
            err.reset();
        }
        else {
            ret = true;
            retry = false;
        }
    } while(retry && !isCancelled());
    return ret;
}

bool FileChangeAttrJob::changeFileTargetUri(const FilePath& path, const GFileInfoPtr& info, const char* targetUri) {
    bool ret = false;
    bool retry;
    do {
        GErrorPtr err;
        if(!g_file_set_attribute_string(path.gfile().get(), G_FILE_ATTRIBUTE_STANDARD_TARGET_URI,
                                        targetUri, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                        cancellable().get(), &err)) {
            retry = handleError(err, path, info, ErrorSeverity::MILD);
            err.reset();
        }
        else {
            ret = true;
            retry = false;
        }
    } while(retry && !isCancelled());
    return ret;
}

} // namespace Fm
