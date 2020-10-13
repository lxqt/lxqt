/*
 * fm-thumbnail-loader.c
 *
 * Copyright 2010 - 2013 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * Copyright 2012-2018 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 * This file is a part of the Libfm library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:fm-thumbnail-loader
 * @short_description: A thumbnails cache loader and generator.
 * @title: FmThumbnailLoader
 *
 * @include: libfm/fm.h
 *
 * This API allows to generate thumbnails for files and save them on
 * disk then use that cache next time to display them.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-thumbnail-loader.h"
#include "glib-compat.h"

#include "fm-config.h"
#include "fm-utils.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#ifdef USE_EXIF
#include <libexif/exif-loader.h>
#endif

/* #define ENABLE_DEBUG */
#ifdef ENABLE_DEBUG
#define DEBUG(...)  g_debug(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

#define THUMBNAILER_TIMEOUT_SEC     30

static gboolean backend_loaded = FALSE;
static FmThumbnailLoaderBackend backend = {NULL};

typedef enum
{
    LOAD_NORMAL = 1 << 0, /* need to load normal thumbnail */
    LOAD_LARGE = 1 << 1, /* need to load large thumbnail */
    GENERATE_NORMAL = 1 << 2, /* need to regenerated normal thumbnail */
    GENERATE_LARGE = 1 << 3, /* need to regenerated large thumbnail */
}ThumbnailTaskFlags;

typedef struct _ThumbnailTask ThumbnailTask;
struct _ThumbnailTask
{
    FmFileInfo* fi;         /* never changed between creation and destroying */
    ThumbnailTaskFlags flags; /* used internally */
    GCancellable *cancellable; /* NULL until work is started */
    char* uri;              /* used internally */
    char* normal_path;      /* used internally */
    char* large_path;       /* used internally */
    GList* requests;        /* access should be locked */
};
/* cancelled above raised when all requests are cancelled and never dropped again */

/* members of this structure cannot have concurrent access */
struct _FmThumbnailLoader
{
    FmFileInfo* fi;
    ThumbnailTask* task;
    FmThumbnailLoaderCallback callback;
    gpointer user_data;
    GObject* pix;
    sig_atomic_t cancelled;
    gshort size;
    gboolean done : 1; /* it has pix set so will be pushed into ready queue */
};

typedef struct _ThumbnailCacheItem ThumbnailCacheItem;
struct _ThumbnailCacheItem
{
    guint size;
    GObject* pix; /* no reference on it */
};

typedef struct _ThumbnailCache ThumbnailCache;
struct _ThumbnailCache
{
    FmPath* path;
    GSList* items;
};

/* FIXME: use thread pool */

/* Lock for loader, generator, and ready queues */
#if GLIB_CHECK_VERSION(2, 32, 0)
static GMutex queue_lock;
#define lock_ptr &queue_lock
static GCond queue_cond;
#define cond_ptr &queue_cond
#else
static GMutex *queue_lock;
#define lock_ptr queue_lock
static GCond *queue_cond;
#define cond_ptr queue_cond
#endif

/* load generated thumbnails */
static GQueue loader_queue = G_QUEUE_INIT; /* consists of ThumbnailTask */
static gboolean loader_thread_running = FALSE;

/* already loaded thumbnails */
static GQueue ready_queue = G_QUEUE_INIT; /* consists of FmThumbnailLoader */
/* idle handler to call ready callback */
static guint ready_idle_handler = 0;

/* cached thumbnails, elements are ThumbnailCache* */
static GHashTable* hash = NULL;

static char* thumb_dir = NULL;

static guint thumbnailer_timeout_id = 0;

static gpointer load_thumbnail_thread(gpointer user_data);
static void load_thumbnails(ThumbnailTask* task);
static void generate_thumbnails(ThumbnailTask* task);
static gboolean generate_thumbnails_with_builtin(ThumbnailTask* task);
static void generate_thumbnails_with_thumbnailers(ThumbnailTask* task);
static GObject* scale_pix(GObject* ori_pix, int size);
static void save_thumbnail_to_disk(ThumbnailTask* task, GObject* pix, const char* path);

/* may be called in thread */
static void fm_thumbnail_loader_free(FmThumbnailLoader* req)
{
    fm_file_info_unref(req->fi);
    if(req->pix)
        g_object_unref(req->pix);
    g_slice_free(FmThumbnailLoader, req);
}

/* in main loop */
static gboolean on_ready_idle(gpointer user_data)
{
    FmThumbnailLoader* req;
    int n = 200; /* max 200 thumbnails in a row */
    g_mutex_lock(lock_ptr);
    while((req = (FmThumbnailLoader*)g_queue_pop_head(&ready_queue)) != NULL)
    {
        g_mutex_unlock(lock_ptr);
        if(!req->cancelled)
            req->callback(req, req->user_data);
        fm_thumbnail_loader_free(req);
        if(--n == 0)
            return TRUE; /* continue on next idle */
        g_mutex_lock(lock_ptr);
    }
    ready_idle_handler = 0;
    g_mutex_unlock(lock_ptr);
    return FALSE;
}

