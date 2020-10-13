//      fm-dir-tree-model.c
//
//      Copyright 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
//      Copyright 2012-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

/**
 * SECTION:fm-dir-tree-model
 * @short_description: A model for directory tree view
 * @title: FmDirTreeModel
 *
 * @include: libfm/fm-gtk.h
 *
 * The #FmDirTreeModel represents tree of folders which can be used by
 * #FmDirTreeView to create tree-like expandable list of directories.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define FM_DISABLE_SEAL

#include "fm-dir-tree-model.h"
#include "fm-folder.h"
#include "fm-icon-pixbuf.h"
#include "fm-config.h"

#include <glib/gi18n-lib.h>
#include <string.h>

typedef struct _FmDirTreeItem FmDirTreeItem;
struct _FmDirTreeItem
{
    FmDirTreeModel* model; /* FIXME: storing model pointer in every item is a waste */
    FmFileInfo* fi;
    FmFolder* folder;
    GdkPixbuf* icon;
    gboolean expanded;
    gboolean loaded;
    GList* parent; /* parent node */
    GList* children; /* child items */
    GList* hidden_children;
};

static GType column_types[N_FM_DIR_TREE_MODEL_COLS];

enum
{
    ROW_LOADED,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

static void fm_dir_tree_model_dispose            (GObject *object);
static void fm_dir_tree_model_tree_model_init(GtkTreeModelIface *iface);
static GtkTreePath *fm_dir_tree_model_get_path ( GtkTreeModel *tree_model, GtkTreeIter *iter );

static inline void item_to_tree_iter(FmDirTreeModel* model, GList* item_l, GtkTreeIter* it);
static inline GtkTreePath* item_to_tree_path(FmDirTreeModel* model, GList* item_l);

G_DEFINE_TYPE_WITH_CODE( FmDirTreeModel, fm_dir_tree_model, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_MODEL, fm_dir_tree_model_tree_model_init)
                        /* G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_DRAG_SOURCE, fm_dir_tree_model_drag_source_init)
                        G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_DRAG_DEST, fm_dir_tree_model_drag_dest_init) */
                        )

/* a varient of g_list_foreach which does the same thing, but pass GList* element
 * itself as the first parameter to func(), not the element data. */
static inline void _g_list_foreach_l(GList* list, GFunc func, gpointer user_data)
{
    while (list)
    {
        GList *next = list->next;
        (*func)(list, user_data);
        list = next;
    }
}

/*
FIXME: this is convenience to have expanders actualized but it may be expensive
static void item_queue_subdir_check(FmDirTreeModel* model, GList* item_l);
*/

static void item_reload_icon(FmDirTreeModel* model, GList* item_l, GtkTreePath* tp)
{
    GtkTreeIter it;
    GList* l;
    FmDirTreeItem *item, *child;

    g_return_if_fail(item_l && tp);
    item = item_l->data;
    g_return_if_fail(item);

    if(item->icon)
    {
        g_object_unref(item->icon);
        item->icon = NULL;
        item_to_tree_iter(model, item_l, &it);
        gtk_tree_model_row_changed(GTK_TREE_MODEL(model), tp, &it);
    }

    if(item->children)
    {
        gtk_tree_path_append_index(tp, 0);
        for(l = item->children; l; l=l->next)
        {
            child = (FmDirTreeItem*)l->data;
            item_reload_icon(model, l, tp);
            gtk_tree_path_next(tp);
        }
        gtk_tree_path_up(tp);
    }

    for(l = item->hidden_children; l; l=l->next)
    {
        child = (FmDirTreeItem*)l->data;
        if(child->icon)
        {
            g_object_unref(child->icon);
            child->icon = NULL;
        }
    }
}

static void fm_dir_tree_model_class_init(FmDirTreeModelClass *klass)
{
    GObjectClass *g_object_class;
    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->dispose = fm_dir_tree_model_dispose;
    /* use finalize from parent class */

    /**
     * FmDirTreeModel::row-loaded:
     * @model: dir tree model instance that received the signal
     * @row:   path to folder row that is ready
     *
     * The #FmDirTreeModel::row-loaded signal is emitted when content of
     * folder @row is completely retrieved. It may happen either
     * after call to fm_dir_tree_model_load_row()
     * or in time of that call (in case if the folder was already cached in
     * memory).
     *
     * See also: fm_dir_tree_model_unload_row().
     *
     * Since: 1.0.0
     */
    signals[ROW_LOADED] =
        g_signal_new("row-loaded",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmDirTreeModelClass, row_loaded),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__BOXED,
                     G_TYPE_NONE, 1, GTK_TYPE_TREE_PATH);
}


static inline FmDirTreeItem* fm_dir_tree_item_new(FmDirTreeModel* model, GList* parent_l)
{
    FmDirTreeItem* item = g_slice_new0(FmDirTreeItem);
    item->model = model;
    item->parent = parent_l;
    return item;
}

static inline void item_free_folder(FmFolder* item, gpointer item_l);

static void fm_dir_tree_item_free_l(GList* item_l);

/* Note: item_l below may be already freed so unusable as GList */
static inline void fm_dir_tree_item_free(FmDirTreeItem* item, gpointer item_l)
{
    if(item->folder)
        item_free_folder(item->folder, item_l);
    if(item->fi)
        fm_file_info_unref(item->fi);
    if(item->icon)
        g_object_unref(item->icon);
    if(item->children)
    {
        _g_list_foreach_l(item->children, (GFunc)fm_dir_tree_item_free_l, NULL);
        g_list_free(item->children);
    }
    if(item->hidden_children)
    {
        _g_list_foreach_l(item->hidden_children, (GFunc)fm_dir_tree_item_free_l, NULL);
        g_list_free(item->hidden_children);
    }
    g_slice_free(FmDirTreeItem, item);
}

