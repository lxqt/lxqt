/*
 *      folder-view.h
 *
 *      Copyright 2009 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#ifndef __STANDARD_VIEW_H__
#define __STANDARD_VIEW_H__

#include <gtk/gtk.h>
#include "fm-folder-view.h"

G_BEGIN_DECLS

#define FM_STANDARD_VIEW_TYPE               (fm_standard_view_get_type())
#define FM_STANDARD_VIEW(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            FM_STANDARD_VIEW_TYPE, FmStandardView))
#define FM_STANDARD_VIEW_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST((klass),\
            FM_STANDARD_VIEW_TYPE, FmStandardViewClass))
#define FM_IS_STANDARD_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            FM_STANDARD_VIEW_TYPE))
#define FM_IS_STANDARD_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),\
            FM_STANDARD_VIEW_TYPE))

/**
 * FmStandardViewMode
 * @FM_FV_ICON_VIEW: standard icon view
 * @FM_FV_COMPACT_VIEW: view with small icons and text on right of them
 * @FM_FV_THUMBNAIL_VIEW: view with big icons/thumbnails
 * @FM_FV_LIST_VIEW: table-form view
 */
typedef enum
{
    FM_FV_ICON_VIEW,
    FM_FV_COMPACT_VIEW,
    FM_FV_THUMBNAIL_VIEW,
    FM_FV_LIST_VIEW
} FmStandardViewMode;

#ifndef FM_DISABLE_DEPRECATED
#define FM_FOLDER_VIEW_MODE_IS_VALID(mode) FM_STANDARD_VIEW_MODE_IS_VALID(mode)
#endif
#define FM_STANDARD_VIEW_MODE_IS_VALID(mode)  ((guint)mode <= FM_FV_LIST_VIEW)

typedef struct _FmStandardView             FmStandardView;
typedef struct _FmStandardViewClass        FmStandardViewClass;

GType       fm_standard_view_get_type(void);
FmStandardView* fm_standard_view_new(FmStandardViewMode mode,
                                     FmFolderViewUpdatePopup update_popup,
                                     FmLaunchFolderFunc open_folders);

void fm_standard_view_set_mode(FmStandardView* fv, FmStandardViewMode mode);
FmStandardViewMode fm_standard_view_get_mode(FmStandardView* fv);

const char* fm_standard_view_mode_to_str(FmStandardViewMode mode);
FmStandardViewMode fm_standard_view_mode_from_str(const char* str);

gint fm_standard_view_get_n_modes(void);
const char *fm_standard_view_get_mode_label(FmStandardViewMode mode);
const char *fm_standard_view_get_mode_tooltip(FmStandardViewMode mode);
const char *fm_standard_view_get_mode_icon(FmStandardViewMode mode);

G_END_DECLS

#endif /* __STANDARD_VIEW_H__ */
