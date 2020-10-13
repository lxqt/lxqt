/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2007 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 */

#ifndef H
#define H

#include <QMainWindow>

#include <gio/gio.h>

#include "typedefs.h"
#include "fr-archive.h"
#include "open-file.h"

enum {
    COLUMN_FILE_DATA,
    COLUMN_ICON,
    COLUMN_NAME,
    COLUMN_EMBLEM,
    COLUMN_SIZE,
    COLUMN_TYPE,
    COLUMN_TIME,
    COLUMN_PATH,
    NUMBER_OF_COLUMNS
};

enum {
    TREE_COLUMN_PATH,
    TREE_COLUMN_ICON,
    TREE_COLUMN_NAME,
    TREE_COLUMN_WEIGHT,
    TREE_NUMBER_OF_COLUMNS
};

typedef enum {
    FR_BATCH_ACTION_NONE,
    FR_BATCH_ACTION_LOAD,
    FR_BATCH_ACTION_OPEN,
    FR_BATCH_ACTION_ADD,
    FR_BATCH_ACTION_EXTRACT,
    FR_BATCH_ACTION_EXTRACT_HERE,
    FR_BATCH_ACTION_EXTRACT_INTERACT,
    FR_BATCH_ACTION_RENAME,
    FR_BATCH_ACTION_PASTE,
    FR_BATCH_ACTION_OPEN_FILES,
    FR_BATCH_ACTION_SAVE_AS,
    FR_BATCH_ACTION_TEST,
    FR_BATCH_ACTION_CLOSE,
    FR_BATCH_ACTION_QUIT,
    FR_BATCH_ACTIONS
} FrBatchActionType;

/* -- FrWindow -- */

typedef enum {
    FR_CLIPBOARD_OP_CUT,
    FR_CLIPBOARD_OP_COPY
} FRClipboardOp;


struct FrWindowPrivateData;
struct FrClipboardData;
struct OverwriteData;
struct ExtractData;
struct OpenFilesData;
struct FRBatchAction;


class FrWindow: public QMainWindow {
    Q_OBJECT
public:
    FrArchive* archive;
    FrWindowPrivateData* priv;

    FrWindow();
    virtual ~FrWindow();

Q_SIGNALS:
    void archive_loaded(gboolean    success);
    void progress(double      fraction, const char* msg);
    void ready(GError*     error);


public:
    void        close();

    /* archive operations */

    gboolean    archive_new(
        const char*    uri);
    FrWindow*   archive_open(const char*    uri, QWidget*     parent);
    void        archive_close();
    const char* get_archive_uri();
    const char* get_paste_archive_uri();
    gboolean    archive_is_present();
    void        archive_save_as(
        const char*    filename,
        const char*    password,
        gboolean       encrypt_header,
        guint          volume_size);
    void        archive_reload();
    void        archive_add_files(
        GList*         file_list, /* GFile list */
        gboolean       update);
    void        archive_add_with_wildcard(
        const char*    include_files,
        const char*    exclude_files,
        const char*    exclude_folders,
        const char*    base_dir,
        const char*    dest_dir,
        gboolean       update,
        gboolean       follow_links);
    void        archive_add_directory(
        const char*    directory,
        const char*    base_dir,
        const char*    dest_dir,
        gboolean       update);
    void        archive_add_items(
        GList*         dir_list,
        const char*    base_dir,
        const char*    dest_dir,
        gboolean       update);
    void        archive_add_dropped_items(
        GList*         item_list,
        gboolean       update);
    void        archive_remove(
        GList*         file_list);
    void        archive_extract(
        GList*         file_list,
        const char*    extract_to_dir,
        const char*    base_dir,
        gboolean       skip_older,
        FrOverwrite    overwrite,
        gboolean       junk_paths,
        gboolean       ask_to_open_destination);
    void        archive_extract_here(
        gboolean       skip_older,
        gboolean       overwrite,
        gboolean       junk_paths);
    void        archive_test();

    /**/