/* should be called with queue lock held */
/* may be called in thread */
/* moves all requests into ready_queue */
inline static void thumbnail_task_free(ThumbnailTask* task)
{
    GList *l;

    for(l = task->requests; l; l = l->next)
    {
        FmThumbnailLoader* req = (FmThumbnailLoader*)l->data;
        req->task = NULL;
        g_queue_push_tail(&ready_queue, req);
        if( 0 == ready_idle_handler ) /* schedule an idle handler if there isn't one. */
            ready_idle_handler = g_idle_add_full(G_PRIORITY_LOW, on_ready_idle, NULL, NULL);
    }
    /* task requests are completely in ready queue now */
    if(task->requests)
        g_list_free(task->requests);
    fm_file_info_unref(task->fi);
    if (task->cancellable)
        g_object_unref(task->cancellable);
    g_slice_free(ThumbnailTask, task);
}

static gint comp_request(gconstpointer a, gconstpointer b)
{
    return ((FmThumbnailLoader*)a)->size - ((FmThumbnailLoader*)b)->size;
}

/* called when cached pixbuf get destroyed */
static void on_pixbuf_destroy(gpointer data, GObject* obj_ptr)
{
    ThumbnailCache* cache = (ThumbnailCache*)data;
    GObject* pix = (GObject*)obj_ptr;
    GSList* l;
    /* remove it from cache */
    DEBUG("remove from cache!");
    g_mutex_lock(lock_ptr);
    for(l=cache->items;l;l=l->next)
    {
        ThumbnailCacheItem* item = (ThumbnailCacheItem*)l->data;
        if(item->pix == pix)
        {
            cache->items = g_slist_delete_link(cache->items, l);
            g_slice_free(ThumbnailCacheItem, item);
            if(!cache->items)
            {
                if(hash) /* it could be already destroyed */
                    g_hash_table_remove(hash, cache->path);
                fm_path_unref(cache->path);
                g_slice_free(ThumbnailCache, cache);
            }
            break;
        }
    }
    g_mutex_unlock(lock_ptr);
}

/* called with queue lock held */
/* in thread */
inline static void cache_thumbnail_in_hash(FmPath* path, GObject* pix, guint size)
{
    ThumbnailCache* cache;
    ThumbnailCacheItem* item;
    GSList* l = NULL;
    cache = (ThumbnailCache*)g_hash_table_lookup(hash, path);
    if(cache)
    {
        for(l=cache->items;l;l=l->next)
        {
            item = (ThumbnailCacheItem*)l->data;
            if(item->size == size)
                break;
        }
    }
    else
    {
        cache = g_slice_new0(ThumbnailCache);
        cache->path = fm_path_ref(path);
        g_hash_table_insert(hash, cache->path, cache);
    }
    if(!l) /* the item is not in cache->items */
    {
        item = g_slice_new(ThumbnailCacheItem);
        item->size = size;
        item->pix = pix;
        cache->items = g_slist_prepend(cache->items, item);
        g_object_weak_ref(G_OBJECT(pix), on_pixbuf_destroy, cache);
    }
}

/* in thread */
static void thumbnail_task_finish(ThumbnailTask* task, GObject* normal_pix, GObject* large_pix)
{
    GObject* cached_pix = NULL;
    gint cached_size = 0;
    GList* l;

    /* sort the requests by requested size to utilize cached scaled pixbuf */
    g_mutex_lock(lock_ptr);
    task->requests = g_list_sort(task->requests, comp_request);
    for(l=task->requests; l; l=l->next)
    {
        FmThumbnailLoader* req = (FmThumbnailLoader*)l->data;
        /* the thumbnail is ready, queue the request in ready queue. */
        /* later, the ready callbacks will be called in idle handler of main thread. */
        if(req->done)
            continue;
        if(req->cancelled)
            continue;
        if(req->size == cached_size)
        {
            req->pix = cached_pix ? (GObject*)g_object_ref(cached_pix) : NULL;
            DEBUG("cache hit!");
            goto push_it;
        }

        g_mutex_unlock(lock_ptr);
        if(G_LIKELY(req->size <= 128)) /* normal */
        {
            if(normal_pix)
                req->pix = scale_pix(normal_pix, req->size);
            else
                req->pix = NULL;
        }
        else /* large */
        {
            if(large_pix)
                req->pix = scale_pix(large_pix, req->size);
            else
                req->pix = NULL;
        }

        if(cached_pix)
            g_object_unref(cached_pix);
        cached_pix = req->pix ? g_object_ref(req->pix) : NULL;
        cached_size = req->size;

        g_mutex_lock(lock_ptr);
        /* cache this in hash table */
        if(cached_pix)
            cache_thumbnail_in_hash(fm_file_info_get_path(req->fi), cached_pix, cached_size);
        else
            continue;

push_it:
        req->done = TRUE;
    }
    g_mutex_unlock(lock_ptr);
    if(cached_pix)
        g_object_unref(cached_pix);
}

