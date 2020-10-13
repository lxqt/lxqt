//      fm-dir-tree-model.h
//
//      Copyright 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
//      Copyright 2012 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.


#ifndef __FM_DIR_TREE_MODEL_H__
#define __FM_DIR_TREE_MODEL_H__

#include <gtk/gtk.h>
#include <glib-object.h>
#include "fm-file-info.h"

#include "fm-seal.h"

G_BEGIN_DECLS


#define FM_TYPE_DIR_TREE_MODEL                (fm_dir_tree_model_get_type())
#define FM_DIR_TREE_MODEL(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            FM_TYPE_DIR_TREE_MODEL, FmDirTreeModel))
#define FM_DIR_TREE_MODEL_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST((klass),\
            FM_TYPE_DIR_TREE_MODEL, FmDirTreeModelClass))
#define FM_IS_DIR_TREE_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            FM_TYPE_DIR_TREE_MODEL))
#define FM_IS_DIR_TREE_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),\
            FM_TYPE_DIR_TREE_MODEL))
#define FM_DIR_TREE_MODEL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj),\
            FM_TYPE_DIR_TREE_MODEL, FmDirTreeModelClass))

/**
 * FmDirTreeModelCol:
 * @FM_DIR_TREE_MODEL_COL_ICON: (#GdkPixbuf *) icon
 * @FM_DIR_TREE_MODEL_COL_DISP_NAME: (#char *) displayed name
 * @FM_DIR_TREE_MODEL_COL_INFO: (#FmFileInfo *) file info
 * @FM_DIR_TREE_MODEL_COL_PATH: (#FmPath *) file path
 * @FM_DIR_TREE_MODEL_COL_FOLDER: (#FmFolder *) folder object
 *
 * Columns of dir tree model
 */
typedef enum {
    FM_DIR_TREE_MODEL_COL_ICON,
    FM_DIR_TREE_MODEL_COL_DISP_NAME,
    FM_DIR_TREE_MODEL_COL_INFO,
    FM_DIR_TREE_MODEL_COL_PATH,
    FM_DIR_TREE_MODEL_COL_FOLDER,
    /*<private>*/
    N_FM_DIR_TREE_MODEL_COLS
} FmDirTreeModelCol;

typedef struct _FmDirTreeModel            FmDirTreeModel;
typedef struct _FmDirTreeModelClass        FmDirTreeModelClass;

struct _FmDirTreeModel
{
    GObject parent;
    /*<private>*/
    GList* FM_SEAL(roots);
    gint FM_SEAL(stamp);
    guint FM_SEAL(icon_size);
    gboolean FM_SEAL(show_hidden);
    gpointer _reserved1;
    gpointer _reserved2;
};

/**
 * FmDirTreeModelClass:
 * @parent_class: the parent class
 * @row_loaded: the class closure for the #FmDirTreeModel::row-loaded signal
 */
struct _FmDirTreeModelClass
{
    GObjectClass parent_class;
    void (*row_loaded)(FmDirTreeModel* model, GtkTreePath* row);
    /*<private>*/
    gpointer _reserved1;
};


GType fm_dir_tree_model_get_type(void);
FmDirTreeModel* fm_dir_tree_model_new(void);

void fm_dir_tree_model_add_root(FmDirTreeModel* model, FmFileInfo* root, GtkTreeIter* iter);

void fm_dir_tree_model_load_row(FmDirTreeModel* model, GtkTreeIter* it, GtkTreePath* tp);
void fm_dir_tree_model_unload_row(FmDirTreeModel* model, GtkTreeIter* it, GtkTreePath* tp);

void fm_dir_tree_model_set_icon_size(FmDirTreeModel* model, guint icon_size);
guint fm_dir_tree_model_get_icon_size(FmDirTreeModel* model);

gboolean fm_dir_tree_row_is_loaded(FmDirTreeModel* model, GtkTreeIter* iter);
GdkPixbuf* fm_dir_tree_row_get_icon(FmDirTreeModel* model, GtkTreeIter* iter);
FmFileInfo* fm_dir_tree_row_get_file_info(FmDirTreeModel* model, GtkTreeIter* iter);
FmPath* fm_dir_tree_row_get_file_path(FmDirTreeModel* model, GtkTreeIter* iter);
const char* fm_dir_tree_row_get_disp_name(FmDirTreeModel* model, GtkTreeIter* iter);

void fm_dir_tree_model_set_show_hidden(FmDirTreeModel* model, gboolean show_hidden);
gboolean fm_dir_tree_model_get_show_hidden(FmDirTreeModel* model);

/* void fm_dir_tree_model_reload(FmDirTreeModel* model); */

G_END_DECLS

#endif /* __FM_DIR_TREE_MODEL_H__ */
