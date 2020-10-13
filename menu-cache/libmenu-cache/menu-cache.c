/*
 *      menu-cache.c
 *
 *      Copyright 2008 PCMan <pcman.tw@gmail.com>
 *      Copyright 2009 Jürgen Hötzel <juergen@archlinux.org>
 *      Copyright 2012-2017 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "version.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <sys/wait.h>

#include <gio/gio.h>

#include "menu-cache.h"

#ifdef G_ENABLE_DEBUG
#define DEBUG(...)  g_debug(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

#if GLIB_CHECK_VERSION(2, 32, 0)
static GRecMutex _cache_lock;
#  define MENU_CACHE_LOCK       g_rec_mutex_lock(&_cache_lock)
#  define MENU_CACHE_UNLOCK     g_rec_mutex_unlock(&_cache_lock)
/* for sync lookup */
static GMutex sync_run_mutex;
static GCond sync_run_cond;
#define SET_CACHE_READY(_cache_) do { \
    g_mutex_lock(&sync_run_mutex); \
    _cache_->ready = TRUE; \
    g_cond_broadcast(&sync_run_cond); \
    g_mutex_unlock(&sync_run_mutex); } while(0)
#else
/* before 2.32 GLib had another entity for statically allocated mutexes */
static GStaticRecMutex _cache_lock = G_STATIC_REC_MUTEX_INIT;
#  define MENU_CACHE_LOCK       g_static_rec_mutex_lock(&_cache_lock)
#  define MENU_CACHE_UNLOCK     g_static_rec_mutex_unlock(&_cache_lock)
/* for sync lookup */
static GMutex *sync_run_mutex = NULL;
static GCond *sync_run_cond = NULL;
#define SET_CACHE_READY(_cache_) do { \
    g_mutex_lock(sync_run_mutex); \
    _cache_->ready = TRUE; \
    if(sync_run_cond) g_cond_broadcast(sync_run_cond); \
    g_mutex_unlock(sync_run_mutex); } while(0)
#endif

typedef struct
{
    char *dir;
    gint n_ref;
} MenuCacheFileDir;

struct _MenuCacheItem
{
    guint n_ref;
    MenuCacheType type;
    char* id;
    char* name;
    char* comment;
    char* icon;
    MenuCacheFileDir* file_dir;
    char* file_name;
    MenuCacheDir* parent;
};

struct _MenuCacheDir
{
    MenuCacheItem item;
    GSList* children;
    guint32 flags;
};

struct _MenuCacheApp
{
    MenuCacheItem item;
    char* generic_name;
    char* exec;
    char* working_dir;
    guint32 show_in_flags;
    guint32 flags;
    char* try_exec;
    const char **categories;
    char* keywords;
};

struct _MenuCache
{
    guint n_ref;
    MenuCacheDir* root_dir;
    char* menu_name;
    char* reg; /* includes md5 sum */
    char* md5; /* link inside of reg */
    char* cache_file;
    char** known_des;
    GSList* notifiers;
    GThread* thr;
    GCancellable* cancellable;
    guint version;
    guint reload_id;
    gboolean ready : 1; /* used for sync access */
};

static int server_fd = -1;
G_LOCK_DEFINE(connect); /* for server_fd */

static GHashTable* hash = NULL;

/* Don't call this API directly. Use menu_cache_lookup instead. */
static MenuCache* menu_cache_new( const char* cache_file );

static gboolean connect_server(GCancellable* cancellable);
static gboolean register_menu_to_server(MenuCache* cache);
static void unregister_menu_from_server( MenuCache* cache );

/* keep them for backward compatibility */
#ifdef G_DISABLE_DEPRECATED
MenuCacheDir* menu_cache_get_root_dir( MenuCache* cache );
MenuCacheDir* menu_cache_item_get_parent( MenuCacheItem* item );
MenuCacheDir* menu_cache_get_dir_from_path( MenuCache* cache, const char* path );
GSList* menu_cache_dir_get_children( MenuCacheDir* dir );
#endif

void menu_cache_init(int flags)
{
#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init();
#endif
}

static MenuCacheItem* read_item(GDataInputStream* f, MenuCache* cache,
                                MenuCacheFileDir** all_used_files, int n_all_used_files);

/* functions read_dir(), read_app(), and read_item() should be called for
   items that aren't accessible yet, therefore no lock is required */
static void read_dir(GDataInputStream* f, MenuCacheDir* dir, MenuCache* cache,
                     MenuCacheFileDir** all_used_files, int n_all_used_files)
{
    MenuCacheItem* item;
    char *line;
    gsize len;

    /* nodisplay flag */
    if (cache->version >= 2)
    {
        line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
        if (G_UNLIKELY(line == NULL))
            return;
        dir->flags = (guint32)atoi(line);
        g_free(line);
    }

    /* load child items in the dir */
    while( (item = read_item( f, cache, all_used_files, n_all_used_files )) )
    {
        /* menu_cache_ref shouldn't be called here for dir.
         * Otherwise, circular reference will happen. */
        item->parent = dir;
        dir->children = g_slist_prepend( dir->children, item );
    }

    dir->children = g_slist_reverse( dir->children );

    /* set flag by children if working with old cache generator */
    if (cache->version == 1)
    {
        if (dir->children == NULL)
            dir->flags = FLAG_IS_NODISPLAY;
        else if ((line = menu_cache_item_get_file_path(MENU_CACHE_ITEM(dir))) != NULL)
        {
            GKeyFile *kf = g_key_file_new();
            if (g_key_file_load_from_file(kf, line, G_KEY_FILE_NONE, NULL) &&
                g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                       G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY, NULL))
                dir->flags = FLAG_IS_NODISPLAY;
            g_key_file_free(kf);
            g_free(line);
        }
    }
}

static char *_unescape_lf(char *str)
{
    char *c, *p = str;
    gsize len = 0;

    while ((c = strchr(p, '\\')) != NULL)
    {
        if (p != &str[len])
            memmove(&str[len], p, c - p);
        len += (c - p);
        if (c[1] == 'n')
        {
            str[len++] = '\n';
            c++;
        }
        else if (c != &str[len])
            str[len++] = *c;
        p = &c[1];
    }
    if (p != &str[len])
        memmove(&str[len], p, strlen(p) + 1);
    return str;
}

static void read_app(GDataInputStream* f, MenuCacheApp* app, MenuCache* cache)
{
    char *line;
    gsize len;
    GString *str;

    /* generic name */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_UNLIKELY(line == NULL))
        return;
    if(G_LIKELY(len > 0))
        app->generic_name = _unescape_lf(line);
    else
        g_free(line);

    /* exec */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_UNLIKELY(line == NULL))
        return;
    if(G_LIKELY(len > 0))
        app->exec = _unescape_lf(line);
    else
        g_free(line);

    /* terminal / startup notify */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_UNLIKELY(line == NULL))
        return;
    app->flags = (guint32)atoi(line);
    g_free(line);

    /* ShowIn flags */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_UNLIKELY(line == NULL))
        return;
    app->show_in_flags = (guint32)atol(line);
    g_free(line);

    if (cache->version < 2)
        return;

    /* TryExec */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if (G_UNLIKELY(line == NULL))
        return;
    if (G_LIKELY(len > 0))
        app->try_exec = g_strchomp(line);
    else
        g_free(line);

    /* Path */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if (G_UNLIKELY(line == NULL))
        return;
    if (G_LIKELY(len > 0))
        app->working_dir = line;
    else
        g_free(line);

    /* Categories */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if (G_UNLIKELY(line == NULL))
        return;
    if (G_LIKELY(len > 0))
    {
        const char **x;

        /* split and intern all the strings so categories can be processed
           later for search doing g_quark_try_string()+g_quark_to_string() */
        app->categories = x = (const char **)g_strsplit(line, ";", 0);
        while (*x != NULL)
        {
            char *cat = (char *)*x;
            *x = g_intern_string(cat);
            g_free(cat);
            x++;
        }
    }
    g_free(line);

    /* Keywords */
    str = g_string_new(MENU_CACHE_ITEM(app)->name);
    if (G_LIKELY(app->exec != NULL))
    {
        char *sp = strchr(app->exec, ' ');
        char *bn = strrchr(app->exec, G_DIR_SEPARATOR);

        g_string_append_c(str, ',');
        if (bn == NULL && sp == NULL)
            g_string_append(str, app->exec);
        else if (bn == NULL || (sp != NULL && sp < bn))
            g_string_append_len(str, app->exec, sp - app->exec);
        else if (sp == NULL)
            g_string_append(str, &bn[1]);
        else
            g_string_append_len(str, &bn[1], sp - &bn[1]);
    }
    if (app->generic_name != NULL)
    {
        g_string_append_c(str, ',');
        g_string_append(str, app->generic_name);
    }
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if (G_UNLIKELY(line == NULL))
        return;
    if (len > 0)
    {
        g_string_append_c(str, ',');
        g_string_append(str, line);
    }
    app->keywords = g_utf8_casefold(str->str, str->len);
    g_string_free(str, TRUE);
    g_free(line);
}