/* in thread */
static gboolean is_thumbnail_outdated(GObject* thumb_pix, const char* thumbnail_path, time_t mtime)
{
    char* thumb_mtime = backend.get_image_text(thumb_pix, "tEXt::Thumb::MTime");
    gboolean outdated = FALSE;
    /* FIXME: compare thumbnail URI with value from tEXt::Thumb::URI */
    /* g_print("thumb_mtime: %s, %ld\n", thumb_mtime, mtime); */
    if(thumb_mtime)
    {
        if(atol(thumb_mtime) != mtime)
            outdated = TRUE;
        g_free(thumb_mtime);
    }
    else
    {
        /* if the thumbnail png file does not contain "tEXt::Thumb::MTime" value,
         * we compare the mtime of the thumbnail with its original directly. */
        /* FIXME: XDG specification requires to regenerate it in such case */
        struct stat statbuf;
        if(stat(thumbnail_path, &statbuf) == 0) /* get mtime of the thumbnail file */
        {
            if(mtime > statbuf.st_mtime)
                outdated = TRUE;
        }
    }

    /* out of date, delete it */
    if(outdated)
    {
        unlink(thumbnail_path); /* delete the out-dated thumbnail. */
        g_object_unref(thumb_pix);
    }
    return outdated;
}

/* in thread */
static void load_thumbnails(ThumbnailTask* task)
{
    GObject* normal_pix = NULL;
    GObject* large_pix = NULL;
    const char* normal_path = task->normal_path;
    const char* large_path = task->large_path;

    if( g_cancellable_is_cancelled(task->cancellable) )
        goto _out;

    DEBUG("loading: %s, %s, %s", fm_file_info_get_name(task->fi), normal_path, large_path);

    if(task->flags & LOAD_NORMAL)
    {
        normal_pix = backend.read_image_from_file(normal_path);
        if(!normal_pix || is_thumbnail_outdated(normal_pix, normal_path, fm_file_info_get_mtime(task->fi)))
        {
            /* normal_pix is freed in is_thumbnail_outdated() if it's out of date. */
            /* generate normal size thumbnail */
            task->flags |= GENERATE_NORMAL;
            normal_pix = NULL;
            /* DEBUG("need to generate normal thumbnail"); */
        }
        else
        {
            DEBUG("normal thumbnail loaded: %p", normal_pix);
        }
    }

    if( g_cancellable_is_cancelled(task->cancellable) )
        goto _out;

    if(task->flags & LOAD_LARGE)
    {
        large_pix = backend.read_image_from_file(large_path);
        if(!large_pix || is_thumbnail_outdated(large_pix, large_path, fm_file_info_get_mtime(task->fi)))
        {
            /* large_pix is freed in is_thumbnail_outdated() if it's out of date. */
            /* generate large size thumbnail */
            task->flags |= GENERATE_LARGE;
            large_pix = NULL;
        }
    }

    /* thumbnails which don't require re-generation should all be loaded at this point. */
    if(!g_cancellable_is_cancelled(task->cancellable) && task->requests)
        thumbnail_task_finish(task, normal_pix, large_pix);

_out:
    if(normal_pix)
        g_object_unref(normal_pix);
    if(large_pix)
        g_object_unref(large_pix);

    return;
}

/* in thread */
static gpointer load_thumbnail_thread(gpointer user_data)
{
    ThumbnailTask* task;
    GChecksum* sum = g_checksum_new(G_CHECKSUM_MD5);
    gchar* normal_path  = g_build_filename(thumb_dir, "normal/00000000000000000000000000000000.png", NULL);
    gchar* normal_basename = strrchr(normal_path, '/') + 1;
    gchar* large_path = g_build_filename(thumb_dir, "large/00000000000000000000000000000000.png", NULL);
    gchar* large_basename = strrchr(large_path, '/') + 1;

    /* ensure thumbnail directories exists */
    *(normal_basename - 1) = '\0';
    g_mkdir_with_parents(normal_path, 0700);
    *(normal_basename - 1) = '/';

    *(large_basename - 1) = '\0';
    g_mkdir_with_parents(large_path, 0700);
    *(large_basename - 1) = '/';

    for(;;)
    {
        g_mutex_lock(lock_ptr);
        task = g_queue_pop_head(&loader_queue);
        if(G_LIKELY(task))
        {
            char* uri;
            const char* md5;
            GList *reql;

            for (reql = task->requests; reql; reql = reql->next)
                if (!((FmThumbnailLoader*)reql->data)->cancelled)
                    break;
            if (reql == NULL) /* all requests were cancelled already */
                goto _free_task;
            if (!task->cancellable)
                task->cancellable = g_cancellable_new();
            g_mutex_unlock(lock_ptr);
            uri = fm_path_to_uri(fm_file_info_get_path(task->fi));

            /* generate filename for the thumbnail */
            g_checksum_update(sum, (guchar*)uri, -1);
            md5 = g_checksum_get_string(sum); /* md5 sum of the URI */

            task->uri = uri;

            if (task->flags & LOAD_NORMAL)
            {
                memcpy( normal_basename, md5, 32 );
                task->normal_path = normal_path;
            }
            if (task->flags & LOAD_LARGE)
            {
                memcpy( large_basename, md5, 32 );
                task->large_path = large_path;
            }
            /* FIXME: support fail/<PRG>/<MD5>.png to skip creation */

            if(task->flags & (GENERATE_NORMAL|GENERATE_LARGE))
                generate_thumbnails(task); /* second cycle */
            else
                load_thumbnails(task); /* first cycle */

            g_checksum_reset(sum);
            task->uri = NULL;
            task->normal_path = NULL;
            task->large_path = NULL;
            g_free(uri);

            g_mutex_lock(lock_ptr);

            if(g_cancellable_is_cancelled(task->cancellable) /* task is done */
               || (task->flags & (GENERATE_NORMAL|GENERATE_LARGE)) == 0)
_free_task:
                thumbnail_task_free(task);
            else
                g_queue_push_tail(&loader_queue, task); /* return it to regen */

            g_mutex_unlock(lock_ptr);
        }
        else /* no task is left in the loader_queue */
        {
            loader_thread_running = FALSE;
            g_mutex_unlock(lock_ptr);
            g_free(normal_path);
            g_free(large_path);
            g_checksum_free(sum);
#if GLIB_CHECK_VERSION(2, 32, 0)
            g_thread_unref(g_thread_self());
#endif
            return NULL;
        }
    }
}

