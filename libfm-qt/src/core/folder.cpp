/*
 *      fm-folder.c
 *
 *      Copyright 2009 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2012-2016 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#include "folder.h"
#include <cstring>
#include <cassert>
#include <QTimer>
#include <QDebug>

#include "dirlistjob.h"
#include "filesysteminfojob.h"
#include "fileinfojob.h"

namespace Fm {

std::unordered_map<FilePath, std::weak_ptr<Folder>, FilePathHash> Folder::cache_;
std::mutex Folder::mutex_;

Folder::Folder():
    dirlist_job{nullptr},
    fsInfoJob_{nullptr},
    volumeManager_{VolumeManager::globalInstance()},
    /* for file monitor */
    has_idle_reload_handler{false},
    has_idle_update_handler{false},
    pending_change_notify{false},
    filesystem_info_pending{false},
    wants_incremental{false},
    stop_emission{false}, /* don't set it 1 bit to not lock other bits */
    /* filesystem info - set in query thread, read in main */
    fs_total_size{0},
    fs_free_size{0},
    has_fs_info{false},
    defer_content_test{false} {

    connect(volumeManager_.get(), &VolumeManager::mountAdded, this, &Folder::onMountAdded);
    connect(volumeManager_.get(), &VolumeManager::mountRemoved, this, &Folder::onMountRemoved);
}

Folder::Folder(const FilePath& path): Folder() {
    dirPath_ = path;
}

Folder::~Folder() {
    if(dirMonitor_) {
        g_signal_handlers_disconnect_by_data(dirMonitor_.get(), this);
        dirMonitor_.reset();
    }

    if(dirlist_job) {
        dirlist_job->cancel();
    }

    // cancel any file info job in progress.
    for(auto job: fileinfoJobs_) {
        job->cancel();
    }

    if(fsInfoJob_) {
        fsInfoJob_->cancel();
    }

    // We store a weak_ptr instead of shared_ptr in the hash table, so the hash table
    // does not own a reference to the folder. When the last reference to Folder is
    // freed, we need to remove its hash table entry.
    std::lock_guard<std::mutex> lock{mutex_};
    auto it = cache_.find(dirPath_);
    if(it != cache_.end()) {
        cache_.erase(it);
    }
}

// static
std::shared_ptr<Folder> Folder::fromPath(const FilePath& path) {
    std::lock_guard<std::mutex> lock{mutex_};
    auto it = cache_.find(path);
    if(it != cache_.end()) {
        auto folder = it->second.lock();
        if(folder) {
            return folder;
        }
        else { // FIXME: is this possible?
            cache_.erase(it);
        }
    }
    auto folder = std::make_shared<Folder>(path);
    folder->reload();
    cache_.emplace(path, folder);
    return folder;
}

// static
// Checks if this is the path of a folder in use.
std::shared_ptr<Folder> Folder::findByPath(const FilePath& path) {
    std::lock_guard<std::mutex> lock{mutex_};
    auto it = cache_.find(path);
    if(it != cache_.end()) {
        auto folder = it->second.lock();
        if(folder) {
            return folder;
        }
    }
    return nullptr;
}

bool Folder::makeDirectory(const char* /*name*/, GError** /*error*/) {
    // TODO:
    // FIXME: what the API is used for in the original libfm C API?
    return false;
}

bool Folder::isIncremental() const {
    return wants_incremental;
}

bool Folder::isValid() const {
    return dirInfo_ != nullptr;
}

bool Folder::isLoaded() const {
    return (dirlist_job == nullptr);
}

std::shared_ptr<const FileInfo> Folder::fileByName(const char* name) const {
    auto it = files_.find(name);
    if(it != files_.end()) {
        return it->second;
    }
    return nullptr;
}

bool Folder::isEmpty() const {
    return files_.empty();
}

bool Folder::hasFileMonitor() const {
    return (dirMonitor_ != nullptr);
}

FileInfoList Folder::files() const {
    FileInfoList ret;
    ret.reserve(files_.size());
    for(const auto& item : files_) {
        ret.push_back(item.second);
    }
    return ret;
}


const FilePath& Folder::path() const {
    auto pathStr = dirPath_.toString();
    // qDebug() << this << "FOLDER_PATH:" << pathStr.get() << dirPath_.gfile().get();
    //assert(!g_str_has_prefix(pathStr.get(), "file:"));
    return dirPath_;
}