static MenuCacheItem* read_item(GDataInputStream* f, MenuCache* cache,
                                MenuCacheFileDir** all_used_files, int n_all_used_files)
{
    MenuCacheItem* item;
    char *line;
    gsize len;
    gint idx;

    /* desktop/menu id */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_UNLIKELY(line == NULL))
        return NULL;

    if( G_LIKELY(len >= 1) )
    {
        if( line[0] == '+' ) /* menu dir */
        {
            item = (MenuCacheItem*)g_slice_new0( MenuCacheDir );
            item->n_ref = 1;
            item->type = MENU_CACHE_TYPE_DIR;
        }
        else if( line[0] == '-' ) /* menu item */
        {
            item = (MenuCacheItem*)g_slice_new0( MenuCacheApp );
            item->n_ref = 1;
            if( G_LIKELY( len > 1 ) ) /* application item */
                item->type = MENU_CACHE_TYPE_APP;
            else /* separator */
            {
                item->type = MENU_CACHE_TYPE_SEP;
                g_free(line);
                return item;
            }
        }
        else
        {
            g_free(line);
            return NULL;
        }

        item->id = g_strndup( line + 1, len - 1 );
        g_free(line);
    }
    else
    {
        g_free(line);
        return NULL;
    }

    /* name */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_UNLIKELY(line == NULL))
        goto _fail;
    if(G_LIKELY(len > 0))
        item->name = _unescape_lf(line);
    else
        g_free(line);

    /* comment */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_UNLIKELY(line == NULL))
        goto _fail;
    if(G_LIKELY(len > 0))
        item->comment = _unescape_lf(line);
    else
        g_free(line);

    /* icon */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_UNLIKELY(line == NULL))
        goto _fail;
    if(G_LIKELY(len > 0))
        item->icon = line;
    else
        g_free(line);

    /* file dir/basename */

    /* file name */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_UNLIKELY(line == NULL))
        goto _fail;
    if(G_LIKELY(len > 0))
        item->file_name = line;
    else if( item->type == MENU_CACHE_TYPE_APP )
    {
        /* When file name is the same as desktop_id, which is
         * quite common in desktop files, we use this trick to
         * save memory usage. */
        item->file_name = item->id;
        g_free(line);
    }
    else
        g_free(line);

    /* desktop file dir */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_UNLIKELY(line == NULL))
    {
_fail:
        g_free(item->id);
        g_free(item->name);
        g_free(item->comment);
        g_free(item->icon);
        if(item->file_name && item->file_name != item->id)
            g_free(item->file_name);
        if(item->type == MENU_CACHE_TYPE_DIR)
            g_slice_free(MenuCacheDir, MENU_CACHE_DIR(item));
        else
            g_slice_free(MenuCacheApp, MENU_CACHE_APP(item));
        return NULL;
    }
    idx = atoi( line );
    g_free(line);
    if( G_LIKELY( idx >=0 && idx < n_all_used_files ) )
    {
        item->file_dir = all_used_files[ idx ];
        g_atomic_int_inc(&item->file_dir->n_ref);
    }

    if( item->type == MENU_CACHE_TYPE_DIR )
        read_dir( f, MENU_CACHE_DIR(item), cache, all_used_files, n_all_used_files );
    else if( item->type == MENU_CACHE_TYPE_APP )
        read_app( f, MENU_CACHE_APP(item), cache );

    return item;
}

static void menu_cache_file_dir_unref(MenuCacheFileDir *file_dir)
{
    if (file_dir && g_atomic_int_dec_and_test(&file_dir->n_ref))
    {
        g_free(file_dir->dir);
        g_free(file_dir);
    }
}

static gint read_all_used_files(GDataInputStream* f, MenuCache* cache,
                                MenuCacheFileDir*** all_used_files)
{
    char *line;
    gsize len;
    int i, n;
    MenuCacheFileDir** dirs;

    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_UNLIKELY(line == NULL))
        return -1;

    n = atoi( line );
    g_free(line);
    if (G_UNLIKELY(n <= 0))
        return n;

    dirs = g_new0( MenuCacheFileDir *, n );

    for( i = 0; i < n; ++i )
    {
        line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
        if(G_UNLIKELY(line == NULL))
        {
            while (i-- > 0)
                menu_cache_file_dir_unref(dirs[i]);
            g_free(dirs);
            return -1;
        }
        dirs[i] = g_new(MenuCacheFileDir, 1);
        dirs[i]->n_ref = 1;
        dirs[i]->dir = line; /* don't include \n */
    }
    *all_used_files = dirs;
    return n;
}

static gboolean read_all_known_des(GDataInputStream* f, MenuCache* cache)
{
    char *line;
    gsize len;
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_UNLIKELY(line == NULL))
        return FALSE;
    cache->known_des = g_strsplit_set( line, ";\n", 0 );
    g_free(line);
    return TRUE;
}

static MenuCache* menu_cache_new( const char* cache_file )
{
    MenuCache* cache;
    cache = g_slice_new0( MenuCache );
    cache->cache_file = g_strdup( cache_file );
    cache->n_ref = 1;
    return cache;
}

/**
 * menu_cache_ref
 * @cache: a menu cache descriptor
 *
 * Increases reference counter on @cache.
 *
 * Returns: @cache.
 *
 * Since: 0.1.0
 */
MenuCache* menu_cache_ref(MenuCache* cache)
{
    g_atomic_int_inc( &cache->n_ref );
    return cache;
}

/**
 * menu_cache_unref
 * @cache: a menu cache descriptor
 *
 * Descreases reference counter on @cache. When reference count becomes 0
 * then resources associated with @cache will be freed.
 *
 * Since: 0.1.0
 */
void menu_cache_unref(MenuCache* cache)
{
    /* DEBUG("cache_unref: %d", cache->n_ref); */
    /* we need a lock here unfortunately because item in hash isn't protected
       by reference therefore another thread may get access to it right now */
    MENU_CACHE_LOCK;
    if( g_atomic_int_dec_and_test(&cache->n_ref) )
    {
        /* g_assert(cache->reload_id != 0); */
        unregister_menu_from_server( cache );
        /* DEBUG("unregister to server"); */
        g_hash_table_remove( hash, cache->menu_name );
        if( g_hash_table_size(hash) == 0 )
        {
            /* DEBUG("destroy hash"); */
            g_hash_table_destroy(hash);

            /* DEBUG("disconnect from server"); */
            G_LOCK(connect);
            shutdown(server_fd, SHUT_RDWR); /* the IO thread will terminate itself */
            server_fd = -1;
            G_UNLOCK(connect);
            hash = NULL;
        }
        MENU_CACHE_UNLOCK;

        if(G_LIKELY(cache->thr))
        {
            g_cancellable_cancel(cache->cancellable);
            g_thread_join(cache->thr);
        }
        g_object_unref(cache->cancellable);
        if( G_LIKELY(cache->root_dir) )
        {
            /* DEBUG("unref root dir"); */
            menu_cache_item_unref( MENU_CACHE_ITEM(cache->root_dir) );
            /* DEBUG("unref root dir finished"); */
        }
        g_free( cache->cache_file );
        g_free( cache->menu_name );
        g_free(cache->reg);
        /* g_free( cache->menu_file_path ); */
        g_strfreev(cache->known_des);
        g_slist_free(cache->notifiers);
        g_slice_free( MenuCache, cache );
    }
    else
        MENU_CACHE_UNLOCK;
}

/**
 * menu_cache_get_root_dir
 * @cache: a menu cache instance
 *
 * Since: 0.1.0
 *
 * Deprecated: 0.3.4: Use menu_cache_dup_root_dir() instead.
 */
