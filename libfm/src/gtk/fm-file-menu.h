/*
 *      fm-file-menu.h
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2013-2018 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifndef __FM_FILE_MENU_H__
#define __FM_FILE_MENU_H__

#include <gtk/gtk.h>
#include "fm-file-info.h"
#include "fm-file-launcher.h"
#include "fm-module.h"

G_BEGIN_DECLS

typedef struct _FmFileMenu FmFileMenu;

FmFileMenu* fm_file_menu_new_for_file(GtkWindow* parent, FmFileInfo* fi, FmPath* cwd, gboolean auto_destroy);
FmFileMenu* fm_file_menu_new_for_files(GtkWindow* parent, FmFileInfoList* files, FmPath* cwd, gboolean auto_destroy);
void fm_file_menu_destroy(FmFileMenu* menu);

gboolean fm_file_menu_is_single_file_type(FmFileMenu* menu);

GtkUIManager* fm_file_menu_get_ui(FmFileMenu* menu);
GtkActionGroup* fm_file_menu_get_action_group(FmFileMenu* menu);

/* build the menu with GtkUIManager */
GtkMenu* fm_file_menu_get_menu(FmFileMenu* menu);

/* call fm_file_info_list_ref() if you need to own reference to the returned list. */
FmFileInfoList* fm_file_menu_get_file_info_list(FmFileMenu* menu);

FmPath* fm_file_menu_get_cwd(FmFileMenu* menu);

void fm_file_menu_set_folder_func(FmFileMenu* menu, FmLaunchFolderFunc func, gpointer user_data);

/**
 * FmFileMenuUpdatePopup
 * @window: the parent window for popup
 * @ui: the object to add interface
 * @xml: container where callback should append XML definition
 * @act_grp: group of actions to add action
 * @menu: the menu descriptor
 * @files: list of files for current popup menu
 * @single_file: %TRUE is @menu was created for single file
 *
 * The callback to update popup menu. It can disable items of menu, add
 * some new, replace actions, etc. depending of the window and files.
 */
typedef void (*FmFileMenuUpdatePopup)(GtkWindow* window, GtkUIManager* ui,
                                      GString* xml, GtkActionGroup* act_grp,
                                      FmFileMenu* menu, FmFileInfoList* files,
                                      gboolean single_file);

/* modules "gtk_menu_mime" stuff */
typedef struct _FmFileMenuMimeAddonInit FmFileMenuMimeAddonInit;

/**
 * FmFileMenuMimeAddonInit:
 * @init: (allow-none): callback for plugin initialization
 * @finalize: (allow-none): callback to free resources allocated by @init
 * @update_file_menu_for_mime_type: (allow-none): callback to update selection context menu
 *
 * The @init and @finalize callbacks are called on application start and exit.
 *
 * The @update_file_menu_for_mime_type callback will be called each time
 * context menu is created for files that have the same context type.
 *
 * This structure is used for "gtk_menu_mime" module initialization. The
 * key for module of this type is content type (MIME name) to support. No
 * wildcards are supported.
 *
 * Since: 1.2.0
 */
struct _FmFileMenuMimeAddonInit
{
    void (*init)(void);
    void (*finalize)(void);
    FmFileMenuUpdatePopup update_file_menu_for_mime_type;
};

#define FM_MODULE_gtk_menu_mime_VERSION 1

extern FmFileMenuMimeAddonInit fm_module_init_gtk_menu_mime;

void _fm_file_menu_init(void);
void _fm_file_menu_finalize(void);

G_END_DECLS

#endif /* __FM_FILE_MENU_H__ */
