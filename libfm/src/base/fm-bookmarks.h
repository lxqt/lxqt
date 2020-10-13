/*
 *      fm-bookmarks.h
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
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


#ifndef __FM_BOOKMARKS_H__
#define __FM_BOOKMARKS_H__

#include <glib-object.h>
#include <gio/gio.h>
#include "fm-path.h"

#include "fm-seal.h"

G_BEGIN_DECLS

#define FM_BOOKMARKS_TYPE               (fm_bookmarks_get_type())
#define FM_BOOKMARKS(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            FM_BOOKMARKS_TYPE, FmBookmarks))
#define FM_BOOKMARKS_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST((klass),\
            FM_BOOKMARKS_TYPE, FmBookmarksClass))
#define FM_IS_BOOKMARKS(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            FM_BOOKMARKS_TYPE))
#define FM_IS_BOOKMARKS_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),\
            FM_BOOKMARKS_TYPE))

typedef struct _FmBookmarks         FmBookmarks;
typedef struct _FmBookmarksClass        FmBookmarksClass;
typedef struct _FmBookmarkItem       FmBookmarkItem;

/**
 * FmBookmarkItem
 * @name: display name of bookmark
 * @path: path to bookmarked directory
 */
struct _FmBookmarkItem
{
    char* name;
    FmPath* path;
    /*<private>*/
    gpointer _reserved1;
    int FM_SEAL(n_ref);
};

struct _FmBookmarks
{
    GObject parent;
    /*< private >*/
    GFile *FM_SEAL(file);
    GFileMonitor* FM_SEAL(mon);
    GList* FM_SEAL(items);
    gpointer _reserved1;
};

/**
 * FmBookmarksClass
 * @parent_class: the parent class
 * @changed: the class closure for #FmBookmarks::changed signal
 */
struct _FmBookmarksClass
{
    GObjectClass parent_class;
    void (*changed)(FmBookmarks*);
};

GType fm_bookmarks_get_type(void);
FmBookmarks* fm_bookmarks_dup(void);

#define fm_bookmarks_append(bookmarks, path, name)  fm_bookmarks_insert(bookmarks, path, name, -1)
FmBookmarkItem* fm_bookmarks_insert(FmBookmarks* bookmarks, FmPath* path, const char* name, int pos);
void fm_bookmarks_remove(FmBookmarks* bookmarks, FmBookmarkItem* item);
void fm_bookmarks_reorder(FmBookmarks* bookmarks, FmBookmarkItem* item, int pos);
void fm_bookmarks_rename(FmBookmarks* bookmarks, FmBookmarkItem* item, const char* new_name);

#ifndef FM_DISABLE_DEPRECATED
/* list all bookmark items in current bookmarks */
const GList* fm_bookmarks_list_all(FmBookmarks* bookmarks);
#endif

FmBookmarkItem* fm_bookmark_item_ref(FmBookmarkItem* item);
void fm_bookmark_item_unref(FmBookmarkItem *item);
GList* fm_bookmarks_get_all(FmBookmarks* bookmarks);

G_END_DECLS

#endif /* __FM_BOOKMARKS_H__ */
