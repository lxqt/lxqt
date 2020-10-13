/*
 *      fm-gtk-utils.h
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
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


#ifndef __FM_GTK_UTILS_H__
#define __FM_GTK_UTILS_H__

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <stdarg.h>

#include "fm-path.h"

G_BEGIN_DECLS

/* Convinient dialog functions */

/* Display an error message to the user */
void fm_show_error(GtkWindow* parent, const char* title, const char* msg);

/* Ask the user a yes-no question. */
gboolean fm_yes_no(GtkWindow* parent, const char* title, const char* question, gboolean default_yes);
gboolean fm_ok_cancel(GtkWindow* parent, const char* title, const char* question, gboolean default_ok);

/* Ask the user a question with a NULL-terminated array of
 * options provided. The return value was index of the selected option. */
int fm_ask(GtkWindow* parent, const char* title, const char* question, ...);
int fm_askv(GtkWindow* parent, const char* title, const char* question, char* const* options);
int fm_ask_valist(GtkWindow* parent, const char* title, const char* question, va_list options);

char* fm_get_user_input(GtkWindow* parent, const char* title, const char* msg, const char* default_text);
gchar* fm_get_user_input_n(GtkWindow* parent, const char* title, const char* msg,
                           const char* default_text, gint n, GtkWidget* extra);
#ifndef FM_DISABLE_DEPRECATED
FmPath* fm_get_user_input_path(GtkWindow* parent, const char* title, const char* msg, FmPath* default_path);
#endif

/* Ask the user to select a folder. */
FmPath* fm_select_folder(GtkWindow* parent, const char* title);
/* TODO: support selecting multiple files */
FmPath* fm_select_file(GtkWindow* parent, 
						const char* title,
						const char* default_folder,
						gboolean local_only,
						gboolean show_preview,
						/* filter1, filter2, ..., NULL */ ...);

/* Mount */
gboolean fm_mount_path(GtkWindow* parent, FmPath* path, gboolean interactive);
gboolean fm_mount_volume(GtkWindow* parent, GVolume* vol, gboolean interactive);
gboolean fm_unmount_mount(GtkWindow* parent, GMount* mount, gboolean interactive);
gboolean fm_unmount_volume(GtkWindow* parent, GVolume* vol, gboolean interactive);
gboolean fm_eject_mount(GtkWindow* parent, GMount* mount, gboolean interactive);
gboolean fm_eject_volume(GtkWindow* parent, GVolume* vol, gboolean interactive);

/* File operations */
void fm_copy_files(GtkWindow* parent, FmPathList* files, FmPath* dest_dir);
void fm_move_files(GtkWindow* parent, FmPathList* files, FmPath* dest_dir);
void fm_link_files(GtkWindow* parent, FmPathList* files, FmPath* dest_dir);

#define fm_copy_file(parent, file, dest_dir) \
    G_STMT_START {    \
        FmPathList* files = fm_path_list_new(); \
        fm_path_list_push_tail(files, file); \
        fm_copy_files(parent, files, dest_dir); \
        fm_path_list_unref(files);   \
    } G_STMT_END

#define fm_move_file(parent, file, dest_dir) \
    G_STMT_START {    \
    FmPathList* files = fm_path_list_new(); \
    fm_path_list_push_tail(files, file); \
    fm_move_files(parent, files, dest_dir); \
    fm_path_list_unref(files);   \
    } G_STMT_END

void fm_move_or_copy_files_to(GtkWindow* parent, FmPathList* files, gboolean is_move);
#define fm_move_files_to(parent, files)   fm_move_or_copy_files_to(parent, files, TRUE)
#define fm_copy_files_to(parent, files)   fm_move_or_copy_files_to(parent, files, FALSE)

void fm_trash_files(GtkWindow* parent, FmPathList* files);
void fm_delete_files(GtkWindow* parent, FmPathList* files);
/* trash or delete files according to FmConfig::use_trash. */
void fm_trash_or_delete_files(GtkWindow* parent, FmPathList* files);

void fm_untrash_files(GtkWindow* parent, FmPathList* files);

/* void fm_rename_files(FmPathList* files); */
void fm_rename_file(GtkWindow* parent, FmPath* file);

void fm_hide_file(GtkWindow* parent, FmPath* file);
void fm_unhide_file(GtkWindow* parent, FmPath* file);

void fm_empty_trash(GtkWindow* parent);

void fm_set_busy_cursor(GtkWidget* widget);
void fm_unset_busy_cursor(GtkWidget* widget);

void fm_widget_menu_fix_tooltips(GtkMenu *menu);

G_END_DECLS

#endif /* __FM_GTK_UTILS_H__ */
