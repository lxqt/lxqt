#include "deletejob.h"
#include "totalsizejob.h"
#include "fileinfo_p.h"

namespace Fm {

bool DeleteJob::deleteFile(const FilePath& path, GFileInfoPtr inf) {
    ErrorAction act = ErrorAction::CONTINUE;
    while(!inf) {
        GErrorPtr err;
        inf = GFileInfoPtr{
            g_file_query_info(path.gfile().get(), "standard::*",
            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
            cancellable().get(), &err),
            false
        };
        if(err) {
            act = emitError(err, ErrorSeverity::SEVERE);
            if(act == ErrorAction::ABORT) {
                return false;
            }
            if(act != ErrorAction::RETRY) {
                break;
            }
        }
    }

    // TODO: get parent dir of the current path.
    //       if there is a Fm::Folder object created for it, block the update for the folder temporarily.

    /* currently processed file. */
    setCurrentFile(path);

    if(g_file_info_get_file_type(inf.get()) == G_FILE_TYPE_DIRECTORY) {
        // delete the content of the dir prior to deleting itself
        deleteDirContent(path, inf);
    }

    bool isTrashRoot = false;
    // special handling for trash:///
    if(!path.isNative() && g_strcmp0(path.uriScheme().get(), "trash") == 0) {
        // little trick: basename of trash root is /
        auto basename = path.baseName();
        if(basename && basename[0] == G_DIR_SEPARATOR) {
            isTrashRoot = true;
        }
    }

    bool hasError = false;
    while(!isCancelled()) {
        GErrorPtr err;
        // try to delete the path directly (but don't delete if it's trash:///)
        if(isTrashRoot || g_file_delete(path.gfile().get(), cancellable().get(), &err)) {
            break;
        }
        if(err) {
            // FIXME: error handling
            /* if it's non-empty dir then descent into it then try again */
            /* trash root gives G_IO_ERROR_PERMISSION_DENIED */
            if(err.domain() == G_IO_ERROR && err.code() == G_IO_ERROR_NOT_EMPTY) {
                deleteDirContent(path, inf);
            }
            else if(err.domain() == G_IO_ERROR && err.code() == G_IO_ERROR_PERMISSION_DENIED) {
                /* special case for trash:/// */
                /* FIXME: is there any better way to handle this? */
                auto scheme = path.uriScheme();
                if(g_strcmp0(scheme.get(), "trash") == 0) {
                    break;
                }
            }
            act = emitError(err, ErrorSeverity::MODERATE);
            if(act != ErrorAction::RETRY) {
                hasError = true;
                break;
            }
        }
    }

    addFinishedAmount(g_file_info_get_size(inf.get()), 1);

    return !hasError;
}

bool DeleteJob::deleteDirContent(const FilePath& path, GFileInfoPtr inf) {
    GErrorPtr err;
    GFileEnumeratorPtr enu {
        g_file_enumerate_children(path.gfile().get(), defaultGFileInfoQueryAttribs,
        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
        cancellable().get(), &err),
        false
    };
    if(!enu) {
        emitError(err, ErrorSeverity::MODERATE);
        return false;
    }

    bool hasError = false;
    while(!isCancelled()) {
        inf = GFileInfoPtr{
            g_file_enumerator_next_file(enu.get(), cancellable().get(), &err),
            false
        };
        if(inf) {
            auto subPath = path.child(g_file_info_get_name(inf.get()));
            if(!deleteFile(subPath, inf)) {
                continue;
            }
        }
        else {
            if(err) {
                emitError(err, ErrorSeverity::MODERATE);
                /* ErrorAction::RETRY is not supported here */
                hasError = true;
            }
            else { /* EOF */
            }
            break;
        }
    }
    g_file_enumerator_close(enu.get(), nullptr, nullptr);
    return !hasError;
}


DeleteJob::DeleteJob(const FilePathList &paths): paths_{paths} {
    setCalcProgressUsingSize(false);
}

DeleteJob::DeleteJob(FilePathList &&paths): paths_{paths} {
    setCalcProgressUsingSize(false);
}

DeleteJob::~DeleteJob() {
}

void DeleteJob::exec() {
    /* prepare the job, count total work needed with FmDeepCountJob */
    TotalSizeJob totalSizeJob{paths_, TotalSizeJob::Flags::PREPARE_DELETE};
    connect(&totalSizeJob, &TotalSizeJob::error, this, &DeleteJob::error);
    connect(this, &DeleteJob::cancelled, &totalSizeJob, &TotalSizeJob::cancel);
    totalSizeJob.run();

    if(isCancelled()) {
        return;
    }

    setTotalAmount(totalSizeJob.totalSize(), totalSizeJob.fileCount());
    Q_EMIT preparedToRun();

    for(auto& path : paths_) {
        if(isCancelled()) {
            break;
        }
        deleteFile(path, GFileInfoPtr{nullptr});
    }
}

} // namespace Fm