MenuCacheDir* menu_cache_get_root_dir( MenuCache* cache )
{
    MenuCacheDir* dir = menu_cache_dup_root_dir(cache);
    /* NOTE: this is very ugly hack but cache->root_dir may be changed by
       cache reload in server-io thread, so we should keep it alive :( */
    if(dir)
        g_timeout_add_seconds(10, (GSourceFunc)menu_cache_item_unref, dir);
    return dir;
}

/**
 * menu_cache_dup_root_dir
 * @cache: a menu cache instance
 *
 * Retrieves root directory for @cache. Returned data should be freed
 * with menu_cache_item_unref() after usage.
 *
 * Returns: (transfer full): root item or %NULL in case of error.
 *
 * Since: 0.3.4
 */
MenuCacheDir* menu_cache_dup_root_dir( MenuCache* cache )
{
    MenuCacheDir* dir;
    MENU_CACHE_LOCK;
    dir = cache->root_dir;
    if(G_LIKELY(dir))
        menu_cache_item_ref(MENU_CACHE_ITEM(dir));
    MENU_CACHE_UNLOCK;
    return dir;
}

/**
 * menu_cache_item_ref
 * @item: a menu cache item
 *
 * Increases reference counter on @item.
 *
 * Returns: @item.
 *
 * Since: 0.1.0
 */
MenuCacheItem* menu_cache_item_ref(MenuCacheItem* item)
{
    g_atomic_int_inc( &item->n_ref );
    /* DEBUG("item_ref %s: %d -> %d", item->id, item->n_ref-1, item->n_ref); */
    return item;
}

static gboolean menu_cache_reload_idle(gpointer cache)
{
    /* do reload once */
    if (!g_source_is_destroyed(g_main_current_source()))
        menu_cache_reload(cache);
    return FALSE;
}

typedef struct _CacheReloadNotifier
{
    MenuCacheReloadNotify func;
    gpointer user_data;
}CacheReloadNotifier;

struct _MenuCacheNotifyId
{
    GSList l;
};

/**
 * menu_cache_add_reload_notify
 * @cache: a menu cache instance
 * @func: callback to call when menu cache is reloaded
 * @user_data: user data provided for @func
 *
 * Adds a @func to list of callbacks that are called each time menu cache
 * is loaded.
 *
 * Returns: an ID of added callback.
 *
 * Since: 0.1.0
 */
MenuCacheNotifyId menu_cache_add_reload_notify(MenuCache* cache, MenuCacheReloadNotify func, gpointer user_data)
{
    GSList* l = g_slist_alloc();
    CacheReloadNotifier* n = g_slice_new(CacheReloadNotifier);
    gboolean is_first;
    n->func = func;
    n->user_data = user_data;
    l->data = n;
    MENU_CACHE_LOCK;
    is_first = (cache->root_dir == NULL && cache->notifiers == NULL);
    cache->notifiers = g_slist_concat( cache->notifiers, l );
    /* reload existing file first so it will be ready right away */
    if(is_first && cache->reload_id == 0)
        cache->reload_id = g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                                           menu_cache_reload_idle,
                                           menu_cache_ref(cache),
                                           (GDestroyNotify)menu_cache_unref);
    MENU_CACHE_UNLOCK;
    return (MenuCacheNotifyId)l;
}

/**
 * menu_cache_remove_reload_notify
 * @cache: a menu cache instance
 * @notify_id: an ID of callback
 *
 * Removes @notify_id from list of callbacks added for @cache by previous
 * call to menu_cache_add_reload_notify().
 *
 * Since: 0.1.0
 */
void menu_cache_remove_reload_notify(MenuCache* cache, MenuCacheNotifyId notify_id)
{
    MENU_CACHE_LOCK;
    g_slice_free( CacheReloadNotifier, ((GSList*)notify_id)->data );
    cache->notifiers = g_slist_delete_link( cache->notifiers, (GSList*)notify_id );
    MENU_CACHE_UNLOCK;
}

static gboolean reload_notify(gpointer data)
{
    MenuCache* cache = (MenuCache*)data;
    GSList* l;
    MENU_CACHE_LOCK;
    /* we have it referenced and there is no source removal so no check */
    for( l = cache->notifiers; l; l = l->next )
    {
        CacheReloadNotifier* n = (CacheReloadNotifier*)l->data;
        if(n->func)
            n->func( cache, n->user_data );
    }
    MENU_CACHE_UNLOCK;
    return FALSE;
}

/**
 * menu_cache_reload
 * @cache: a menu cache instance
 *
 * Reloads menu cache from file generated by menu-cached.
 *
 * Returns: %TRUE if reload was successful.
 *
 * Since: 0.1.0
 */
gboolean menu_cache_reload( MenuCache* cache )
{
    char* line;
    gsize len;
    GFile* file;
    GFileInputStream* istr = NULL;
    GDataInputStream* f;
    MenuCacheFileDir** all_used_files;
    int i, n;
    int ver_maj, ver_min;

    MENU_CACHE_LOCK;
    if (cache->reload_id)
        g_source_remove(cache->reload_id);
    cache->reload_id = 0;
    MENU_CACHE_UNLOCK;
    file = g_file_new_for_path(cache->cache_file);
    if(!file)
        return FALSE;
    istr = g_file_read(file, cache->cancellable, NULL);
    g_object_unref(file);
    if(!istr)
        return FALSE;
    f = g_data_input_stream_new(G_INPUT_STREAM(istr));
    g_object_unref(istr);
    if( ! f )
        return FALSE;

    /* the first line is version number */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_LIKELY(line))
    {
        len = sscanf(line, "%d.%d", &ver_maj, &ver_min);
        g_free(line);
        if(len < 2)
            goto _fail;
        if( ver_maj != VER_MAJOR ||
            ver_min > VER_MINOR || ver_min < VER_MINOR_SUPPORTED )
            goto _fail;
    }
    else
        goto _fail;

    g_debug("menu cache: got file version 1.%d", ver_min);
    /* the second line is menu name */
    line = g_data_input_stream_read_line(f, &len, cache->cancellable, NULL);
    if(G_UNLIKELY(line == NULL))
        goto _fail;
    g_free(line);

    /* FIXME: this may lock other threads for some time */
    MENU_CACHE_LOCK;
    if(cache->notifiers == NULL)
    {
        /* nobody aware of reloads, stupid clients may think root is forever */
        MENU_CACHE_UNLOCK;
        goto _fail;
    }

    /* get all used files */
    n = read_all_used_files( f, cache, &all_used_files );
    if (n <= 0)
    {
        MENU_CACHE_UNLOCK;
        goto _fail;
    }

    /* read known DEs */
    g_strfreev( cache->known_des );
    if( ! read_all_known_des( f, cache ) )
    {
        cache->known_des = NULL;
        MENU_CACHE_UNLOCK;
        for (i = 0; i < n; i++)
            menu_cache_file_dir_unref(all_used_files[i]);
        g_free(all_used_files);
_fail:
        g_object_unref(f);
        return FALSE;
    }
    cache->version = ver_min;

    if(cache->root_dir)
        menu_cache_item_unref( MENU_CACHE_ITEM(cache->root_dir) );

    cache->root_dir = (MenuCacheDir*)read_item( f, cache, all_used_files, n );
    g_object_unref(f);

    g_idle_add_full(G_PRIORITY_HIGH_IDLE, reload_notify, menu_cache_ref(cache),
                    (GDestroyNotify)menu_cache_unref);
    MENU_CACHE_UNLOCK;

    for (i = 0; i < n; i++)
        menu_cache_file_dir_unref(all_used_files[i]);
    g_free(all_used_files);

    return TRUE;
}

/**
 * menu_cache_item_unref
 * @item: a menu cache item
 *
 * Decreases reference counter on @item. When reference count becomes 0
 * then resources associated with @item will be freed.
 *
 * Returns: %FALSE (since 0.5.0)
 *
 * Since: 0.1.0
 */