const std::shared_ptr<const FileInfo>& Folder::info() const {
    return dirInfo_;
}

#if 0
void Folder::init(FmFolder* folder) {
    files = fm_file_info_list_new();
    G_LOCK(hash);
    if(G_UNLIKELY(hash_uses == 0)) {
        hash = g_hash_table_new((GHashFunc)fm_path_hash, (GEqualFunc)fm_path_equal);
        volume_monitor = g_volume_monitor_get();
        if(G_LIKELY(volume_monitor)) {
            g_signal_connect(volume_monitor, "mount-added", G_CALLBACK(on_mount_added), nullptr);
            g_signal_connect(volume_monitor, "mount-removed", G_CALLBACK(on_mount_removed), nullptr);
        }
    }
    hash_uses++;
    G_UNLOCK(hash);
}
#endif

void Folder::onIdleReload() {
    /* check if folder still exists */
    reload();
    // G_LOCK(query);
    has_idle_reload_handler = false;
    // G_UNLOCK(query);
}

void Folder::queueReload() {
    // G_LOCK(query);
    if(!has_idle_reload_handler) {
        has_idle_reload_handler = true;
        QTimer::singleShot(0, this, &Folder::onIdleReload);
    }
    // G_UNLOCK(query);
}

void Folder::onFileInfoFinished() {
    FileInfoJob* job = static_cast<FileInfoJob*>(sender());
    fileinfoJobs_.erase(std::find(fileinfoJobs_.cbegin(), fileinfoJobs_.cend(), job));

    /* NOTE: The pending changes should be processed in the order reported by GIO;
       otherwise; the final file info might be wrong. Here, we ensure that the next
       pending changes are processed only after the current info job is finished. */

    if(job->isCancelled()) {
        has_idle_update_handler = false; // allow future updates
        return;
    }

    FileInfoList files_to_add;
    FileInfoList files_to_delete;
    std::vector<FileInfoPair> files_to_update;

    const auto& paths = job->paths();
    const auto& infos = job->files();
    auto path_it = paths.cbegin();
    auto info_it = infos.cbegin();
    for(; path_it != paths.cend() && info_it != infos.cend(); ++path_it, ++info_it) {
        const auto& path = *path_it;
        const auto& info = *info_it;

        if(path == dirPath_) { // got the info for the folder itself.
            dirInfo_ = info;
        }
        else {
            auto it = files_.find(info->path().baseName().get());
            if(it != files_.end()) { // the file already exists, update
                files_to_update.push_back(std::make_pair(it->second, info));
            }
            else { // newly added
                files_to_add.push_back(info);
            }
            files_[info->path().baseName().get()] = info;
        }
    }
    if(!files_to_add.empty()) {
        Q_EMIT filesAdded(files_to_add);
    }
    if(!files_to_update.empty()) {
        Q_EMIT filesChanged(files_to_update);
    }
    Q_EMIT contentChanged();

    // process the changes accumulated during this info job
    if(filesystem_info_pending // means a pending change; see "onFileSystemInfoFinished()"
       || !paths_to_update.empty() || !paths_to_add.empty() || !paths_to_del.empty()) {
        QTimer::singleShot(0, this, &Folder::processPendingChanges);
    }
    // there's no pending change at the moment; let the next one be processed
    else {
        has_idle_update_handler = false;
    }
}