/* should be called with queue locked */
/* in main loop */
inline static GObject* find_thumbnail_in_hash(FmPath* path, guint size)
{
    ThumbnailCache* cache = (ThumbnailCache*)g_hash_table_lookup(hash, path);
    if(cache)
    {
        GSList* l;
        for(l=cache->items;l;l=l->next)
        {
            ThumbnailCacheItem* item = (ThumbnailCacheItem*)l->data;
            if(item->size == size)
                return item->pix;
        }
    }
    return NULL;
}

/* should be called with queue locked */
/* may be called in thread */
static ThumbnailTask* find_queued_task(GQueue* queue, FmFileInfo* fi)
{
    GList* l;
    for( l = queue->head; l; l=l->next )
    {
        ThumbnailTask* task = (ThumbnailTask*)l->data;
        /* if it's processing then it's too late to add */
        if(task->cancellable)
            continue;
        if(G_UNLIKELY(task->fi == fi || fm_path_equal(fm_file_info_get_path(task->fi), fm_file_info_get_path(fi))))
            return task;
    }
    return NULL;
}

/**
 * fm_thumbnail_loader_load
 * @src_file: an image file
 * @size: thumbnail size
 * @callback: callback to requestor
 * @user_data: data provided for @callback
 *
 * Schedules loading/generation of thumbnail for @src_file. If the
 * request isn't cancelled then ready thumbnail will be given to the
 * requestor in @callback. Returned descriptor can be used to cancel
 * the job.
 *
 * Returns: (transfer none): request descriptor.
 *
 * Since: 1.2.0
 */
/* in main loop */
FmThumbnailLoader* fm_thumbnail_loader_load(FmFileInfo* src_file,
                                            guint size,
                                            FmThumbnailLoaderCallback callback,
                                            gpointer user_data)
{
    FmThumbnailLoader* req;
    ThumbnailTask* task;
    GObject* pix;
    FmPath* src_path = fm_file_info_get_path(src_file);
    gboolean thread_running;

    g_return_val_if_fail(hash != NULL, NULL);
    g_assert(callback != NULL);
    req = g_slice_new(FmThumbnailLoader);
    req->fi = fm_file_info_ref(src_file);
    req->size = size;
    req->callback = callback;
    req->user_data = user_data;
    req->pix = NULL;
    req->task = NULL;
    req->done = FALSE;
    req->cancelled = FALSE;

    DEBUG("request thumbnail: %s", fm_path_get_basename(src_path));

    g_mutex_lock(lock_ptr);

    /* find in the cache first to see if thumbnail is already cached */
    pix = find_thumbnail_in_hash(src_path, size);
    if(pix)
    {
        DEBUG("cache found!");
        req->pix = (GObject*)g_object_ref(pix);
        /* call the ready callback in main loader_thread_id from idle handler. */
        g_queue_push_tail(&ready_queue, req);
        if( 0 == ready_idle_handler ) /* schedule an idle handler if there isn't one. */
            ready_idle_handler = g_idle_add_full(G_PRIORITY_LOW, on_ready_idle, NULL, NULL);
        g_mutex_unlock(lock_ptr);
        return req;
    }

    /* if it's not cached, add it to the loader_queue for loading. */
    task = find_queued_task(&loader_queue, src_file);

    if(!task)
    {
        task = g_slice_new0(ThumbnailTask);
        task->fi = fm_file_info_ref(src_file);
        g_queue_push_tail(&loader_queue, task);
    }
    else
    {
        DEBUG("task already in the queue: %p", task);
    }
    req->task = task;

    if(size > 128)
        task->flags |= LOAD_LARGE;
    else
        task->flags |= LOAD_NORMAL;

    task->requests = g_list_append(task->requests, req);

    thread_running = loader_thread_running;
    loader_thread_running = TRUE;
    g_mutex_unlock(lock_ptr);

    if(!thread_running)
#if GLIB_CHECK_VERSION(2, 32, 0)
        g_thread_new("loader", load_thumbnail_thread, NULL);
        /* we don't use loader_thread_id but Glib 2.32 crashes if we unref
           GThread while it's in creation progress. It is a bug of GLib
           certainly but as workaround we'll unref it in the thread itself */
#else
        g_thread_create( load_thumbnail_thread, NULL, FALSE, NULL);
#endif

    return req;
}