gboolean menu_cache_item_unref(MenuCacheItem* item)
{
    /* DEBUG("item_unref(%s): %d", item->id, item->n_ref); */
    /* We need a lock here unfortunately since another thread may have access
       to it via some child->parent which isn't protected by reference */
    MENU_CACHE_LOCK; /* lock may be recursive here */
    if( g_atomic_int_dec_and_test( &item->n_ref ) )
    {
        /* DEBUG("free item: %s", item->id); */
        g_free( item->id );
        g_free( item->name );
        g_free( item->comment );
        g_free( item->icon );

        menu_cache_file_dir_unref(item->file_dir);

        if( item->file_name && item->file_name != item->id )
            g_free( item->file_name );

        if( item->parent )
        {
            /* DEBUG("remove %s from parent %s", item->id, MENU_CACHE_ITEM(item->parent)->id); */
            /* remove ourselve from the parent node. */
            item->parent->children = g_slist_remove(item->parent->children, item);
        }

        if( item->type == MENU_CACHE_TYPE_DIR )
        {
            MenuCacheDir* dir = MENU_CACHE_DIR(item);
            GSList* l;
            for(l = dir->children; l; )
            {
                MenuCacheItem* child = MENU_CACHE_ITEM(l->data);
                /* remove ourselve from the children. */
                child->parent = NULL;
                l = l->next;
                menu_cache_item_unref(child);
            }
            g_slist_free( dir->children );
            g_slice_free( MenuCacheDir, dir );
        }
        else
        {
            MenuCacheApp* app = MENU_CACHE_APP(item);
            g_free(app->generic_name);
            g_free( app->exec );
            g_free(app->try_exec);
            g_free(app->working_dir);
            g_free(app->categories);
            g_free(app->keywords);
            g_slice_free( MenuCacheApp, app );
        }
    }
    MENU_CACHE_UNLOCK;
    return FALSE;
}

/**
 * menu_cache_item_get_type
 * @item: a menu cache item
 *
 * Checks type of @item.
 *
 * Returns: type of @item.
 *
 * Since: 0.1.0
 */
MenuCacheType menu_cache_item_get_type( MenuCacheItem* item )
{
    return item->type;
}

/**
 * menu_cache_item_get_id
 * @item: a menu cache item
 *
 * Retrieves ID (short name such as 'application.desktop') of @item.
 * Returned data are owned by menu cache and should be not freed by caller.
 *
 * Returns: (transfer none): item ID.
 *
 * Since: 0.1.0
 */
const char* menu_cache_item_get_id( MenuCacheItem* item )
{
    return item->id;
}

/**
 * menu_cache_item_get_name
 * @item: a menu cache item
 *
 * Retrieves display name of @item. Returned data are owned by menu
 * cache and should be not freed by caller.
 *
 * Returns: (transfer none): @item display name or %NULL.
 *
 * Since: 0.1.0
 */
const char* menu_cache_item_get_name( MenuCacheItem* item )
{
    return item->name;
}

/**
 * menu_cache_item_get_comment
 * @item: a menu cache item
 *
 * Retrieves comment of @item. The comment can be used to show tooltip
 * on @item. Returned data are owned by menu cache and should be not
 * freed by caller.
 *
 * Returns: (transfer none): @item comment or %NULL.
 *
 * Since: 0.1.0
 */
const char* menu_cache_item_get_comment( MenuCacheItem* item )
{
    return item->comment;
}

/**
 * menu_cache_item_get_icon
 * @item: a menu cache item
 *
 * Retrieves name of icon of @item. Returned data are owned by menu
 * cache and should be not freed by caller.
 *
 * Returns: (transfer none): @item icon name or %NULL.
 *
 * Since: 0.1.0
 */
const char* menu_cache_item_get_icon( MenuCacheItem* item )
{
    return item->icon;
}

/**
 * menu_cache_item_get_file_basename
 * @item: a menu cache item
 *
 * Retrieves basename of @item. This API can return %NULL if @item is a
 * directory and have no directory desktop entry file. Returned data are
 * owned by menu cache and should be not freed by caller.
 *
 * Returns: (transfer none): @item file basename or %NULL.
 *
 * Since: 0.2.0
 */
const char* menu_cache_item_get_file_basename( MenuCacheItem* item )
{
    return item->file_name;
}

/**
 * menu_cache_item_get_file_dirname
 * @item: a menu cache item
 *
 * Retrieves path to directory where @item desktop enrty file is located.
 * This API can return %NULL if @item is a directory and have no
 * desktop entry file. Returned data are owned by menu cache and should
 * be not freed by caller.
 *
 * Returns: (transfer none): @item file parent directory path or %NULL.
 *
 * Since: 0.2.0
 */
const char* menu_cache_item_get_file_dirname( MenuCacheItem* item )
{
    return item->file_dir ? item->file_dir->dir + 1 : NULL;
}

/**
 * menu_cache_item_get_file_path
 * @item: a menu cache item
 *
 * Retrieves path to @item desktop enrty file. This API can return %NULL
 * if @item is a directory and have no desktop entry file. Returned data
 * should be freed with g_free() after usage.
 *
 * Returns: (transfer full): @item file path or %NULL.
 *
 * Since: 0.2.0
 */
char* menu_cache_item_get_file_path( MenuCacheItem* item )
{
    if( ! item->file_name || ! item->file_dir )
        return NULL;
    return g_build_filename( item->file_dir->dir + 1, item->file_name, NULL );
}

/**
 * menu_cache_item_get_parent
 * @item: a menu cache item
 *
 * Since: 0.1.0
 *
 * Deprecated: 0.3.4: Use menu_cache_item_dup_parent() instead.
 */
MenuCacheDir* menu_cache_item_get_parent( MenuCacheItem* item )
{
    MenuCacheDir* dir = menu_cache_item_dup_parent(item);
    /* NOTE: this is very ugly hack but parent may be changed by item freeing
       so we should keep it alive :( */
    if(dir)
        g_timeout_add_seconds(10, (GSourceFunc)menu_cache_item_unref, dir);
    return dir;
}

/**
 * menu_cache_item_dup_parent
 * @item: a menu item
 *
 * Retrieves parent (directory) for @item. Returned data should be freed
 * with menu_cache_item_unref() after usage.
 *
 * Returns: (transfer full): parent item or %NULL in case of error.
 *
 * Since: 0.3.4
 */
MenuCacheDir* menu_cache_item_dup_parent( MenuCacheItem* item )
{
    MenuCacheDir* dir;
    MENU_CACHE_LOCK;
    dir = item->parent;
    if(G_LIKELY(dir))
        menu_cache_item_ref(MENU_CACHE_ITEM(dir));
    MENU_CACHE_UNLOCK;
    return dir;
}

/**
 * menu_cache_dir_get_children
 * @dir: a menu cache item
 *
 * Retrieves list of items contained in @dir. Returned data are owned by
 * menu cache and should be not freed by caller.
 * This API is thread unsafe and should be never called from outside of
 * default main loop.
 *
 * Returns: (transfer none) (element-type MenuCacheItem): list of items.
 *
 * Since: 0.1.0
 *
 * Deprecated: 0.4.0: Use menu_cache_dir_list_children() instead.
 */
GSList* menu_cache_dir_get_children( MenuCacheDir* dir )
{
    /* NOTE: this is very ugly hack but dir may be freed by cache reload
       in server-io thread, so we should keep it alive :( */
    g_timeout_add_seconds(10, (GSourceFunc)menu_cache_item_unref,
                          menu_cache_item_ref(MENU_CACHE_ITEM(dir)));
    return dir->children;
}

/**
 * menu_cache_dir_list_children
 * @dir: a menu cache item
 *
 * Retrieves list of items contained in @dir. Returned data should be
 * freed with g_slist_free_full(list, menu_cache_item_unref) after usage.
 *
 * Returns: (transfer full) (element-type MenuCacheItem): list of items.
 *
 * Since: 0.4.0
 */
GSList* menu_cache_dir_list_children(MenuCacheDir* dir)
{
    GSList *children, *l;

    if(MENU_CACHE_ITEM(dir)->type != MENU_CACHE_TYPE_DIR)
        return NULL;
    MENU_CACHE_LOCK;
    children = g_slist_copy(dir->children);
    for(l = children; l; l = l->next)
        menu_cache_item_ref(l->data);
    MENU_CACHE_UNLOCK;
    return children;
}

/**
 * menu_cache_find_child_by_id
 * @dir: a menu cache item
 * @id: a string to find
 *
 * Checks if @dir has a child with given @id. Returned data should be
 * freed with menu_cache_item_unref() when no longer needed.
 *
 * Returns: (transfer full): found item or %NULL.
 *
 * Since: 0.5.0
 */