/* Free the GList* element along with its associated FmDirTreeItem */
static void fm_dir_tree_item_free_l(GList* item_l)
{
    FmDirTreeItem* item = (FmDirTreeItem*)item_l->data;

    if(!item) /* is it possible? */
        return;
    fm_dir_tree_item_free(item, item_l);
}

static inline void item_to_tree_iter(FmDirTreeModel* model, GList* item_l, GtkTreeIter* it)
{
    it->stamp = model->stamp;
    /* We simply store a GList pointer in the iter */
    it->user_data = item_l;
    it->user_data2 = it->user_data3 = NULL;
}

static inline GtkTreePath* item_to_tree_path(FmDirTreeModel* model, GList* item_l)
{
    GtkTreeIter it;
    item_to_tree_iter(model, item_l, &it);
    return fm_dir_tree_model_get_path((GtkTreeModel*)model, &it);
}

static void on_theme_changed(GtkIconTheme* theme, FmDirTreeModel* model)
{
    GList* l;
    GtkTreePath* tp = gtk_tree_path_new_first();
    for(l = model->roots; l; l=l->next)
    {
        item_reload_icon(model, l, tp);
        gtk_tree_path_next(tp);
    }
    gtk_tree_path_free(tp);
}

static void fm_dir_tree_model_dispose(GObject *object)
{
    FmDirTreeModel *model;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_DIR_TREE_MODEL(object));

    model = (FmDirTreeModel*)object;

    g_signal_handlers_disconnect_by_func(gtk_icon_theme_get_default(),
                                         on_theme_changed, model);

    if(model->roots)
    {
        _g_list_foreach_l(model->roots, (GFunc)fm_dir_tree_item_free_l, NULL);
        g_list_free(model->roots);
        model->roots = NULL;
    }

    /* TODO: g_object_unref(model->subdir_cancellable); */

    G_OBJECT_CLASS(fm_dir_tree_model_parent_class)->dispose(object);
}

static void fm_dir_tree_model_init(FmDirTreeModel *model)
{
    g_signal_connect(gtk_icon_theme_get_default(), "changed",
                     G_CALLBACK(on_theme_changed), model);
    model->icon_size = 16;
    model->stamp = g_random_int();
    /* TODO:
    g_queue_init(&model->subdir_checks);
    model->subdir_checks_mutex = g_mutex_new();
    model->subdir_cancellable = g_cancellable_new();
    */
}

static GtkTreeModelFlags fm_dir_tree_model_get_flags (GtkTreeModel *tree_model)
{
    return GTK_TREE_MODEL_ITERS_PERSIST;
}

static gint fm_dir_tree_model_get_n_columns(GtkTreeModel *tree_model)
{
    return N_FM_DIR_TREE_MODEL_COLS;
}

static GType fm_dir_tree_model_get_column_type(GtkTreeModel *tree_model, gint index)
{
    g_return_val_if_fail( index >= 0 && (guint)index < G_N_ELEMENTS(column_types), G_TYPE_INVALID );
    return column_types[index];
}

static gboolean fm_dir_tree_model_get_iter(GtkTreeModel *tree_model,
                                    GtkTreeIter *iter,
                                    GtkTreePath *path )
{
    FmDirTreeModel *model;
    gint *indices, i, depth;
    GList *children, *child = NULL;

    g_assert(FM_IS_DIR_TREE_MODEL(tree_model));
    g_assert(path!=NULL);

    model = (FmDirTreeModel*)tree_model;
    if( G_UNLIKELY(!model || !model->roots) )
        return FALSE;

    indices = gtk_tree_path_get_indices(path);
    depth   = gtk_tree_path_get_depth(path);

    children = model->roots;
    for( i = 0; i < depth; ++i )
    {
        FmDirTreeItem* item;
        child = g_list_nth(children, indices[i]);
        if( !child )
            return FALSE;
        item = (FmDirTreeItem*)child->data;
        children = item->children;
    }
    item_to_tree_iter(model, child, iter);
    return TRUE;
}

static GtkTreePath *fm_dir_tree_model_get_path(GtkTreeModel *tree_model,
                                               GtkTreeIter *iter )
{
    GList* item_l;
    GList* children;
    FmDirTreeItem* item;
    GtkTreePath* path;
    int i;
    FmDirTreeModel* model;

    g_return_val_if_fail (FM_IS_DIR_TREE_MODEL(tree_model), NULL);
    model = (FmDirTreeModel*)tree_model;
    g_return_val_if_fail (iter->stamp == model->stamp, NULL);
    g_return_val_if_fail (iter != NULL, NULL);
    g_return_val_if_fail (iter->user_data != NULL, NULL);

    item_l = (GList*)iter->user_data;
    item = (FmDirTreeItem*)item_l->data;

    if(item->parent == NULL) /* root item */
    {
        i = g_list_position(model->roots, item_l);
        path = gtk_tree_path_new_first();
        gtk_tree_path_get_indices(path)[0] = i;
    }
    else
    {
        path = gtk_tree_path_new();
        do
        {
            FmDirTreeItem* parent_item = (FmDirTreeItem*)item->parent->data;
            children = parent_item->children;
            i = g_list_position(children, item_l);
            if(G_UNLIKELY(i == -1)) /* bug? the item is not a child of its parent? */
            {
                gtk_tree_path_free(path);
                return NULL;
            }
            /* FIXME: gtk_tree_path_prepend_index() is inefficient */
            gtk_tree_path_prepend_index(path, i);
            item_l = item->parent; /* go up a level */
            item = (FmDirTreeItem*)item_l->data;
        }while(G_UNLIKELY(item->parent)); /* we're not at toplevel yet */

        /* we have reached toplevel */
        children = model->roots;
        i = g_list_position(children, item_l);
        gtk_tree_path_prepend_index(path, i);
    }
    return path;
}

