#include "untrashjob.h"
#include "filetransferjob.h"

namespace Fm {

UntrashJob::UntrashJob(FilePathList srcPaths):
    srcPaths_{std::move(srcPaths)} {
}

void UntrashJob::exec() {
    // preparing for the job
    FilePathList validSrcPaths;
    FilePathList origPaths;
    for(auto& srcPath: srcPaths_) {
        if(isCancelled()) {
            break;
        }
        GErrorPtr err;
        GFileInfoPtr srcInfo{
            g_file_query_info(srcPath.gfile().get(),
                              "trash::orig-path",
                              G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                              cancellable().get(),
                              &err),
            false
        };
        if(srcInfo) {
            const char* orig_path_str = g_file_info_get_attribute_byte_string(srcInfo.get(), "trash::orig-path");
            if(orig_path_str) {
                validSrcPaths.emplace_back(srcPath);
                origPaths.emplace_back(FilePath::fromPathStr(orig_path_str));
            }
            else {
                g_set_error(&err, G_IO_ERROR, G_IO_ERROR_FAILED,
                            tr("Cannot untrash file '%s': original path not known").toUtf8().constData(),
                            g_file_info_get_display_name(srcInfo.get()));
                // FIXME: do we need to retry here?
                emitError(err, ErrorSeverity::MODERATE);
            }
        }
        else {
            // FIXME: do we need to retry here?
            emitError(err);
        }
    }

    // collected original paths of the trashed files
    // use the file transfer job to handle the actual file move
    FileTransferJob fileTransferJob{std::move(validSrcPaths), std::move(origPaths), FileTransferJob::Mode::MOVE};
    // FIXME:
    // I'm not sure why specifying Qt::DirectConnection is needed here since the caller & receiver are in the same thread. :-(
    // However without this, the signals/slots here will cause deadlocks.
    connect(&fileTransferJob, &FileTransferJob::preparedToRun, this, &UntrashJob::preparedToRun, Qt::DirectConnection);
    connect(&fileTransferJob, &FileTransferJob::error, this, &UntrashJob::error, Qt::DirectConnection);
    connect(&fileTransferJob, &FileTransferJob::fileExists, this, &UntrashJob::fileExists, Qt::DirectConnection);

    // cancel the file transfer subjob if the parent job is cancelled
    connect(this, &UntrashJob::cancelled, &fileTransferJob,
            [&fileTransferJob]() {
                if(!fileTransferJob.isCancelled()) {
                    fileTransferJob.cancel();
                }
            }, Qt::DirectConnection);

    // cancel the parent job if the file transfer subjob is cancelled
    connect(&fileTransferJob, &FileTransferJob::cancelled, this,
            [this]() {
                if(!isCancelled()) {
                    cancel();
                }
            }, Qt::DirectConnection);
    fileTransferJob.run();
}

} // namespace Fm
