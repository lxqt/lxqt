/*
 *      fm-app-menu-view.h
 *      
 *      Copyright 2010 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#ifndef __FM_APP_MENU_VIEW_H__
#define __FM_APP_MENU_VIEW_H__

#include <gtk/gtk.h>
#include "fm-path.h"

G_BEGIN_DECLS

GtkTreeView* fm_app_menu_view_new(void);

GAppInfo* fm_app_menu_view_dup_selected_app(GtkTreeView* view);

char* fm_app_menu_view_dup_selected_app_desktop_id(GtkTreeView* view);

char* fm_app_menu_view_dup_selected_app_desktop_file_path(GtkTreeView* view);

FmPath * fm_app_menu_view_dup_selected_app_desktop_path(GtkTreeView* view);

gboolean fm_app_menu_view_is_app_selected(GtkTreeView* view);

gboolean fm_app_menu_view_is_item_app(GtkTreeView* view, GtkTreeIter* it);

G_END_DECLS

#endif /* __FM_APP_MENU_VIEW_H__ */