static void fm_dir_tree_model_get_value ( GtkTreeModel *tree_model,
                              GtkTreeIter *iter,
                              gint column,
                              GValue *value )
{
    FmDirTreeModel* model;
    GList* item_l;
    FmDirTreeItem* item;
    FmIcon* icon;

    g_return_if_fail (FM_IS_DIR_TREE_MODEL(tree_model));
    model = (FmDirTreeModel*)tree_model;
    g_return_if_fail (iter->stamp == model->stamp);

    g_value_init (value, column_types[column] );
    item_l = (GList*)iter->user_data;
    item = (FmDirTreeItem*)item_l->data;

    switch((FmDirTreeModelCol)column)
    {
    case FM_DIR_TREE_MODEL_COL_ICON:
        if(item->fi && (icon = fm_file_info_get_icon(item->fi)))
        {
            if(!item->icon)
                /* FIXME: use "emblem-symbolic-link" if file is some kind of link */
                item->icon = fm_pixbuf_from_icon(icon, model->icon_size);
            g_value_set_object(value, item->icon);
        }
        else
            g_value_set_object(value, NULL);
        break;
    case FM_DIR_TREE_MODEL_COL_DISP_NAME:
        if(item->fi)
            g_value_set_string( value, fm_file_info_get_disp_name(item->fi));
        else /* this is a place holder item */
        {
            /* parent is always non NULL. otherwise it's a bug. */
            FmDirTreeItem* parent = (FmDirTreeItem*)item->parent->data;
            if(parent->folder && fm_folder_is_loaded(parent->folder))
                g_value_set_string( value, _("<No subfolders>"));
            else
                g_value_set_string( value, _("Loading..."));
        }
        break;
    case FM_DIR_TREE_MODEL_COL_INFO:
        g_value_set_pointer(value, item->fi);
        break;
    case FM_DIR_TREE_MODEL_COL_PATH:
        g_value_set_pointer(value, item->fi ? fm_file_info_get_path(item->fi) : NULL);
        break;
    case FM_DIR_TREE_MODEL_COL_FOLDER:
        g_value_set_pointer(value, item->folder);
        break;
    case N_FM_DIR_TREE_MODEL_COLS: ; /* not a column */
    }
}

static gboolean fm_dir_tree_model_iter_next(GtkTreeModel *tree_model,
                                            GtkTreeIter *iter)
{
    FmDirTreeModel* model;
    GList* item_l;
    g_return_val_if_fail (FM_IS_DIR_TREE_MODEL (tree_model), FALSE);
    if (iter == NULL || iter->user_data == NULL)
        return FALSE;

    item_l = (GList*)iter->user_data;
    /* Is this the last child in the parent node? */
    item_l = item_l->next;
    if(!item_l)
        return FALSE;

    model = (FmDirTreeModel*)tree_model;
    item_to_tree_iter(model, item_l, iter);
    return TRUE;
}

static gboolean fm_dir_tree_model_iter_children(GtkTreeModel *tree_model,
                                                GtkTreeIter *iter,
                                                GtkTreeIter *parent)
{
    FmDirTreeModel* model;
    GList *first_child;

    g_return_val_if_fail(parent == NULL || parent->user_data != NULL, FALSE);
    g_return_val_if_fail(FM_IS_DIR_TREE_MODEL(tree_model), FALSE);
    model = (FmDirTreeModel*)tree_model;

    if(parent)
    {
        GList* parent_l = (GList*)parent->user_data;
        FmDirTreeItem *parent_item = (FmDirTreeItem*)parent_l->data;
        first_child = parent_item->children;
    }
    else /* toplevel item */
    {
        /* parent == NULL is a special case; we need to return the first top-level row */
        first_child = model->roots;
    }
    if(!first_child)
        return FALSE;

    /* Set iter to first item in model */
    item_to_tree_iter(model, first_child, iter);
    return TRUE;
}

static gboolean fm_dir_tree_model_iter_has_child(GtkTreeModel *tree_model,
                                                 GtkTreeIter *iter)
{
    GList* item_l;
    FmDirTreeItem* item;
    g_return_val_if_fail( iter != NULL, FALSE );
    g_return_val_if_fail( iter->stamp == FM_DIR_TREE_MODEL(tree_model)->stamp, FALSE );

    item_l = (GList*)iter->user_data;
    item = (FmDirTreeItem*)item_l->data;
    return (item->children != NULL);
}

static gint fm_dir_tree_model_iter_n_children(GtkTreeModel *tree_model,
                                              GtkTreeIter *iter)
{
    FmDirTreeModel* model;
    GList* children;
    g_return_val_if_fail(FM_IS_DIR_TREE_MODEL(tree_model), -1);

    model = (FmDirTreeModel*)tree_model;
    /* special case: if iter == NULL, return number of top-level rows */
    if(!iter)
        children = model->roots;
    else
    {
        GList* item_l = (GList*)iter->user_data;
        FmDirTreeItem* item = (FmDirTreeItem*)item_l->data;
        children = item->children;
    }
    return g_list_length(children);
}

static gboolean fm_dir_tree_model_iter_nth_child(GtkTreeModel *tree_model,
                                                 GtkTreeIter *iter,
                                                 GtkTreeIter *parent,
                                                 gint n)
{
    FmDirTreeModel *model;
    GList* children;
    GList *child_l;

    g_return_val_if_fail (FM_IS_DIR_TREE_MODEL (tree_model), FALSE);
    model = (FmDirTreeModel*)tree_model;

    if(G_LIKELY(parent))
    {
        GList* parent_l = (GList*)parent->user_data;
        FmDirTreeItem* item = (FmDirTreeItem*)parent_l->data;
        children = item->children;
    }
    else /* special case: if parent == NULL, set iter to n-th top-level row */
        children = model->roots;
    child_l = g_list_nth(children, n);
    if(!child_l)
        return FALSE;

    item_to_tree_iter(model, child_l, iter);
    return TRUE;
}

