/*
 *      fm-folder-model.h
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2012-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifndef _FM_FOLDER_MODEL_H_
#define _FM_FOLDER_MODEL_H_

#include "fm-module.h"
#include "fm-folder.h"
#include "fm-sortable.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>

#include <sys/types.h>

G_BEGIN_DECLS

#define FM_TYPE_FOLDER_MODEL             (fm_folder_model_get_type())
#define FM_FOLDER_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),  FM_TYPE_FOLDER_MODEL, FmFolderModel))
#define FM_FOLDER_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  FM_TYPE_FOLDER_MODEL, FmFolderModelClass))
#define FM_IS_FOLDER_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FM_TYPE_FOLDER_MODEL))
#define FM_IS_FOLDER_MODEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  FM_TYPE_FOLDER_MODEL))
#define FM_FOLDER_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  FM_TYPE_FOLDER_MODEL, FmFolderModelClass))

/**
 * FmFolderModelCol:
 * @FM_FOLDER_MODEL_COL_GICON: (#GIcon *) icon descriptor
 * @FM_FOLDER_MODEL_COL_ICON: (#GdkPixbuf *) icon image
 * @FM_FOLDER_MODEL_COL_NAME: (#gchar *) file display name
 * @FM_FOLDER_MODEL_COL_SIZE: (#gchar *) file size text
 * @FM_FOLDER_MODEL_COL_DESC: (#gchar *) file MIME description
 * @FM_FOLDER_MODEL_COL_PERM: (#gchar *) file permissions like "rw-r--r--"
 * @FM_FOLDER_MODEL_COL_OWNER: (#gchar *) file owner username
 * @FM_FOLDER_MODEL_COL_MTIME: (#gchar *) modification time text
 * @FM_FOLDER_MODEL_COL_INFO: (#FmFileInfo *) file info
 * @FM_FOLDER_MODEL_COL_DIRNAME: (#gchar *) path of dir containing the file
 * @FM_FOLDER_MODEL_COL_EXT: (#gchar *) (since 1.2.0) last suffix of file name
 *
 * Columns of folder view
 */
typedef enum {
    FM_FOLDER_MODEL_COL_GICON = 0,
    FM_FOLDER_MODEL_COL_ICON,
    FM_FOLDER_MODEL_COL_NAME,
    FM_FOLDER_MODEL_COL_SIZE,
    FM_FOLDER_MODEL_COL_DESC,
    FM_FOLDER_MODEL_COL_PERM,
    FM_FOLDER_MODEL_COL_OWNER,
    FM_FOLDER_MODEL_COL_MTIME,
    FM_FOLDER_MODEL_COL_INFO,
    FM_FOLDER_MODEL_COL_DIRNAME,
    FM_FOLDER_MODEL_COL_EXT,
    /*< private >*/
    FM_FOLDER_MODEL_N_COLS
} FmFolderModelCol;

/**
 * FM_FOLDER_MODEL_COL_UNSORTED:
 *
 * for 'Unsorted' folder view use 'FileInfo' column which is ambiguous for sorting
 */
#define FM_FOLDER_MODEL_COL_UNSORTED FM_FOLDER_MODEL_COL_INFO

/**
 * FM_FOLDER_MODEL_COL_DEFAULT:
 *
 * value which means do not change sorting column.
 */
#define FM_FOLDER_MODEL_COL_DEFAULT ((FmFolderModelCol)-1)

#ifndef FM_DISABLE_DEPRECATED
/* for backward compatiblity, kept until soname 6 */
#define FmFolderModelViewCol    FmFolderModelCol
#define COL_FILE_GICON          FM_FOLDER_MODEL_COL_GICON
#define COL_FILE_ICON           FM_FOLDER_MODEL_COL_ICON
#define COL_FILE_NAME           FM_FOLDER_MODEL_COL_NAME
#define COL_FILE_SIZE           FM_FOLDER_MODEL_COL_SIZE
#define COL_FILE_DESC           FM_FOLDER_MODEL_COL_DESC
#define COL_FILE_PERM           FM_FOLDER_MODEL_COL_PERM
#define COL_FILE_OWNER          FM_FOLDER_MODEL_COL_OWNER
#define COL_FILE_MTIME          FM_FOLDER_MODEL_COL_MTIME
#define COL_FILE_INFO           FM_FOLDER_MODEL_COL_INFO
#define COL_FILE_UNSORTED       FM_FOLDER_MODEL_COL_UNSORTED
#define FM_FOLDER_MODEL_COL_IS_VALID(col) fm_folder_model_col_is_valid((guint)col)
#endif

typedef struct _FmFolderModel FmFolderModel;
typedef struct _FmFolderModelClass FmFolderModelClass;

/**
 * FmFolderModelClass:
 * @parent: the parent class
 * @row_deleting: the class closure for the #FmFolderModel::row-deleting signal
 * @filter_changed: the class closure for the #FmFolderModel::filter-changed signal
 */
struct _FmFolderModelClass
{
    GObjectClass parent;
    void (*row_deleting)(FmFolderModel* model, GtkTreePath* tp,
                         GtkTreeIter* iter, gpointer data);
    void (*filter_changed)(FmFolderModel* model);
    /*< private >*/
    gpointer _reserved1;
    gpointer _reserved2;
};