/**
 * fm_thumbnail_loader_cancel
 * @req: the request descriptor
 *
 * Cancels request. After return from this call the @req becomes invalid
 * and cannot be used. Caller will never get callback for cancelled
 * request either.
 *
 * Since: 1.2.0
 */
/* in main loop */
void fm_thumbnail_loader_cancel(FmThumbnailLoader* req)
{
    GList* l;

    g_return_if_fail(req != NULL);

    g_mutex_lock(lock_ptr);
    req->cancelled = TRUE;

    if(req->task == NULL)
        goto done;

    for(l = req->task->requests; l; l = l->next)
    {
        req = (FmThumbnailLoader*)l->data;
        if(!req->cancelled)
            break;
    }
    if(l == NULL && req->task->cancellable != NULL)
    {
        g_cancellable_cancel(req->task->cancellable);
        DEBUG("cancelling the task");
    }

done:
    g_mutex_unlock(lock_ptr);
    g_cond_broadcast(cond_ptr); /* if it is loading right now then let it die */
}

/**
 * fm_thumbnail_loader_get_data
 * @req: request descriptor
 *
 * Retrieves loaded thumbnail. Returned data are owned by @req and should
 * be not freed by caller.
 *
 * Returns: (transfer none): thumbnail.
 *
 * Since: 1.2.0
 */
/* in main loop */
GObject* fm_thumbnail_loader_get_data(FmThumbnailLoader* req)
{
    return req->pix;
}

/**
 * fm_thumbnail_loader_get_file_info
 * @req: request descriptor
 *
 * Retrieves file descriptor that request is for. Returned data are
 * owned by @req and should be not freed by caller.
 *
 * Returns: (transfer none): file descriptor.
 *
 * Since: 1.2.0
 */
/* in main loop */
FmFileInfo* fm_thumbnail_loader_get_file_info(FmThumbnailLoader* req)
{
    return req->fi;
}

/**
 * fm_thumbnail_loader_get_size
 * @req: request descriptor
 *
 * Retrieves thumbnail size that request is for.
 *
 * Returns: size in pixels.
 *
 * Since: 1.2.0
 */
/* in main loop */
guint fm_thumbnail_loader_get_size(FmThumbnailLoader* req)
{
    return req->size;
}

/* in main loop */
void _fm_thumbnail_loader_init()
{
    thumb_dir = g_build_filename(fm_get_home_dir(), ".thumbnails", NULL);
    hash = g_hash_table_new((GHashFunc)fm_path_hash, (GEqualFunc)fm_path_equal);
#if !GLIB_CHECK_VERSION(2, 32, 0)
    lock_ptr = g_mutex_new();
    cond_ptr = g_cond_new();
#endif
}

static gboolean fm_thumbnail_loader_cleanup(gpointer unused)
{
    FmThumbnailLoader* req;

    /* loader_queue is empty and cur_loading is finished */
    while((req = g_queue_pop_head(&ready_queue)))
        fm_thumbnail_loader_free(req);
    g_hash_table_destroy(hash); /* caches will be destroyed by pixbufs */
    hash = NULL;
    g_free(thumb_dir);
    thumb_dir = NULL;
    return FALSE;
}

/* in main loop */
void _fm_thumbnail_loader_finalize(void)
{
    ThumbnailTask* task;
    GList *qlist, *rlist;

    g_mutex_lock(lock_ptr);
    /* cancel all pending requests before destroying hash */
    for (qlist = g_queue_peek_head_link(&loader_queue); qlist; qlist = qlist->next)
    {
        task = qlist->data;
        if (task->cancellable)
            g_cancellable_cancel(task->cancellable);
        for (rlist = task->requests; rlist; rlist = rlist->next)
            //g_assert(!((FmThumbnailLoader*)rlist->data)->cancelled);
            ((FmThumbnailLoader*)rlist->data)->cancelled = TRUE;
    }
    g_mutex_unlock(lock_ptr);
    /* if thread was alive it will die after that */
    g_cond_broadcast(cond_ptr);
    g_mutex_lock(lock_ptr);
    while (loader_thread_running)
        g_cond_wait(cond_ptr, lock_ptr);
    g_mutex_unlock(lock_ptr);
#if !GLIB_CHECK_VERSION(2, 32, 0)
    g_mutex_free(lock_ptr);
    g_cond_free(cond_ptr);
#endif
    while((task = g_queue_pop_head(&loader_queue)))
        thumbnail_task_free(task);
    fm_thumbnail_loader_cleanup(NULL);
}