static gboolean fm_dir_tree_model_iter_parent(GtkTreeModel *tree_model,
                                              GtkTreeIter *iter,
                                              GtkTreeIter *child)
{
    GList* child_l;
    FmDirTreeItem* child_item;
    FmDirTreeModel* model;
    g_return_val_if_fail( iter != NULL && child != NULL, FALSE );

    model = FM_DIR_TREE_MODEL( tree_model );
    child_l = (GList*)child->user_data;
    child_item = (FmDirTreeItem*)child_l->data;

    if(G_LIKELY(child_item->parent))
    {
        item_to_tree_iter(model, child_item->parent, iter);
        return TRUE;
    }
    return FALSE;
}

static void fm_dir_tree_model_tree_model_init(GtkTreeModelIface *iface)
{
    iface->get_flags = fm_dir_tree_model_get_flags;
    iface->get_n_columns = fm_dir_tree_model_get_n_columns;
    iface->get_column_type = fm_dir_tree_model_get_column_type;
    iface->get_iter = fm_dir_tree_model_get_iter;
    iface->get_path = fm_dir_tree_model_get_path;
    iface->get_value = fm_dir_tree_model_get_value;
    iface->iter_next = fm_dir_tree_model_iter_next;
    iface->iter_children = fm_dir_tree_model_iter_children;
    iface->iter_has_child = fm_dir_tree_model_iter_has_child;
    iface->iter_n_children = fm_dir_tree_model_iter_n_children;
    iface->iter_nth_child = fm_dir_tree_model_iter_nth_child;
    iface->iter_parent = fm_dir_tree_model_iter_parent;

    column_types[FM_DIR_TREE_MODEL_COL_ICON] = GDK_TYPE_PIXBUF;
    column_types[FM_DIR_TREE_MODEL_COL_DISP_NAME] = G_TYPE_STRING;
    column_types[FM_DIR_TREE_MODEL_COL_INFO] = G_TYPE_POINTER;
    column_types[FM_DIR_TREE_MODEL_COL_PATH] = G_TYPE_POINTER;
    column_types[FM_DIR_TREE_MODEL_COL_FOLDER] = G_TYPE_POINTER;
}

/**
 * fm_dir_tree_model_new
 *
 * Creates new #FmDirTreeModel instance.
 *
 * Returns: a new #FmDirTreeModel object.
 *
 * Since: 0.1.16
 */
FmDirTreeModel *fm_dir_tree_model_new(void)
{
    return (FmDirTreeModel*)g_object_new(FM_TYPE_DIR_TREE_MODEL, NULL);
}

static void add_place_holder_child_item(FmDirTreeModel* model, GList* parent_l, GtkTreePath* parent_tp, gboolean emit_signal)
{
    FmDirTreeItem* parent_item = (FmDirTreeItem*)parent_l->data;
    FmDirTreeItem* item = fm_dir_tree_item_new(model, parent_l);
    parent_item->children = g_list_prepend(parent_item->children, item);

    if(emit_signal)
    {
        GtkTreeIter it;
        GtkTreePath* ph_tp;
        item_to_tree_iter(model, parent_item->children, &it);
        ph_tp = gtk_tree_path_copy(parent_tp);
        gtk_tree_path_append_index(ph_tp, 0);
        gtk_tree_model_row_inserted(GTK_TREE_MODEL(model), ph_tp, &it);
        gtk_tree_path_free(ph_tp);
    }
}

/* Add a new node to parent node to proper position.
 * GtkTreePath tp is the tree path of parent node.
 * Returns GList item where new_item is incapsulated */
static GList* insert_item(FmDirTreeModel* model, GList* parent_l, GtkTreePath* tp, FmDirTreeItem* new_item)
{
    GList* new_item_l;
    FmDirTreeItem* parent_item = (FmDirTreeItem*)parent_l->data;
    const char* new_key = fm_file_info_get_collate_key(new_item->fi);
    GList* item_l, *last_l = NULL;
    GtkTreePath* new_tp;
    GtkTreeIter it;
    int n = 0;
    for(item_l = parent_item->children; item_l; item_l=item_l->next, ++n)
    {
        FmDirTreeItem* item = (FmDirTreeItem*)item_l->data;
        const char* key;
        last_l = item_l;
        if( G_UNLIKELY(!item->fi) )
            continue;
        key = fm_file_info_get_collate_key(item->fi);
        if(strcmp(new_key, key) <= 0)
            break;
    }

    parent_item->children = g_list_insert_before(parent_item->children, item_l, new_item);
    /* get the GList* of the newly inserted item */
    if(!item_l) /* the new item becomes the last item of the list */
        new_item_l = last_l ? last_l->next : parent_item->children;
    else /* the new item is just previous item of its sibling. */
        new_item_l = item_l->prev;

    g_assert( new_item->fi != NULL );
    g_assert( new_item == new_item_l->data );
    g_assert( ((FmDirTreeItem*)new_item_l->data)->fi != NULL );

    /* emit row-inserted signal for the new item */
    item_to_tree_iter(model, new_item_l, &it);
    new_tp = gtk_tree_path_copy(tp);
    gtk_tree_path_append_index(new_tp, n);
    gtk_tree_model_row_inserted(GTK_TREE_MODEL(model), new_tp, &it);

    /* add a placeholder child item to make the node expandable */
    if(!fm_config->no_child_non_expandable || fm_file_info_is_accessible(new_item->fi))
        add_place_holder_child_item(model, new_item_l, new_tp, TRUE);
    gtk_tree_path_free(new_tp);

    /* TODO: check if the dir has subdirs and make it expandable if needed. */
    /* item_queue_subdir_check(model, new_item_l); */

    return new_item_l;
}

/* Add file info to parent node to proper position.
 * GtkTreePath tp is the tree path of parent node. */