/**
 * FmFolderModelFilterFunc:
 * @file: the file to check filtering
 * @user_data: data supplied to fm_folder_model_add_filter()
 *
 * A callback used by #FmFolderModel to filter visible items in the model.
 *
 * Returns: %TRUE if @file should be visible.
 *
 * Since: 1.0.2
 */
typedef gboolean (*FmFolderModelFilterFunc)(FmFileInfo* file, gpointer user_data);

GType fm_folder_model_get_type (void);

FmFolderModel *fm_folder_model_new( FmFolder* dir, gboolean show_hidden );

void fm_folder_model_set_folder( FmFolderModel* model, FmFolder* dir );
FmFolder* fm_folder_model_get_folder(FmFolderModel* model);
FmPath* fm_folder_model_get_folder_path(FmFolderModel* model);

void fm_folder_model_set_item_userdata(FmFolderModel* model, GtkTreeIter* it,
                                       gpointer user_data);
gpointer fm_folder_model_get_item_userdata(FmFolderModel* model, GtkTreeIter* it);

gboolean fm_folder_model_get_show_hidden( FmFolderModel* model );

void fm_folder_model_set_show_hidden( FmFolderModel* model, gboolean show_hidden );

void fm_folder_model_file_created( FmFolderModel* model, FmFileInfo* file);

void fm_folder_model_file_deleted( FmFolderModel* model, FmFileInfo* file);

void fm_folder_model_file_changed( FmFolderModel* model, FmFileInfo* file);

gboolean fm_folder_model_find_iter_by_filename( FmFolderModel* model, GtkTreeIter* it, const char* name);

void fm_folder_model_set_icon_size(FmFolderModel* model, guint icon_size);
guint fm_folder_model_get_icon_size(FmFolderModel* model);

void fm_folder_model_add_filter(FmFolderModel* model, FmFolderModelFilterFunc func, gpointer user_data);
void fm_folder_model_remove_filter(FmFolderModel* model, FmFolderModelFilterFunc func, gpointer user_data);
void fm_folder_model_apply_filters(FmFolderModel* model);

void fm_folder_model_set_sort(FmFolderModel* model, FmFolderModelCol col, FmSortMode mode);
gboolean fm_folder_model_get_sort(FmFolderModel* model, FmFolderModelCol *col, FmSortMode *mode);

/* void fm_folder_model_set_thumbnail_size(FmFolderModel* model, guint size); */

/**
 * FmFolderModelExtraFilePos:
 * @FM_FOLDER_MODEL_ITEMPOS_SORTED: insert extra item into main sorted list
 * @FM_FOLDER_MODEL_ITEMPOS_PRE: insert extra item before main list
 * @FM_FOLDER_MODEL_ITEMPOS_POST: insert extra item after main list
 *
 * Where the fm_folder_model_extra_file_add() should insert extra file item.
 */
typedef enum
{
    FM_FOLDER_MODEL_ITEMPOS_SORTED = 0,
    FM_FOLDER_MODEL_ITEMPOS_PRE,
    FM_FOLDER_MODEL_ITEMPOS_POST
} FmFolderModelExtraFilePos;

gboolean fm_folder_model_extra_file_add(FmFolderModel* model, FmFileInfo* file,
                                        FmFolderModelExtraFilePos where);
gboolean fm_folder_model_extra_file_remove(FmFolderModel* model, FmFileInfo* file);

/* APIs for FmFolderModelCol */

const char* fm_folder_model_col_get_title(FmFolderModel* model, FmFolderModelCol col_id);
gboolean fm_folder_model_col_is_sortable(FmFolderModel* model, FmFolderModelCol col_id);
const char* fm_folder_model_col_get_name(FmFolderModelCol col_id);
FmFolderModelCol fm_folder_model_get_col_by_name(const char* str);
gint fm_folder_model_col_get_default_width(FmFolderModel* model, FmFolderModelCol col_id);
gboolean fm_folder_model_col_is_valid(FmFolderModelCol col_id);

/* --- columns extensions --- */
typedef struct _FmFolderModelColumnInit FmFolderModelColumnInit;

/**
 * FmFolderModelColumnInit:
 * @title: column title
 * @default_width: default width for column (0 means auto)
 * @get_type: function to get #GType of column data
 * @get_value: function to retrieve column data
 * @compare: sorting routine (%NULL if column isn't sortable)
 *
 * This structure is used for "gtk_folder_col" module initialization. The
 * key for module of this type is new unique column name.
 *
 * Since: 1.2.0
 */
struct _FmFolderModelColumnInit
{
    const char *title;
    gint default_width;
    GType (*get_type)(void);
    void (*get_value)(FmFileInfo *fi, GValue *value);
    gint (*compare)(FmFileInfo *fi1, FmFileInfo *fi2);
};

FmFolderModelCol fm_folder_model_add_custom_column(const char* name, FmFolderModelColumnInit* init);

/* modules */
#define FM_MODULE_gtk_folder_col_VERSION 1
extern FmFolderModelColumnInit fm_module_init_gtk_folder_col;

void _fm_folder_model_init(void);
void _fm_folder_model_finalize(void);

G_END_DECLS

#endif