MenuCacheItem *menu_cache_find_child_by_id(MenuCacheDir *dir, const char *id)
{
    GSList *child;
    MenuCacheItem *item = NULL;

    if (MENU_CACHE_ITEM(dir)->type != MENU_CACHE_TYPE_DIR || id == NULL)
        return NULL;
    MENU_CACHE_LOCK;
    for (child = dir->children; child; child = child->next)
        if (g_strcmp0(MENU_CACHE_ITEM(child->data)->id, id) == 0)
        {
            item = menu_cache_item_ref(child->data);
            break;
        }
    MENU_CACHE_UNLOCK;
    return item;
}

/**
 * menu_cache_find_child_by_name
 * @dir: a menu cache item
 * @name: a string to find
 *
 * Checks if @dir has a child with given @name. Returned data should be
 * freed with menu_cache_item_unref() when no longer needed.
 *
 * Returns: (transfer full): found item or %NULL.
 *
 * Since: 0.5.0
 */
MenuCacheItem *menu_cache_find_child_by_name(MenuCacheDir *dir, const char *name)
{
    GSList *child;
    MenuCacheItem *item = NULL;

    if (MENU_CACHE_ITEM(dir)->type != MENU_CACHE_TYPE_DIR || name == NULL)
        return NULL;
    MENU_CACHE_LOCK;
    for (child = dir->children; child; child = child->next)
        if (g_strcmp0(MENU_CACHE_ITEM(child->data)->name, name) == 0)
        {
            item = menu_cache_item_ref(child->data);
            break;
        }
    MENU_CACHE_UNLOCK;
    return item;
}

/**
 * menu_cache_dir_is_visible
 * @dir: a menu cache item
 *
 * Checks if @dir should be visible.
 *
 * Returns: %TRUE if @dir is visible.
 *
 * Since: 0.5.0
 */
gboolean menu_cache_dir_is_visible(MenuCacheDir *dir)
{
    return ((dir->flags & FLAG_IS_NODISPLAY) == 0);
}

/**
 * menu_cache_app_get_generic_name
 * @app: a menu cache item
 *
 * Retrieves generic name for @app. Returned data are owned by menu
 * cache and should not be freed by caller.
 *
 * Returns: (transfer none): app's generic name or %NULL.
 *
 * Since: 1.0.3
 */
const char* menu_cache_app_get_generic_name( MenuCacheApp* app )
{
	return app->generic_name;
}

/**
 * menu_cache_app_get_exec
 * @app: a menu cache item
 *
 * Retrieves execution string for @app. Returned data are owned by menu
 * cache and should be not freed by caller.
 *
 * Returns: (transfer none): item execution string or %NULL.
 *
 * Since: 0.1.0
 */
const char* menu_cache_app_get_exec( MenuCacheApp* app )
{
    return app->exec;
}

/**
 * menu_cache_app_get_working_dir
 * @app: a menu cache item
 *
 * Retrieves working directory for @app. Returned data are owned by menu
 * cache and should be not freed by caller.
 *
 * Returns: (transfer none): item working directory or %NULL.
 *
 * Since: 0.1.0
 */
const char* menu_cache_app_get_working_dir( MenuCacheApp* app )
{
    return app->working_dir;
}

/**
 * menu_cache_app_get_categories
 * @app: a menu cache item
 *
 * Retrieves list of categories for @app. Returned data are owned by menu
 * cache and should be not freed by caller.
 *
 * Returns: (transfer none): list of categories or %NULL.
 *
 * Since: 1.0.0
 */
const char * const * menu_cache_app_get_categories(MenuCacheApp* app)
{
    return app->categories;
}

/**
 * menu_cache_app_get_use_terminal
 * @app: a menu cache item
 *
 * Checks if @app should be ran in terminal.
 *
 * Returns: %TRUE if @app requires terminal to run.
 *
 * Since: 0.1.0
 */
gboolean menu_cache_app_get_use_terminal( MenuCacheApp* app )
{
    return ( (app->flags & FLAG_USE_TERMINAL) != 0 );
}

/**
 * menu_cache_app_get_use_sn
 * @app: a menu cache item
 *
 * Checks if @app wants startup notification.
 *
 * Returns: %TRUE if @app wants startup notification.
 *
 * Since: 0.1.0
 */
gboolean menu_cache_app_get_use_sn( MenuCacheApp* app )
{
    return ( (app->flags & FLAG_USE_SN) != 0 );
}

/**
 * menu_cache_app_get_show_flags
 * @app: a menu cache item
 *
 * Retrieves list of desktop environments where @app should be visible.
 *
 * Returns: bit mask of DE.
 *
 * Since: 0.2.0
 */
guint32 menu_cache_app_get_show_flags( MenuCacheApp* app )
{
    return app->show_in_flags;
}

static gboolean _can_be_exec(MenuCacheApp *app)
{
    char *path;

    if (app->try_exec == NULL)
        return TRUE;
    path = g_find_program_in_path(app->try_exec);
    g_free(path);
    return (path != NULL);
}

/**
 * menu_cache_app_get_is_visible
 * @app: a menu cache item
 * @de_flags: bit mask of DE to test
 *
 * Checks if @app should be visible in any of desktop environments
 * @de_flags.
 *
 * Returns: %TRUE if @app is visible.
 *
 * Since: 0.2.0
 */
gboolean menu_cache_app_get_is_visible( MenuCacheApp* app, guint32 de_flags )
{
    if(app->flags & FLAG_IS_NODISPLAY)
        return FALSE;
    return (!app->show_in_flags || (app->show_in_flags & de_flags)) &&
           _can_be_exec(app);
}

/*
MenuCacheApp* menu_cache_find_app_by_exec( const char* exec )
{
    return NULL;
}
*/

/**
 * menu_cache_get_dir_from_path
 * @cache: a menu cache instance
 * @path: item path
 *
 * Since: 0.1.0
 *
 * Deprecated: 0.3.4: Use menu_cache_item_from_path() instead.
 */
MenuCacheDir* menu_cache_get_dir_from_path( MenuCache* cache, const char* path )
{
    char** names = g_strsplit( path + 1, "/", -1 );
    int i = 0;
    MenuCacheDir* dir = NULL;

    if( !names )
        return NULL;

    if( G_UNLIKELY(!names[0]) )
    {
        g_strfreev(names);
        return NULL;
    }
    /* the topmost dir of the path should be the root menu dir. */
    MENU_CACHE_LOCK;
    dir = cache->root_dir;
    if (G_UNLIKELY(dir == NULL) || strcmp(names[0], MENU_CACHE_ITEM(dir)->id))
    {
        MENU_CACHE_UNLOCK;
        return NULL;
    }

    for( ++i; names[i]; ++i )
    {
        GSList* l;
        for( l = dir->children; l; l = l->next )
        {
            MenuCacheItem* item = MENU_CACHE_ITEM(l->data);
            if( item->type == MENU_CACHE_TYPE_DIR && 0 == strcmp( item->id, names[i] ) )
                dir = MENU_CACHE_DIR(item);
        }
        /* FIXME: we really should ref it on return since other thread may
           destroy the parent at this time and returned data become invalid.
           Therefore this call isn't thread-safe! */
        if( ! dir )
        {
            MENU_CACHE_UNLOCK;
            return NULL;
        }
    }
    MENU_CACHE_UNLOCK;
    return dir;
}

/**
 * menu_cache_item_from_path
 * @cache: cache to inspect
 * @path: item path
 *
 * Searches item @path in the @cache. The @path consists of item IDs
 * separated by slash ('/'). Returned data should be freed with
 * menu_cache_item_unref() after usage.
 *
 * Returns: (transfer full): found item or %NULL if no item found.
 *
 * Since: 0.3.4
 */
MenuCacheItem* menu_cache_item_from_path( MenuCache* cache, const char* path )
{
    char** names = g_strsplit( path + 1, "/", -1 );
    int i;
    MenuCacheDir* dir;
    MenuCacheItem* item = NULL;

    if( !names )
        return NULL;

    if( G_UNLIKELY(!names[0]) )
    {
        g_strfreev(names);
        return NULL;
    }
    /* the topmost dir of the path should be the root menu dir. */
    MENU_CACHE_LOCK;
    dir = cache->root_dir;
    if( G_UNLIKELY(!dir) || strcmp(names[0], MENU_CACHE_ITEM(dir)->id) != 0 )
        goto _end;

    for( i = 1; names[i]; ++i )
    {
        GSList* l;
        item = NULL;
        if( !dir )
            break;
        l = dir->children;
        dir = NULL;
        for( ; l; l = l->next )
        {
            item = MENU_CACHE_ITEM(l->data);
            if( g_strcmp0( item->id, names[i] ) == 0 )
            {
                if( item->type == MENU_CACHE_TYPE_DIR )
                    dir = MENU_CACHE_DIR(item);
                break;
            }
            item = NULL;
        }
        if( !item )
            break;
    }
    if(item)
        menu_cache_item_ref(item);
_end:
    MENU_CACHE_UNLOCK;
    g_strfreev(names);
    return item;
}