void Folder::processPendingChanges() {
    // FmFileInfoJob* job = nullptr;
    std::lock_guard<std::mutex> lock{mutex_};

    // idle_handler = 0;
    /* if we were asked to block updates let delay it for now */
    if(stop_emission) {
        has_idle_update_handler = false;
        return;
    }

    FileInfoJob* info_job = nullptr;
    if(!paths_to_update.empty() || !paths_to_add.empty()) {
        FilePathList paths;
        paths.insert(paths.end(), paths_to_add.cbegin(), paths_to_add.cend());
        paths.insert(paths.end(), paths_to_update.cbegin(), paths_to_update.cend());
        info_job = new FileInfoJob{paths};
        paths_to_update.clear();
        paths_to_add.clear();
    }
    else {
        // let the next pending changes be processed; see "onFileInfoFinished()"
        has_idle_update_handler = false;
    }

    if(info_job) {
        fileinfoJobs_.push_back(info_job);
        info_job->setAutoDelete(true);
        connect(info_job, &FileInfoJob::finished, this, &Folder::onFileInfoFinished, Qt::BlockingQueuedConnection);
        info_job->runAsync();
#if 0
        pending_jobs = g_slist_prepend(pending_jobs, job);
        if(!fm_job_run_async(FM_JOB(job))) {
            pending_jobs = g_slist_remove(pending_jobs, job);
            g_object_unref(job);
            g_critical("failed to start folder update job");
        }
#endif
    }

    // process deletion
    FileInfoList deleted_files;
    auto path_it = paths_to_del.begin();
    while(path_it != paths_to_del.end()) {
        const auto& path = *path_it;
        auto name = path.baseName();
        auto it = files_.find(name.get());
        if(it != files_.end()) {
            deleted_files.push_back(it->second);
            files_.erase(it);
            path_it = paths_to_del.erase(path_it);
        }
        else {
            ++path_it;
        }
    }
    if(!deleted_files.empty()) {
        Q_EMIT filesRemoved(deleted_files);
        Q_EMIT contentChanged();
    }

    if(pending_change_notify) {
        Q_EMIT changed();
        /* update volume info */
        queryFilesystemInfo();
        pending_change_notify = false;
    }

    if(filesystem_info_pending) {
        Q_EMIT fileSystemChanged();
        filesystem_info_pending = false;
    }
}

/* should be called only with G_LOCK(lists) on! */
void Folder::queueUpdate() {
    // qDebug() << "queue_update:" << !has_idle_handler << paths_to_add.size() << paths_to_update.size() << paths_to_del.size();
    if(!has_idle_update_handler) {
        QTimer::singleShot(0, this, &Folder::processPendingChanges);
        has_idle_update_handler = true;
    }
}


/* NOTE: When queuing files for addition/update/deletion in the following functions,
   the currently detected files (namely, "files_") should not be taken into account
   because they might be changed soon due to a previous call to queueUpdate(). */

/* returns true if reference was taken from path */
bool Folder::eventFileAdded(const FilePath &path) {
    bool added = true;
    // G_LOCK(lists);
    if(std::find(paths_to_del.cbegin(), paths_to_del.cend(), path) != paths_to_del.cend()) {
        // if the file was going to be deleted, its addition means an update,
        // so remove it from the deletion queue and add it to the update queue
        paths_to_del.erase(std::remove(paths_to_del.begin(), paths_to_del.end(), path), paths_to_del.cend());
        if(std::find(paths_to_update.cbegin(), paths_to_update.cend(), path) == paths_to_update.cend()) {
            paths_to_update.push_back(path);
        }
    }
    else if(std::find(paths_to_add.cbegin(), paths_to_add.cend(), path) == paths_to_add.cend()) {
        paths_to_add.push_back(path);
    }
    else { // file already queued for adding, don't duplicate
        added = false;
    }

    if(added) {
        queueUpdate();
    }
    // G_UNLOCK(lists);
    return added;
}

bool Folder::eventFileChanged(const FilePath &path) {
    bool added;
    // G_LOCK(lists);
    if(std::find(paths_to_update.cbegin(), paths_to_update.cend(), path) == paths_to_update.cend()
       && std::find(paths_to_add.cbegin(), paths_to_add.cend(), path) == paths_to_add.cend()) {
        paths_to_update.push_back(path);
        added = true;
        queueUpdate();
    }
    else {
        added = false;
    }
    // G_UNLOCK(lists);
    return added;
}

void Folder::eventFileDeleted(const FilePath& path) {
    bool deleted = true;
    // qDebug() << "delete " << path.baseName().get();
    // G_LOCK(lists);
    /* WARNING: If the file is in the addition queue, we shouldn not remove it from that queue
       and ignore its deletion because it may have been added by the directory list job, in
       which case, ignoring an addition-deletion sequence would result in a nonexistent file. */
    if(std::find(paths_to_del.cbegin(), paths_to_del.cend(), path) == paths_to_del.cend()) {
        paths_to_del.push_back(path);
        // the update queue can be cancelled for a file that is going to be deleted
        paths_to_update.erase(std::remove(paths_to_update.begin(), paths_to_update.end(), path), paths_to_update.cend());
    }
    else {
        deleted = false;
    }

    if(deleted) {
        queueUpdate();
    }
    // G_UNLOCK(lists);
}


