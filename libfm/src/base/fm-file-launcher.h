/*
 *      fm-file-launcher.h
 *
 *      Copyright 2009 - 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 *      This file is a part of the Libfm library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __FM_FILE_LAUNCHER_H__
#define __FM_FILE_LAUNCHER_H__

#include <glib.h>
#include <gio/gio.h>
#include "fm-file-info.h"

G_BEGIN_DECLS

typedef gboolean (*FmLaunchFolderFunc)(GAppLaunchContext* ctx, GList* folder_infos, gpointer user_data, GError** err);

/**
 * FmFileLauncherExecAction:
 * @FM_FILE_LAUNCHER_EXEC: execute the file
 * @FM_FILE_LAUNCHER_EXEC_IN_TERMINAL: execute the file in terminal
 * @FM_FILE_LAUNCHER_EXEC_OPEN: open file with some application
 * @FM_FILE_LAUNCHER_EXEC_CANCEL: do nothing
 */
typedef enum {
    FM_FILE_LAUNCHER_EXEC = 1,
    FM_FILE_LAUNCHER_EXEC_IN_TERMINAL,
    FM_FILE_LAUNCHER_EXEC_OPEN,
    FM_FILE_LAUNCHER_EXEC_CANCEL
} FmFileLauncherExecAction;

typedef struct _FmFileLauncher FmFileLauncher;

/**
 * FmFileLauncher:
 * @get_app: callback to get new #GAppInfo
 * @open_folder: callback to open folders
 * @exec_file: callback to select file execution mode
 * @error: callback to show error message; returns TRUE to continue, FALSE to retry
 * @ask: callback to ask for user interaction; returns choise from btn_labels
 */
struct _FmFileLauncher
{
    GAppInfo* (*get_app)(GList* file_infos, FmMimeType* mime_type, gpointer user_data, GError** err);
    /* gboolean (*before_open)(GAppLaunchContext* ctx, GList* folder_infos, gpointer user_data); */
    FmLaunchFolderFunc open_folder;
    FmFileLauncherExecAction (*exec_file)(FmFileInfo* file, gpointer user_data);
    /* returns TRUE to continue, FALSE to retry */
    gboolean (*error)(GAppLaunchContext* ctx, GError* err, FmPath* file, gpointer user_data);
    int (*ask)(const char* msg, char* const* btn_labels, int default_btn, gpointer user_data);
    /*< private >*/
    gpointer _reserved1;
};

gboolean fm_launch_files(GAppLaunchContext* ctx, GList* file_infos, FmFileLauncher* launcher, gpointer user_data);
gboolean fm_launch_paths(GAppLaunchContext* ctx, GList* paths, FmFileLauncher* launcher, gpointer user_data);
gboolean fm_launch_desktop_entry(GAppLaunchContext* ctx, const char* file_or_id, GList* uris, FmFileLauncher* launcher, gpointer user_data);

G_END_DECLS

#endif