/* in thread */
static void generate_thumbnails(ThumbnailTask* task)
{
    if (fm_file_info_is_image(task->fi) &&
        /* if the image file is too large, don't generate thumbnail for it. */
        (fm_config->thumbnail_max == 0 ||
         fm_file_info_get_size(task->fi) <= (fm_config->thumbnail_max << 10)))
    {
        generate_thumbnails_with_builtin(task);
    }
    else
        generate_thumbnails_with_thumbnailers(task);

    /* mark it as fully done, see thread loop */
    g_cancellable_cancel(task->cancellable);
}

/* in thread */
static GObject* scale_pix(GObject* ori_pix, int size)
{
    GObject* scaled_pix;
    /* keep aspect ratio and scale to thumbnail size: 128 or 512 */
    int width = backend.get_image_width(ori_pix);
    int height = backend.get_image_height(ori_pix);
    int new_width;
    int new_height;

    if(width > height)
    {
        gdouble aspect = (gdouble)height / width;
        new_width = size;
        new_height = size * aspect;
    }
    else if(width < height)
    {
        gdouble aspect = (gdouble)width / height;
        new_height = size;
        new_width = size * aspect;
    }
    else
    {
        new_width = new_height = size;
    }

    if((new_width == width && new_height == height) ||
       (size > width && size > height )) /* don't scale up */
    {
        /* if size is not changed or original size is smaller, use original size. */
        scaled_pix = (GObject*)g_object_ref(ori_pix);
    }
    else
    {
        /* avoid width or height of 0 pixel.
         * FIXME: or we should just fail creating the thumbnail for the image? */
        if(new_width == 0)
            new_width = 1;
        if(new_height == 0)
            new_height = 1;
        scaled_pix = backend.scale_image(ori_pix, new_width, new_height);
    }

    return scaled_pix;
}

/* in thread */
static void save_thumbnail_to_disk(ThumbnailTask* task, GObject* pix, const char* path)
{
    /* do not save thumbnails generated in thumbail cache directory
     * (prevents runaway thumbnailing when browsing thumbail cache directory) */
    if(strncmp(path,thumb_dir,strlen(thumb_dir)) == 0)
    {
        return;
    }
    /* save the generated thumbnail to disk */
    char* tmpfile = g_strconcat(path, ".XXXXXX", NULL);
    gint fd;
    fd = g_mkstemp(tmpfile); /* save to a temp file first */
    if(fd != -1)
    {
        char mtime_str[100];
        g_snprintf( mtime_str, 100, "%lu", fm_file_info_get_mtime(task->fi));
        chmod( tmpfile, 0600 );  /* only the owner can read it. */
        backend.set_image_text(pix, "tEXt::Thumb::URI", task->uri);
        backend.set_image_text(pix, "tEXt::Thumb::MTime", mtime_str);
        backend.write_image(pix, tmpfile);
        close(fd);
        g_rename(tmpfile, path);
        g_free(tmpfile);
    }
    DEBUG("generator: save to %s", path);
}

