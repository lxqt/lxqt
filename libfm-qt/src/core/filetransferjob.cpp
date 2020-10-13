#include "filetransferjob.h"
#include "totalsizejob.h"
#include "fileinfo_p.h"

namespace Fm {

FileTransferJob::FileTransferJob(FilePathList srcPaths, Mode mode):
    FileOperationJob{},
    srcPaths_{std::move(srcPaths)},
    mode_{mode} {
}

FileTransferJob::FileTransferJob(FilePathList srcPaths, FilePathList destPaths, Mode mode):
    FileTransferJob{std::move(srcPaths), mode} {
    destPaths_ = std::move(destPaths);
}

FileTransferJob::FileTransferJob(FilePathList srcPaths, const FilePath& destDirPath, Mode mode):
    FileTransferJob{std::move(srcPaths), mode} {
    setDestDirPath(destDirPath);
}

void FileTransferJob::setSrcPaths(FilePathList srcPaths) {
    srcPaths_ = std::move(srcPaths);
}

void FileTransferJob::setDestPaths(FilePathList destPaths) {
    destPaths_ = std::move(destPaths);
}

void FileTransferJob::setDestDirPath(const FilePath& destDirPath) {
    destPaths_.clear();
    destPaths_.reserve(srcPaths_.size());
    for(const auto& srcPath: srcPaths_) {
        FilePath destPath;
        if(mode_ == Mode::LINK && !srcPath.isNative()) {
            // special handling for URLs
            auto fullBasename = srcPath.baseName();
            char* basename = fullBasename.get();
            char* dname = nullptr;
            // if we drop URI query onto native filesystem, omit query part
            if(!srcPath.isNative()) {
                dname = strchr(basename, '?');
            }
            // if basename consist only from query then use first part of it
            if(dname == basename) {
                basename++;
                dname = strchr(basename, '&');
            }

            CStrPtr _basename;
            if(dname) {
                _basename = CStrPtr{g_strndup(basename, dname - basename)};
                dname = strrchr(_basename.get(), G_DIR_SEPARATOR);
                g_debug("cutting '%s' to '%s'", basename, dname ? &dname[1] : _basename.get());
                if(dname) {
                    basename = &dname[1];
                }
                else {
                    basename = _basename.get();
                }
            }
            destPath = destDirPath.child(basename);
        }
        else {
            destPath = destDirPath.child(srcPath.baseName().get());
        }
        destPaths_.emplace_back(std::move(destPath));
    }
}

void FileTransferJob::gfileCopyProgressCallback(goffset current_num_bytes, goffset total_num_bytes, FileTransferJob* _this) {
    _this->setCurrentFileProgress(total_num_bytes, current_num_bytes);
}

bool FileTransferJob::moveFileSameFs(const FilePath& srcPath, const GFileInfoPtr& srcInfo, FilePath& destPath) {
    int flags = G_FILE_COPY_ALL_METADATA | G_FILE_COPY_NOFOLLOW_SYMLINKS;
    GErrorPtr err;
    bool retry;
    do {
        retry = false;
        err.reset();
        // do the file operation
        if(!g_file_move(srcPath.gfile().get(), destPath.gfile().get(), GFileCopyFlags(flags), cancellable().get(),
                       nullptr, this, &err)) {
            // Specially with mounts bound to /mnt, g_file_move() may give the recursive error
            // and fail, in which case, we ignore the error and try copying and deleting.
            if(err.code() == G_IO_ERROR_WOULD_RECURSE) {
              if(auto parent = destPath.parent()) {
                  return copyFile(srcPath, srcInfo, parent, destPath.baseName().get());
              }
            }
            retry = handleError(err, srcPath, srcInfo, destPath, flags);
        }
        else {
            return true;
        }
    } while(retry && !isCancelled());
    return false;
}

bool FileTransferJob::copyRegularFile(const FilePath& srcPath, const GFileInfoPtr& srcInfo, FilePath& destPath) {
    int flags = G_FILE_COPY_ALL_METADATA | G_FILE_COPY_NOFOLLOW_SYMLINKS;
    GErrorPtr err;
    bool retry;
    do {
        retry = false;
        err.reset();

        // reset progress of the current file (only for copy)
        auto size = g_file_info_get_size(srcInfo.get());
        setCurrentFileProgress(size, 0);

        // do the file operation
        if(!g_file_copy(srcPath.gfile().get(), destPath.gfile().get(), GFileCopyFlags(flags), cancellable().get(),
                       (GFileProgressCallback)&gfileCopyProgressCallback, this, &err)) {
            retry = handleError(err, srcPath, srcInfo, destPath, flags);
        }
        else {
            return true;
        }
    } while(retry && !isCancelled());
    return false;
}

bool FileTransferJob::copySpecialFile(const FilePath& srcPath, const GFileInfoPtr& srcInfo, FilePath &destPath) {
    bool ret = false;
    // only handle FIFO for local files
    if(srcPath.isNative() && destPath.isNative()) {
        auto src_path = srcPath.localPath();
        struct stat src_st;
        int r;
        r = lstat(src_path.get(), &src_st);
        if(r == 0) {
            // Handle FIFO on native file systems.
            if(S_ISFIFO(src_st.st_mode)) {
                auto dest_path = destPath.localPath();
                if(mkfifo(dest_path.get(), src_st.st_mode) == 0) {
                    ret = true;
                }
            }
            // FIXME: how about block device, char device, and socket?
        }
    }
    if(!ret) {
        GErrorPtr err;
        g_set_error(&err, G_IO_ERROR, G_IO_ERROR_FAILED,
                    ("Cannot copy file '%s': not supported"),
                    g_file_info_get_display_name(srcInfo.get()));
        emitError(err, ErrorSeverity::MODERATE);
    }
    return ret;
}

bool FileTransferJob::copyDirContent(const FilePath& srcPath, GFileInfoPtr /*srcInfo*/, FilePath& destPath, bool skip) {
    bool ret = false;
    // copy dir content
    GErrorPtr err;
    auto enu = GFileEnumeratorPtr{
            g_file_enumerate_children(srcPath.gfile().get(),
                                      defaultGFileInfoQueryAttribs,
                                      G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                      cancellable().get(), &err),
            false};
    if(enu) {
        int n_children = 0;
        int n_copied = 0;
        ret = true;
        while(!isCancelled()) {
            err.reset();
            GFileInfoPtr inf{g_file_enumerator_next_file(enu.get(), cancellable().get(), &err), false};
            if(inf) {
                ++n_children;
                const char* name = g_file_info_get_name(inf.get());
                FilePath childPath = srcPath.child(name);
                bool child_ret = copyFile(childPath, inf, destPath, name, skip);
                if(child_ret) {
                    ++n_copied;
                }
                else {
                    ret = false;
                }
            }
            else {
                if(err) {
                    // fail to read directory content
                    // NOTE: since we cannot read the source dir, we cannot calculate the progress correctly, either.
                    emitError(err, ErrorSeverity::MODERATE);
                    err.reset();
                    /* ErrorAction::RETRY is not supported here */
                    ret = false;
                }
                else { /* EOF is reached */
                    /* all files are successfully copied. */
                    if(isCancelled()) {
                        ret = false;
                    }
                    else {
                        /* some files are not copied */
                        if(n_children != n_copied) {
                            /* if the copy actions are skipped deliberately, it's ok */
                            if(!skip) {
                                ret = false;
                            }
                        }
                        /* else job->skip_dir_content is true */
                    }
                    break;
                }
            }
        }
        g_file_enumerator_close(enu.get(), nullptr, &err);
    }
    else {
        if(err) {
            emitError(err, ErrorSeverity::MODERATE);
        }
    }
    return ret;
}

bool FileTransferJob::makeDir(const FilePath& srcPath, GFileInfoPtr srcInfo, FilePath& destPath) {
    if(isCancelled()) {
        return false;
    }

    bool mkdir_done = false;
    do {
        GErrorPtr err;
        mkdir_done = g_file_make_directory_with_parents(destPath.gfile().get(), cancellable().get(), &err);
        if(!mkdir_done) {
            if(err->domain == G_IO_ERROR && (err->code == G_IO_ERROR_EXISTS ||
                                             err->code == G_IO_ERROR_INVALID_FILENAME ||
                                             err->code == G_IO_ERROR_FILENAME_TOO_LONG)) {
                GFileInfoPtr destInfo = GFileInfoPtr {
                    g_file_query_info(destPath.gfile().get(),
                    defaultGFileInfoQueryAttribs,
                    G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                    cancellable().get(), nullptr),
                    false
                };
                if(!destInfo) {
                    // FIXME: error handling
                    break;
                }

                FilePath newDestPath;
                FileExistsAction opt = askRename(FileInfo{srcInfo, srcPath}, FileInfo{destInfo, destPath}, newDestPath);
                switch(opt) {
                case FileOperationJob::RENAME:
                    destPath = std::move(newDestPath);
                    break;
                case FileOperationJob::SKIP:
                    /* when a dir is skipped, we need to know its total size to calculate correct progress */
                    mkdir_done = true; /* pretend that dir creation succeeded */
                    break;
                case FileOperationJob::OVERWRITE:
                    mkdir_done = true; /* pretend that dir creation succeeded */
                    break;
                case FileOperationJob::CANCEL:
                    cancel();
                    return false;
                case FileOperationJob::SKIP_ERROR: ; /* FIXME */
                }
            }
            else {
                ErrorAction act = emitError(err, ErrorSeverity::MODERATE);
                if(act != ErrorAction::RETRY) {
                    break;
                }
            }
        }
    } while(!mkdir_done && !isCancelled());

    bool chmod_done = false;
    if(mkdir_done && !isCancelled()) {
        mode_t mode = g_file_info_get_attribute_uint32(srcInfo.get(), G_FILE_ATTRIBUTE_UNIX_MODE);
        if(mode) {
            mode |= (S_IRUSR | S_IWUSR); /* ensure we have rw permission to this file. */
            do {
                GErrorPtr err;
                // chmod the newly created dir properly
                // if(!fm_job_is_cancelled(fmjob) && !job->skip_dir_content)
                chmod_done = g_file_set_attribute_uint32(destPath.gfile().get(),
                                                         G_FILE_ATTRIBUTE_UNIX_MODE,
                                                         mode, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                                         cancellable().get(), &err);
                if(!chmod_done) {
                    /* NOTE: Some filesystems may not support this. So, ignore errors for now. */
                    break;
                    /*ErrorAction act = emitError(err, ErrorSeverity::MODERATE);
                    if(act != ErrorAction::RETRY) {
                        break;
                    }*/
                }
            } while(!chmod_done && !isCancelled());
        }
    }
    return mkdir_done/* && chmod_done*/;
}

bool FileTransferJob::handleError(GErrorPtr &err, const FilePath &srcPath, const GFileInfoPtr &srcInfo, FilePath &destPath, int& flags) {
    bool retry = false;
    /* handle existing files or file name conflict */
    if(err.domain() == G_IO_ERROR && (err.code() == G_IO_ERROR_EXISTS ||
                                     err.code() == G_IO_ERROR_INVALID_FILENAME ||
                                     err.code() == G_IO_ERROR_FILENAME_TOO_LONG)) {
        flags &= ~G_FILE_COPY_OVERWRITE;

        // get info of the existing file
        GFileInfoPtr destInfo = GFileInfoPtr {
            g_file_query_info(destPath.gfile().get(),
            defaultGFileInfoQueryAttribs,
            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
            cancellable().get(), nullptr),
            false
        };

        // ask the user to rename or overwrite the existing file
        if(!isCancelled() && destInfo) {
            FilePath newDestPath;
            FileExistsAction opt = askRename(FileInfo{srcInfo, srcPath},
                                             FileInfo{destInfo, destPath},
                                             newDestPath);
            switch(opt) {
            case FileOperationJob::RENAME:
                // try a new file name
                if(newDestPath.isValid()) {
                    destPath = std::move(newDestPath);
                    // FIXME: handle the error when newDestPath is invalid.
                }
                retry = true;
                break;
            case FileOperationJob::OVERWRITE:
                // overwrite existing file
                flags |= G_FILE_COPY_OVERWRITE;
                retry = true;
                err.reset();
                break;
            case FileOperationJob::CANCEL:
                // cancel the whole job.
                cancel();
                break;
            case FileOperationJob::SKIP:
                // skip current file and don't copy it
            case FileOperationJob::SKIP_ERROR: ; /* FIXME */
                retry = false;
                break;
            }
            err.reset();
        }
    }

    // show error message
    if(!isCancelled() && err) {
        ErrorAction act = emitError(err, ErrorSeverity::MODERATE);
        err.reset();
        if(act == ErrorAction::RETRY) {
            // the user wants retry the operation again
            retry = true;
        }
        const bool is_no_space = (err.domain() == G_IO_ERROR && err.code() == G_IO_ERROR_NO_SPACE);
        /* FIXME: ask to leave partial content? */
        if(is_no_space) {
            // run out of disk space. delete the partial content we copied.
            g_file_delete(destPath.gfile().get(), cancellable().get(), nullptr);
        }
    }
    return retry;
}

bool FileTransferJob::processPath(const FilePath& srcPath, const FilePath& destDirPath, const char* destFileName) {
    GErrorPtr err;
    GFileInfoPtr srcInfo = GFileInfoPtr {
        g_file_query_info(srcPath.gfile().get(),
        defaultGFileInfoQueryAttribs,
        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
        cancellable().get(), &err),
        false
    };
    if(!srcInfo || isCancelled()) {
        // FIXME: report error
        return false;
    }

    // Use GIO's copy name for destination if existing. This is especially good for copying
    // from another file system or from places like recent:/// and also handles encoding.
    const char* destCopyName = g_file_info_get_attribute_string(srcInfo.get(), "standard::copy-name");

    bool ret;
    switch(mode_) {
    case Mode::MOVE:
        ret = moveFile(srcPath, srcInfo, destDirPath, destCopyName ? destCopyName : destFileName);
        break;
    case Mode::COPY: {
        bool deleteSrc = false;
        ret = copyFile(srcPath, srcInfo, destDirPath, destCopyName ? destCopyName : destFileName, deleteSrc);
        break;
    }
    case Mode::LINK:
        ret = linkFile(srcPath, srcInfo, destDirPath,
                        // see setDestDirPath()
                        srcPath.isNative() && destCopyName ? destCopyName : destFileName);
        break;
    default:
        ret = false;
        break;
    }
    return ret;
}

bool FileTransferJob::moveFile(const FilePath &srcPath, const GFileInfoPtr &srcInfo, const FilePath &destDirPath, const char *destFileName) {
    setCurrentFile(srcPath);

    GErrorPtr err;
    GFileInfoPtr destDirInfo = GFileInfoPtr {
        g_file_query_info(destDirPath.gfile().get(),
        "id::filesystem",
        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
        cancellable().get(), &err),
        false
    };

    if(!destDirInfo || isCancelled()) {
        // FIXME: report errors
        return false;
    }

    // If src and dest are on the same filesystem, do move.
    // Exception: if src FS is trash:///, we always do move
    // Otherwise, do copy & delete src files.
    auto src_fs = g_file_info_get_attribute_string(srcInfo.get(), "id::filesystem");
    auto dest_fs = g_file_info_get_attribute_string(destDirInfo.get(), "id::filesystem");
    bool ret;
    if(src_fs && dest_fs && (strcmp(src_fs, dest_fs) == 0 || g_str_has_prefix(src_fs, "trash"))) {
        // src and dest are on the same filesystem
        auto destPath = destDirPath.child(destFileName);
        ret = moveFileSameFs(srcPath, srcInfo, destPath);

        // increase current progress
        // FIXME: it's not appropriate to calculate the progress of move operations using file size
        // since the time required to move a file is not related to it's file size.
        auto size = g_file_info_get_size(srcInfo.get());
        addFinishedAmount(size, 1);
    }
    else {
        // cross device/filesystem move: copy & delete
        ret = copyFile(srcPath, srcInfo, destDirPath, destFileName);
        // NOTE: do not need to increase progress here since it's done by copyPath().
    }
    return ret;
}

bool FileTransferJob::copyFile(const FilePath& srcPath, const GFileInfoPtr& srcInfo, const FilePath& destDirPath, const char* destFileName, bool skip) {
    setCurrentFile(srcPath);

    auto size = g_file_info_get_size(srcInfo.get());
    bool success = false;
    setCurrentFileProgress(size, 0);

    auto destPath = destDirPath.child(destFileName);
    auto file_type = g_file_info_get_file_type(srcInfo.get());
    if(!skip) {
        switch(file_type) {
        case G_FILE_TYPE_DIRECTORY:
            // prevent a dir from being copied into itself
            if(srcPath.isPrefixOf(destPath)) {
                GErrorPtr err = GErrorPtr{
                                G_IO_ERROR,
                                G_IO_ERROR_NOT_SUPPORTED,
                                tr("Cannot copy a directory into itself!")
                };
                emitError(err, ErrorSeverity::MODERATE);
            }
            else {
                success = makeDir(srcPath, srcInfo, destPath);
            }
            break;
        case G_FILE_TYPE_SPECIAL:
            success = copySpecialFile(srcPath, srcInfo, destPath);
            break;
        default:
            success = copyRegularFile(srcPath, srcInfo, destPath);
            break;
        }
    }
    else { // skip the file
        success = true;
    }

    if(success) {
        // finish copying the file
        addFinishedAmount(size, 1);
        setCurrentFileProgress(0, 0);

        // recursively copy dir content
        if(file_type == G_FILE_TYPE_DIRECTORY) {
            success = copyDirContent(srcPath, srcInfo, destPath, skip);
        }

        if(!skip && success && mode_ == Mode::MOVE) {
            // delete the source file for cross-filesystem move
            GErrorPtr err;
            if(g_file_delete(srcPath.gfile().get(), cancellable().get(), &err)) {
                // FIXME: add some file size to represent the amount of work need to delete a file
                addFinishedAmount(1, 1);
            }
            else {
                success = false;
            }
        }
    }
    return success;
}

bool FileTransferJob::linkFile(const FilePath &srcPath, const GFileInfoPtr &srcInfo, const FilePath &destDirPath, const char *destFileName) {
    setCurrentFile(srcPath);

    bool ret = false;
    // cannot create links on non-native filesystems
    if(!destDirPath.isNative()) {
        auto msg = tr("Cannot create a link on non-native filesystem");
        GErrorPtr err{g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED, msg.toUtf8().constData())};
        emitError(err, ErrorSeverity::CRITICAL);
        return false;
    }

