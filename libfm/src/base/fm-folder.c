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

/**
 * SECTION:fm-folder
 * @short_description: Folder loading and monitoring.
 * @title: FmFolder
 *
 * @include: libfm/fm.h
 *
 * The #FmFolder object allows to open and monitor items of some directory
 * (either local or remote), i.e. files and directories, to have fast access
 * to their info and to info of the directory itself as well.
 */

#include "fm-folder.h"
#include "fm-monitor.h"
#include "fm-marshal.h"
#include "fm-dummy-monitor.h"
#include "fm-file.h"
#include "fm-config.h"

#include <string.h>

enum {
    FILES_ADDED,
    FILES_REMOVED,
    FILES_CHANGED,
    START_LOADING,
    FINISH_LOADING,
    UNMOUNT,
    CHANGED,
    REMOVED,
    CONTENT_CHANGED,
    FS_INFO,
    ERROR,
    N_SIGNALS
};

struct _FmFolder
{
    GObject parent;

    /*<private>*/
    FmPath* dir_path;
    GFile* gf;
    GFileMonitor* mon;
    FmDirListJob* dirlist_job;
    FmFileInfo* dir_fi;
    FmFileInfoList* files;

    /* for file monitor */
    guint idle_handler;
    GSList* files_to_add;
    GSList* files_to_update;
    GSList* files_to_del;
    GSList* pending_jobs;
    gboolean pending_change_notify;
    gboolean filesystem_info_pending;
    gboolean wants_incremental;
    guint idle_reload_handler;
    gboolean stop_emission; /* don't set it 1 bit to not lock other bits */

    /* filesystem info - set in query thread, read in main */
    guint64 fs_total_size;
    guint64 fs_free_size;
    GCancellable* fs_size_cancellable;
    gboolean has_fs_info : 1;
    gboolean fs_info_not_avail : 1;
    gboolean defer_content_test : 1;
};

static void fm_folder_dispose(GObject *object);
static void fm_folder_finalize(GObject *object);
static void fm_folder_content_changed(FmFolder* folder);

static GList* _fm_folder_get_file_by_path(FmFolder* folder, FmPath *path);

G_DEFINE_TYPE(FmFolder, fm_folder, G_TYPE_OBJECT);

static guint signals[N_SIGNALS];
static GHashTable* hash = NULL;
static int hash_uses = 0;

static GVolumeMonitor* volume_monitor = NULL;

static void on_mount_added(GVolumeMonitor* vm, GMount* mount, gpointer user_data);
static void on_mount_removed(GVolumeMonitor* vm, GMount* mount, gpointer user_data);

/* used for on_query_filesystem_info_finished() to lock folder */
G_LOCK_DEFINE_STATIC(query);
/* protects hash access */
G_LOCK_DEFINE_STATIC(hash);
/* protects access to files_to_add, files_to_update and files_to_del */
G_LOCK_DEFINE_STATIC(lists);

static void fm_folder_class_init(FmFolderClass *klass)
{
    GObjectClass *g_object_class;
    FmFolderClass* folder_class;
    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->dispose = fm_folder_dispose;
    g_object_class->finalize = fm_folder_finalize;
    fm_folder_parent_class = (GObjectClass*)g_type_class_peek(G_TYPE_OBJECT);

    folder_class = FM_FOLDER_CLASS(klass);
    folder_class->content_changed = fm_folder_content_changed;

    /**
     * FmFolder::files-added:
     * @folder: the monitored directory
     * @list: #GList of newly added #FmFileInfo
     *
     * The #FmFolder::files-added signal is emitted when there is some
     * new file created in the directory.
     *
     * Since: 0.1.0
     */
    signals[ FILES_ADDED ] =
        g_signal_new ( "files-added",
                       G_TYPE_FROM_CLASS ( klass ),
                       G_SIGNAL_RUN_FIRST,
                       G_STRUCT_OFFSET ( FmFolderClass, files_added ),
                       NULL, NULL,
                       g_cclosure_marshal_VOID__POINTER,
                       G_TYPE_NONE, 1, G_TYPE_POINTER );

    /**
     * FmFolder::files-removed:
     * @folder: the monitored directory
     * @list: #GList of #FmFileInfo that were deleted
     *
     * The #FmFolder::files-removed signal is emitted when some file was
     * deleted from the directory.
     *
     * Since: 0.1.0
     */
    signals[ FILES_REMOVED ] =
        g_signal_new ( "files-removed",
                       G_TYPE_FROM_CLASS ( klass ),
                       G_SIGNAL_RUN_FIRST,
                       G_STRUCT_OFFSET ( FmFolderClass, files_removed ),
                       NULL, NULL,
                       g_cclosure_marshal_VOID__POINTER,
                       G_TYPE_NONE, 1, G_TYPE_POINTER );

    /**
     * FmFolder::files-changed:
     * @folder: the monitored directory
     * @list: #GList of #FmFileInfo that were changed
     *
     * The #FmFolder::files-changed signal is emitted when some file in
     * the directory was changed.
     *
     * Since: 0.1.0
     */
    signals[ FILES_CHANGED ] =
        g_signal_new ( "files-changed",
                       G_TYPE_FROM_CLASS ( klass ),
                       G_SIGNAL_RUN_FIRST,
                       G_STRUCT_OFFSET ( FmFolderClass, files_changed ),
                       NULL, NULL,
                       g_cclosure_marshal_VOID__POINTER,
                       G_TYPE_NONE, 1, G_TYPE_POINTER );

    /**
     * FmFolder::start-loading:
     * @folder: the monitored directory
     *
     * The #FmFolder::start-loading signal is emitted when the folder is
     * about to be reloaded.
     *
     * Since: 1.0.0
     */
    signals[START_LOADING] =
        g_signal_new ( "start-loading",
                       G_TYPE_FROM_CLASS (klass),
                       G_SIGNAL_RUN_FIRST,
                       G_STRUCT_OFFSET( FmFolderClass, start_loading),
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);

    /**
     * FmFolder::finish-loading:
     * @folder: the monitored directory
     *
     * The #FmFolder::finish-loading signal is emitted when the content
     * of the folder is loaded.
     * This signal may be emitted more than once and can be induced
     * by calling fm_folder_reload().
     *
     * Since: 1.0.0
     */
    signals[ FINISH_LOADING ] =
        g_signal_new ( "finish-loading",
                       G_TYPE_FROM_CLASS ( klass ),
                       G_SIGNAL_RUN_FIRST,
                       G_STRUCT_OFFSET ( FmFolderClass, finish_loading ),
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);

    /**
     * FmFolder::unmount:
     * @folder: the monitored directory
     *
     * The #FmFolder::unmount signal is emitted when the folder was unmounted.
     *
     * Since: 0.1.1
     */
    signals[ UNMOUNT ] =
        g_signal_new ( "unmount",
                       G_TYPE_FROM_CLASS ( klass ),
                       G_SIGNAL_RUN_FIRST,
                       G_STRUCT_OFFSET ( FmFolderClass, unmount ),
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);

    /**
     * FmFolder::changed:
     * @folder: the monitored directory
     *
     * The #FmFolder::changed signal is emitted when the folder itself
     * was changed.
     *
     * Since: 0.1.16
     */
    signals[ CHANGED ] =
        g_signal_new ( "changed",
                       G_TYPE_FROM_CLASS ( klass ),
                       G_SIGNAL_RUN_FIRST,
                       G_STRUCT_OFFSET ( FmFolderClass, changed ),
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);

    /**
     * FmFolder::removed:
     * @folder: the monitored directory
     *
     * The #FmFolder::removed signal is emitted when the folder itself
     * was deleted.
     *
     * Since: 0.1.16
     */
    signals[ REMOVED ] =
        g_signal_new ( "removed",
                       G_TYPE_FROM_CLASS ( klass ),
                       G_SIGNAL_RUN_FIRST,
                       G_STRUCT_OFFSET ( FmFolderClass, removed ),
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);

    /**
     * FmFolder::content-changed:
     * @folder: the monitored directory
     *
     * The #FmFolder::content-changed signal is emitted when content
     * of the folder is changed (some files are added, removed, or changed).
     *
     * Since: 0.1.16
     */
    signals[ CONTENT_CHANGED ] =
        g_signal_new ( "content-changed",
                       G_TYPE_FROM_CLASS ( klass ),
                       G_SIGNAL_RUN_FIRST,
                       G_STRUCT_OFFSET ( FmFolderClass, content_changed ),
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);

    /**
     * FmFolder::fs-info:
     * @folder: the monitored directory
     *
     * The #FmFolder::fs-info signal is emitted when filesystem
     * information is available.
     *
     * Since: 0.1.16
     */
    signals[ FS_INFO ] =
        g_signal_new ( "fs-info",
                       G_TYPE_FROM_CLASS ( klass ),
                       G_SIGNAL_RUN_FIRST,
                       G_STRUCT_OFFSET ( FmFolderClass, fs_info ),
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);

    /**
     * FmFolder::error:
     * @folder: the monitored directory
     * @error: error descriptor
     * @severity: #FmJobErrorSeverity of the error
     *
     * The #FmFolder::error signal is emitted when some error happens. A case
     * if more than one handler is connected to this signal is ambiguous.
     *
     * Return value: #FmJobErrorAction that should be performed on that error.
     *
     * Since: 0.1.1
     */
    signals[ERROR] =
        g_signal_new( "error",
                      G_TYPE_FROM_CLASS ( klass ),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET ( FmFolderClass, error ),
                      NULL, NULL,
#if GLIB_CHECK_VERSION(2,26,0)
                      fm_marshal_UINT__BOXED_UINT,
                      G_TYPE_UINT, 2, G_TYPE_ERROR, G_TYPE_UINT );
#else
                      fm_marshal_INT__POINTER_INT,
                      G_TYPE_INT, 2, G_TYPE_POINTER, G_TYPE_INT );