/* in thread */
static gboolean generate_thumbnails_with_builtin(ThumbnailTask* task)
{
    /* FIXME: only formats supported by GObject should be handled this way. */
    GFile* gf = fm_path_to_gfile(fm_file_info_get_path(task->fi));
    char *file_name;
    GObject* normal_pix = NULL;
    GObject* large_pix = NULL;
    GCancellable *cancellable = task->cancellable;

    DEBUG("generate thumbnail for %s", fm_file_info_get_name(task->fi));

    GObject* ori_pix = NULL;
    int rotate_degrees = 0;
#ifdef USE_EXIF
    FmMimeType* mime_type;

    /* use libexif to extract thumbnails embedded in jpeg files */
    mime_type = fm_file_info_get_mime_type(task->fi);
    if(strcmp(fm_mime_type_get_type(mime_type), "image/jpeg") == 0) /* if this is a jpeg file */
    {
        /* try to extract thumbnails embedded in jpeg files */
        ExifLoader *exif_loader = exif_loader_new();
        ExifData *exif_data;
        GFileInputStream* ins = g_file_read(gf, cancellable, NULL);
        if(ins)
        {
            while(!g_cancellable_is_cancelled(cancellable)) {
                unsigned char buf[4096];
                gssize read_size = g_input_stream_read((GInputStream*)ins, buf, 4096, cancellable, NULL);
                if(read_size == 0) /* EOF */
                    break;
                if(exif_loader_write(exif_loader, buf, read_size) == 0)
                    break; /* no more EXIF data */
            }
            exif_data = exif_loader_get_data(exif_loader);
            exif_loader_unref(exif_loader);
            if(exif_data)
            {
                /* reference for EXIF orientation tag:
                 * http://www.impulseadventure.com/photo/exif-orientation.html */
                ExifEntry* orient_ent = exif_data_get_entry(exif_data, EXIF_TAG_ORIENTATION);
                if(orient_ent) /* orientation flag found in EXIF */
                {
                    gushort orient;
                    ExifByteOrder bo = exif_data_get_byte_order(exif_data);
                    /* bo == EXIF_BYTE_ORDER_INTEL ; */
                    orient = exif_get_short (orient_ent->data, bo);
                    switch(orient) {
                    case 1: /* no rotation */
                        rotate_degrees = 0;
                        break;
                    case 8:
                        rotate_degrees = 90;
                        break;
                    case 3:
                        rotate_degrees = 180;
                        break;
                    case 6:
                        rotate_degrees = 270;
                        break;
                    }
                    /* g_print("orientation flag found, rotate: %d\n", rotate_degrees); */
                }
                if(exif_data->data) /* if an embedded thumbnail is available */
                {
                    /* load the embedded jpeg thumbnail */
                    GInputStream* mem_stream = g_memory_input_stream_new_from_data(exif_data->data, exif_data->size, NULL);
                    ori_pix = backend.read_image_from_stream(mem_stream, exif_data->size, cancellable);
                    g_object_unref(mem_stream);
                }
                exif_data_unref(exif_data);
            }
            g_input_stream_close(G_INPUT_STREAM(ins), NULL, NULL);
            g_object_unref(ins);
        }
    }

    if(!ori_pix)
    {
#endif
        file_name = g_file_get_path(gf);
        if (file_name)
            ori_pix = backend.read_image_from_file(file_name);
        g_free(file_name);
#ifdef USE_EXIF
    }
#endif

    if(ori_pix) /* if the original image is successfully loaded */
    {
        int width = backend.get_image_width(ori_pix);
        int height = backend.get_image_height(ori_pix);
        gboolean need_save;

        if(task->flags & GENERATE_NORMAL)
        {
            /* don't create thumbnails for images which are too small */
            if(width <=128 && height <= 128)
            {
                normal_pix = (GObject*)g_object_ref(ori_pix);
                need_save = FALSE;
            }
            else
            {
                normal_pix = scale_pix(ori_pix, 128);
                need_save = TRUE;
            }
            if(rotate_degrees != 0) // rotate the image by EXIF oritation
            {
                GObject* rotated;
                rotated = backend.rotate_image(normal_pix, rotate_degrees);
                g_object_unref(normal_pix);
                normal_pix = rotated;
            }
            if(need_save && normal_pix)
                save_thumbnail_to_disk(task, normal_pix, task->normal_path);
        }

        if(task->flags & GENERATE_LARGE)
        {
            /* don't create thumbnails for images which are too small */
            if (width <= 512 && height <= 512)
            {
                large_pix = (GObject*)g_object_ref(ori_pix);
                need_save = FALSE;
            }
            else
            {
                large_pix = scale_pix(ori_pix, 512);
                need_save = TRUE;
            }
            if(rotate_degrees != 0)
            {
                GObject* rotated;
                rotated = backend.rotate_image(large_pix, rotate_degrees);
                g_object_unref(large_pix);
                large_pix = rotated;
            }
            if(need_save && large_pix)
                save_thumbnail_to_disk(task, large_pix, task->large_path);
        }
        g_object_unref(ori_pix);
    }
    else
    {
        g_object_unref(gf);
        /* g_debug("failed to generate thumbnail internally, revert to external"); */
        return FALSE;
    }

    thumbnail_task_finish(task, normal_pix, large_pix);

    if(normal_pix)
        g_object_unref(normal_pix);
    if(large_pix)
        g_object_unref(large_pix);

    g_object_unref(gf);
    return TRUE;
}

/* call from main thread */
static gboolean on_thumbnailer_timeout(gpointer user_data)
{
    gboolean *timed_out;

    /* check if it is destroyed already */
    if(g_source_is_destroyed(g_main_current_source()))
        return FALSE;
    timed_out = user_data;
    /* g_print("thumbnail timeout!\n"); */
    g_mutex_lock(lock_ptr);
    *timed_out = TRUE;
    thumbnailer_timeout_id = 0;
    g_mutex_unlock(lock_ptr);
    g_cond_broadcast(cond_ptr);
    return FALSE;
}

typedef struct
{
    gboolean finished;
    int status;
} ThumbnailerStatus;

/* this is in main loop due to g_child_watch_add() */
static void _pid_watcher(GPid pid, gint status, gpointer user_data)
{
    ThumbnailerStatus *st = user_data;

    DEBUG("pid %d terminated", (int)pid);
    st->status = status;
    st->finished = TRUE;
    g_cond_broadcast(cond_ptr);
}