/**
 * menu_cache_dir_make_path
 * @dir: a menu cache item
 *
 * Retrieves path of @dir. The path consists of item IDs separated by
 * slash ('/'). Returned data should be freed with g_free() after usage.
 *
 * Returns: (transfer full): item path.
 *
 * Since: 0.1.0
 */
char* menu_cache_dir_make_path( MenuCacheDir* dir )
{
    GString* path = g_string_sized_new(1024);
    MenuCacheItem* it;

    MENU_CACHE_LOCK;
    while( (it = MENU_CACHE_ITEM(dir)) ) /* this is not top dir */
    {
        g_string_prepend( path, menu_cache_item_get_id(it) );
        g_string_prepend_c( path, '/' );
        /* FIXME: if parent is already unref'd by another thread then
           path being made will be broken. Is there any way to avoid that? */
        dir = it->parent;
    }
    MENU_CACHE_UNLOCK;
    return g_string_free( path, FALSE );
}

static void get_socket_name( char* buf, int len )
{
    char* dpy = g_strdup(g_getenv("DISPLAY"));
    if(dpy && *dpy)
    {
        char* p = strchr(dpy, ':');
        for(++p; *p && *p != '.' && *p != '\n';)
            ++p;
        if(*p)
            *p = '\0';
    }
#if GLIB_CHECK_VERSION(2, 28, 0)
    g_snprintf( buf, len, "%s/menu-cached-%s", g_get_user_runtime_dir(),
                dpy ? dpy : ":0" );
#else
    g_snprintf( buf, len, "%s/.menu-cached-%s-%s", g_get_tmp_dir(),
                dpy ? dpy : ":0", g_get_user_name() );
#endif
    g_free(dpy);
}

#define MAX_RETRIES 25

static gboolean fork_server(const char *path)
{
    int ret, pid, status;

    if (!g_file_test (MENUCACHE_LIBEXECDIR "/menu-cached", G_FILE_TEST_IS_EXECUTABLE))
    {
        g_error("failed to find menu-cached");
    }

    /* Start daemon */
    pid = fork();
    if (pid == 0)
    {
        execl(MENUCACHE_LIBEXECDIR "/menu-cached", MENUCACHE_LIBEXECDIR "/menu-cached",
              path, NULL);
        g_print("failed to exec %s %s\n", MENUCACHE_LIBEXECDIR "/menu-cached", path);
    }

    /*
     * do a waitpid on the intermediate process to avoid zombies.
     */
retry_wait:
    ret = waitpid(pid, &status, 0);
    if (ret < 0) {
        if (errno == EINTR)
            goto retry_wait;
    }
    return TRUE;
}

/* this thread is started by connect_server() */
static gpointer server_io_thread(gpointer data)
{
    char buf[1024]; /* protocol has a lot shorter strings */
    ssize_t sz;
    size_t ptr = 0;
    int fd = GPOINTER_TO_INT(data);
    GHashTableIter it;
    char* menu_name;
    MenuCache* cache;

    while(fd >= 0)
    {
        sz = read(fd, &buf[ptr], sizeof(buf) - ptr);
        if(sz <= 0) /* socket error or EOF */
        {
            MENU_CACHE_LOCK;
            ptr = hash ? g_hash_table_size(hash) : 0;
            MENU_CACHE_UNLOCK;
            if (ptr == 0) /* don't need it anymore */
                break;
            G_LOCK(connect);
            if(fd != server_fd) /* someone replaced us?! go out immediately! */
            {
                G_UNLOCK(connect);
                break;
            }
            server_fd = -1;
            G_UNLOCK(connect);
            DEBUG("connect failed, trying reconnect");
            sleep(1);
            if( ! connect_server(NULL) )
            {
                g_critical("fail to re-connect to the server.");
                MENU_CACHE_LOCK;
                if(hash)
                {
                    g_hash_table_iter_init(&it, hash);
                    while(g_hash_table_iter_next(&it, (gpointer*)&menu_name, (gpointer*)&cache))
                        SET_CACHE_READY(cache);
                }
                MENU_CACHE_UNLOCK;
                break;
            }
            DEBUG("successfully reconnected server, re-register menus.");
            /* re-register all menu caches */
            MENU_CACHE_LOCK;
            if(hash)
            {
                g_hash_table_iter_init(&it, hash);
                while(g_hash_table_iter_next(&it, (gpointer*)&menu_name, (gpointer*)&cache))
                    register_menu_to_server(cache);
                    /* FIXME: need we remove it from hash if failed? */
            }
            MENU_CACHE_UNLOCK;
            break; /* next thread will do it */
        }
        while(sz > 0)
        {
            while(sz > 0)
            {
                if(buf[ptr] == '\n')
                    break;
                sz--;
                ptr++;
            }
            if(ptr == sizeof(buf)) /* EOB reached, seems we got garbage */
            {
                g_warning("menu cache: got garbage from server, break connect");
                shutdown(fd, SHUT_RDWR); /* drop connection */
                break; /* we handle it above */
            }
            else if(sz == 0) /* incomplete line, wait for data again */
                break;
            /* we got a line, let check what we got */
            buf[ptr] = '\0';
            if(memcmp(buf, "REL:", 4) == 0) /* reload */
            {
                DEBUG("server ask us to reload cache: %s", &buf[4]);
                MENU_CACHE_LOCK;
                if(hash)
                {
                    g_hash_table_iter_init(&it, hash);
                    while(g_hash_table_iter_next(&it, (gpointer*)&menu_name, (gpointer*)&cache))
                    {
                        if(memcmp(cache->md5, &buf[4], 32) == 0)
                        {
                            DEBUG("RELOAD!");
                            menu_cache_reload(cache);
                            SET_CACHE_READY(cache);
                            break;
                        }
                    }
                }
                MENU_CACHE_UNLOCK;
                /* DEBUG("cache reloaded"); */
            }
            else
                g_warning("menu cache: unrecognized input: %s", buf);
            /* go to next line */
            sz--;
            if(sz > 0)
                memmove(buf, &buf[ptr+1], sz);
            ptr = 0;
        }
    }
    G_LOCK(connect);
    if (fd == server_fd)
        server_fd = -1;
    G_UNLOCK(connect);
    close(fd);
    /* DEBUG("server io thread terminated"); */
#if GLIB_CHECK_VERSION(2, 32, 0)
    g_thread_unref(g_thread_self());
#endif
    return NULL;
}

static gboolean connect_server(GCancellable* cancellable)
{
    int fd, rc;
    struct sockaddr_un addr;
    int retries = 0;

    G_LOCK(connect);
    if(server_fd != -1 || (cancellable && g_cancellable_is_cancelled(cancellable)))
    {
        G_UNLOCK(connect);
        return TRUE;
    }

retry:
    fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
    {
        g_print("Failed to create socket\n");
        G_UNLOCK(connect);
        return FALSE;
    }

    fcntl (fd, F_SETFD, FD_CLOEXEC);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    get_socket_name( addr.sun_path, sizeof( addr.sun_path ) );

    if( connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        rc = errno;
        close(fd);
        if(cancellable && g_cancellable_is_cancelled(cancellable))
        {
            G_UNLOCK(connect);
            return TRUE;
        }
        if((rc == ECONNREFUSED || rc == ENOENT) && retries == 0)
        {
            DEBUG("no running server found, starting it");
            fork_server(addr.sun_path);
            ++retries;
            goto retry;
        }
        if(retries < MAX_RETRIES)
        {
            usleep(50000);
            ++retries;
            goto retry;
        }
        g_print("Unable to connect\n");
        G_UNLOCK(connect);
        return FALSE;
    }
    server_fd = fd;
    G_UNLOCK(connect);
#if GLIB_CHECK_VERSION(2, 32, 0)
    g_thread_new("menu-cache-io", server_io_thread, GINT_TO_POINTER(fd));
#else
    g_thread_create(server_io_thread, GINT_TO_POINTER(fd), FALSE, NULL);
#endif
    return TRUE;
}

