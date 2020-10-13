#include "totalsizejob.h"

namespace Fm {

static const char query_str[] =
    G_FILE_ATTRIBUTE_STANDARD_TYPE","
    G_FILE_ATTRIBUTE_STANDARD_NAME","
    G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL","
    G_FILE_ATTRIBUTE_STANDARD_SIZE","
    G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE","
    G_FILE_ATTRIBUTE_ID_FILESYSTEM;


TotalSizeJob::TotalSizeJob(FilePathList paths, Flags flags):
    paths_{std::move(paths)},
    flags_{flags},
    totalSize_{0},
    totalOndiskSize_{0},
    fileCount_{0},
    dest_fs_id{nullptr} {
}


void TotalSizeJob::exec(FilePath path, GFileInfoPtr inf) {
    GFileType type;
    const char* fs_id;
    bool descend;

_retry_query_info:
    if(!inf) {
        GErrorPtr err;
        inf = GFileInfoPtr {
            g_file_query_info(path.gfile().get(), query_str,
            (flags_ & FOLLOW_LINKS) ? G_FILE_QUERY_INFO_NONE : G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
            cancellable().get(), &err),
            false
        };
        if(!inf) {
            ErrorAction act = emitError( err, ErrorSeverity::MILD);
            err = nullptr;
            if(act == ErrorAction::RETRY) {
                goto _retry_query_info;
            }
            return;
        }
    }
    if(isCancelled()) {
        return;
    }

    type = g_file_info_get_file_type(inf.get());
    descend = true;

    ++fileCount_;
    /* SF bug #892: dir file size is not relevant in the summary */
    if(type != G_FILE_TYPE_DIRECTORY) {
        totalSize_ += g_file_info_get_size(inf.get());
    }
    totalOndiskSize_ += g_file_info_get_attribute_uint64(inf.get(), G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE);

    /* prepare for moving across different devices */
    if(flags_ & PREPARE_MOVE) {
        fs_id = g_file_info_get_attribute_string(inf.get(), G_FILE_ATTRIBUTE_ID_FILESYSTEM);
        if(fs_id && dest_fs_id && (strcmp(fs_id, dest_fs_id) == 0 || g_str_has_prefix(fs_id, "trash"))) {
            // same filesystem or move from trash:///
            descend = false;
        }
        else {
            /* files on different device requires an additional 'delete' for the source file. */
            ++totalSize_; /* this is for the additional delete */
            ++totalOndiskSize_;
            ++fileCount_;
            descend = true;
        }
    }

    if(type == G_FILE_TYPE_DIRECTORY) {
        /* check if we need to decends into the dir. */
        /* trash:/// doesn't support deleting files recursively (but we want to descend into trash root "trash:///" */
        if(flags_ & PREPARE_DELETE && path.hasUriScheme("trash") && path.baseName()[0] != '/') {
            descend = false;
        }
        else {
            /* only descends into files on the same filesystem */
            if(flags_ & SAME_FS) {
                fs_id = g_file_info_get_attribute_string(inf.get(), G_FILE_ATTRIBUTE_ID_FILESYSTEM);
                descend = (g_strcmp0(fs_id, dest_fs_id) == 0);
            }
        }

        inf = nullptr;
        if(descend) {
_retry_enum_children:
            GErrorPtr err;
            auto enu = GFileEnumeratorPtr {
                g_file_enumerate_children(path.gfile().get(), query_str,
                G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                cancellable().get(), &err),
                false
            };
            if(enu) {
                while(!isCancelled()) {
                    inf = GFileInfoPtr{g_file_enumerator_next_file(enu.get(), cancellable().get(), &err), false};
                    if(inf) {
                        FilePath child = path.child(g_file_info_get_name(inf.get()));
                        exec(std::move(child), std::move(inf));
                    }
                    else {
                        if(err) { /* error! */
                            /* ErrorAction::RETRY is not supported */
                            emitError( err, ErrorSeverity::MILD);
                            err = nullptr;
                        }
                        else {
                            /* EOF is reached, do nothing. */
                            break;
                        }
                    }
                }
                g_file_enumerator_close(enu.get(), nullptr, nullptr);
            }
            else {
                ErrorAction act = emitError( err, ErrorSeverity::MILD);
                err = nullptr;
                if(act == ErrorAction::RETRY) {
                    goto _retry_enum_children;
                }
            }
        }
    }
}


void TotalSizeJob::exec() {
    for(auto& path : paths_) {
        exec(path, GFileInfoPtr{});
    }
}


} // namespace Fm