#endif
}


static void fm_folder_init(FmFolder *folder)
{
    folder->files = fm_file_info_list_new();
    G_LOCK(hash);
    if (G_UNLIKELY(hash_uses == 0))
    {
        hash = g_hash_table_new((GHashFunc)fm_path_hash, (GEqualFunc)fm_path_equal);
        volume_monitor = g_volume_monitor_get();
        if (G_LIKELY(volume_monitor))
        {
            g_signal_connect(volume_monitor, "mount-added", G_CALLBACK(on_mount_added), NULL);
            g_signal_connect(volume_monitor, "mount-removed", G_CALLBACK(on_mount_removed), NULL);
        }
    }
    hash_uses++;
    G_UNLOCK(hash);
}

static gboolean on_idle_reload(FmFolder* folder)
{
    /* check if folder still exists */
    if(g_source_is_destroyed(g_main_current_source()))
    {
        /* FIXME: it should be impossible, folder cannot be disposed at this point */
        return FALSE;
    }
    fm_folder_reload(folder);
    G_LOCK(query);
    folder->idle_reload_handler = 0;
    G_UNLOCK(query);
    g_object_unref(folder);
    return FALSE;
}

static void queue_reload(FmFolder* folder)
{
    G_LOCK(query);
    if(!folder->idle_reload_handler)
        folder->idle_reload_handler = g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc)on_idle_reload,
                                                      g_object_ref(folder), NULL);
    G_UNLOCK(query);
}

static void on_file_info_job_finished(FmFileInfoJob* job, FmFolder* folder)
{
    GList* l;
    GSList* files_to_add = NULL;
    GSList* files_to_update = NULL;
    if(!fm_job_is_cancelled(FM_JOB(job)))
    {
        gboolean need_added = g_signal_has_handler_pending(folder, signals[FILES_ADDED], 0, TRUE);
        gboolean need_changed = g_signal_has_handler_pending(folder, signals[FILES_CHANGED], 0, TRUE);

        for(l=fm_file_info_list_peek_head_link(job->file_infos);l;l=l->next)
        {
            FmFileInfo* fi = (FmFileInfo*)l->data;
            FmPath* path = fm_file_info_get_path(fi);
            GList* l2;
            if (fm_folder_is_valid(folder) && path == fm_file_info_get_path(folder->dir_fi))
                /* update for folder itself, also see FIXME below! */
                fm_file_info_update(folder->dir_fi, fi);
            else if ((l2 = _fm_folder_get_file_by_path(folder, path)))
                /* the file is already in the folder, update */
            {
                FmFileInfo* fi2 = (FmFileInfo*)l2->data;
                /* FIXME: will fm_file_info_update here cause problems?
                 *        the file info might be referenced by others, too.
                 *        we're mofifying an object referenced by others.
                 *        we should redesign the API, or document this clearly
                 *        in future API doc.
                 */
                fm_file_info_update(fi2, fi);
                if(need_changed)
                    files_to_update = g_slist_prepend(files_to_update, fi2);
            }
            else
            {
                if(need_added)
                    files_to_add = g_slist_prepend(files_to_add, fi);
                fm_file_info_list_push_tail(folder->files, fi);
            }
        }
        if(files_to_add)
        {
            g_signal_emit(folder, signals[FILES_ADDED], 0, files_to_add);
            g_slist_free(files_to_add);
        }
        if(files_to_update)
        {
            g_signal_emit(folder, signals[FILES_CHANGED], 0, files_to_update);
            g_slist_free(files_to_update);
        }
        g_signal_emit(folder, signals[CONTENT_CHANGED], 0);
    }
    folder->pending_jobs = g_slist_remove(folder->pending_jobs, job);
    g_object_unref(job);
}

