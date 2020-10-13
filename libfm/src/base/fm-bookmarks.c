/*
 *      fm-bookmarks.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2012-2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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
 * SECTION:fm-bookmarks
 * @short_description: Bookmarks support for libfm.
 * @title: FmBookmarks
 *
 * @include: libfm/fm.h
 *
 * The application that uses libfm can use user-wide bookmark list via
 * class FmBookmarks.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define FM_DISABLE_SEAL

#include "fm-bookmarks.h"
#include <stdio.h>
#include <string.h>
#include "fm-utils.h"

enum
{
    CHANGED,
    N_SIGNALS
};

static FmBookmarks* fm_bookmarks_new (void);

static void fm_bookmarks_finalize           (GObject *object);
static GList* load_bookmarks(GFile *file);

G_DEFINE_TYPE(FmBookmarks, fm_bookmarks, G_TYPE_OBJECT);

G_LOCK_DEFINE_STATIC(bookmarks);

static FmBookmarks* singleton = NULL;
static guint signals[N_SIGNALS];

static guint idle_handler = 0;

static void fm_bookmarks_class_init(FmBookmarksClass *klass)
{
    GObjectClass *g_object_class;
    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->finalize = fm_bookmarks_finalize;

    /**
     * FmBookmarks::changed:
     * @bookmarks: pointer to bookmarks list singleton descriptor
     *
     * The "changed" signal is emitted when some bookmark item is
     * changed, added, or removed.
     *
     * Since: 0.1.0
     */
    signals[CHANGED] =
        g_signal_new("changed",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmBookmarksClass, changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0 );
}

static void fm_bookmarks_finalize(GObject *object)
{
    FmBookmarks *self;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_BOOKMARKS(object));

    self = FM_BOOKMARKS(object);

    if(idle_handler)
    {
        g_source_remove(idle_handler);
        idle_handler = 0;
    }

    g_list_foreach(self->items, (GFunc)fm_bookmark_item_unref, NULL);
    g_list_free(self->items);

    g_object_unref(self->mon);
    g_object_unref(self->file);

    G_OBJECT_CLASS(fm_bookmarks_parent_class)->finalize(object);
}


static inline char *get_legacy_bookmarks_file(void)
{
    return g_build_filename(fm_get_home_dir(), ".gtk-bookmarks", NULL);
}

static inline char *get_new_bookmarks_file(void)
{
    return g_build_filename(g_get_user_config_dir(), "gtk-3.0", "bookmarks", NULL);
}

static void on_changed( GFileMonitor* mon, GFile* gf, GFile* other,
                    GFileMonitorEvent evt, FmBookmarks* bookmarks )
{
    G_LOCK(bookmarks);
    /* reload bookmarks */
    g_list_foreach(bookmarks->items, (GFunc)fm_bookmark_item_unref, NULL);
    g_list_free(bookmarks->items);

    bookmarks->items = load_bookmarks(bookmarks->file);
    G_UNLOCK(bookmarks);
    g_signal_emit(bookmarks, signals[CHANGED], 0);
}

static FmBookmarkItem* new_item(char* line)
{
    FmBookmarkItem* item = g_slice_new0(FmBookmarkItem);
    char* sep;
    sep = strchr(line, '\n');
    if(sep)
        *sep = '\0';
    sep = strchr(line, ' ');
    if(sep)
        *sep = '\0';

    item->path = fm_path_new_for_uri(line);
    if(sep)
        item->name = g_strdup(sep+1);
    else
        item->name = g_filename_display_name(fm_path_get_basename(item->path));

    item->n_ref = 1;
    return item;
}

static GList* load_bookmarks(GFile *file)
{
    char *fpath = g_file_get_path(file);
    FILE* f;
    char buf[1024];
    FmBookmarkItem* item;
    GList* items = NULL;

    /* load the file */
    f = fopen(fpath, "r");
    g_free(fpath);
    if(f)
    {
        while(fgets(buf, 1024, f))
        {
            item = new_item(buf);
            items = g_list_prepend(items, item);
        }
        fclose(f);
    }
    items = g_list_reverse(items);
    return items;
}

