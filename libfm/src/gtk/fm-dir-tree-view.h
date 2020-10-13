//      fm-dir-tree-view.h
//
//      Copyright 2011 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
//      Copyright 2012-2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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


#ifndef __FM_DIR_TREE_VIEW_H__
#define __FM_DIR_TREE_VIEW_H__

#include <gtk/gtk.h>
#include "fm-path.h"
#include "fm-dnd-dest.h"

#include "fm-seal.h"

G_BEGIN_DECLS


#define FM_TYPE_DIR_TREE_VIEW                (fm_dir_tree_view_get_type())
#define FM_DIR_TREE_VIEW(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            FM_TYPE_DIR_TREE_VIEW, FmDirTreeView))
#define FM_DIR_TREE_VIEW_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST((klass),\
            FM_TYPE_DIR_TREE_VIEW, FmDirTreeViewClass))
#define FM_IS_DIR_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            FM_TYPE_DIR_TREE_VIEW))
#define FM_IS_DIR_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),\
            FM_TYPE_DIR_TREE_VIEW))
#define FM_DIR_TREE_VIEW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj),\
            FM_TYPE_DIR_TREE_VIEW, FmDirTreeViewClass))

typedef struct _FmDirTreeView            FmDirTreeView;
typedef struct _FmDirTreeViewClass        FmDirTreeViewClass;

struct _FmDirTreeView
{
    GtkTreeView parent;
    FmPath* FM_SEAL(cwd);

    /* <private> */
    FmDndDest* FM_SEAL(dd);
    gpointer _reserved1;

    /* used for chdir */
    GSList* FM_SEAL(paths_to_expand);
    GtkTreeRowReference* FM_SEAL(current_row);
};

/**
 * FmDirTreeViewClass:
 * @parent_class: the parent class
 * @chdir: the class closure for the #FmDirTreeView::chdir signal.
 * @item_popup: the class closure for the #FmDirTreeView::item-popup signal.
 */
struct _FmDirTreeViewClass
{
    GtkTreeViewClass parent_class;
    void (*chdir)(FmDirTreeView* view, guint button, FmPath* path);
    void (*item_popup)(FmDirTreeView* view, GtkUIManager* ui, GtkActionGroup* act_grp, FmFileInfo* file);
    /*< private >*/
    gpointer _reserved1;
};


GType        fm_dir_tree_view_get_type        (void);
FmDirTreeView* fm_dir_tree_view_new            (void);

FmPath* fm_dir_tree_view_get_cwd(FmDirTreeView* view);
void fm_dir_tree_view_chdir(FmDirTreeView* view, FmPath* path);

G_END_DECLS

#endif /* __FM_DIR_TREE_VIEW_H__ */