static gboolean on_idle(FmFolder* folder)
{
    GSList* l;
    FmFileInfoJob* job = NULL;
    GSList *files_to_add, *files_to_del, *files_to_update;
    gboolean stop_emission;

    /* check if folder still exists */
    if(g_source_is_destroyed(g_main_current_source()))
    {
        /* it should be impossible, folder cannot be disposed at this point */
        return FALSE;
    }
    G_LOCK(lists);
    folder->idle_handler = 0;
    stop_emission = folder->stop_emission;
    if (!stop_emission)
    {
        files_to_add = folder->files_to_add;
        folder->files_to_add = NULL;
        files_to_del = folder->files_to_del;
        folder->files_to_del = NULL;
        files_to_update = folder->files_to_update;
        folder->files_to_update = NULL;
    }
    G_UNLOCK(lists);

    /* if we were asked to block updates let delay it for now */
    if (stop_emission)
        goto _finish;

    /* g_debug("folder: on_idle() started"); */

    if(files_to_update || files_to_add)
        job = (FmFileInfoJob*)fm_file_info_job_new(NULL, 0);

    if(files_to_update)
    {
        for(l=files_to_update; l; l = l->next)
        {
            FmPath *path = l->data;
            fm_file_info_job_add(job, path);
            fm_path_unref(path);
        }
        g_slist_free(files_to_update);
    }

    if(files_to_add)
    {
        for(l=files_to_add;l;l=l->next)
        {
            FmPath *path = l->data;
            fm_file_info_job_add(job, path);
            fm_path_unref(path);
        }
        g_slist_free(files_to_add);
    }

    if(job)
    {
        g_signal_connect(job, "finished", G_CALLBACK(on_file_info_job_finished), folder);
        folder->pending_jobs = g_slist_prepend(folder->pending_jobs, job);
        if (!fm_job_run_async(FM_JOB(job)))
        {
            folder->pending_jobs = g_slist_remove(folder->pending_jobs, job);
            g_object_unref(job);
            g_critical("failed to start folder update job");
        }
        /* the job will be freed automatically in on_file_info_job_finished() */
    }

    if(files_to_del)
    {
        GSList* ll;
        for(ll=files_to_del;ll;ll=ll->next)
        {
            GList* l= (GList*)ll->data;
            ll->data = l->data;
            fm_file_info_list_delete_link_nounref(folder->files, l);
        }
        g_signal_emit(folder, signals[FILES_REMOVED], 0, files_to_del);
        g_slist_foreach(files_to_del, (GFunc)fm_file_info_unref, NULL);
        g_slist_free(files_to_del);

        g_signal_emit(folder, signals[CONTENT_CHANGED], 0);
    }

    if(folder->pending_change_notify)
    {
        g_signal_emit(folder, signals[CHANGED], 0);
        /* update volume info */
        fm_folder_query_filesystem_info(folder);
        folder->pending_change_notify = FALSE;
    }

    G_LOCK(query);
    if(folder->filesystem_info_pending)
    {
        folder->filesystem_info_pending = FALSE;
        G_UNLOCK(query);
        g_signal_emit(folder, signals[FS_INFO], 0);
    }
    else
        G_UNLOCK(query);
    /* g_debug("folder: on_idle() done"); */
_finish:
    /* release reference borrowed in queue_update() */
    g_object_unref(folder);

    return FALSE;
}

/* should be called only with G_LOCK(lists) on! */
static inline void queue_update(FmFolder *folder)
{
    if (!folder->idle_handler)
        /* borrow reference on folder */
        folder->idle_handler = g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc)on_idle,
                                               g_object_ref(folder), NULL);
}

/* returns TRUE if reference was taken from path */
gboolean _fm_folder_event_file_added(FmFolder *folder, FmPath *path)
{
    gboolean added = TRUE;

    G_LOCK(lists);
    /* make sure that the file is not already queued for addition. */
    if(!g_slist_find(folder->files_to_add, path))
    {
        GList *l = _fm_folder_get_file_by_path(folder, path);
        if(!l) /* it's new file */
        {
            /* add the file name to queue for addition. */
            folder->files_to_add = g_slist_append(folder->files_to_add, path);
        }
        else if(g_slist_find(folder->files_to_update, path))
        {
            /* file already queued for update, don't duplicate */
            added = FALSE;
        }
        /* if we already have the file in FmFolder, update the existing one instead. */
        else
        {
            /* bug #3591771: 'ln -fns . test' leave no file visible in folder.
               If it is queued for deletion then cancel that operation */
            folder->files_to_del = g_slist_remove(folder->files_to_del, l);
            /* update the existing item. */
            folder->files_to_update = g_slist_append(folder->files_to_update, path);
        }
    }
    else
        /* file already queued for adding, don't duplicate */
        added = FALSE;
    if (added)
        queue_update(folder);
    G_UNLOCK(lists);
    return added;
}

gboolean _fm_folder_event_file_changed(FmFolder *folder, FmPath *path)
{
    gboolean added;

    G_LOCK(lists);
    /* make sure that the file is not already queued for changes or
     * it's already queued for addition. */
    if(!g_slist_find(folder->files_to_update, path) &&
       !g_slist_find(folder->files_to_add, path) &&
       _fm_folder_get_file_by_path(folder, path)) /* ensure it is our file */
    {
        folder->files_to_update = g_slist_append(folder->files_to_update, path);
        added = TRUE;
        queue_update(folder);
    }
    else
    {
        added = FALSE;
    }
    G_UNLOCK(lists);
    return added;
}

void _fm_folder_event_file_deleted(FmFolder *folder, FmPath *path)
{
    GList *l;
    GSList *sl;

    G_LOCK(lists);
    l = _fm_folder_get_file_by_path(folder, path);
    if(l && !g_slist_find(folder->files_to_del, l) )
        folder->files_to_del = g_slist_prepend(folder->files_to_del, l);
    /* if the file is already queued for addition or update, that operation
       will be just a waste, therefore cancel it right now */
    sl = g_slist_find(folder->files_to_update, path);
    if(sl)
    {
        folder->files_to_update = g_slist_delete_link(folder->files_to_update, sl);
    }
    else if((sl = g_slist_find(folder->files_to_add, path)))
    {
        folder->files_to_add = g_slist_delete_link(folder->files_to_add, sl);
    }
    else
        path = NULL;
    queue_update(folder);
    G_UNLOCK(lists);
    if (path != NULL)
        fm_path_unref(path); /* link was freed above so we should unref it */
}

