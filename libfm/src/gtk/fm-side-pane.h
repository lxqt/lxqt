//      fm-side-pane.h
//
//      Copyright 2011 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
//      Copyright 2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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


#ifndef __FM_SIDE_PANE_H__
#define __FM_SIDE_PANE_H__

#include <gtk/gtk.h>
#include "fm-file-info.h"

#include "fm-seal.h"

G_BEGIN_DECLS


#define FM_TYPE_SIDE_PANE                (fm_side_pane_get_type())
#define FM_SIDE_PANE(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            FM_TYPE_SIDE_PANE, FmSidePane))
#define FM_SIDE_PANE_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST((klass),\
            FM_TYPE_SIDE_PANE, FmSidePaneClass))
#define FM_IS_SIDE_PANE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            FM_TYPE_SIDE_PANE))
#define FM_IS_SIDE_PANE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),\
            FM_TYPE_SIDE_PANE))
#define FM_SIDE_PANE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj),\
            FM_TYPE_SIDE_PANE, FmSidePaneClass))

typedef struct _FmSidePane            FmSidePane;
typedef struct _FmSidePaneClass        FmSidePaneClass;

/**
 * FmSidePaneMode:
 * @FM_SP_NONE: invalid mode
 * @FM_SP_PLACES: #FmPlacesView mode
 * @FM_SP_DIR_TREE: #FmDirTreeView mode
 * @FM_SP_REMOTE: reserved mode
 *
 * Mode of side pane view.
 */
typedef enum
{
    FM_SP_NONE,
    FM_SP_PLACES,
    FM_SP_DIR_TREE,
    FM_SP_REMOTE
}FmSidePaneMode;

/**
 * FmSidePaneUpdatePopup:
 * @sp: the side pane widget
 * @ui: the object to add interface
 * @act_grp: group of actions to add action
 * @file: the file the popup menu was created for
 * @user_data: pointer passed to fm_side_pane_set_popup_updater()
 *
 * The callback to update popup menu. It can disable items of menu, add
 * some new, replace actions, etc. depending of the window and files.
 * In some cases @file may be incomplete so callback should check that.
 */
typedef void (*FmSidePaneUpdatePopup)(FmSidePane* sp, GtkUIManager* ui,
                                      GtkActionGroup* act_grp,
                                      FmFileInfo* file, gpointer user_data);

struct _FmSidePane
{
    /*< private >*/
    GtkVBox parent;
    FmPath* FM_SEAL(cwd);
    GtkWidget* FM_SEAL(title_bar);
    GtkWidget* FM_SEAL(menu_btn);
    GtkWidget* FM_SEAL(menu_label);
    GtkWidget* FM_SEAL(menu);
    GtkWidget* FM_SEAL(scroll);
    GtkWidget* FM_SEAL(view);
    FmSidePaneMode FM_SEAL(mode);
    GtkUIManager* FM_SEAL(ui);
    FmSidePaneUpdatePopup FM_SEAL(update_popup);
    gpointer FM_SEAL(popup_user_data);
    gpointer _reserved1;
    gpointer _reserved2;
};

/**
 * FmSidePaneClass:
 * @parent_class: the parent class
 * @chdir: the class closure for the #FmSidePane::chdir signal
 * @mode_changed: the class closure for the #FmSidePane::mode-changed signal
 */
struct _FmSidePaneClass
{
    GtkVBoxClass parent_class;
    void (*chdir)(FmSidePane* sp, guint button, FmPath* path);
    void (*mode_changed)(FmSidePane* sp);
    /*< private >*/
    gpointer _reserved1;
    gpointer _reserved2;
};


GType fm_side_pane_get_type        (void);
FmSidePane* fm_side_pane_new       (void);

FmPath* fm_side_pane_get_cwd(FmSidePane* sp);
void fm_side_pane_chdir(FmSidePane* sp, FmPath* path);

void fm_side_pane_set_mode(FmSidePane* sp, FmSidePaneMode mode);
FmSidePaneMode fm_side_pane_get_mode(FmSidePane* sp);

GtkWidget* fm_side_pane_get_title_bar(FmSidePane* sp);

/* setup a popup menu extension for the window */
void fm_side_pane_set_popup_updater(FmSidePane* sp,
                                    FmSidePaneUpdatePopup update_popup,
                                    gpointer user_data);

const char *fm_side_pane_get_mode_name(FmSidePaneMode mode);
FmSidePaneMode fm_side_pane_get_mode_by_name(const char *str);
gint fm_side_pane_get_n_modes(void);
const char *fm_side_pane_get_mode_label(FmSidePaneMode mode);
const char *fm_side_pane_get_mode_tooltip(FmSidePaneMode mode);

gboolean fm_side_pane_set_show_hidden(FmSidePane *sp, gboolean show_hidden);
gboolean fm_side_pane_set_home_dir(FmSidePane *sp, const char *home_dir);

G_END_DECLS

#endif /* __FM_SIDE_PANE_H__ */