static GList* insert_file_info(FmDirTreeModel* model, GList* parent_l, GtkTreePath* tp, FmFileInfo* fi)
{
    GList* item_l;
    FmDirTreeItem* parent_item = (FmDirTreeItem*)parent_l->data;
    FmDirTreeItem* item = fm_dir_tree_item_new(model, parent_l);
    item->fi = fm_file_info_ref(fi);

    if(!model->show_hidden && fm_file_info_is_hidden(fi)) /* hidden folder */
    {
        parent_item->hidden_children = g_list_prepend(parent_item->hidden_children, item);
        item_l = parent_item->hidden_children;
    }
    else
        item_l = insert_item(model, parent_l, tp, item);
    return item_l;
}

/* deletes item from lists but not frees data */
static void remove_item_l(FmDirTreeModel* model, GList* item_l)
{
    GtkTreePath* tp;
    FmDirTreeItem* item = (FmDirTreeItem*)item_l->data;

    g_return_if_fail(item != NULL);
    tp = item_to_tree_path(model, item_l);

    if(item->parent)
    {
        FmDirTreeItem* parent_item = item->parent->data;

        parent_item->children = g_list_delete_link(parent_item->children, item_l);
        /* signal the view that we removed the item. */
        gtk_tree_model_row_deleted(GTK_TREE_MODEL(model), tp);
        /* If the item being removed is the last child item of parent_item,
         * we need to insert a place holder item to keep it expandable. */
        if(parent_item->children == NULL)
        {
            GList* parent_l = item->parent;
            gtk_tree_path_up(tp);
            if(fm_config->no_child_non_expandable)
            {
                GtkTreeIter it;
                item_to_tree_iter(model, parent_l, &it);
                /* signal the view to redraw row removing expander */
                gtk_tree_model_row_has_child_toggled(GTK_TREE_MODEL(model), tp, &it);
            }
            else
                add_place_holder_child_item(model, parent_l, tp, TRUE);
        }
    }
    else /* root item */
    {
        /* FIXME: this needs more testing. */
        model->roots = g_list_delete_link(model->roots, item_l);
        /* signal the view that we removed the item. */
        gtk_tree_model_row_deleted(GTK_TREE_MODEL(model), tp);
    }

    gtk_tree_path_free(tp);
}

/* deletes and frees item with data */
static void remove_item(FmDirTreeModel* model, GList* item_l)
{
    FmDirTreeItem* item = (FmDirTreeItem*)item_l->data;
    remove_item_l(model, item_l);
    fm_dir_tree_item_free(item, item_l); /* item_l is freed already */
}

/* find child item by filename, and retrive its index if idx is not NULL. */
static GList* children_by_name(FmDirTreeModel* model, GList* children, const char* name, int* idx)
{
    GList* l;
    int i = 0;
    for(l = children; l; l=l->next, ++i)
    {
        FmDirTreeItem* item = (FmDirTreeItem*)l->data;
        FmPath* path;
        if(G_LIKELY(item->fi) &&
           G_LIKELY((path = fm_file_info_get_path(item->fi))) &&
           G_UNLIKELY(strcmp(fm_path_get_basename(path), name) == 0))
        {
            if(idx)
                *idx = i;
            return l;
        }
    }
    return NULL;
}

static void remove_all_children(FmDirTreeModel* model, GList* item_l, GtkTreePath* tp)
{
    FmDirTreeItem* item = (FmDirTreeItem*)item_l->data;
    if(G_UNLIKELY(!item->children))
        return;
    gtk_tree_path_append_index(tp, 0);
    /* FIXME: How to improve performance?
     * TODO: study the horrible source code of GtkTreeView */
    while(item->children)
    {
        fm_dir_tree_item_free_l(item->children);
        item->children = g_list_delete_link(item->children, item->children);
        /* signal the view that we removed the placeholder item. */
        gtk_tree_model_row_deleted(GTK_TREE_MODEL(model), tp);
        /* everytime we remove the first item, its next item became the
         * first item, so there is no need to update tp. */
    }

    if(item->hidden_children)
    {
        _g_list_foreach_l(item->hidden_children, (GFunc)fm_dir_tree_item_free_l, NULL);
        g_list_free(item->hidden_children);
        item->hidden_children = NULL;
    }
    gtk_tree_path_up(tp);
}

/**
 * fm_dir_tree_model_add_root
 * @model: the model instance
 * @root: a root path to add
 * @iter: (allow-none): pointer to iterator to set
 *
 * Adds a root node for file @root into @model. If @iter is not %NULL
 * then on return it will be set to new row data.
 *
 * Since: 0.1.16
 */
void fm_dir_tree_model_add_root(FmDirTreeModel* model, FmFileInfo* root, GtkTreeIter* iter)
{
    GtkTreeIter it;
    GtkTreePath* tp;
    GList* item_l;
    FmDirTreeItem* item = fm_dir_tree_item_new(model, NULL);
    item->fi = fm_file_info_ref(root);
    model->roots = g_list_append(model->roots, item);
    item_l = g_list_last(model->roots); /* FIXME: this is inefficient */
    add_place_holder_child_item(model, item_l, NULL, FALSE);

    /* emit row-inserted signal for the new root item */
    item_to_tree_iter(model, item_l, &it);
    tp = item_to_tree_path(model, item_l);
    gtk_tree_model_row_inserted(GTK_TREE_MODEL(model), tp, &it);

    if(iter)
        *iter = it;
    gtk_tree_path_free(tp);
}