static void on_folder_changed(GFileMonitor* mon, GFile* gf, GFile* other, GFileMonitorEvent evt, FmFolder* folder)
{
    FmPath* path;

    /* const char* names[]={
        "G_FILE_MONITOR_EVENT_CHANGED",
        "G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT",
        "G_FILE_MONITOR_EVENT_DELETED",
        "G_FILE_MONITOR_EVENT_CREATED",
        "G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED",
        "G_FILE_MONITOR_EVENT_PRE_UNMOUNT",
        "G_FILE_MONITOR_EVENT_UNMOUNTED"
    }; */
    
    /*
    char *name = g_file_get_basename(gf);
    g_debug("folder: %p, file %s event: %s", folder, name, names[evt]);
    g_free(name);
    */

    if(g_file_equal(gf, folder->gf))
    {
        /* g_debug("event of the folder itself: %d", evt); */

        /* NOTE: g_object_ref() should be used here.
         * Sometimes the folder will be freed by signal handlers 
         * during emission of the change notifications. */
        g_object_ref(folder);
        switch(evt)
        {
        case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
            /* g_debug("folder is going to be unmounted"); */
            break;
        case G_FILE_MONITOR_EVENT_UNMOUNTED:
            g_signal_emit(folder, signals[UNMOUNT], 0);
            /* g_debug("folder is unmounted"); */
            queue_reload(folder);
            break;
        case G_FILE_MONITOR_EVENT_DELETED:
            g_signal_emit(folder, signals[REMOVED], 0);
            /* g_debug("folder is deleted"); */
            break;
        case G_FILE_MONITOR_EVENT_CREATED:
            queue_reload(folder);
            break;
        case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
        case G_FILE_MONITOR_EVENT_CHANGED:
            folder->pending_change_notify = TRUE;
            G_LOCK(lists);
            if (g_slist_find(folder->files_to_update, folder->dir_path) == NULL)
            {
                folder->files_to_update = g_slist_append(folder->files_to_update, fm_path_ref(folder->dir_path));
                queue_update(folder);
            }
            G_UNLOCK(lists);
            /* g_debug("folder is changed"); */
            break;
#if GLIB_CHECK_VERSION(2,24,0)
        case G_FILE_MONITOR_EVENT_MOVED:
#endif
        case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
            ;
        }
        g_object_unref(folder);
        return;
    }

    path = fm_path_new_for_gfile(gf);

    /* NOTE: sometimes, for unknown reasons, GFileMonitor gives us the
     * same event of the same file for multiple times. So we need to 
     * check for duplications ourselves here. */
    switch(evt)
    {
    case G_FILE_MONITOR_EVENT_CREATED:
        if (!_fm_folder_event_file_added(folder, path))
            fm_path_unref(path);
        break;
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
    case G_FILE_MONITOR_EVENT_CHANGED:
        if (!_fm_folder_event_file_changed(folder, path))
            fm_path_unref(path);
        break;
    case G_FILE_MONITOR_EVENT_DELETED:
        _fm_folder_event_file_deleted(folder, path);
        fm_path_unref(path);
        break;
    default:
        /* g_debug("folder %p %s event: %s", folder, name, names[evt]); */
        fm_path_unref(path);
        return;
    }
    G_LOCK(lists);
    queue_update(folder);
    G_UNLOCK(lists);
}

static void on_dirlist_job_finished(FmDirListJob* job, FmFolder* folder)
{
    GSList* files = NULL;
    /* actually manually disconnecting from 'finished' signal is not
     * needed since the signal is only emit once, and later the job
     * object will be distroyed very soon. */
    /* g_signal_handlers_disconnect_by_func(job, on_dirlist_job_finished, folder); */

    if(!fm_job_is_cancelled(FM_JOB(job)) && !folder->wants_incremental)
    {
        GList* l;
        for(l = fm_file_info_list_peek_head_link(job->files); l; l=l->next)
        {
            FmFileInfo* inf = (FmFileInfo*)l->data;
            files = g_slist_prepend(files, inf);
            fm_file_info_list_push_tail(folder->files, inf);
        }
        if(G_LIKELY(files))
        {
            GSList *l;

            G_LOCK(lists);
            if (folder->defer_content_test && fm_path_is_native(folder->dir_path))
                /* we got only basic info on content, schedule update it now */
                for (l = files; l; l = l->next)
                    folder->files_to_update = g_slist_prepend(folder->files_to_update,
                                                fm_path_ref(fm_file_info_get_path(l->data)));
            G_UNLOCK(lists);
            g_signal_emit(folder, signals[FILES_ADDED], 0, files);
            g_slist_free(files);
        }

        if(job->dir_fi)
            folder->dir_fi = fm_file_info_ref(job->dir_fi);

        /* Some new files are created while FmDirListJob is loading the folder. */
        G_LOCK(lists);
        if(G_UNLIKELY(folder->files_to_add))
        {
            /* This should be a very rare case. Could this happen? */
            GSList* l;
            for(l = folder->files_to_add; l;)
            {
                FmPath *path = l->data;
                GSList* next = l->next;
                if(_fm_folder_get_file_by_path(folder, path))
                {
                    /* we already have the file. remove it from files_to_add, 
                     * and put it in files_to_update instead.
                     * No strdup for name is needed here. We steal
                     * the string from files_to_add.*/
                    folder->files_to_update = g_slist_prepend(folder->files_to_update, path);
                    folder->files_to_add = g_slist_delete_link(folder->files_to_add, l);
                }
                l = next;
            }
        }
        G_UNLOCK(lists);
    }
    else if(!folder->dir_fi && job->dir_fi)
        /* we may need dir_fi for incremental folders too */
        folder->dir_fi = fm_file_info_ref(job->dir_fi);
    g_object_unref(folder->dirlist_job);
    folder->dirlist_job = NULL;

    g_object_ref(folder);
    g_signal_emit(folder, signals[FINISH_LOADING], 0);
    g_object_unref(folder);
}

static void on_dirlist_job_files_found(FmDirListJob* job, GSList* files, gpointer user_data)
{
    FmFolder* folder = FM_FOLDER(user_data);
    GSList* l;
    for(l = files; l; l = l->next)
    {
        FmFileInfo* file = FM_FILE_INFO(l->data);
        fm_file_info_list_push_tail(folder->files, file);
    }
    if (G_UNLIKELY(!folder->dir_fi && job->dir_fi))
        /* we may want info while folder is still loading */
        folder->dir_fi = fm_file_info_ref(job->dir_fi);
    g_signal_emit(folder, signals[FILES_ADDED], 0, files);
}

static FmJobErrorAction on_dirlist_job_error(FmDirListJob* job, GError* err, FmJobErrorSeverity severity, FmFolder* folder)
{
    guint ret;
    /* it's possible that some signal handlers tries to free the folder
     * when errors occurs, so let's g_object_ref here. */
    g_object_ref(folder);
    g_signal_emit(folder, signals[ERROR], 0, err, (guint)severity, &ret);
    g_object_unref(folder);
    return ret;
}