    void        set_password(
        const char*    password);
    void        set_password_for_paste(
        const char*    password);
    const char* get_password();
    void        set_encrypt_header(
        gboolean       encrypt_header);
    gboolean    get_encrypt_header();
    void        set_compression(
        FrCompression  compression);
    FrCompression get_compression();
    void        set_volume_size(
        guint          volume_size);
    guint       get_volume_size();

    /**/

    void       go_to_location(
        const char*     path,
        gboolean        force_update);
    const char* get_current_location();
    void       go_up_one_level();
    void       go_back();
    void       go_forward();
    void       current_folder_activated(
        gboolean        from_sidebar);
    void       set_list_mode(
        FrWindowListMode  list_mode);

    /**/

    void       update_list_order();
    GList*     get_file_list_selection(
        gboolean     recursive,
        gboolean*    has_dirs);
    GList*     get_file_list_from_path_list(
        GList*       path_list,
        gboolean*    has_dirs);
    GList*     get_file_list_pattern(
        const char*  pattern);
    int        get_n_selected_files();
    GList*     get_folder_tree_selection(
        gboolean     recursive,
        gboolean*    has_dirs);
    GList*     get_selection(
        gboolean     from_sidebar,
        char**       return_base_dir);

    /*
    GtkTreeModel* get_list_store();
    */

    void       find();
    void       select_all();
    void       unselect_all();

    /*
    void       set_sort_type(GtkSortType  sort_type);
    */


    void       rename_selection(
        gboolean     from_sidebar);
    void       cut_selection(
        gboolean     from_sidebar);
    void       copy_selection(
        gboolean     from_sidebar);
    void       paste_selection(
        gboolean     from_sidebar);

    /**/

    void       stop();
    void       start_activity_mode();
    void       stop_activity_mode();

    /**/

    void        view_last_output(
        const char* title);

    void        open_files(
        GList*      file_list,
        gboolean    ask_application);
    void        open_files_with_command(
        GList*      file_list,
        char*       command);
    void        open_files_with_application(
        GList*      file_list,
        GAppInfo*   app);
    gboolean    update_files(
        GList*      file_list);
    void        update_columns_visibility();
    void        update_history_list();
    void        set_default_dir(
        const char* default_dir,
        gboolean    freeze);
    void        set_open_default_dir(
        const char* default_dir);
    const char* get_open_default_dir();
    void        set_add_default_dir(
        const char* default_dir);
    const char* get_add_default_dir();
    void        set_extract_default_dir(
        const char* default_dir,
        gboolean    freeze);
    const char* get_extract_default_dir();
    void        push_message(
        const char* msg);
    void        pop_message();
    void        set_toolbar_visibility(
        gboolean    value);
    void        set_statusbar_visibility(
        gboolean    value);
    void        set_folders_visibility(
        gboolean    value);
    void        use_progress_dialog(
        gboolean    value);

    /* batch mode procedures. */

    void       new_batch(
        const char*    title);
    void       set_current_batch_action(
        FrBatchActionType  action,
        void*          data,
        GFreeFunc      free_func);
    void       reset_current_batch_action();
    void       restart_current_batch_action();
    void       append_batch_action(
        FrBatchActionType  action,
        void*          data,
        GFreeFunc      free_func);
    void       start_batch();
    void       stop_batch();
    void       resume_batch();
    gboolean   is_batch_mode();
    void       set_batch__extract(
        const char*    filename,
        const char*    dest_dir);
    void       set_batch__extract_here(
        const char*    filename);
    void       set_batch__add(
        const char*    archive,
        GList*         file_list);
    void       destroy_with_error_dialog();


    /*
    gboolean   file_list_drag_data_get(
        GdkDragContext*   context,
        GtkSelectionData* selection_data,
        GList*            path_list);
    */