#define CACHE_VERSION __num2str(VER_MAJOR) "." __num2str(VER_MINOR)
#define __num2str(s) __def2str(s)
#define __def2str(s) #s

static inline char *_validate_env(const char *env)
{
    char *res, *c;

    if (env)
        res = g_strdup(env);
    else
        res = g_strdup("");
    for (c = res; *c; c++)
        if (*c == '\n' || *c == '\t')
            *c = ' ';
    return res;
}

static MenuCache* menu_cache_create(const char* menu_name)
{
    MenuCache* cache;
    const gchar * const * langs = g_get_language_names();
    const char* xdg_cfg_env = g_getenv("XDG_CONFIG_DIRS");
    const char* xdg_prefix_env = g_getenv("XDG_MENU_PREFIX");
    const char* xdg_data_env = g_getenv("XDG_DATA_DIRS");
    const char* xdg_cfg_home_env = g_getenv("XDG_CONFIG_HOME");
    const char* xdg_data_home_env = g_getenv("XDG_DATA_HOME");
    const char* xdg_cache_home_env = g_getenv("XDG_CACHE_HOME");
    char *xdg_cfg, *xdg_prefix, *xdg_data, *xdg_cfg_home, *xdg_data_home, *xdg_cache_home;
    char* buf;
    const char* md5;
    char* file_name;
    int len = 0;
    GChecksum *sum;
    char *langs_list;

    xdg_cfg = _validate_env(xdg_cfg_env);
    xdg_prefix = _validate_env(xdg_prefix_env);
    xdg_data = _validate_env(xdg_data_env);
    xdg_cfg_home = _validate_env(xdg_cfg_home_env);
    xdg_data_home = _validate_env(xdg_data_home_env);
    xdg_cache_home = _validate_env(xdg_cache_home_env);

    /* reconstruct languages list in form as it should be in $LANGUAGES */
    langs_list = g_strjoinv(":", (char **)langs);
    for (buf = langs_list; *buf; buf++) /* reusing buf var as char pointer */
        if (*buf == '\n' || *buf == '\t')
            *buf = ' ';

    buf = g_strdup_printf( "REG:%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t" CACHE_VERSION
                           "\t00000000000000000000000000000000\n",
                            menu_name,
                            langs_list,
                            xdg_cache_home,
                            xdg_cfg,
                            xdg_prefix,
                            xdg_data,
                            xdg_cfg_home,
                            xdg_data_home );

    /* calculate the md5 sum of menu name + lang + all environment variables */
    sum = g_checksum_new(G_CHECKSUM_MD5);
    len = strlen(buf);
    g_checksum_update(sum, (guchar*)buf + 4, len - 38);
    md5 = g_checksum_get_string(sum);
    file_name = g_build_filename( g_get_user_cache_dir(), "menus", md5, NULL );
    DEBUG("cache file_name = %s", file_name);
    cache = menu_cache_new( file_name );
    cache->reg = buf;
    cache->md5 = buf + len - 33;
    memcpy( cache->md5, md5, 32 );
    cache->menu_name = g_strdup(menu_name);
    g_free( file_name );
    g_free(langs_list);
    g_free(xdg_cfg);
    g_free(xdg_prefix);
    g_free(xdg_data);
    g_free(xdg_cfg_home);
    g_free(xdg_data_home);
    g_free(xdg_cache_home);

    g_checksum_free(sum); /* md5 is also freed here */

    MENU_CACHE_LOCK;
    g_hash_table_insert( hash, g_strdup(menu_name), cache );
    MENU_CACHE_UNLOCK;

    return cache;
}

static gboolean register_menu_to_server(MenuCache* cache)
{
    ssize_t len = strlen(cache->reg);
    /* FIXME: do unblocking I/O */
    if(write(server_fd, cache->reg, len) < len)
    {
        DEBUG("register_menu_to_server: sending failed");
        return FALSE; /* socket write failed */
    }
    return TRUE;
}

static void unregister_menu_from_server( MenuCache* cache )
{
    char buf[38];
    g_snprintf( buf, 38, "UNR:%s\n", cache->md5 );
    /* FIXME: do unblocking I/O */
    if(write( server_fd, buf, 37 ) <= 0)
    {
        DEBUG("unregister_menu_from_server: sending failed");
    }
}

static gpointer menu_cache_loader_thread(gpointer data)
{
    MenuCache* cache = (MenuCache*)data;

    /* try to connect server now */
    if(!connect_server(cache->cancellable))
    {
        g_print("unable to connect to menu-cached.\n");
        SET_CACHE_READY(cache);
        return NULL;
    }
    /* and request update from server */
    if ((cache->cancellable && g_cancellable_is_cancelled(cache->cancellable)) ||
        !register_menu_to_server(cache))
        SET_CACHE_READY(cache);
    return NULL;
}

/**
 * menu_cache_lookup
 * @menu_name: a menu name
 *
 * Searches for connection to menu-cached for @menu_name. If there is no
 * such connection exist then creates new one. Caller can be notified
 * when cache is (re)loaded by adding callback. Caller should check if
 * the cache is already loaded trying to retrieve its root.
 *
 * See also: menu_cache_add_reload_notify(), menu_cache_item_dup_parent().
 *
 * Returns: (transfer full): menu cache descriptor.
 *
 * Since: 0.1.0
 */
MenuCache* menu_cache_lookup( const char* menu_name )
{
    MenuCache* cache;

    /* lookup in a hash table for already loaded menus */
    MENU_CACHE_LOCK;
#if !GLIB_CHECK_VERSION(2, 32, 0)
    /* FIXME: destroy them on application exit? */
    if(!sync_run_mutex)
        sync_run_mutex = g_mutex_new();
    if(!sync_run_cond)
        sync_run_cond = g_cond_new();
#endif
    if( G_UNLIKELY( ! hash ) )
        hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL );
    else
    {
        cache = (MenuCache*)g_hash_table_lookup(hash, menu_name);
        if( cache )
        {
            menu_cache_ref(cache);
            MENU_CACHE_UNLOCK;
            return cache;
        }
    }
    MENU_CACHE_UNLOCK;

    cache = menu_cache_create(menu_name);
    cache->cancellable = g_cancellable_new();
#if GLIB_CHECK_VERSION(2, 32, 0)
    cache->thr = g_thread_new(menu_name, menu_cache_loader_thread, cache);
#else
    cache->thr = g_thread_create(menu_cache_loader_thread, cache, TRUE, NULL);
#endif
    return cache;
}

/**
 * menu_cache_lookup_sync
 * @menu_name: a menu name
 *
 * Searches for data from menu-cached for @menu_name. If no connection
 * exists yet then creates new one and retrieves all data.
 *
 * Returns: (transfer full): menu cache descriptor.
 *
 * Since: 0.3.1
 */
MenuCache* menu_cache_lookup_sync( const char* menu_name )
{
    MenuCache* mc = menu_cache_lookup(menu_name);
    MenuCacheDir* root_dir = menu_cache_dup_root_dir(mc);
    /* ensure that the menu cache is loaded */
    if(root_dir)
        menu_cache_item_unref(MENU_CACHE_ITEM(root_dir));
    else /* if it's not yet loaded */
    {
        MenuCacheNotifyId notify_id;
        /* add stub */
        notify_id = menu_cache_add_reload_notify(mc, NULL, NULL);
#if GLIB_CHECK_VERSION(2, 32, 0)
        g_mutex_lock(&sync_run_mutex);
        while(!mc->ready)
            g_cond_wait(&sync_run_cond, &sync_run_mutex);
        g_mutex_unlock(&sync_run_mutex);
#else
        g_mutex_lock(sync_run_mutex);
        g_debug("menu_cache_lookup_sync: enter wait %p", mc);
        while(!mc->ready)
            g_cond_wait(sync_run_cond, sync_run_mutex);
        g_debug("menu_cache_lookup_sync: leave wait");
        g_mutex_unlock(sync_run_mutex);
#endif
        menu_cache_remove_reload_notify(mc, notify_id);
    }
    return mc;
}