void Folder::onDirChanged(GFileMonitorEvent evt) {
    switch(evt) {
    case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
        /* g_debug("folder is going to be unmounted"); */
        break;
    case G_FILE_MONITOR_EVENT_UNMOUNTED:
        Q_EMIT unmount();
        /* g_debug("folder is unmounted"); */
        queueReload();
        break;
    case G_FILE_MONITOR_EVENT_DELETED:
        Q_EMIT removed();
        /* g_debug("folder is deleted"); */
        break;
    case G_FILE_MONITOR_EVENT_CREATED:
        queueReload();
        break;
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
    case G_FILE_MONITOR_EVENT_CHANGED: {
        std::lock_guard<std::mutex> lock{mutex_};
        pending_change_notify = true;
        if(std::find(paths_to_update.cbegin(), paths_to_update.cend(), dirPath_) != paths_to_update.cend()) {
            paths_to_update.push_back(dirPath_);
            queueUpdate();
        }
        /* g_debug("folder is changed"); */
        break;
    }
#if GLIB_CHECK_VERSION(2,24,0)
    case G_FILE_MONITOR_EVENT_MOVED:
#endif
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
        ;
    default:
        break;
    }
}

void Folder::onFileChangeEvents(GFileMonitor* /*monitor*/, GFile* gf, GFile* /*other_file*/, GFileMonitorEvent evt) {
    /* const char* names[]={
        "G_FILE_MONITOR_EVENT_CHANGED",
        "G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT",
        "G_FILE_MONITOR_EVENT_DELETED",
        "G_FILE_MONITOR_EVENT_CREATED",
        "G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED",
        "G_FILE_MONITOR_EVENT_PRE_UNMOUNT",
        "G_FILE_MONITOR_EVENT_UNMOUNTED"
    }; */
    if(dirPath_ == gf) {
        onDirChanged(evt);
        return;
    }
    else {
        std::lock_guard<std::mutex> lock{mutex_};
        auto path = FilePath{gf, true};
        /* NOTE: sometimes, for unknown reasons, GFileMonitor gives us the
         * same event of the same file for multiple times. So we need to
         * check for duplications ourselves here. */
        switch(evt) {
        case G_FILE_MONITOR_EVENT_CREATED:
            eventFileAdded(path);
            break;
        case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
        case G_FILE_MONITOR_EVENT_CHANGED:
            eventFileChanged(path);
            break;
        case G_FILE_MONITOR_EVENT_DELETED:
            eventFileDeleted(path);
            break;
        default:
            break;
        }
    }
}

void Folder::onDirListFinished() {
    DirListJob* job = static_cast<DirListJob*>(sender());
    if(job->isCancelled()) { // this is a cancelled job, ignore!
        if(job == dirlist_job) {
            dirlist_job = nullptr;
            Q_EMIT finishLoading(); // this was the last job until now
        }
        return;
    }
    dirInfo_ = job->dirInfo();

    FileInfoList files_to_add;
    std::vector<FileInfoPair> files_to_update;
    const auto& infos = job->files();

    // with "search://", there is no update for infos and all of them should be added
    if(strcmp(dirPath_.uriScheme().get(), "search") == 0) {
        files_to_add = infos;
        for(auto& file: files_to_add) {
            files_[file->path().baseName().get()] = file;
        }
    }
    else {
        auto info_it = infos.cbegin();
        for(; info_it != infos.cend(); ++info_it) {
            const auto& info = *info_it;
            auto it = files_.find(info->path().baseName().get());
            if(it != files_.end()) {
                files_to_update.push_back(std::make_pair(it->second, info));
            }
            else {
                files_to_add.push_back(info);
            }
            files_[info->path().baseName().get()] = info;
        }
    }

    if(!files_to_add.empty()) {
        Q_EMIT filesAdded(files_to_add);
    }
    if(!files_to_update.empty()) {
        Q_EMIT filesChanged(files_to_update);
    }

#if 0
    if(dirlist_job->isCancelled() && !wants_incremental) {
        GList* l;
        for(l = fm_file_info_list_peek_head_link(job->files); l; l = l->next) {
            FmFileInfo* inf = (FmFileInfo*)l->data;
            files = g_slist_prepend(files, inf);
            fm_file_info_list_push_tail(files, inf);
        }
        if(G_LIKELY(files)) {
            GSList* l;

            G_LOCK(lists);
            if(defer_content_test && fm_path_is_native(dir_path))
                /* we got only basic info on content, schedule update it now */
                for(l = files; l; l = l->next)
                    files_to_update = g_slist_prepend(files_to_update,
                                              fm_path_ref(fm_file_info_get_path(l->data)));
            G_UNLOCK(lists);
            g_signal_emit(folder, signals[FILES_ADDED], 0, files);
            g_slist_free(files);
        }

        if(job->dir_fi) {
            dir_fi = fm_file_info_ref(job->dir_fi);
        }

        /* Some new files are created while FmDirListJob is loading the folder. */
        G_LOCK(lists);
        if(G_UNLIKELY(files_to_add)) {
            /* This should be a very rare case. Could this happen? */
            GSList* l;
            for(l = files_to_add; l;) {
                FmPath* path = l->data;
                GSList* next = l->next;
                if(_Folder::get_file_by_path(folder, path)) {
                    /* we already have the file. remove it from files_to_add,
                     * and put it in files_to_update instead.
                     * No strdup for name is needed here. We steal
                     * the string from files_to_add.*/
                    files_to_update = g_slist_prepend(files_to_update, path);
                    files_to_add = g_slist_delete_link(files_to_add, l);
                }
                l = next;
            }
        }
        G_UNLOCK(lists);
    }
    else if(!dir_fi && job->dir_fi)
        /* we may need dir_fi for incremental folders too */
    {
        dir_fi = fm_file_info_ref(job->dir_fi);
    }
    g_object_unref(dirlist_job);
#endif

    dirlist_job = nullptr;
    Q_EMIT finishLoading();
}