    if(srcPath.isNative()) {
        // create symlinks for native files
        auto destPath = destDirPath.child(destFileName);
        ret = createSymlink(srcPath, srcInfo, destPath);
    }
    else {
        // ensure that the dest file has *.desktop filename extension.
        CStrPtr desktopEntryFileName{g_strconcat(destFileName, ".desktop", nullptr)};
        auto destPath = destDirPath.child(desktopEntryFileName.get());
        ret = createShortcut(srcPath, srcInfo, destPath);
    }

    // update progress
    // FIXME: increase the progress by 1 byte is not appropriate
    addFinishedAmount(1, 1);
    return ret;
}

bool FileTransferJob::createSymlink(const FilePath &srcPath, const GFileInfoPtr &srcInfo, FilePath &destPath) {
    bool ret = false;
    auto src = srcPath.localPath();
    int flags = 0;
    GErrorPtr err;
    bool retry;
    do {
        retry = false;
        if(flags & G_FILE_COPY_OVERWRITE) {  // overwrite existing file
            // creating symlink cannot overwrite existing files directly, so we delete the existing file first.
            g_file_delete(destPath.gfile().get(), cancellable().get(), nullptr);
        }
        if(!g_file_make_symbolic_link(destPath.gfile().get(), src.get(), cancellable().get(), &err)) {
            retry = handleError(err, srcPath, srcInfo, destPath, flags);
        }
        else {
            ret = true;
            break;
        }
    } while(!isCancelled() && retry);
    return ret;
}