    void       update_dialog_closed();

private:
    void free_batch_data();
    void clipboard_remove_file_list(GList* file_list);
    void history_clear();
    void free_open_files();
    void convert_data_free(gboolean all);
    void free_private_data();
    void history_add(const char* path);
    void history_pop();
    GPtrArray* get_current_dir_list();
    guint64 get_dir_size(const char* current_dir, const char* name);
    gboolean file_data_respects_filter(FileData* fdata);
    gboolean compute_file_list_name(FileData* fdata, const char* current_dir, int current_dir_len, GHashTable* names_hash, gboolean* different_name);
    void compute_list_names(GPtrArray* files);
    gboolean dir_exists_in_archive(const char* dir_name);
    void convert__action_performed(FrArchive* archive, FrAction action, FrProcError* error, gpointer data);
    void action_performed(FrArchive* archive, FrAction action, FrProcError* error, gpointer data);
    GList* get_dir_list_from_path(char* path);
    GList* get_file_list();
    FileData* get_selected_item_from_file_list();
    char* get_selected_folder_in_tree_view();
    FrClipboardData* get_clipboard_data_from_selection_data(const char* data);
    gboolean handle_errors(FrArchive* archive, FrAction action, FrProcError* error);
    void close_progress_dialog(gboolean close_now);
    void update_dir_tree();
    void update_filter_bar_visibility();
    void update_file_list(gboolean update_view);
    void update_title();
    void update_sensitivity();
    void action_started(FrArchive* archive, FrAction action, gpointer data);
    void progress_dialog_update_action_description();
    gboolean working_archive_cb(FrCommand* command, const char* archive_filename, FrWindow* window);
    gboolean message_cb(FrCommand* command, const char* msg, FrWindow* window);
    void create_the_progress_dialog();
    gboolean display_progress_dialog(gpointer data);
    void open_progress_dialog(gboolean open_now);
    gboolean progress_cb(FrArchive* archive, double fraction);
    void open_progress_dialog_with_open_destination();
    void open_progress_dialog_with_open_archive();
    void add_to_recent_list(char* uri);
    void remove_from_recent_list(char* filename);
    GList* get_selection_as_fd();
    void update_statusbar_list_info();
    void populate_file_list(GPtrArray* files);
    void update_current_location();
    void exec_next_batch_action();
    void exec_current_batch_action();
    GList* get_dir_list_from_file_data(FileData* fdata);
    char* get_selection_data_from_clipboard_data(FrClipboardData* data);
    void deactivate_filter();
    void pref_history_len_changed(GSettings* settings, const char* key, gpointer user_data);
    void pref_view_toolbar_changed(GSettings* settings, const char* key, gpointer user_data);
    gboolean fake_load(FrArchive* archive, gpointer data);
    void activate_filter();
    void construct();
    void set_archive_uri(const char* uri);
    gboolean archive_is_encrypted(GList* file_list);
    void archive_extract_here(gboolean skip_older, FrOverwrite overwrite, gboolean junk_paths);
    void archive_extract_from_edata(ExtractData* edata);
    void ask_overwrite_dialog(OverwriteData* odata);
    int activity_cb(gpointer data);
    // FIXME: void copy_or_cut_selection(FRClipboardOp op, gboolean from_sidebar);
    gboolean name_is_present(const char* current_dir, const char* new_name, char** reason);
    void add_pasted_files(FrClipboardData* data);
    void copy_from_archive_action_performed_cb(FrArchive* archive, FrAction action, FrProcError* error, gpointer data);
    void copy_or_cut_selection(FRClipboardOp op, gboolean from_sidebar);
    void paste_from_clipboard_data(FrClipboardData* data);
    void paste_selection_to(const char* current_dir);
    void open_file_modified_cb(GFileMonitor* monitor, GFile* monitor_file, GFile* other_file, GFileMonitorEvent event_type, gpointer user_data);
    void monitor_open_file(OpenFile* file);
    void monitor_extracted_files(OpenFilesData* odata);
    gboolean open_extracted_files(OpenFilesData* odata);
    void open_files__extract_done_cb(FrArchive* archive, FrAction action, FrProcError* error, gpointer callback_data);
    void exec_batch_action(FRBatchAction* action);
};

#endif /* H */