#if 0


void on_dirlist_job_files_found(FmDirListJob* job, GSList* files, gpointer user_data) {
    FmFolder* folder = FM_FOLDER(user_data);
    GSList* l;
    for(l = files; l; l = l->next) {
        FmFileInfo* file = FM_FILE_INFO(l->data);
        fm_file_info_list_push_tail(files, file);
    }
    if(G_UNLIKELY(!dir_fi && job->dir_fi))
        /* we may want info while folder is still loading */
    {
        dir_fi = fm_file_info_ref(job->dir_fi);
    }
    g_signal_emit(folder, signals[FILES_ADDED], 0, files);
}

ErrorAction on_dirlist_job_error(FmDirListJob* job, GError* err, FmJobErrorSeverity severity, FmFolder* folder) {
    guint ret;
    /* it's possible that some signal handlers tries to free the folder
     * when errors occurs, so let's g_object_ref here. */
    g_object_ref(folder);
    g_signal_emit(folder, signals[ERROR], 0, err, (guint)severity, &ret);
    g_object_unref(folder);
    return ret;
}

void free_dirlist_job(FmFolder* folder) {
    if(wants_incremental) {
        g_signal_handlers_disconnect_by_func(dirlist_job, on_dirlist_job_files_found, folder);
    }
    g_signal_handlers_disconnect_by_func(dirlist_job, on_dirlist_job_finished, folder);
    g_signal_handlers_disconnect_by_func(dirlist_job, on_dirlist_job_error, folder);
    fm_job_cancel(FM_JOB(dirlist_job));
    g_object_unref(dirlist_job);
    dirlist_job = nullptr;
}

#endif