static void fm_bookmarks_init(FmBookmarks *self)
{
    /* trying the XDG-3.0 first and use it if it exists */
    char* fpath = get_new_bookmarks_file();

    self->file = g_file_new_for_path(fpath);
    g_free(fpath);
    self->items = load_bookmarks(self->file);
    if (!self->items) /* not found, use legacy file */
    {
        g_object_unref(self->file);
        fpath = get_legacy_bookmarks_file();
        self->file = g_file_new_for_path(fpath);
        g_free(fpath);
        self->items = load_bookmarks(self->file);
    }
    self->mon = g_file_monitor_file(self->file, 0, NULL, NULL);
    if (self->mon)
        g_signal_connect(self->mon, "changed", G_CALLBACK(on_changed), self);
}

static FmBookmarks *fm_bookmarks_new(void)
{
    return g_object_new(FM_BOOKMARKS_TYPE, NULL);
}

/**
 * fm_bookmarks_dup
 *
 * Returns reference to bookmarks list singleton descriptor.
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Return value: (transfer full): a reference to bookmarks list
 *
 * Since: 0.1.99
 */
FmBookmarks* fm_bookmarks_dup(void)
{
    G_LOCK(bookmarks);
    if( G_LIKELY(singleton) )
        g_object_ref(singleton);
    else
    {
        singleton = fm_bookmarks_new();
        g_object_add_weak_pointer(G_OBJECT(singleton), (gpointer*)&singleton);
    }
    G_UNLOCK(bookmarks);
    return singleton;
}

/**
 * fm_bookmarks_list_all
 * @bookmarks: bookmarks list
 *
 * Returns list of FmBookmarkItem retrieved from bookmarks list. Returned
 * list is owned by bookmarks list and should not be freed by caller.
 *
 * Return value: (transfer none) (element-type FmBookmarkItem): list of bookmark items
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.0.2: Use fm_bookmarks_get_all() instead.
 */
const GList* fm_bookmarks_list_all(FmBookmarks* bookmarks)
{
    return bookmarks->items;
}

/**
 * fm_bookmark_item_ref
 * @item: an item
 *
 * Increases reference counter on @item.
 *
 * Returns: @item.
 *
 * Since: 1.0.2
 */
FmBookmarkItem* fm_bookmark_item_ref(FmBookmarkItem* item)
{
    g_return_val_if_fail(item != NULL, NULL);
    g_atomic_int_inc(&item->n_ref);
    return item;
}

/**
 * fm_bookmark_item_unref
 * @item: item to be freed
 *
 * Decreases reference counter on @item and frees data when it reaches 0.
 *
 * Since: 1.0.2
 */
void fm_bookmark_item_unref(FmBookmarkItem *item)
{
    g_return_if_fail(item != NULL);
    if(g_atomic_int_dec_and_test(&item->n_ref))
    {
        g_free(item->name);
        fm_path_unref(item->path);
        g_slice_free(FmBookmarkItem, item);
    }
}

/**
 * fm_bookmarks_get_all
 * @bookmarks: bookmarks list
 *
 * Returns list of FmBookmarkItem retrieved from bookmarks list. Returned
 * list should be freed with g_list_free_full(list, fm_bookmark_item_unref).
 *
 * Return value: (transfer full) (element-type FmBookmarkItem): list of bookmark items
 *
 * Since: 1.0.2
 */
GList* fm_bookmarks_get_all(FmBookmarks* bookmarks)
{
    GList *copy = NULL, *l;

    G_LOCK(bookmarks);
    for(l = bookmarks->items; l; l = l->next)
    {
        fm_bookmark_item_ref(l->data);
        copy = g_list_prepend(copy, l->data);
    }
    copy = g_list_reverse(copy);
    G_UNLOCK(bookmarks);
    return copy;
}