static GSList* list_app_in_dir(MenuCacheDir* dir, GSList* list)
{
    GSList* l;
    for( l = dir->children; l; l = l->next )
    {
        MenuCacheItem* item = MENU_CACHE_ITEM(l->data);
        switch( menu_cache_item_get_type(item) )
        {
        case MENU_CACHE_TYPE_DIR:
            list = list_app_in_dir( MENU_CACHE_DIR(item), list );
            break;
        case MENU_CACHE_TYPE_APP:
            list = g_slist_prepend(list, menu_cache_item_ref(item));
            break;
        case MENU_CACHE_TYPE_NONE:
        case MENU_CACHE_TYPE_SEP:
            break;
        }
    }
    return list;
}

/**
 * menu_cache_list_all_apps
 * @cache: a menu cache descriptor
 *
 * Retrieves full list of applications in menu cache. Returned list
 * should be freed with g_slist_free_full(list, menu_cache_item_unref)
 * after usage.
 *
 * Returns: (transfer full) (element-type MenuCacheItem): list of items.
 *
 * Since: 0.1.2
 */
GSList* menu_cache_list_all_apps(MenuCache* cache)
{
    GSList* list;
    MENU_CACHE_LOCK;
    if (G_UNLIKELY(!cache->root_dir)) /* empty cache */
        list = NULL;
    else
        list = list_app_in_dir(cache->root_dir, NULL);
    MENU_CACHE_UNLOCK;
    return list;
}

/**
 * menu_cache_get_desktop_env_flag
 * @cache: a menu cache descriptor
 * @desktop_env: desktop environment name
 *
 * Makes bit mask of desktop environment from its name. The @desktop_env
 * may be simple string or colon separated list of compatible session
 * names according to XDG_CURRENT_DESKTOP freedesktop.org specification.
 *
 * Returns: DE bit mask.
 *
 * Since: 0.2.0
 */
guint32 menu_cache_get_desktop_env_flag( MenuCache* cache, const char* desktop_env )
{
    char** de;
    char **envs;
    guint32 flags = 0;
    int j;

    if (desktop_env == NULL || desktop_env[0] == '\0')
        return flags;

    envs = g_strsplit(desktop_env, ":", -1);
    MENU_CACHE_LOCK;
    de = cache->known_des;
    for (j = 0; envs[j]; j++)
    {
        if( de )
        {
            int i;
            for( i = 0; de[i]; ++i )
                if (strcmp(envs[j], de[i]) == 0)
                    break;
            if (de[i])
            {
                flags |= 1 << (i + N_KNOWN_DESKTOPS);
                continue;
            }
        }
        if (strcmp(envs[j], "GNOME") == 0)
            flags |= SHOW_IN_GNOME;
        else if (strcmp(envs[j], "KDE") == 0)
            flags |= SHOW_IN_KDE;
        else if (strcmp(envs[j], "XFCE") == 0)
            flags |= SHOW_IN_XFCE;
        else if (strcmp(envs[j], "LXDE") == 0)
            flags |= SHOW_IN_LXDE;
        else if (strcmp(envs[j], "ROX") == 0)
            flags |= SHOW_IN_ROX;
    }
    MENU_CACHE_UNLOCK;
    g_strfreev(envs);
    return flags;
}

static MenuCacheItem *_scan_by_id(MenuCacheItem *item, const char *id)
{
    GSList *l;

    if (item)
        switch (menu_cache_item_get_type(item))
        {
            case MENU_CACHE_TYPE_DIR:
                for (l = MENU_CACHE_DIR(item)->children; l; l = l->next)
                {
                    item = _scan_by_id(MENU_CACHE_ITEM(l->data), id);
                    if (item)
                        return item;
                }
                break;
            case MENU_CACHE_TYPE_APP:
                if (g_strcmp0(menu_cache_item_get_id(item), id) == 0)
                    return item;
                break;
            default: ;
        }
    return NULL;
}

/**
 * menu_cache_find_item_by_id
 * @cache: a menu cache descriptor
 * @id: item ID (name such as 'application.desktop')
 *
 * Searches if @id already exists within @cache and returns found item.
 * Returned data should be freed with menu_cache_item_unref() after usage.
 *
 * Returns: (transfer full): found item or %NULL.
 *
 * Since: 0.5.0
 */
MenuCacheItem *menu_cache_find_item_by_id(MenuCache *cache, const char *id)
{
    MenuCacheItem *item = NULL;

    MENU_CACHE_LOCK;
    if (cache && id)
        item = _scan_by_id(MENU_CACHE_ITEM(cache->root_dir), id);
    if (item)
        menu_cache_item_ref(item);
    MENU_CACHE_UNLOCK;
    return item;
}

static GSList* list_app_in_dir_for_cat(MenuCacheDir *dir, GSList *list, const char *id)
{
    const char **cat;
    GSList *l;

    for (l = dir->children; l; l = l->next)
    {
        MenuCacheItem *item = MENU_CACHE_ITEM(l->data);
        switch (item->type)
        {
        case MENU_CACHE_TYPE_DIR:
            list = list_app_in_dir_for_cat(MENU_CACHE_DIR(item), list, id);
            break;
        case MENU_CACHE_TYPE_APP:
            cat = MENU_CACHE_APP(item)->categories;
            if (cat) while (*cat)
                if (*cat++ == id)
                {
                    list = g_slist_prepend(list, menu_cache_item_ref(item));
                    break;
                }
            break;
        case MENU_CACHE_TYPE_NONE:
        case MENU_CACHE_TYPE_SEP:
            break;
        }
    }
    return list;
}

/**
 * menu_cache_list_all_for_category
 * @cache: a menu cache descriptor
 * @category: category to list items
 *
 * Retrieves list of applications in menu cache which have @category in
 * their list of categories. The search is case-sensitive. Returned list
 * should be freed with g_slist_free_full(list, menu_cache_item_unref)
 * after usage.
 *
 * Returns: (transfer full) (element-type MenuCacheItem): list of items.
 *
 * Since: 1.0.0
 */
GSList *menu_cache_list_all_for_category(MenuCache* cache, const char *category)
{
    GQuark q;
    GSList *list;

    g_return_val_if_fail(cache != NULL && category != NULL, NULL);
    q = g_quark_try_string(category);
    if (q == 0)
        return NULL;
    MENU_CACHE_LOCK;
    if (G_UNLIKELY(cache->root_dir == NULL))
        list = NULL;
    else
        list = list_app_in_dir_for_cat(cache->root_dir, NULL, g_quark_to_string(q));
    MENU_CACHE_UNLOCK;
    return list;
}

static GSList* list_app_in_dir_for_kw(MenuCacheDir *dir, GSList *list, const char *kw)
{
    GSList *l;

    for (l = dir->children; l; l = l->next)
    {
        MenuCacheItem *item = MENU_CACHE_ITEM(l->data);
        switch (item->type)
        {
        case MENU_CACHE_TYPE_DIR:
            list = list_app_in_dir_for_kw(MENU_CACHE_DIR(item), list, kw);
            break;
        case MENU_CACHE_TYPE_APP:
            if (strstr(MENU_CACHE_APP(item)->keywords, kw) != NULL)
                list = g_slist_prepend(list, menu_cache_item_ref(item));
            break;
        case MENU_CACHE_TYPE_NONE:
        case MENU_CACHE_TYPE_SEP:
            break;
        }
    }
    return list;
}

/**
 * menu_cache_list_all_for_keyword
 * @cache: a menu cache descriptor
 * @keyword: a keyword to search
 *
 * Retrieves list of applications in menu cache which have a @keyword
 * as either a word or part of word in exec command, name, generic name
 * or defined keywords. The search is case-insensitive. Returned list
 * should be freed with g_slist_free_full(list, menu_cache_item_unref)
 * after usage.
 *
 * Returns: (transfer full) (element-type MenuCacheItem): list of items.
 *
 * Since: 1.0.0
 */
GSList *menu_cache_list_all_for_keyword(MenuCache* cache, const char *keyword)
{
    char *casefolded = g_utf8_casefold(keyword, -1);
    GSList *list;

    g_return_val_if_fail(cache != NULL && keyword != NULL, NULL);
    MENU_CACHE_LOCK;
    if (G_UNLIKELY(cache->root_dir == NULL))
        list = NULL;
    else
        list = list_app_in_dir_for_kw(cache->root_dir, NULL, casefolded);
    MENU_CACHE_UNLOCK;
    g_free(casefolded);
    return list;
}