void Folder::reload() {
    // cancel in-progress jobs if there are any
    if(dirlist_job) {
        dirlist_job->cancel();
    }
    GError* err = nullptr;
    // cancel directory monitoring
    if(dirMonitor_) {
        g_signal_handlers_disconnect_by_data(dirMonitor_.get(), this);
        dirMonitor_.reset();
    }

    /* clear all update-lists now, see SF bug #919 - if update comes before
       listing job is finished, a duplicate may be created in the folder */
    if(has_idle_update_handler) {
        // FIXME: cancel the idle handler
        paths_to_add.clear();
        paths_to_update.clear();
        paths_to_del.clear();

        // cancel any file info job in progress.
        for(auto job: fileinfoJobs_) {
            job->cancel();
            disconnect(job, &FileInfoJob::finished, this, &Folder::onFileInfoFinished);
        }
        fileinfoJobs_.clear();

        // ensure future changes will be processed
        has_idle_update_handler = false;
    }

    /* remove all existing files */
    if(!files_.empty()) {
        // FIXME: this is not very efficient :(
        auto tmp = files();
        files_.clear();
        Q_EMIT filesRemoved(tmp);
    }

    /* Tell the world that we're about to reload the folder.
     * It might be a good idea for users of the folder to disconnect
     * from the folder temporarily and reconnect to it again after
     * the folder complete the loading. This might reduce some
     * unnecessary signal handling and UI updates. */
    Q_EMIT startLoading();

    dirInfo_.reset(); // clear dir info

    /* also re-create a new file monitor */
    // mon = GFileMonitorPtr{fm_monitor_directory(dir_path.gfile().get(), &err), false};
    // FIXME: should we make this cancellable?
    dirMonitor_ = GFileMonitorPtr{
            g_file_monitor_directory(dirPath_.gfile().get(), G_FILE_MONITOR_WATCH_MOUNTS, nullptr, &err),
            false
    };

    if(dirMonitor_) {
        g_signal_connect(dirMonitor_.get(), "changed", G_CALLBACK(_onFileChangeEvents), this);
    }
    else {
        qDebug("file monitor cannot be created: %s", err->message);
        g_error_free(err);
    }

    Q_EMIT contentChanged();

    /* run a new dir listing job */
    // FIXME:
    // defer_content_test = fm_config->defer_content_test;
    dirlist_job = new DirListJob(dirPath_, defer_content_test ? DirListJob::FAST : DirListJob::DETAILED);
    dirlist_job->setAutoDelete(true);
    connect(dirlist_job, &DirListJob::error, this, &Folder::error, Qt::BlockingQueuedConnection);
    connect(dirlist_job, &DirListJob::finished, this, &Folder::onDirListFinished, Qt::BlockingQueuedConnection);

#if 0
    if(wants_incremental) {
        g_signal_connect(dirlist_job, "files-found", G_CALLBACK(on_dirlist_job_files_found), folder);
    }
    fm_dir_list_job_set_incremental(dirlist_job, wants_incremental);
#endif

    dirlist_job->runAsync();

    /* also reload filesystem info.
     * FIXME: is this needed? */
    queryFilesystemInfo();
}

#if 0

/**
 * Folder::is_incremental
 * @folder: folder to test
 *
 * Checks if a folder is incrementally loaded.
 * After an FmFolder object is obtained from calling Folder::from_path(),
 * if it's not yet loaded, it begins loading the content of the folder
 * and emits "start-loading" signal. Most of the time, the info of the
 * files in the folder becomes available only after the folder is fully
 * loaded. That means, after the "finish-loading" signal is emitted.
 * Before the loading is finished, Folder::get_files() returns nothing.
 * You can tell if a folder is still being loaded with Folder::is_loaded().
 *
 * However, for some special FmFolder types, such as the ones handling
 * search:// URIs, we want to access the file infos while the folder is
 * still being loaded (the search is still ongoing).
 * The content of the folder grows incrementally and Folder::get_files()
 * returns files currently being loaded even when the folder is not
 * fully loaded. This is what we called incremental.
 * Folder::is_incremental() tells you if the FmFolder has this feature.
 *
 * Returns: %true if @folder is incrementally loaded
 *
 * Since: 1.0.2
 */
bool Folder::is_incremental(FmFolder* folder) {
    return wants_incremental;
}

#endif

bool Folder::getFilesystemInfo(uint64_t* total_size, uint64_t* free_size) const {
    if(has_fs_info) {
        *total_size = fs_total_size;
        *free_size = fs_free_size;
        return true;
    }
    return false;
}


void Folder::onFileSystemInfoFinished() {
    FileSystemInfoJob* job = static_cast<FileSystemInfoJob*>(sender());
    if(job->isCancelled() || job != fsInfoJob_) { // this is a cancelled job, ignore!
        fsInfoJob_ = nullptr;
        has_fs_info = false;
        return;
    }
    has_fs_info = job->isAvailable();
    fs_total_size = job->size();
    fs_free_size = job->freeSize();
    filesystem_info_pending = true;
    fsInfoJob_ = nullptr;
    queueUpdate();
}


void Folder::queryFilesystemInfo() {
    // G_LOCK(query);
    if(fsInfoJob_)
        return;
    fsInfoJob_ = new FileSystemInfoJob{dirPath_};
    fsInfoJob_->setAutoDelete(true);
    connect(fsInfoJob_, &FileSystemInfoJob::finished, this, &Folder::onFileSystemInfoFinished, Qt::BlockingQueuedConnection);

    fsInfoJob_->runAsync();
    // G_UNLOCK(query);
}


