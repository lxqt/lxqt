/*
 *      fm-places-model.h
 *
 *      Copyright 2010 PCMan <pcman.tw@gmail.com>
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

#ifndef __FM_PLACES_MODEL_H__
#define __FM_PLACES_MODEL_H__

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "fm-file-info.h"
#include "fm-bookmarks.h"
#include "fm-icon-pixbuf.h"

G_BEGIN_DECLS


#define FM_TYPE_PLACES_MODEL                (fm_places_model_get_type())
#define FM_PLACES_MODEL(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            FM_TYPE_PLACES_MODEL, FmPlacesModel))
#define FM_PLACES_MODEL_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST((klass),\
            FM_TYPE_PLACES_MODEL, FmPlacesModelClass))
#define FM_IS_PLACES_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            FM_TYPE_PLACES_MODEL))
#define FM_IS_PLACES_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),\
            FM_TYPE_PLACES_MODEL))
#define FM_PLACES_MODEL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj),\
            FM_TYPE_PLACES_MODEL, FmPlacesModelClass))

typedef struct _FmPlacesModel            FmPlacesModel;
typedef struct _FmPlacesModelClass        FmPlacesModelClass;

/**
 * FmPlacesCol:
 * @FM_PLACES_MODEL_COL_ICON: (#GdkPixbuf *) icon if the row
 * @FM_PLACES_MODEL_COL_LABEL: (#char *) name of the row
 * @FM_PLACES_MODEL_COL_INFO: (#FmPlacesItem *) internal representation
 *
 * Data of the row in the #FmPlacesModel.
 */
typedef enum
{
    FM_PLACES_MODEL_COL_ICON,
    FM_PLACES_MODEL_COL_LABEL,
    FM_PLACES_MODEL_COL_INFO,
    /*< private >*/
    FM_PLACES_MODEL_N_COLS
} FmPlacesCol;

/**
 * FmPlacesType:
 * @FM_PLACES_ITEM_NONE: separator
 * @FM_PLACES_ITEM_PATH: some path - standard one or bookmark
 * @FM_PLACES_ITEM_VOLUME: a mountable device
 * @FM_PLACES_ITEM_MOUNT: some mounted media or virtual drive
 */
typedef enum
{
    FM_PLACES_ITEM_NONE,
    FM_PLACES_ITEM_PATH,
    FM_PLACES_ITEM_VOLUME,
    FM_PLACES_ITEM_MOUNT
} FmPlacesType;

typedef struct _FmPlacesItem FmPlacesItem;

GType fm_places_model_get_type        (void);
FmPlacesModel* fm_places_model_new            (void);

GtkTreePath* fm_places_model_get_separator_path(FmPlacesModel* model);

FmBookmarks* fm_places_model_get_bookmarks(FmPlacesModel* model);

gboolean fm_places_model_iter_is_separator(FmPlacesModel* model, GtkTreeIter* it);

gboolean fm_places_model_path_is_separator(FmPlacesModel* model, GtkTreePath* tp);
gboolean fm_places_model_path_is_bookmark(FmPlacesModel* model, GtkTreePath* tp);
gboolean fm_places_model_path_is_places(FmPlacesModel* model, GtkTreePath* tp);

void fm_places_model_mount_indicator_cell_data_func(GtkCellLayout *cell_layout,
                                           GtkCellRenderer *render,
                                           GtkTreeModel *tree_model,
                                           GtkTreeIter *it,
                                           gpointer user_data);

gboolean fm_places_model_get_iter_by_fm_path(FmPlacesModel* model, GtkTreeIter* iter, FmPath* path);


FmPlacesType fm_places_item_get_type(FmPlacesItem* item);

gboolean fm_places_item_is_mounted(FmPlacesItem* item);

FmIcon* fm_places_item_get_icon(FmPlacesItem* item);

FmFileInfo* fm_places_item_get_info(FmPlacesItem* item);

GVolume* fm_places_item_get_volume(FmPlacesItem* item);

GMount* fm_places_item_get_mount(FmPlacesItem* item);

FmPath* fm_places_item_get_path(FmPlacesItem* item);

FmBookmarkItem* fm_places_item_get_bookmark_item(FmPlacesItem* item);

G_END_DECLS

#endif /* __FM_PLACES_MODEL_H__ */