static FmFolder* fm_folder_new_internal(FmPath* path, GFile* gf)
{
    FmFolder* folder = (FmFolder*)g_object_new(FM_TYPE_FOLDER, NULL);
    folder->dir_path = fm_path_ref(path);
    folder->gf = (GFile*)g_object_ref(gf);
    folder->wants_incremental = fm_file_wants_incremental(gf);
    fm_folder_reload(folder);
    return folder;
}

/* NB: increases reference on returned object */
static FmFolder* fm_folder_get_internal(FmPath* path, GFile* gf)
{
    FmFolder* folder;
    /* FIXME: should we provide a generic FmPath cache in fm-path.c
     * to associate all kinds of data structures with FmPaths? */

    G_LOCK(hash);
    folder = hash ? (FmFolder*)g_hash_table_lookup(hash, path) : NULL;

    if( G_UNLIKELY(!folder) )
    {
        GFile* _gf = NULL;

        G_UNLOCK(hash);
        if(!gf)
            _gf = gf = fm_path_to_gfile(path);
        folder = fm_folder_new_internal(path, gf);
        if(_gf)
            g_object_unref(_gf);
        G_LOCK(hash);
        g_hash_table_insert(hash, folder->dir_path, folder);
    }
    else
        g_object_ref(folder);
    G_UNLOCK(hash);
    return folder;
}

static void free_dirlist_job(FmFolder* folder)
{
    if(folder->wants_incremental)
        g_signal_handlers_disconnect_by_func(folder->dirlist_job, on_dirlist_job_files_found, folder);
    g_signal_handlers_disconnect_by_func(folder->dirlist_job, on_dirlist_job_finished, folder);
    g_signal_handlers_disconnect_by_func(folder->dirlist_job, on_dirlist_job_error, folder);
    fm_job_cancel(FM_JOB(folder->dirlist_job));
    g_object_unref(folder->dirlist_job);
    folder->dirlist_job = NULL;
}

static void fm_folder_dispose(GObject *object)
{
    FmFolder *folder;
    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_FOLDER(object));

    /* g_debug("fm_folder_dispose"); */

    folder = (FmFolder*)object;

    if(folder->dirlist_job)
        free_dirlist_job(folder);

    if(folder->pending_jobs)
    {
        GSList* l;
        for(l = folder->pending_jobs;l;l=l->next)
        {
            FmJob* job = FM_JOB(l->data);
            g_signal_handlers_disconnect_by_func(job, on_file_info_job_finished, folder);
            fm_job_cancel(job);
            g_object_unref(job);
        }
        g_slist_free(folder->pending_jobs);
        folder->pending_jobs = NULL;
    }

    if(folder->mon)
    {
        g_signal_handlers_disconnect_by_func(folder->mon, on_folder_changed, folder);
        g_object_unref(folder->mon);
        folder->mon = NULL;
    }

    G_LOCK(query);
    if(folder->idle_reload_handler)
    {
        /* FIXME: it should be impossible, folder should be referenced if handler added */
        g_source_remove(folder->idle_reload_handler);
        folder->idle_reload_handler = 0;
    }

    if(folder->fs_size_cancellable)
    {
        g_cancellable_cancel(folder->fs_size_cancellable);
        g_object_unref(folder->fs_size_cancellable);
        folder->fs_size_cancellable = NULL;
    }
    G_UNLOCK(query);

    G_LOCK(lists);
    if(folder->idle_handler)
    {
        /* FIXME: it should be impossible, folder should be referenced if handler added */
        g_source_remove(folder->idle_handler);
        folder->idle_handler = 0;
        if(folder->files_to_add)
        {
            g_slist_foreach(folder->files_to_add, (GFunc)fm_path_unref, NULL);
            g_slist_free(folder->files_to_add);
            folder->files_to_add = NULL;
        }
        if(folder->files_to_update)
        {
            g_slist_foreach(folder->files_to_update, (GFunc)fm_path_unref, NULL);
            g_slist_free(folder->files_to_update);
            folder->files_to_update = NULL;
        }
        if(folder->files_to_del)
        {
            g_slist_free(folder->files_to_del);
            folder->files_to_del = NULL;
        }
    }
    G_UNLOCK(lists);

    /* remove from hash table */
    if(folder->dir_path)
    {
        G_LOCK(hash);
        g_hash_table_remove(hash, folder->dir_path);
        G_UNLOCK(hash);
        fm_path_unref(folder->dir_path);
        folder->dir_path = NULL;
    }

    if(folder->dir_fi)
    {
        fm_file_info_unref(folder->dir_fi);
        folder->dir_fi = NULL;
    }

    if(folder->gf)
    {
        g_object_unref(folder->gf);
        folder->gf = NULL;
    }

    if(folder->files)
    {
        fm_file_info_list_unref(folder->files);
        folder->files = NULL;
    }

    (* G_OBJECT_CLASS(fm_folder_parent_class)->dispose)(object);
}

static void fm_folder_finalize(GObject *object)
{
    G_LOCK(hash);
    hash_uses--;
    if (G_UNLIKELY(hash_uses == 0))
    {
        g_hash_table_destroy(hash);
        hash = NULL;
        if(volume_monitor)
        {
            g_signal_handlers_disconnect_by_func(volume_monitor, on_mount_added, NULL);
            g_signal_handlers_disconnect_by_func(volume_monitor, on_mount_removed, NULL);
            g_object_unref(volume_monitor);
            volume_monitor = NULL;
        }
    }
    G_UNLOCK(hash);

    (* G_OBJECT_CLASS(fm_folder_parent_class)->finalize)(object);
}

/**
 * fm_folder_from_gfile
 * @gf: #GFile file descriptor
 *
 * Retrieves a folder corresponding to @gf. Returned data may be freshly
 * created or already loaded. Caller should call g_object_unref() on the
 * returned data after usage.
 *
 * Before 1.0.0 this call had name fm_folder_get_for_gfile.
 *
 * Returns: (transfer full): #FmFolder corresponding to @gf.
 *
 * Since: 0.1.1
 */
FmFolder* fm_folder_from_gfile(GFile* gf)
{
    FmPath* path = fm_path_new_for_gfile(gf);
    FmFolder* folder = fm_folder_get_internal(path, gf);
    fm_path_unref(path);
    return folder;
}

/**
 * fm_folder_from_path_name
 * @path: POSIX path to the folder
 *
 * Retrieves a folder corresponding to @path. Returned data may be freshly
 * created or already loaded. Caller should call g_object_unref() on the
 * returned data after usage.
 *
 * Before 1.0.0 this call had name fm_folder_get_for_path_name.
 *
 * Returns: (transfer full): #FmFolder corresponding to @path.
 *
 * Since: 0.1.0
 */