/* call from the thumbnail thread */
static gboolean run_thumbnailer(FmThumbnailer* thumbnailer, ThumbnailTask* task,
                                const char* output_file, guint size)
{
    /* g_print("run_thumbnailer: uri: %s\n", uri); */
    ThumbnailerStatus status = { FALSE, 0 };
    gboolean timed_out = FALSE;
    GPid _pid = fm_thumbnailer_launch_for_uri_async(thumbnailer, task->uri,
                                                    output_file, size, NULL);
    if(_pid <= 0) /* failed to launch */
        /* FIXME: print error message from failed thumbnailer */
        return FALSE;
    g_mutex_lock(lock_ptr);
    if(thumbnailer_timeout_id)
    {
        g_mutex_unlock(lock_ptr);
        g_critical("libfm: run_thumbnailer() concurrent process attempt");
        kill(_pid, SIGTERM);
        return FALSE;
    }
    thumbnailer_timeout_id = g_timeout_add_seconds(THUMBNAILER_TIMEOUT_SEC,
                                                   on_thumbnailer_timeout, &timed_out);
    g_child_watch_add(_pid, _pid_watcher, &status);
    /* g_print("pid: %d\n", thumbnailer_pid); */
    while (!timed_out && !status.finished &&
           !g_cancellable_is_cancelled(task->cancellable))
        g_cond_wait(cond_ptr, lock_ptr);
    if (thumbnailer_timeout_id)
        g_source_remove(thumbnailer_timeout_id);
    thumbnailer_timeout_id = 0;
    if (!status.finished)
        kill(_pid, SIGTERM);
    /* wait for the thumbnailer process to terminate */
    while (!status.finished)
        g_cond_wait(cond_ptr, lock_ptr);
    g_mutex_unlock(lock_ptr);

    /* the process is terminated */
    return (WIFEXITED(status.status) && WEXITSTATUS(status.status) == 0);
}

/* in thread */
static void generate_thumbnails_with_thumbnailers(ThumbnailTask* task)
{
    /* external thumbnailer support */
    GObject* normal_pix = NULL;
    GObject* large_pix = NULL;
    FmMimeType* mime_type = fm_file_info_get_mime_type(task->fi);
    /* TODO: we need to add timeout for external thumbnailers.
     * If a thumbnailer program is broken or locked for unknown reason,
     * the thumbnailer process should be killed once a timeout is reached. */
    if(mime_type)
    {
        GList* thumbnailers = fm_mime_type_get_thumbnailers_list(mime_type);
        GList* l;
        guint generated = 0;
        /* g_debug("run thumbnailer: %s, %s, %s", fm_file_info_get_name(task->fi), task->normal_path, task->large_path); */
        for(l = thumbnailers; l; l = l->next)
        {
            FmThumbnailer* thumbnailer = FM_THUMBNAILER(l->data);
            if((task->flags & GENERATE_NORMAL) && !(generated & GENERATE_NORMAL))
            {
                if(run_thumbnailer(thumbnailer, task, task->normal_path, 128))
                {
                    generated |= GENERATE_NORMAL;
                    normal_pix = backend.read_image_from_file(task->normal_path);
                    if (normal_pix)
                    {
                        char *thumb_mtime = backend.get_image_text(normal_pix, "tEXt::Thumb::MTime");
                        /* Re-save generated thumbnail to have required data
                           in them. Some external thumbnailers not follow the
                           specification and not set any of Thumb::URI nor
                           Thumb::MTime, that leads to regeneration each time. */
                        if (thumb_mtime == NULL)
                            save_thumbnail_to_disk(task, normal_pix, task->normal_path);
                        else
                            g_free(thumb_mtime);
                    }
                }
            }
            if((task->flags & GENERATE_LARGE) && !(generated & GENERATE_LARGE))
            {
                if (run_thumbnailer(thumbnailer, task, task->large_path, 512))
                {
                    generated |= GENERATE_LARGE;
                    large_pix = backend.read_image_from_file(task->large_path);
                    if (large_pix)
                    {
                        char *thumb_mtime = backend.get_image_text(large_pix, "tEXt::Thumb::MTime");
                        if (thumb_mtime == NULL)
                            save_thumbnail_to_disk(task, large_pix, task->large_path);
                        else
                            g_free(thumb_mtime);
                    }
                }
            }

            /* if both large and normal thumbnails are generated, quit */
            if(generated == task->flags)
                break;
        }
        g_list_free_full(thumbnailers, (GDestroyNotify)fm_thumbnailer_unref);
    }
    thumbnail_task_finish(task, normal_pix, large_pix);

    if(normal_pix)
        g_object_unref(normal_pix);
    if(large_pix)
        g_object_unref(large_pix);
}

/**
 * fm_thumbnail_loader_set_backend
 * @_backend: callbacks list to set
 *
 * Sets callbacks list for further usage by thumbnail loader. Callbacks
 * should implement operations with image representation for application
 * specific model. This function can be called only once per application
 * and every subsequent call will return %FALSE and change nothing.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 1.2.0
 */
gboolean fm_thumbnail_loader_set_backend(FmThumbnailLoaderBackend* _backend)
{
    if(backend_loaded)
        return FALSE;
    backend = *_backend;
    backend_loaded = TRUE;
    return TRUE;
}