#if 0
/**
 * Folder::block_updates
 * @folder: folder to apply
 *
 * Blocks emitting signals for changes in folder, i.e. if some file was
 * added, changed, or removed in folder after this API, no signal will be
 * sent until next call to Folder::unblock_updates().
 *
 * Since: 1.2.0
 */
void Folder::block_updates(FmFolder* folder) {
    /* g_debug("Folder::block_updates %p", folder); */
    G_LOCK(lists);
    /* just set the flag */
    stop_emission = true;
    G_UNLOCK(lists);
}

/**
 * Folder::unblock_updates
 * @folder: folder to apply
 *
 * Unblocks emitting signals for changes in folder. If some changes were
 * in folder after previous call to Folder::block_updates() then these
 * changes will be sent after this call.
 *
 * Since: 1.2.0
 */
void Folder::unblock_updates(FmFolder* folder) {
    /* g_debug("Folder::unblock_updates %p", folder); */
    G_LOCK(lists);
    stop_emission = false;
    /* query update now */
    queue_update(folder);
    G_UNLOCK(lists);
    /* g_debug("Folder::unblock_updates OK"); */
}

/**
 * Folder::make_directory
 * @folder: folder to apply
 * @name: display name for new directory
 * @error: (allow-none) (out): location to save error
 *
 * Creates new directory in given @folder.
 *
 * Returns: %true in case of success.
 *
 * Since: 1.2.0
 */
bool Folder::make_directory(FmFolder* folder, const char* name, GError** error) {
    GFile* dir, *gf;
    FmPath* path;
    bool ok;

    dir = fm_path_to_gfile(dir_path);
    gf = g_file_get_child_for_display_name(dir, name, error);
    g_object_unref(dir);
    if(gf == nullptr) {
        return false;
    }
    ok = g_file_make_directory(gf, nullptr, error);
    if(ok) {
        path = fm_path_new_for_gfile(gf);
        if(!_Folder::event_file_added(folder, path)) {
            fm_path_unref(path);
        }
    }
    g_object_unref(gf);
    return ok;
}

void Folder::content_changed(FmFolder* folder) {
    if(has_fs_info && !fs_info_not_avail) {
        Folder::query_filesystem_info(folder);
    }
}

#endif

/* NOTE:
 * GFileMonitor has some significant limitations:
 * 1. Currently it can correctly emit unmounted event for a directory.
 * 2. After a directory is unmounted, its content changes.
 *    Inotify does not fire events for this so a forced reload is needed.
 * 3. If a folder is empty, and later a filesystem is mounted to the
 *    folder, its content should reflect the content of the newly mounted
 *    filesystem. However, GFileMonitor and inotify do not emit events
 *    for this case. A forced reload might be needed for this case as well.
 * 4. Some limitations come from Linux/inotify. If FAM/gamin is used,
 *    the condition may be different. More testing is needed.
 */
void Folder::onMountAdded(const Mount& mnt) {
    /* If a filesystem is mounted over an existing folder,
     * we need to refresh the content of the folder to reflect
     * the changes. Besides, we need to create a new GFileMonitor
     * for the newly-mounted filesystem as the inode already changed.
     * GFileMonitor cannot detect this kind of changes caused by mounting.
     * So let's do it ourselves. */
    auto mountRoot = mnt.root();
    if(mountRoot.isPrefixOf(dirPath_)) {
        queueReload();
    }
    /* g_debug("FmFolder::mount_added"); */
}

void Folder::onMountRemoved(const Mount& mnt) {
    /* g_debug("FmFolder::mount_removed"); */

    /* NOTE: gvfs does not emit unmount signals for remote folders since
     * GFileMonitor does not support remote filesystems at all.
     * So here is the side effect, no unmount notifications.
     * We need to generate the signal ourselves. */
    if(!dirMonitor_) {
        // this is only needed when we don't have a GFileMonitor
        auto mountRoot = mnt.root();
        if(mountRoot.isPrefixOf(dirPath_)) {
            // if the current folder is under the unmounted path, generate the event ourselves
            onDirChanged(G_FILE_MONITOR_EVENT_UNMOUNTED);
        }
    }
}

} // namespace Fm