FmFolder* fm_folder_from_path_name(const char* path)
{
    /* it is very likely the GFile will be required and since creation
       of new GFile is much cheaper than fm_path_to_gfile() let make it */
    GFile* gf = g_file_new_for_path(path);
    FmPath* fm_path = fm_path_new_for_path(path);
    FmFolder* folder = fm_folder_get_internal(fm_path, gf);
    g_object_unref(gf);
    fm_path_unref(fm_path);
    return folder;
}

/**
 * fm_folder_from_uri
 * @uri: URI for the folder
 *
 * Retrieves a folder corresponding to @uri. Returned data may be freshly
 * created or already loaded. Caller should call g_object_unref() on the
 * returned data after usage.
 *
 * Before 1.0.0 this call had name fm_folder_get_for_uri.
 *
 * Returns: (transfer full): #FmFolder corresponding to @uri.
 *
 * Since: 0.1.0
 */
FmFolder*    fm_folder_from_uri    (const char* uri)
{
    /* it is very likely the GFile will be required and since creation
       of new GFile is much cheaper than fm_path_to_gfile() let make it */
    GFile* gf = fm_file_new_for_uri(uri);
    FmPath* path = fm_path_new_for_uri(uri);
    FmFolder* folder = fm_folder_get_internal(path, gf);
    g_object_unref(gf);
    fm_path_unref(path);
    return folder;
}

/**
 * fm_folder_reload
 * @folder: folder to be reloaded
 *
 * Causes to retrieve all data for the @folder as if folder was freshly
 * opened.
 *
 * Since: 0.1.1
 */
void fm_folder_reload(FmFolder* folder)
{
    GError* err = NULL;

    /* Tell the world that we're about to reload the folder.
     * It might be a good idea for users of the folder to disconnect
     * from the folder temporarily and reconnect to it again after
     * the folder complete the loading. This might reduce some
     * unnecessary signal handling and UI updates. */
    g_signal_emit(folder, signals[START_LOADING], 0);

    if(folder->dir_fi)
    {
        /* we need to reload folde info. */
        fm_file_info_unref(folder->dir_fi);
        folder->dir_fi = NULL;
    }

    /* clear all update-lists now, see SF bug #919 - if update comes before
       listing job is finished, a duplicate may be created in the folder */
    if (folder->idle_handler)
    {
        g_source_remove(folder->idle_handler);
        folder->idle_handler = 0;
        if (folder->files_to_add)
        {
            g_slist_foreach(folder->files_to_add, (GFunc)fm_path_unref, NULL);
            g_slist_free(folder->files_to_add);
            folder->files_to_add = NULL;
        }
        if (folder->files_to_update)
        {
            g_slist_foreach(folder->files_to_update, (GFunc)fm_path_unref, NULL);
            g_slist_free(folder->files_to_update);
            folder->files_to_update = NULL;
        }
        if (folder->files_to_del)
        {
            g_slist_free(folder->files_to_del);
            folder->files_to_del = NULL;
        }
    }

    /* remove all items and re-run a dir list job. */
    GList* l = fm_file_info_list_peek_head_link(folder->files);

    /* cancel running dir listing job if there is any. */
    if(folder->dirlist_job)
        free_dirlist_job(folder);

    /* remove all existing files */
    if(l)
    {
        if(g_signal_has_handler_pending(folder, signals[FILES_REMOVED], 0, TRUE))
        {
            /* need to emit signal of removal */
            GSList* files_to_del = NULL;
            for(;l;l=l->next)
                files_to_del = g_slist_prepend(files_to_del, (FmFileInfo*)l->data);
            g_signal_emit(folder, signals[FILES_REMOVED], 0, files_to_del);
            g_slist_free(files_to_del);
        }
        fm_file_info_list_clear(folder->files); /* fm_file_info_unref will be invoked. */
    }

    /* also re-create a new file monitor */
    if(folder->mon)
    {
        g_signal_handlers_disconnect_by_func(folder->mon, on_folder_changed, folder);
        g_object_unref(folder->mon);
    }
    folder->mon = fm_monitor_directory(folder->gf, &err);
    if(folder->mon)
    {
        g_signal_connect(folder->mon, "changed", G_CALLBACK(on_folder_changed), folder);
    }
    else
    {
        g_debug("file monitor cannot be created: %s", err->message);
        g_error_free(err);
        folder->mon = NULL;
    }

    g_signal_emit(folder, signals[CONTENT_CHANGED], 0);

    /* run a new dir listing job */
    folder->defer_content_test = fm_config->defer_content_test;
    folder->dirlist_job = fm_dir_list_job_new2(folder->dir_path,
            folder->defer_content_test ? FM_DIR_LIST_JOB_FAST : FM_DIR_LIST_JOB_DETAILED);

    g_signal_connect(folder->dirlist_job, "finished", G_CALLBACK(on_dirlist_job_finished), folder);
    if(folder->wants_incremental)
        g_signal_connect(folder->dirlist_job, "files-found", G_CALLBACK(on_dirlist_job_files_found), folder);
    fm_dir_list_job_set_incremental(folder->dirlist_job, folder->wants_incremental);
    g_signal_connect(folder->dirlist_job, "error", G_CALLBACK(on_dirlist_job_error), folder);
    if (!fm_job_run_async(FM_JOB(folder->dirlist_job)))
    {
        folder->dirlist_job = NULL;
        g_object_unref(folder->dirlist_job);
        g_critical("failed to start directory listing job for the folder");
    }

    /* also reload filesystem info.
     * FIXME: is this needed? */
    fm_folder_query_filesystem_info(folder);
}

/**
 * fm_folder_get_files
 * @folder: folder to retrieve file list
 *
 * Retrieves list of currently known files and subdirectories in the
 * @folder. Returned list is owned by #FmFolder and should be not modified
 * by caller. If caller wants to keep a reference to the returned list it
 * should do fm_file_info_list_ref&lpar;) on the returned data.
 *
 * Before 1.0.0 this call had name fm_folder_get.
 *
 * Returns: (transfer none): list of items that @folder currently contains.
 *
 * Since: 0.1.1
 */
FmFileInfoList* fm_folder_get_files (FmFolder* folder)
{
    return folder->files;
}

/**
 * fm_folder_is_empty
 * @folder: folder to test
 *
 * Checks if folder has no files or subdirectories.
 *
 * Returns: %TRUE if folder is empty.
 *
 * Since: 1.0.0
 */
gboolean fm_folder_is_empty(FmFolder* folder)
{
    return fm_file_info_list_is_empty(folder->files);
}

/**
 * fm_folder_get_info
 * @folder: folder to retrieve info
 *
 * Retrieves #FmFileInfo data about the folder itself. Returned data is
 * owned by #FmFolder and should be not modified or freed by caller.
 *
 * Returns: (transfer none): info descriptor of the @folder.
 *
 * Since: 1.0.0
 */