static void on_folder_finish_loading(FmFolder* folder, GList* item_l)
{
    FmDirTreeItem* item = (FmDirTreeItem*)item_l->data;
    FmDirTreeModel* model = item->model;
    GList* place_holder_l;
    GtkTreePath* tp = item_to_tree_path(model, item_l);

    /* set 'loaded' flag beforehand as callback may check it */
    item->loaded = TRUE;
    place_holder_l = item->children;
    /* don't leave expanders if not stated in config */
    /* if we have loaded sub dirs, remove the place holder */
    if(fm_config->no_child_non_expandable || !place_holder_l || place_holder_l->next)
    {
        /* remove the fake placeholder item showing "Loading..." */
        /* #3614965: crash after removing only child from existing directory:
           after reload first item may be absent or may be not a placeholder,
           if no_child_non_expandable is unset, place_holder_l cannot be NULL */
        if (place_holder_l && ((FmDirTreeItem*)place_holder_l->data)->fi == NULL)
            remove_item(model, place_holder_l);
        /* in case if no_child_non_expandable was unset while reloading, it may
           be still place_holder_l is NULL but let leave empty folder still */
    }
    else /* if we have no sub dirs, leave the place holder and let it show "Empty" */
    {
        GtkTreeIter it;
        item_to_tree_iter(model, place_holder_l, &it);
        /* if the folder is empty, the place holder item
         * shows "<Empty>" instead of "Loading..." */
        gtk_tree_path_append_index(tp, 0);
        gtk_tree_model_row_changed(GTK_TREE_MODEL(model), tp, &it);
        gtk_tree_path_up(tp);
    }
    g_signal_emit(model, signals[ROW_LOADED], 0, tp);
    gtk_tree_path_free(tp);
    /* FIXME: should we really cease monitoring non-expandable folder? */
//    if(!item->children)
//    {
//        item_free_folder(item->folder, item_l);
//        item->folder = NULL;
//        item->expanded = FALSE;
//        item->loaded = FALSE;
//    }
}

static void on_folder_files_added(FmFolder* folder, GSList* files, GList* item_l)
{
    GSList* l;
    FmDirTreeItem* item = (FmDirTreeItem*)item_l->data;
    FmDirTreeModel* model = item->model;
    GtkTreePath* tp = item_to_tree_path(model, item_l);
    for(l = files; l; l = l->next)
    {
        FmFileInfo* fi = FM_FILE_INFO(l->data);
        if(fm_file_info_is_dir(fi)) /* FIXME: maybe adding files can be allowed later */
        {
            /* Ideally FmFolder should not emit files-added signals for files that
             * already exists. So there is no need to check for duplication here. */
            insert_file_info(model, item_l, tp, fi);
        }
    }
    gtk_tree_path_free(tp);
}

static void on_folder_files_removed(FmFolder* folder, GSList* files, GList* item_l)
{
    GSList* l;
    FmDirTreeItem* item = (FmDirTreeItem*)item_l->data;
    FmDirTreeModel* model = item->model;

    for(l = files; l; l = l->next)
    {
        FmFileInfo* fi = FM_FILE_INFO(l->data);
        FmPath* path = fm_file_info_get_path(fi);
        GList* rm_item_l = children_by_name(model, item->children,
                                            fm_path_get_basename(path), NULL);
        if(rm_item_l)
            remove_item(model, rm_item_l);
    }
}

static void on_folder_files_changed(FmFolder* folder, GSList* files, GList* item_l)
{
    GSList* l;
    FmDirTreeItem* item = (FmDirTreeItem*)item_l->data;
    FmDirTreeModel* model = item->model;
    GtkTreePath* tp = item_to_tree_path(model, item_l);

    /* g_debug("files changed!!"); */

    for(l = files; l; l = l->next)
    {
        FmFileInfo* fi = FM_FILE_INFO(l->data);
        int idx;
        FmPath* path = fm_file_info_get_path(fi);
        GList* changed_item_l = children_by_name(model, item->children,
                                                 fm_path_get_basename(path), &idx);
        /* g_debug("changed file: %s", fi->path->name); */
        if(changed_item_l)
        {
            FmDirTreeItem* changed_item = (FmDirTreeItem*)changed_item_l->data;
            GtkTreeIter it;
            if(changed_item->fi)
                fm_file_info_unref(changed_item->fi);
            changed_item->fi = fm_file_info_ref(fi);
            /* inform gtk tree view about the change */
            item_to_tree_iter(model, changed_item_l, &it);
            gtk_tree_path_append_index(tp, idx);
            gtk_tree_model_row_changed(GTK_TREE_MODEL(model), tp, &it);
            gtk_tree_path_up(tp);

            /* FIXME and TODO: check if we have sub folder */
            /* item_queue_subdir_check(model, changed_item_l); */
        }
    }
    gtk_tree_path_free(tp);
}

static inline void item_free_folder(FmFolder* folder, gpointer item_l)
{
    g_signal_handlers_disconnect_by_func(folder, on_folder_finish_loading, item_l);
    g_signal_handlers_disconnect_by_func(folder, on_folder_files_added, item_l);
    g_signal_handlers_disconnect_by_func(folder, on_folder_files_removed, item_l);
    g_signal_handlers_disconnect_by_func(folder, on_folder_files_changed, item_l);
    g_object_unref(folder);
}

/**
 * fm_dir_tree_model_load_row
 * @model: the model instance
 * @it: iterator of row to load
 * @tp: path of row to load
 *
 * If row with iterator @it has its children already retrieved then
 * does nothing. Else starts retrieving of list of children for row
 * with iterator @it and path @tp. When children are loaded then a
 * #FmDirTreeModel::row-loaded signal will be emitted. The folder
 * associated with the row will be monitored for changes in children
 * after that.
 * This API is used when the row is expanded in the view.
 *
 * Before 1.0.0 this API had name fm_dir_tree_model_expand_row.
 *
 * See also: fm_dir_tree_row_is_loaded(), fm_dir_tree_model_unload_row().
 *
 * Since: 0.1.16
 */