bool FileTransferJob::createShortcut(const FilePath &srcPath, const GFileInfoPtr &srcInfo, FilePath &destPath) {
    bool ret = false;
    const char* iconName = nullptr;
    GIcon* icon = g_file_info_get_icon(srcInfo.get());
    if(icon && G_IS_THEMED_ICON(icon)) {
        auto iconNames = g_themed_icon_get_names(G_THEMED_ICON(icon));
        if(iconNames && iconNames[0]) {
            iconName = iconNames[0];
        }
    }

    CStrPtr srcPathUri;
    auto uri = g_file_info_get_attribute_string(srcInfo.get(), G_FILE_ATTRIBUTE_STANDARD_TARGET_URI);
    if(!uri) {
        srcPathUri = srcPath.uri();
        uri = srcPathUri.get();
    }

    CStrPtr srcPathDispName;
    auto name = g_file_info_get_display_name(srcInfo.get());
    if(!name) {
        srcPathDispName = srcPath.displayName();
        name = srcPathDispName.get();
    }

    GKeyFile* kf = g_key_file_new();
    if(kf) {
        g_key_file_set_string(kf, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_TYPE, "Link");
        g_key_file_set_string(kf, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, name);
        if(iconName) {
            g_key_file_set_string(kf, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_ICON, iconName);
        }
        if(uri) {
            g_key_file_set_string(kf, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_URL, uri);
        }
        gsize contentLen;
        CStrPtr content{g_key_file_to_data(kf, &contentLen, nullptr)};
        g_key_file_free(kf);

        int flags = 0;
        if(content) {
            bool retry;
            GErrorPtr err;
            do {
                retry = false;
                if(flags & G_FILE_COPY_OVERWRITE) {  // overwrite existing file
                    g_file_delete(destPath.gfile().get(), cancellable().get(), nullptr);
                }

                if(!g_file_replace_contents(destPath.gfile().get(), content.get(), contentLen, nullptr, false, G_FILE_CREATE_NONE, nullptr, cancellable().get(), &err)) {
                    retry = handleError(err, srcPath, srcInfo, destPath, flags);
                    err.reset();
                }
                else {
                    ret = true;
                }
            } while(!isCancelled() && retry);
            ret = true;
        }
    }
    return ret;
}