FmFileInfo* fm_folder_get_info(FmFolder* folder)
{
    return folder->dir_fi;
}

/**
 * fm_folder_get_path
 * @folder: folder to retrieve path
 *
 * Retrieves path of the folder. Returned data is owned by #FmFolder and
 * should be not modified or freed by caller.
 *
 * Returns: (transfer none): path of the folder.
 *
 * Since: 1.0.0
 */
FmPath* fm_folder_get_path(FmFolder* folder)
{
    return folder->dir_path;
}

static GList* _fm_folder_get_file_by_path(FmFolder* folder, FmPath *path)
{
    GList* l = fm_file_info_list_peek_head_link(folder->files);
    for(;l;l=l->next)
    {
        FmFileInfo* fi = (FmFileInfo*)l->data;
        FmPath* lpath = fm_file_info_get_path(fi);
        if(lpath == path)
            return l;
    }
    return NULL;
}

/**
 * fm_folder_get_file_by_name
 * @folder: folder to search
 * @name: basename of file in @folder
 *
 * Tries to find a file with basename @name in the @folder. Returned data
 * is owned by #FmFolder and should be not freed by caller.
 *
 * Returns: (transfer none): info descriptor of file or %NULL if no file was found.
 *
 * Since: 0.1.16
 */
FmFileInfo* fm_folder_get_file_by_name(FmFolder* folder, const char* name)
{
    FmPath *path = fm_path_new_child(folder->dir_path, name);
    GList* l = _fm_folder_get_file_by_path(folder, path);
    fm_path_unref(path);
    return l ? (FmFileInfo*)l->data : NULL;
}

/**
 * fm_folder_from_path
 * @path: path descriptor for the folder
 *
 * Retrieves a folder corresponding to @path. Returned data may be freshly
 * created or already loaded. Caller should call g_object_unref() on the
 * returned data after usage.
 *
 * Before 1.0.0 this call had name fm_folder_get.
 *
 * Returns: (transfer full): #FmFolder corresponding to @path.
 *
 * Since: 0.1.1
 */
FmFolder* fm_folder_from_path(FmPath* path)
{
    return fm_folder_get_internal(path, NULL);
}

/**
 * fm_folder_is_loaded
 * @folder: folder to test
 *
 * Checks if all data for @folder is completely loaded.
 *
 * Before 1.0.0 this call had name fm_folder_get_is_loaded.
 *
 * Returns: %TRUE is loading of folder is already completed.
 *
 * Since: 0.1.16
 */
gboolean fm_folder_is_loaded(FmFolder* folder)
{
    return (folder->dirlist_job == NULL);
}

/**
 * fm_folder_is_valid
 * @folder: folder to test
 *
 * Checks if directory described by @folder exists.
 *
 * Returns: %TRUE if @folder describes a valid existing directory.
 *
 * Since: 1.0.0
 */
gboolean fm_folder_is_valid(FmFolder* folder)
{
    return (folder->dir_fi != NULL);
}

/**
 * fm_folder_is_incremental
 * @folder: folder to test
 *
 * Checks if a folder is incrementally loaded.
 * After an FmFolder object is obtained from calling fm_folder_from_path(),
 * if it's not yet loaded, it begins loading the content of the folder
 * and emits "start-loading" signal. Most of the time, the info of the 
 * files in the folder becomes available only after the folder is fully 
 * loaded. That means, after the "finish-loading" signal is emitted.
 * Before the loading is finished, fm_folder_get_files() returns nothing.
 * You can tell if a folder is still being loaded with fm_folder_is_loaded().
 * 
 * However, for some special FmFolder types, such as the ones handling
 * search:// URIs, we want to access the file infos while the folder is
 * still being loaded (the search is still ongoing).
 * The content of the folder grows incrementally and fm_folder_get_files()
 * returns files currently being loaded even when the folder is not
 * fully loaded. This is what we called incremental.
 * fm_folder_is_incremental() tells you if the FmFolder has this feature.
 *
 * Returns: %TRUE if @folder is incrementally loaded
 *
 * Since: 1.0.2
 */
gboolean fm_folder_is_incremental(FmFolder* folder)
{
    return folder->wants_incremental;
}


/**
 * fm_folder_get_filesystem_info
 * @folder: folder to retrieve info
 * @total_size: pointer to counter of total size of the filesystem
 * @free_size: pointer to counter of free space on the filesystem
 *
 * Retrieves info about total and free space on the filesystem which
 * contains the @folder.
 *
 * Returns: %TRUE if information can be retrieved.
 *
 * Since: 0.1.16
 */
gboolean fm_folder_get_filesystem_info(FmFolder* folder, guint64* total_size, guint64* free_size)
{
    if(folder->has_fs_info)
    {
        *total_size = folder->fs_total_size;
        *free_size = folder->fs_free_size;
        return TRUE;
    }
    return FALSE;
}