void fm_dir_tree_model_load_row(FmDirTreeModel* model, GtkTreeIter* it, GtkTreePath* tp)
{
    GList* item_l = (GList*)it->user_data;
    FmDirTreeItem* item = (FmDirTreeItem*)item_l->data;
    g_return_if_fail(item != NULL);
    if(!item->expanded)
    {
        /* dynamically load content of the folder. */
        FmFolder* folder = fm_folder_from_path(fm_file_info_get_path(item->fi));
        item->folder = folder;

        /* g_debug("fm_dir_tree_model_load_row()"); */
        /* associate the data with loaded handler */
        g_signal_connect(folder, "finish-loading", G_CALLBACK(on_folder_finish_loading), item_l);
        g_signal_connect(folder, "files-added", G_CALLBACK(on_folder_files_added), item_l);
        g_signal_connect(folder, "files-removed", G_CALLBACK(on_folder_files_removed), item_l);
        g_signal_connect(folder, "files-changed", G_CALLBACK(on_folder_files_changed), item_l);

        if(!item->children)
            add_place_holder_child_item(model, item_l, tp, TRUE);
        /* set 'expanded' flag beforehand as callback may check it */
        item->expanded = TRUE;
        /* if the folder is already loaded, call "loaded" handler ourselves */
        if(fm_folder_is_loaded(folder)) /* already loaded */
        {
            FmDirTreeItem* item = (FmDirTreeItem*)item_l->data;
            FmDirTreeModel* model = item->model;
            GList* file_l;
            FmFileInfoList* files = fm_folder_get_files(folder);
            for(file_l = fm_file_info_list_peek_head_link(files); file_l; file_l = file_l->next)
            {
                FmFileInfo* fi = file_l->data;
                if(fm_file_info_is_dir(fi))
                {
                    /* FIXME: later we can try to support adding
                     *        files to the tree, too so this model
                     *        can be even more useful. */
                    insert_file_info(model, item_l, tp, fi);
                    /* g_debug("insert: %s", fi->path->name); */
                }
            }
            on_folder_finish_loading(folder, item_l);
        }
    }
}

/**
 * fm_dir_tree_model_unload_row
 * @model: the model instance
 * @it: iterator of row to unload
 * @tp: path of row to unload
 *
 * If children of row with iterator @it are retrieved or monitored
 * then stops monitoring the folder and forgets children of the row.
 * This API is used when the row is collapsed in the view.
 *
 * Before 1.0.0 this API had name fm_dir_tree_model_collapse_row.
 *
 * Since: 0.1.16
 */
void fm_dir_tree_model_unload_row(FmDirTreeModel* model, GtkTreeIter* it, GtkTreePath* tp)
{
    GList* item_l = (GList*)it->user_data;
    FmDirTreeItem* item = (FmDirTreeItem*)item_l->data;
    g_return_if_fail(item != NULL);
    if(item->expanded) /* do some cleanup */
    {
        GList* had_children = item->children;
        /* remove all children, and replace them with a dummy child
         * item to keep expander in the tree view around. */
        remove_all_children(model, item_l, tp);

        /* now, GtkTreeView think that we have no child since all
         * child items are removed. So we add a place holder child
         * item to keep the expander around. */
        /* don't leave expanders if not stated in config */
        if(had_children)
            add_place_holder_child_item(model, item_l, tp, TRUE);
        /* deactivate folder since it will be reactivated on expand */
        item_free_folder(item->folder, item_l);
        item->folder = NULL;
        item->expanded = FALSE;
        item->loaded = FALSE;
        /* g_debug("fm_dir_tree_model_unload_row()"); */
    }
}

/**
 * fm_dir_tree_model_set_icon_size
 * @model: the model instance
 * @icon_size: new preferrable icon size
 *
 * Sets size of icons which are associated with rows.
 *
 * Since: 0.1.16
 */
void fm_dir_tree_model_set_icon_size(FmDirTreeModel* model, guint icon_size)
{
    if(model->icon_size != icon_size)
    {
        /* reload existing icons */
        GtkTreePath* tp = gtk_tree_path_new_first();
        GList* l;
        for(l = model->roots; l; l=l->next)
        {
            item_reload_icon(model, l, tp);
            gtk_tree_path_next(tp);
        }
        gtk_tree_path_free(tp);
    }
}

/**
 * fm_dir_tree_model_get_icon_size
 * @model: the model instance
 *
 * Retrieves size of icons which are associated with rows.
 *
 * Before 1.0.0 this API had name fm_dir_tree_get_icon_size.
 *
 * Returns: preferrable icom size for the @model.
 *
 * Since: 0.1.16
 */
guint fm_dir_tree_model_get_icon_size(FmDirTreeModel* model)
{
    return model->icon_size;
}

/**
 * fm_dir_tree_row_get_icon
 * @model: the model instance
 * @iter: the iterator of row to retrieve data
 *
 * Retrieves an icon associated with row of @iter in @model. Returned
 * data are owned by @model and should be not freed by caller.
 *
 * Returns: (transfer none): the icon for the row.
 *
 * Since: 1.0.0
 */
GdkPixbuf* fm_dir_tree_row_get_icon(FmDirTreeModel* model, GtkTreeIter* iter)
{
    GList* item_l;
    FmDirTreeItem *item;
    FmIcon* icon;

    g_return_val_if_fail (iter->stamp == model->stamp, NULL);

    item_l = (GList*)iter->user_data;
    item = (FmDirTreeItem*)item_l->data;

    if(item->icon)
        return item->icon;
    if(item->fi && (icon = fm_file_info_get_icon(item->fi)))
        /* FIXME: use "emblem-symbolic-link" if file is some kind of link */
        item->icon = fm_pixbuf_from_icon(icon, model->icon_size);
    return item->icon;
}

/**
 * fm_dir_tree_row_get_file_info
 * @model: the model instance
 * @iter: the iterator of row to retrieve data
 *
 * Retrieves info for file associated with row of @iter in @model.
 * Returned data are owned by @model and should be not freed by caller.
 *
 * Returns: (transfer none): the file info for the row.
 *
 * Since: 1.0.0
 */
