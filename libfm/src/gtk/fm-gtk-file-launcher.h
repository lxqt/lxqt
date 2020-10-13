/*
 *      fm-gtk-file-launcher.h
 *
 *      Copyright 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2012 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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


#ifndef __FM_GTK_FILE_LAUNCHER_H__
#define __FM_GTK_FILE_LAUNCHER_H__

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <stdarg.h>

#include "fm-file-info.h"
#include "fm-file-launcher.h"

G_BEGIN_DECLS

gboolean fm_launch_files_simple(GtkWindow* parent, GAppLaunchContext* ctx, GList* file_infos, FmLaunchFolderFunc func, gpointer user_data);
gboolean fm_launch_file_simple(GtkWindow* parent, GAppLaunchContext* ctx, FmFileInfo* file_info, FmLaunchFolderFunc func, gpointer user_data);

gboolean fm_launch_paths_simple(GtkWindow* parent, GAppLaunchContext* ctx, GList* paths, FmLaunchFolderFunc func, gpointer user_data);
gboolean fm_launch_path_simple(GtkWindow* parent, GAppLaunchContext* ctx, FmPath* path, FmLaunchFolderFunc func, gpointer user_data);

gboolean fm_launch_desktop_entry_simple(GtkWindow* parent, GAppLaunchContext* ctx, FmFileInfo* entry, FmPathList* files);
gboolean fm_launch_command_simple(GtkWindow *parent, GAppLaunchContext *ctx, GAppInfoCreateFlags flags, const char *cmd, FmPathList *files);

gboolean fm_launch_search_simple(GtkWindow* parent, GAppLaunchContext* ctx, const GList* paths, FmLaunchFolderFunc func, gpointer user_data);

G_END_DECLS

#endif /* __FM_GTK_UTILS_H__ */