static gboolean save_bookmarks(FmBookmarks* bookmarks)
{
    FmBookmarkItem* item;
    GList* l;
    GString* buf;
    GError *err = NULL;

    if(g_source_is_destroyed(g_main_current_source()))
        return FALSE;

    buf = g_string_sized_new(1024);
    G_LOCK(bookmarks);
    for( l=bookmarks->items; l; l=l->next )
    {
        char* uri;
        item = (FmBookmarkItem*)l->data;
        uri = fm_path_to_uri(item->path);
        g_string_append(buf, uri);
        g_free(uri);
        g_string_append_c(buf, ' ');
        g_string_append(buf, item->name);
        g_string_append_c(buf, '\n');
    }
    idle_handler = 0;
    G_UNLOCK(bookmarks);

    if (!g_file_replace_contents(bookmarks->file, buf->str, buf->len, NULL,
                                 FALSE, 0, NULL, NULL, &err))
    {
        g_critical("%s", err->message);
        g_error_free(err);
    }

    g_string_free(buf, TRUE);
    /* we changed bookmarks list, let inform who interested in that */
    g_signal_emit(bookmarks, signals[CHANGED], 0);
    return FALSE;
}

static void queue_save_bookmarks(FmBookmarks* bookmarks)
{
    if(!idle_handler)
        idle_handler = g_idle_add((GSourceFunc)save_bookmarks, bookmarks);
}

/**
 * fm_bookmarks_insert
 * @bookmarks: bookmarks list
 * @path: path requested to add to bookmarks
 * @name: name new bookmark will be seen in list with
 * @pos: where to insert a bookmark into list
 *
 * Adds a bookmark into bookmark list. Returned structure is managed by
 * bookmarks list and should not be freed by caller. If you want to save
 * returned data then call fm_bookmark_item_ref() on it.
 *
 * Return value: (transfer none): new created bookmark item
 *
 * Since: 0.1.0
 */
FmBookmarkItem* fm_bookmarks_insert(FmBookmarks* bookmarks, FmPath* path, const char* name, int pos)
{
    FmBookmarkItem* item = g_slice_new0(FmBookmarkItem);
    item->path = fm_path_ref(path);
    item->name = g_strdup(name);
    item->n_ref = 1;
    G_LOCK(bookmarks);
    /* FIXME: disable creation of duplicate bookmark if set in config */
    bookmarks->items = g_list_insert(bookmarks->items, item, pos);
    /* g_debug("insert %s at %d", name, pos); */
    queue_save_bookmarks(bookmarks);
    G_UNLOCK(bookmarks);
    return item;
}

/**
 * fm_bookmarks_remove
 * @bookmarks: bookmarks list
 * @item: bookmark item for deletion
 *
 * Removes a bookmark from bookmark list.
 *
 * Since: 0.1.0
 */
void fm_bookmarks_remove(FmBookmarks* bookmarks, FmBookmarkItem* item)
{
    G_LOCK(bookmarks);
    bookmarks->items = g_list_remove(bookmarks->items, item);
    fm_bookmark_item_unref(item);
    queue_save_bookmarks(bookmarks);
    G_UNLOCK(bookmarks);
}

/**
 * fm_bookmarks_rename
 * @bookmarks: bookmarks list
 * @item: bookmark item which will be changed
 * @new_name: new name for bookmark item to be seen in list
 *
 * Changes name of existing bookmark item.
 *
 * Since: 0.1.0
 */
void fm_bookmarks_rename(FmBookmarks* bookmarks, FmBookmarkItem* item, const char* new_name)
{
    G_LOCK(bookmarks);
    g_free(item->name);
    item->name = g_strdup(new_name);
    queue_save_bookmarks(bookmarks);
    G_UNLOCK(bookmarks);
}

/**
 * fm_bookmarks_reorder
 * @bookmarks: bookmarks list
 * @item: bookmark item which will be changed
 * @pos: new position for bookmark item in list
 *
 * Changes position of existing bookmark item.
 *
 * Since: 0.1.0
 */
void fm_bookmarks_reorder(FmBookmarks* bookmarks, FmBookmarkItem* item, int pos)
{
    G_LOCK(bookmarks);
    bookmarks->items = g_list_remove(bookmarks->items, item);
    bookmarks->items = g_list_insert(bookmarks->items, item, pos);
    queue_save_bookmarks(bookmarks);
    G_UNLOCK(bookmarks);
}
