#include "trashjob.h"

#include "core/legacy/fm-config.h"

namespace Fm {

TrashJob::TrashJob(FilePathList paths): paths_{std::move(paths)} {
    // calculate progress using finished file counts rather than their sizes
    setCalcProgressUsingSize(false);
}

void TrashJob::exec() {
    setTotalAmount(paths_.size(), paths_.size());
    Q_EMIT preparedToRun();

    /* FIXME: we shouldn't trash a file already in trash:/// */
    for(auto& path : paths_) {
        if(isCancelled()) {
            break;
        }

        setCurrentFile(path);

        // TODO: get parent dir of the current path.
        //       if there is a Fm::Folder object created for it, block the update for the folder temporarily.

        for(;;) {  // retry the i/o operation on errors
            auto gf = path.gfile();
            bool ret = false;
            // FIXME: do not depend on fm_config
            if(fm_config->no_usb_trash) {
                GMountPtr mnt{g_file_find_enclosing_mount(gf.get(), nullptr, nullptr), false};
                if(mnt) {
                    ret = g_mount_can_unmount(mnt.get()); /* TRUE if it's removable media */
                    if(ret) {
                        unsupportedFiles_.push_back(path);
                        break;  // don't trash the file
                    }
                }
            }

            // move the file to trash
            GErrorPtr err;
            ret = g_file_trash(gf.get(), cancellable().get(), &err);
            if(ret) {  // trash operation succeeded
                break;
            }
            else {  // failed
                // if trashing is not supported by the file system
                if(err.domain() == G_IO_ERROR && err.code() == G_IO_ERROR_NOT_SUPPORTED) {
                    unsupportedFiles_.push_back(path);
                     break;
                }
                else {
                    ErrorAction act = emitError(err, ErrorSeverity::MODERATE);
                    if(act == ErrorAction::RETRY) {
                        err.reset();
                    }
                    else if(act == ErrorAction::ABORT) {
                        cancel();
                        return;
                    }
                    else {
                        break;
                    }
                }
            }
        }
        addFinishedAmount(1, 1);
    }
}


} // namespace Fm