FmFileInfo* fm_dir_tree_row_get_file_info(FmDirTreeModel* model, GtkTreeIter* iter)
{
    GList* item_l;
    FmDirTreeItem *item;

    g_return_val_if_fail (iter->stamp == model->stamp, NULL);

    item_l = (GList*)iter->user_data;
    item = (FmDirTreeItem*)item_l->data;

    return item->fi;
}

/**
 * fm_dir_tree_row_get_file_path
 * @model: the model instance
 * @iter: the iterator of row to retrieve data
 *
 * Retrieves #FmPath for file associated with row of @iter in @model.
 * Returned data are owned by @model and should be not freed by caller.
 *
 * Returns: (transfer none): the file path for the row.
 *
 * Since: 1.0.0
 */
FmPath* fm_dir_tree_row_get_file_path(FmDirTreeModel* model, GtkTreeIter* iter)
{
    GList* item_l;
    FmDirTreeItem *item;

    g_return_val_if_fail (iter->stamp == model->stamp, NULL);

    item_l = (GList*)iter->user_data;
    item = (FmDirTreeItem*)item_l->data;

    return item->fi ? fm_file_info_get_path(item->fi) : NULL;
}

/**
 * fm_dir_tree_row_get_disp_name
 * @model: the model instance
 * @iter: the iterator of row to retrieve data
 *
 * Retrieves display name of file associated with row of @iter in @model.
 * Returned data are owned by @model and should be not freed by caller.
 *
 * Returns: (transfer none): the file display name for the row.
 *
 * Since: 1.0.0
 */
const char* fm_dir_tree_row_get_disp_name(FmDirTreeModel* model, GtkTreeIter* iter)
{
    GList* item_l;
    FmDirTreeItem *item, *parent;

    g_return_val_if_fail (iter->stamp == model->stamp, NULL);

    item_l = (GList*)iter->user_data;
    item = (FmDirTreeItem*)item_l->data;

    if(item->fi)
        return fm_file_info_get_disp_name(item->fi);
    /* else this is a place holder item */
    /* parent is always non NULL. otherwise it's a bug. */
    parent = (FmDirTreeItem*)item->parent->data;
    if(parent->folder && fm_folder_is_loaded(parent->folder))
        return _("<No subfolders>");
    return _("Loading...");
}

/**
 * fm_dir_tree_row_is_loaded
 * @model: the model instance
 * @iter: the iterator of row to inspect
 *
 * Checks if the row has its children already retrieved. This check may
 * need be done since any duplicate calls to fm_dir_tree_model_load_row()
 * will not emit any duplicate #FmDirTreeModel::row-loaded signal.
 *
 * Returns: %TRUE if the row has children already loaded.
 *
 * Since: 1.0.0
 */
gboolean fm_dir_tree_row_is_loaded(FmDirTreeModel* model, GtkTreeIter* iter)
{
    GList* item_l;
    FmDirTreeItem *item;

    g_return_val_if_fail (iter->stamp == model->stamp, FALSE);

    item_l = (GList*)iter->user_data;
    item = (FmDirTreeItem*)item_l->data;

    return item->loaded;
}

static void item_hide_hidden_children(FmDirTreeModel *model, GList *item_l)
{
    FmDirTreeItem *item = (FmDirTreeItem*)item_l->data;
    FmDirTreeItem *child;
    GList *child_l, *next;

    if (item) for (child_l = item->children; child_l; child_l = next)
    {
        next = child_l->next;
        child = child_l->data;
        if (G_UNLIKELY(child->fi == NULL)) /* placeholder */
            continue;
        if (fm_file_info_is_hidden(child->fi))
        {
            /* remove from visibility in model */
            remove_item_l(model, child_l);
            /* do cleanup on item data */
            if (child->folder)
                item_free_folder(child->folder, child_l);
            child->folder = NULL;
            child->expanded = FALSE;
            child->loaded = FALSE;
            if(child->children)
            {
                _g_list_foreach_l(child->children, (GFunc)fm_dir_tree_item_free_l, NULL);
                g_list_free(child->children);
                child->children = NULL;
            }
            if(child->hidden_children)
            {
                _g_list_foreach_l(child->hidden_children, (GFunc)fm_dir_tree_item_free_l, NULL);
                g_list_free(child->hidden_children);
                child->hidden_children = NULL;
            }
            /* item is clean so can be added to hidden children */
            item->hidden_children = g_list_prepend(item->hidden_children, child);
        }
        else
            item_hide_hidden_children(model, child_l); /* do recursion */
    }
}

static void item_show_hidden_children(FmDirTreeModel *model, GList *item_l)
{
    FmDirTreeItem *item = (FmDirTreeItem*)item_l->data;
    FmDirTreeItem *child;
    GtkTreePath *tp;
    GList *child_l;

    tp = item_to_tree_path(model, item_l);
    for (child_l = item->children; child_l; child_l = child_l->next)
        item_show_hidden_children(model, child_l); /* do recursion */
    while (item->hidden_children)
    {
        /* isolate child */
        child = item->hidden_children->data;
        item->hidden_children = g_list_delete_link(item->hidden_children,
                                                   item->hidden_children);
        /* insert it into visible list */
        insert_item(model, item_l, tp, child);
    }
    gtk_tree_path_free(tp);
}

void fm_dir_tree_model_set_show_hidden(FmDirTreeModel* model, gboolean show_hidden)
{
    GList *l;

    if(show_hidden != model->show_hidden)
    {
        /* filter the model to hide hidden folders */
        if(model->show_hidden)
            for (l = model->roots; l; l = l->next)
                item_hide_hidden_children(model, l);
        /* filter the model to show hidden folders */
        else
            for (l = model->roots; l; l = l->next)
                item_show_hidden_children(model, l);
        model->show_hidden = show_hidden;
    }
}

gboolean fm_dir_tree_model_get_show_hidden(FmDirTreeModel* model)
{
    return model->show_hidden;
}

#if 0
/* TODO: */ void fm_dir_tree_model_reload(FmDirTreeModel* model)
{
}
#endif