/* this function is run in GIO thread! */
static void on_query_filesystem_info_finished(GObject *src, GAsyncResult *res, FmFolder* folder)
{
    GFile* gf = G_FILE(src);
    GError* err = NULL;
    GFileInfo* inf = g_file_query_filesystem_info_finish(gf, res, &err);
    if(!inf)
    {
        folder->fs_total_size = folder->fs_free_size = 0;
        folder->has_fs_info = FALSE;
        folder->fs_info_not_avail = TRUE;

        /* FIXME: examine unsupported filesystems */

        g_error_free(err);
        goto _out;
    }
    if(g_file_info_has_attribute(inf, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE))
    {
        folder->fs_total_size = g_file_info_get_attribute_uint64(inf, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE);
        folder->fs_free_size = g_file_info_get_attribute_uint64(inf, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
        folder->has_fs_info = TRUE;
    }
    else
    {
        folder->fs_total_size = folder->fs_free_size = 0;
        folder->has_fs_info = FALSE;
        folder->fs_info_not_avail = TRUE;
    }
    g_object_unref(inf);

_out:
    G_LOCK(query);
    if(folder->fs_size_cancellable)
    {
        g_object_unref(folder->fs_size_cancellable);
        folder->fs_size_cancellable = NULL;
    }

    folder->filesystem_info_pending = TRUE;
    G_UNLOCK(query);
    /* we have a reference borrowed by async query still */
    G_LOCK(lists);
    queue_update(folder);
    G_UNLOCK(lists);
    g_object_unref(folder);
}

/**
 * fm_folder_query_filesystem_info
 * @folder: folder to retrieve info
 *
 * Queries to retrieve info about filesystem which contains the @folder if
 * the filesystem supports such query.
 *
 * Since: 0.1.16
 */
void fm_folder_query_filesystem_info(FmFolder* folder)
{
    G_LOCK(query);
    if(!folder->fs_size_cancellable && !folder->fs_info_not_avail)
    {
        folder->fs_size_cancellable = g_cancellable_new();
        g_file_query_filesystem_info_async(folder->gf,
                G_FILE_ATTRIBUTE_FILESYSTEM_SIZE","
                G_FILE_ATTRIBUTE_FILESYSTEM_FREE,
                G_PRIORITY_LOW, folder->fs_size_cancellable,
                (GAsyncReadyCallback)on_query_filesystem_info_finished,
                g_object_ref(folder));
    }
    G_UNLOCK(query);
}

/**
 * fm_folder_find_by_path
 * @path: path descriptor
 *
 * Checks if folder by @path is already in use.
 *
 * Returns: (transfer full): found folder or %NULL.
 *
 * Since: 1.2.0
 */
FmFolder *fm_folder_find_by_path(FmPath *path)
{
    FmFolder *folder;

    G_LOCK(hash);
    folder = hash ? (FmFolder*)g_hash_table_lookup(hash, path) : NULL;
    G_UNLOCK(hash);
    return folder ? g_object_ref(folder) : NULL;
}

/**
 * fm_folder_block_updates
 * @folder: folder to apply
 *
 * Blocks emitting signals for changes in folder, i.e. if some file was
 * added, changed, or removed in folder after this API, no signal will be
 * sent until next call to fm_folder_unblock_updates().
 *
 * Since: 1.2.0
 */
void fm_folder_block_updates(FmFolder *folder)
{
    /* g_debug("fm_folder_block_updates %p", folder); */
    G_LOCK(lists);
    /* just set the flag */
    folder->stop_emission = TRUE;
    G_UNLOCK(lists);
}

/**
 * fm_folder_unblock_updates
 * @folder: folder to apply
 *
 * Unblocks emitting signals for changes in folder. If some changes were
 * in folder after previous call to fm_folder_block_updates() then these
 * changes will be sent after this call.
 *
 * Since: 1.2.0
 */
void fm_folder_unblock_updates(FmFolder *folder)
{
    /* g_debug("fm_folder_unblock_updates %p", folder); */
    G_LOCK(lists);
    folder->stop_emission = FALSE;
    /* query update now */
    queue_update(folder);
    G_UNLOCK(lists);
    /* g_debug("fm_folder_unblock_updates OK"); */
}

/**
 * fm_folder_make_directory
 * @folder: folder to apply
 * @name: display name for new directory
 * @error: (allow-none) (out): location to save error
 *
 * Creates new directory in given @folder.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 1.2.0
 */
gboolean fm_folder_make_directory(FmFolder *folder, const char *name, GError **error)
{
    GFile *dir, *gf;
    FmPath *path;
    gboolean ok;

    dir = fm_path_to_gfile(folder->dir_path);
    gf = g_file_get_child_for_display_name(dir, name, error);
    g_object_unref(dir);
    if (gf == NULL)
        return FALSE;
    ok = g_file_make_directory(gf, NULL, error);
    if (ok)
    {
        path = fm_path_new_for_gfile(gf);
        if (!_fm_folder_event_file_added(folder, path))
            fm_path_unref(path);
    }
    g_object_unref(gf);
    return ok;
}

static void fm_folder_content_changed(FmFolder* folder)
{
    if(folder->has_fs_info && !folder->fs_info_not_avail)
        fm_folder_query_filesystem_info(folder);
}

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
static void on_mount_added(GVolumeMonitor* vm, GMount* mount, gpointer user_data)
{
    /* If a filesystem is mounted over an existing folder,
     * we need to refresh the content of the folder to reflect
     * the changes. Besides, we need to create a new GFileMonitor
     * for the newly-mounted filesystem as the inode already changed.
     * GFileMonitor cannot detect this kind of changes caused by mounting.
     * So let's do it ourselves. */

    GFile* gfile = g_mount_get_root(mount);
    /* g_debug("FmFolder::mount_added"); */
    if(gfile)
    {
        GHashTableIter it;
        FmPath* path;
        FmFolder* folder;
        FmPath* mounted_path = fm_path_new_for_gfile(gfile);
        g_object_unref(gfile);

        G_LOCK(hash);
        g_hash_table_iter_init(&it, hash);
        while(g_hash_table_iter_next(&it, (gpointer*)&path, (gpointer*)&folder))
        {
            if(path == mounted_path)
                queue_reload(folder);
            else if(fm_path_has_prefix(path, mounted_path))
            {
                /* see if currently cached folders are below the mounted path.
                 * Folders below the mounted folder are removed.
                 * FIXME: should we emit "removed" signal for them, or 
                 * keep the folders and only reload them? */
                /* g_signal_emit(folder, signals[REMOVED], 0); */
                queue_reload(folder);
            }
        }
        G_UNLOCK(hash);
        fm_path_unref(mounted_path);
    }
}

static void on_mount_removed(GVolumeMonitor* vm, GMount* mount, gpointer user_data)
{
    /* g_debug("FmFolder::mount_removed"); */

    /* NOTE: gvfs does not emit unmount signals for remote folders since
     * GFileMonitor does not support remote filesystems at all. We do fake
     * file monitoring with FmDummyMonitor dirty hack.
     * So here is the side effect, no unmount notifications.
     * We need to generate the signal ourselves. */

    GFile* gfile = g_mount_get_root(mount);
    if(gfile)
    {
        GSList* dummy_monitor_folders = NULL, *l;
        GHashTableIter it;
        FmPath* path;
        FmFolder* folder;
        FmPath* mounted_path = fm_path_new_for_gfile(gfile);
        g_object_unref(gfile);

        G_LOCK(hash);
        g_hash_table_iter_init(&it, hash);
        while(g_hash_table_iter_next(&it, (gpointer*)&path, (gpointer*)&folder))
        {
            if(fm_path_has_prefix(path, mounted_path))
            {
                /* see if currently cached folders are below the mounted path.
                 * Folders below the mounted folder are removed. */
                if(FM_IS_DUMMY_MONITOR(folder->mon))
                    dummy_monitor_folders = g_slist_prepend(dummy_monitor_folders, folder);
            }
        }
        G_UNLOCK(hash);
        fm_path_unref(mounted_path);

        for(l = dummy_monitor_folders; l; l = l->next)
        {
            folder = FM_FOLDER(l->data);
            g_object_ref(folder);
            g_signal_emit_by_name(folder->mon, "changed", folder->gf, NULL, G_FILE_MONITOR_EVENT_UNMOUNTED);
            /* FIXME: should we emit a fake deleted event here? */
            /* g_signal_emit_by_name(folder->mon, "changed", folder->gf, NULL, G_FILE_MONITOR_EVENT_DELETED); */
            g_object_unref(folder);
        }
        g_slist_free(dummy_monitor_folders);
    }
}

void _fm_folder_init()
{
}

void _fm_folder_finalize()
{
}