void FileTransferJob::exec() {
    // calculate the total size of files to copy
    auto totalSizeFlags = (mode_ == Mode::COPY ? TotalSizeJob::DEFAULT : TotalSizeJob::PREPARE_MOVE);
    TotalSizeJob totalSizeJob{srcPaths_, totalSizeFlags};
    connect(&totalSizeJob, &TotalSizeJob::error, this, &FileTransferJob::error);
    connect(this, &FileTransferJob::cancelled, &totalSizeJob, &TotalSizeJob::cancel);
    totalSizeJob.run();
    if(isCancelled()) {
        return;
    }

    // ready to start
    setTotalAmount(totalSizeJob.totalSize(), totalSizeJob.fileCount());
    Q_EMIT preparedToRun();

    if(srcPaths_.size() != destPaths_.size()) {
        qWarning("error: srcPaths.size() != destPaths.size() when copying files");
        return;
    }

    // copy the files
    for(size_t i = 0; i < srcPaths_.size(); ++i) {
        if(isCancelled()) {
            break;
        }
        const auto& srcPath = srcPaths_[i];
        const auto& destPath = destPaths_[i];
        auto destDirPath = destPath.parent();
        processPath(srcPath, destDirPath, destPath.baseName().get());
    }
}


} // namespace Fm
