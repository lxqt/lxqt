/*
 *      fm-gtk-file-launcher.c
 *
 *      Copyright 2010-2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2010 Shae Smittle <starfall87@gmail.com>
 *      Copyright 2012-2015 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

/**
 * SECTION:fm-gtk-file-launcher
 * @short_description: Gtk file launcher utilities.
 * @title: Gtk file launcher
 *
 * @include: libfm/fm-gtk.h
 *
 * Utilities to launch files using libfm file launchers.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include <gio/gdesktopappinfo.h>
#include <math.h>

#include "fm-gtk-file-launcher.h"

#include "fm-gtk-utils.h"
#include "fm-app-chooser-dlg.h"

#include "fm.h"
#include "gtk-compat.h"

/* for open() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* for read() */
#include <unistd.h>

/* for strtoul() */
#include <stdlib.h>

typedef struct _LaunchData LaunchData;
struct _LaunchData
{
    GtkWindow* parent;
    FmLaunchFolderFunc folder_func;
    gpointer user_data;
};

static GAppInfo* choose_app(GList* file_infos, FmMimeType* mime_type, gpointer user_data, GError** err)
{
    LaunchData* data = (LaunchData*)user_data;
    return fm_choose_app_for_mime_type(data->parent, mime_type, mime_type != NULL);
}

static gboolean on_launch_error(GAppLaunchContext* ctx, GError* err,
                                FmPath* path, gpointer user_data)
{
    LaunchData* data = (LaunchData*)user_data;

    /* ask for mount if trying to launch unmounted path */
    if(err->domain == G_IO_ERROR)
    {
        if(path && err->code == G_IO_ERROR_NOT_MOUNTED)
        {
            if(fm_mount_path(data->parent, path, TRUE))
                return FALSE; /* ask to retry */
        }
        else if(err->code == G_IO_ERROR_FAILED_HANDLED)
            return TRUE; /* don't show error message */
    }
    fm_show_error(data->parent, NULL, err->message);
    return TRUE;
}

static gboolean on_open_folder(GAppLaunchContext* ctx, GList* folder_infos, gpointer user_data, GError** err)
{
    LaunchData* data = (LaunchData*)user_data;
    if (data->folder_func)
        return data->folder_func(ctx, folder_infos, data->user_data, err);
    else
        return FALSE;
}

static int on_launch_ask(const char* msg, char* const* btn_labels, int default_btn, gpointer user_data)
{
    LaunchData* data = (LaunchData*)user_data;
    /* FIXME: set default button properly */
    return fm_askv(data->parent, NULL, msg, btn_labels);
}

static FmFileLauncherExecAction on_exec_file(FmFileInfo* file, gpointer user_data)
{
    GtkBuilder* b;
    GtkDialog* dlg;
    GtkLabel *msg;
    GtkImage *icon;
    char* msg_str;
    int res;
    FmIcon* fi_icon;

    if (fm_config->quick_exec)
    {
        /* SF bug#838: open terminal for each script may be just a waste.
           User should open a terminal and start the script there
           in case if user wants to see the script output anyway.
        if (fm_file_info_is_text(file))
            return FM_FILE_LAUNCHER_EXEC_IN_TERMINAL; */
        return FM_FILE_LAUNCHER_EXEC;
    }
    b = gtk_builder_new();
    fi_icon = fm_file_info_get_icon(file);
    gtk_builder_set_translation_domain(b, GETTEXT_PACKAGE);
    gtk_builder_add_from_file(b, PACKAGE_UI_DIR "/exec-file.ui", NULL);
    dlg = GTK_DIALOG(gtk_builder_get_object(b, "dlg"));
    msg = GTK_LABEL(gtk_builder_get_object(b, "msg"));
    icon = GTK_IMAGE(gtk_builder_get_object(b, "icon"));
    gtk_image_set_from_gicon(icon, G_ICON(fi_icon), GTK_ICON_SIZE_DIALOG);
    gtk_box_set_homogeneous(GTK_BOX(gtk_dialog_get_action_area(dlg)), FALSE);

    /* If we reached this point then file is executable: either script or binary */
    /* If it's a script, ask the user first. */
    if(fm_file_info_is_text(file))
    {
        msg_str = g_strdup_printf(_("This text file '%s' seems to be an executable script.\nWhat do you want to do with it?"), fm_file_info_get_disp_name(file));
        gtk_dialog_set_default_response(dlg, FM_FILE_LAUNCHER_EXEC_IN_TERMINAL);
    }
    else
    {
        GtkWidget* open = GTK_WIDGET(gtk_builder_get_object(b, "open"));
        gtk_widget_destroy(open);
        msg_str = g_strdup_printf(_("This file '%s' is executable. Do you want to execute it?"), fm_file_info_get_disp_name(file));
        gtk_dialog_set_default_response(dlg, FM_FILE_LAUNCHER_EXEC);
    }
    gtk_label_set_text(msg, msg_str);
    g_free(msg_str);

    res = gtk_dialog_run(dlg);
    gtk_widget_destroy(GTK_WIDGET(dlg));
    g_object_unref(b);

    if(res <=0)
        res = FM_FILE_LAUNCHER_EXEC_CANCEL;

    return res;
}

/**
 * fm_launch_files_simple
 * @parent: (allow-none): window to determine launch screen
 * @ctx: (allow-none): launch context
 * @file_infos: (element-type FmFileInfo): files to launch
 * @func: callback to launch folder
 * @user_data: data supplied for @func
 *
 * Launches files using @func to launch folders. If @ctx is %NULL
 * then new context on the same screen as @parent will be created for
 * launching.
 *
 * Returns: %TRUE if launch was succesful.
 *
 * Since: 0.1.0
 */
gboolean fm_launch_files_simple(GtkWindow* parent, GAppLaunchContext* ctx, GList* file_infos, FmLaunchFolderFunc func, gpointer user_data)
{
    FmFileLauncher launcher = {
        .get_app = choose_app,
        .open_folder = on_open_folder,
        .exec_file = on_exec_file,
        .error = on_launch_error,
        .ask = on_launch_ask
    };
    LaunchData data = {parent, func, user_data};
    GdkAppLaunchContext* _ctx = NULL;
    gboolean ret;

    if (!func)
        launcher.open_folder = NULL;

    if(ctx == NULL)
    {
        _ctx = gdk_display_get_app_launch_context(gdk_display_get_default());
        gdk_app_launch_context_set_screen(_ctx, parent ? gtk_widget_get_screen(GTK_WIDGET(parent)) : gdk_screen_get_default());
        gdk_app_launch_context_set_timestamp(_ctx, gtk_get_current_event_time());
        /* FIXME: how to handle gdk_app_launch_context_set_icon? */
        ctx = G_APP_LAUNCH_CONTEXT(_ctx);
    }
    ret = fm_launch_files(ctx, file_infos, &launcher, &data);
    if(_ctx)
        g_object_unref(_ctx);
    return ret;
}

/**
 * fm_launch_paths_simple
 * @parent: (allow-none): window to determine launch screen
 * @ctx: (allow-none): launch context
 * @paths: (element-type FmPath): files to launch
 * @func: callback to launch folder
 * @user_data: data supplied for @func
 *
 * Launches files using @func to launch folders. If @ctx is %NULL
 * then new context on the same screen as @parent will be created for
 * launching.
 *
 * Returns: %TRUE if launch was succesful.
 *
 * Since: 0.1.0
 */
gboolean fm_launch_paths_simple(GtkWindow* parent, GAppLaunchContext* ctx, GList* paths, FmLaunchFolderFunc func, gpointer user_data)
{
    FmFileLauncher launcher = {
        .get_app = choose_app,
        .open_folder = on_open_folder,
        .exec_file = on_exec_file,
        .error = on_launch_error,
        .ask = on_launch_ask
    };
    LaunchData data = {parent, func, user_data};
    GdkAppLaunchContext* _ctx = NULL;
    gboolean ret;
    if(ctx == NULL)
    {
        _ctx = gdk_display_get_app_launch_context(gdk_display_get_default());
        gdk_app_launch_context_set_screen(_ctx, parent ? gtk_widget_get_screen(GTK_WIDGET(parent)) : gdk_screen_get_default());
        gdk_app_launch_context_set_timestamp(_ctx, gtk_get_current_event_time());
        /* FIXME: how to handle gdk_app_launch_context_set_icon? */
        ctx = G_APP_LAUNCH_CONTEXT(_ctx);
    }
    ret = fm_launch_paths(ctx, paths, &launcher, &data);
    if(_ctx)
        g_object_unref(_ctx);
    return ret;
}

/**
 * fm_launch_file_simple
 * @parent: (allow-none): window to determine launch screen
 * @ctx: (allow-none): launch context
 * @file_info: file to launch
 * @func: callback to launch folder
 * @user_data: data supplied for @func
 *
 * Launches file. If @file_info is folder then uses @func to launch it.
 * If @ctx is %NULL then new context on the same screen as @parent will
 * be created for launching.
 *
 * Returns: %TRUE if launch was succesful.
 *
 * Since: 0.1.0
 */
gboolean fm_launch_file_simple(GtkWindow* parent, GAppLaunchContext* ctx, FmFileInfo* file_info, FmLaunchFolderFunc func, gpointer user_data)
{
    gboolean ret;
    GList* files = g_list_prepend(NULL, file_info);
    ret = fm_launch_files_simple(parent, ctx, files, func, user_data);
    g_list_free(files);
    return ret;
}

/**
 * fm_launch_path_simple
 * @parent: (allow-none): window to determine launch screen
 * @ctx: (allow-none): launch context
 * @path: file to launch
 * @func: callback to launch folder
 * @user_data: data supplied for @func
 *
 * Launches file. If @path is folder then uses @func to launch it.
 * If @ctx is %NULL then new context on the same screen as @parent will
 * be created for launching.
 *
 * Returns: %TRUE if launch was succesful.
 *
 * Since: 0.1.0
 */
gboolean fm_launch_path_simple(GtkWindow* parent, GAppLaunchContext* ctx, FmPath* path, FmLaunchFolderFunc func, gpointer user_data)
{
    gboolean ret;
    GList* files = g_list_prepend(NULL, path);
    ret = fm_launch_paths_simple(parent, ctx, files, func, user_data);
    g_list_free(files);
    return ret;
}

/**
 * fm_launch_desktop_entry_simple
 * @parent: (allow-none): window to determine launch screen
 * @ctx: (allow-none): launch context
 * @entry: desktop entry file to launch
 * @files: (allow-none): files to supply launch
 *
 * Launches desktop entry. @files will be supplied to launch if @entry
 * accepts arguments. If @ctx is %NULL then new context on the same
 * screen as @parent will be created for launching the application.
 *
 * Returns: %TRUE if launch was succesful.
 *
 * Since: 1.0.1
 */
gboolean fm_launch_desktop_entry_simple(GtkWindow* parent, GAppLaunchContext* ctx,
                                        FmFileInfo* entry, FmPathList* files)
{
    FmFileLauncher launcher = {
        .get_app = NULL,
        .open_folder = NULL,
        .exec_file = NULL,
        .error = on_launch_error,
        .ask = on_launch_ask
    };
    LaunchData data = {parent, NULL, NULL};
    GdkAppLaunchContext *_ctx = NULL;
    GList *l, *uris = NULL;
    FmPath *path;
    char *entry_path;
    gboolean ret;

    if(!entry || (path = fm_file_info_get_path(entry)) == NULL)
        return FALSE;
    if(ctx == NULL)
    {
        _ctx = gdk_display_get_app_launch_context(gdk_display_get_default());
        gdk_app_launch_context_set_screen(_ctx, parent ? gtk_widget_get_screen(GTK_WIDGET(parent)) : gdk_screen_get_default());
        gdk_app_launch_context_set_timestamp(_ctx, gtk_get_current_event_time());
        /* FIXME: how to handle gdk_app_launch_context_set_icon? */
        ctx = G_APP_LAUNCH_CONTEXT(_ctx);
    }
    if(files) for(l = fm_path_list_peek_head_link(files); l; l = l->next)
        uris = g_list_append(uris, fm_path_to_uri(FM_PATH(l->data)));
    /* special handling for shortcuts */
    if (fm_file_info_is_shortcut(entry))
        entry_path = g_strdup(fm_file_info_get_target(entry));
    else
        entry_path = fm_path_to_str(path);
    ret = fm_launch_desktop_entry(ctx, entry_path, uris, &launcher, &data);
    g_list_foreach(uris, (GFunc)g_free, NULL);
    g_list_free(uris);
    g_free(entry_path);
    if(_ctx)
        g_object_unref(_ctx);
    return ret;
}

/**
 * fm_launch_command_simple
 * @parent: (allow-none): window to determine launch screen
 * @ctx: (allow-none): launch context
 * @flags: flags to launch command
 * @cmd: command to launch
 * @files: (allow-none): files to supply launch
 *
 * Launches command. @files will be supplied to launch if @cmd
 * accepts arguments. If @ctx is %NULL then new context on the same
 * screen as @parent will be created for launching the command.
 *
 * Returns: %TRUE if launch was succesful.
 *
 * Since: 1.2.0
 */
gboolean fm_launch_command_simple(GtkWindow *parent, GAppLaunchContext *ctx,
                                  GAppInfoCreateFlags flags, const char *cmd,
                                  FmPathList *files)
{
    GAppInfo *appinfo;
    GError *error = NULL;
    GList *l, *fl = NULL;
    GdkAppLaunchContext *_ctx = NULL;
    gboolean ok;

    appinfo = fm_app_info_create_from_commandline(cmd, NULL, flags, &error);
    if (appinfo == NULL)
    {
        fm_show_error(parent, NULL, error->message);
        g_error_free(error);
        return FALSE;
    }
    if (ctx == NULL && parent != NULL)
    {
        _ctx = gdk_display_get_app_launch_context(gdk_display_get_default());
        gdk_app_launch_context_set_screen(_ctx, gtk_widget_get_screen(GTK_WIDGET(parent)));
        gdk_app_launch_context_set_timestamp(_ctx, gtk_get_current_event_time());
        /* FIXME: how to handle gdk_app_launch_context_set_icon? */
        ctx = G_APP_LAUNCH_CONTEXT(_ctx);
    }
    if (files) for (l = fm_path_list_peek_head_link(files); l; l = l->next)
        fl = g_list_append(fl, fm_path_to_gfile(FM_PATH(l->data)));
    ok = fm_app_info_launch(appinfo, fl, ctx, &error);
    if (!ok)
    {
        fm_show_error(parent, NULL, error->message);
        g_error_free(error);
    }
    g_list_free_full(fl, g_object_unref);
    g_object_unref(appinfo);
    if (_ctx)
        g_object_unref(_ctx);
    return ok;
}

/*
 * file-search-ui.c
 */

/* generated by glade-connect-gen, defined in file-search-ui.c */
static void filesearch_glade_connect_signals(GtkBuilder* builder, gpointer user_data);

typedef struct
{
    GtkDialog * dlg;
    GtkTreeView * path_tree_view;
    GtkEntry* name_entry;
    GtkCheckButton* name_case_insensitive_checkbutton;
    GtkCheckButton* name_regex_checkbutton;
    GtkCheckButton* search_recursive_checkbutton;
    GtkCheckButton* search_hidden_files_checkbutton;

    GtkCheckButton* text_file_checkbutton;
    GtkCheckButton* image_file_checkbutton;
    GtkCheckButton* audio_file_checkbutton;
    GtkCheckButton* video_file_checkbutton;
    GtkCheckButton* doc_file_checkbutton;
    GtkCheckButton* dir_file_checkbutton;
    GtkCheckButton* other_file_checkbutton;
    GtkEntry* other_file_entry;

    GtkEntry* content_entry;
    GtkCheckButton* content_case_insensitive_checkbutton;
    GtkCheckButton* content_regex_checkbutton;

    GtkSpinButton* bigger_spinbutton;
    GtkComboBox* bigger_unit_combo;

    GtkSpinButton* smaller_spinbutton;
    GtkComboBox* smaller_unit_combo;

    GtkCheckButton* min_mtime_checkbutton;
    GtkButton* min_mtime_button;
    GtkCheckButton* max_mtime_checkbutton;
    GtkButton* max_mtime_button;

    GtkListStore * path_list_store;

    GtkDialog* date_dlg;
    GtkCalendar* calendar;

    GtkWindow* parent;
    GAppLaunchContext* ctx;
    FmLaunchFolderFunc func;
    gpointer user_data;
} FileSearchUI;

#if 0
static gchar * document_mime_types[] = {
    "application/pdf",
    "application/msword",
    "application/vnd.ms-excel",
    "application/vnd.ms-powerpoint",
    "application/vnd.oasis.opendocument.chart",
    "application/vnd.oasis.opendocument.database",
    "application/vnd.oasis.opendocument.formula",
    "application/vnd.oasis.opendocument.graphics",
    "application/vnd.oasis.opendocument.image",
    "application/vnd.oasis.opendocument.presentation",
    "application/vnd.oasis.opendocument.spreadsheet",
    "application/vnd.oasis.opendocument.text",
    "application/vnd.oasis.opendocument.text-master",
    "application/vnd.oasis.opendocument.text-web",
    "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    "application/vnd.openxmlformats-officedocument.presentationml.presentation",
    "application/vnd.openxmlformats-officedocument.presentationml.slideshow",
    "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
    "application/x-abiword",
    "application/x-gnumeric",
    "application/x-dvi",
    NULL
};
#endif

typedef enum
{
    FM_SAVED_SEARCH_NAME_CI = 1<<0,
    FM_SAVED_SEARCH_NAME_REGEXP = 1<<1,
    FM_SAVED_SEARCH_RECURSION = 1<<2,
    FM_SAVED_SEARCH_HIDDEN = 1<<3,
    FM_SAVED_SEARCH_TYPE_TEXT = 1<<4,
    FM_SAVED_SEARCH_TYPE_IMAGE = 1<<5,
    FM_SAVED_SEARCH_TYPE_AUDIO = 1<<6,
    FM_SAVED_SEARCH_TYPE_VIDEO = 1<<7,
    FM_SAVED_SEARCH_TYPE_DOCS = 1<<8,
    FM_SAVED_SEARCH_TYPE_DIRS = 1<<9,
    FM_SAVED_SEARCH_CONTENT_CI = 1<<10,
    FM_SAVED_SEARCH_CONTENT_REGEXP = 1<<11
} FmSavedSearch;

/* UI Signal Handlers */

static gboolean launch_search(FileSearchUI* ui)
{
    GString* search_uri = g_string_sized_new(1024);
    GdkAppLaunchContext *_ctx = NULL;
    GtkTreeModel* model;
    GtkTreeIter it;
    gboolean ret;
    FmSavedSearch saved = 0;

    /* build the search:// URI to perform the search */
    g_string_append(search_uri, "search://");

    model = GTK_TREE_MODEL(ui->path_list_store);
    if(gtk_tree_model_get_iter_first(model, &it)) /* we need to have at least one dir path */
    {
        gboolean recursive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->search_recursive_checkbutton));
        gboolean show_hidden = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->search_hidden_files_checkbutton));
        const char* name_patterns = gtk_entry_get_text(ui->name_entry);
        gboolean name_ci = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->name_case_insensitive_checkbutton));
        gboolean name_regex = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->name_regex_checkbutton));
        const char* content_pattern = gtk_entry_get_text(ui->content_entry);
        gboolean content_ci = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->content_case_insensitive_checkbutton));
        gboolean content_regex = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->content_regex_checkbutton));
        const char *other_file_pattern = gtk_entry_get_text(ui->other_file_entry);
        const guint unit_bytes[] = {1, (1024), (1024*1024), (1024*1024*1024)};
        GSList* mime_types = NULL;
        FmPath* search_path;
        GList* search_path_list;
        char *escaped;
        FmFileLauncher launcher = {
            .get_app = choose_app,
            .open_folder = on_open_folder,
            .exec_file = on_exec_file,
            .error = on_launch_error,
            .ask = on_launch_ask
        };
        LaunchData data = {ui->parent, ui->func, ui->user_data};

        /* add paths */
        for(;;)
        {
            char *path_str;
            gtk_tree_model_get(model, &it, 0, &path_str, -1);

            /* escape possible '?' and ',' */
            escaped = g_uri_escape_string(path_str, "!$&'()*+:;=/@", TRUE);
            g_free(path_str);
            g_string_append(search_uri, escaped);
            g_free(escaped);

            if(!gtk_tree_model_iter_next(model, &it)) /* no more items */
                break;
            g_string_append_c(search_uri, ','); /* separator for paths */
        }

        g_string_append_c(search_uri, '?');
        g_string_append_printf(search_uri, "recursive=%c", recursive ? '1' : '0');
        g_string_append_printf(search_uri, "&show_hidden=%c", show_hidden ? '1' : '0');
        if(name_patterns && *name_patterns)
        {
            /* escape ampersands in pattern */
            escaped = g_uri_escape_string(name_patterns, ":/?#[]@!$'()*+,;", TRUE);
            if(name_regex)
                g_string_append_printf(search_uri, "&name_regex=%s", escaped);
            else
                g_string_append_printf(search_uri, "&name=%s", escaped);
            if(name_ci)
                g_string_append_printf(search_uri, "&name_ci=%c", name_ci ? '1' : '0');
            g_free(escaped);
        }

        if(content_pattern && *content_pattern)
        {
            /* escape ampersands in pattern */
            escaped = g_uri_escape_string(content_pattern, ":/?#[]@!$'()*+,;^<>{}", TRUE);
            if(content_regex)
                g_string_append_printf(search_uri, "&content_regex=%s", escaped);
            else
                g_string_append_printf(search_uri, "&content=%s", escaped);
            g_free(escaped);
            if(content_ci)
                g_string_append_printf(search_uri, "&content_ci=%c", content_ci ? '1' : '0');
        }

        /* search for the files of specific mime-types */
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->text_file_checkbutton)))
            mime_types = g_slist_prepend(mime_types, (gpointer)"text/plain");
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->image_file_checkbutton)))
            mime_types = g_slist_prepend(mime_types, (gpointer)"image/*");
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->audio_file_checkbutton)))
            mime_types = g_slist_prepend(mime_types, (gpointer)"audio/*");
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->video_file_checkbutton)))
            mime_types = g_slist_prepend(mime_types, (gpointer)"video/*");
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->doc_file_checkbutton)))
        {
            mime_types = g_slist_prepend(mime_types, (gpointer)
                "application/pdf;"
                /* "text/html;" */
                "application/vnd.oasis.opendocument.*;"
                "application/vnd.openxmlformats-officedocument.*;"
                "application/msword;application/vnd.ms-word;"
                "application/msexcel;application/vnd.ms-excel"
            );
        }
        if (ui->dir_file_checkbutton &&
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->dir_file_checkbutton)))
            mime_types = g_slist_prepend(mime_types, (gpointer)"inode/directory");
        if (ui->other_file_checkbutton &&
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->other_file_checkbutton)) &&
            other_file_pattern && other_file_pattern[0] &&
            strchr(other_file_pattern, '&') == NULL) /* & is invalid here */
        {
            mime_types = g_slist_prepend(mime_types, (gpointer)other_file_pattern);
        }
        else
            other_file_pattern = NULL;

        if(mime_types)
        {
            GSList* l;
            g_string_append(search_uri, "&mime_types=");
            for(l = mime_types; l; l=l->next)
            {
                const char* mime_type = (const char*)l->data;
                g_string_append(search_uri, mime_type);
                if(l->next)
                    g_string_append_c(search_uri, ';');
            }
            g_slist_free(mime_types);
        }

        if(gtk_widget_get_sensitive(GTK_WIDGET(ui->bigger_spinbutton)))
        {
            gdouble min_size = gtk_spin_button_get_value(ui->bigger_spinbutton);
            gint unit_index = gtk_combo_box_get_active(ui->bigger_unit_combo);
            /* convert to bytes */
            min_size *= unit_bytes[unit_index];
            g_string_append_printf(search_uri, "&min_size=%llu",
                                   (long long unsigned int)min_size);
        }

        if(gtk_widget_get_sensitive(GTK_WIDGET(ui->smaller_spinbutton)))
        {
            gdouble max_size = gtk_spin_button_get_value(ui->smaller_spinbutton);
            gint unit_index = gtk_combo_box_get_active(ui->smaller_unit_combo);
            /* convert to bytes */
            max_size *= unit_bytes[unit_index];
            g_string_append_printf(search_uri, "&max_size=%llu",
                                   (long long unsigned int)max_size);
        }

        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->min_mtime_checkbutton)))
        {
            const char* label = gtk_button_get_label(ui->min_mtime_button);
            if(g_strcmp0(label, _("(None)")) != 0)
                g_string_append_printf(search_uri, "&min_mtime=%s", label);
        }

        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->max_mtime_checkbutton)))
        {
            const char* label = gtk_button_get_label(ui->max_mtime_button);
            if(g_strcmp0(label, _("(None)")) != 0)
                g_string_append_printf(search_uri, "&max_mtime=%s", label);
        }

        search_path = fm_path_new_for_uri(search_uri->str);

        if(ui->ctx == NULL)
        {
            _ctx = gdk_display_get_app_launch_context(gdk_display_get_default());
            gdk_app_launch_context_set_screen(_ctx, ui->parent ? gtk_widget_get_screen(GTK_WIDGET(ui->parent)) : gdk_screen_get_default());
            gdk_app_launch_context_set_timestamp(_ctx, gtk_get_current_event_time());
            /* FIXME: how to handle gdk_app_launch_context_set_icon? */
            ui->ctx = G_APP_LAUNCH_CONTEXT(_ctx);
        }

        search_path_list = g_list_append(NULL, search_path);

        ret = fm_launch_paths(ui->ctx, search_path_list, &launcher, &data);

        g_list_free(search_path_list);
        fm_path_unref(search_path);
        /* save search */
        if (name_ci)
            saved = FM_SAVED_SEARCH_NAME_CI;
        if (name_regex)
            saved |= FM_SAVED_SEARCH_NAME_REGEXP;
        if (recursive)
            saved |= FM_SAVED_SEARCH_RECURSION;
        if (show_hidden)
            saved |= FM_SAVED_SEARCH_HIDDEN;
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->text_file_checkbutton)))
            saved |= FM_SAVED_SEARCH_TYPE_TEXT;
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->image_file_checkbutton)))
            saved |= FM_SAVED_SEARCH_TYPE_IMAGE;
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->audio_file_checkbutton)))
            saved |= FM_SAVED_SEARCH_TYPE_AUDIO;
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->video_file_checkbutton)))
            saved |= FM_SAVED_SEARCH_TYPE_VIDEO;
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->doc_file_checkbutton)))
            saved |= FM_SAVED_SEARCH_TYPE_DOCS;
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui->dir_file_checkbutton)))
            saved |= FM_SAVED_SEARCH_TYPE_DIRS;
        if (content_ci)
            saved |= FM_SAVED_SEARCH_CONTENT_CI;
        if (content_regex)
            saved |= FM_SAVED_SEARCH_CONTENT_REGEXP;
        g_string_printf(search_uri, "%lx", (long int)saved);
        if (other_file_pattern)
            g_string_append_printf(search_uri, "&%s&", other_file_pattern);
        g_string_append_printf(search_uri, "/%s/%s", name_patterns, content_pattern);
        g_free(fm_config->saved_search);
        fm_config->saved_search = g_string_free(search_uri, FALSE);
        fm_config_emit_changed(fm_config, "saved_search");
        if(_ctx)
            g_object_unref(_ctx);
    }
    else
    {
        /* show error if no paths are added */
        fm_show_error(GTK_WINDOW(ui->dlg), NULL, _("No folders are specified."));
        ret = FALSE;
    }
    return ret;
}

static void on_dlg_response(GtkDialog* dlg, int response, gpointer user_data)
{
    FileSearchUI * ui = (FileSearchUI *)user_data;

    if(response == GTK_RESPONSE_OK)
    {
        if(!launch_search(ui))
            return;
    }

    gtk_widget_destroy(GTK_WIDGET(ui->date_dlg));
    gtk_widget_destroy(GTK_WIDGET(dlg));
}

static void add_path(GtkListStore * list_store, const char* uri)
{
    char* filename;
    GtkTreeIter it;
    gtk_list_store_append(list_store, &it);
    filename = g_filename_from_uri(uri, NULL, NULL);
    if(filename)
    {
        gtk_list_store_set(list_store, &it, 0, filename, -1);
        g_free(filename);
    }
    else
        gtk_list_store_set(list_store, &it, 0, uri, -1);
}

static void on_add_path_button_clicked(GtkButton * btn, gpointer user_data)
{
    FileSearchUI * ui = (FileSearchUI *)user_data;
    GtkWidget* dlg = gtk_file_chooser_dialog_new(
                          _("Select Folder"), GTK_WINDOW(ui->dlg),
                          GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OPEN, GTK_RESPONSE_OK,
                          NULL );
    gtk_dialog_set_alternative_button_order(GTK_DIALOG(dlg), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dlg), TRUE);

    if(gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
    {
        GSList* uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dlg));
        GSList* l;
        for(l = uris; l; l=l->next)
        {
            char* uri = (char*)l->data;
            add_path(ui->path_list_store, uri);
            g_free(uri);
        }
        g_slist_free(uris);
    }
    gtk_widget_destroy(dlg);
}

static void on_remove_path_button_clicked(GtkButton * btn, gpointer user_data)
{
    FileSearchUI * ui = (FileSearchUI *)user_data;

    GtkTreeIter it;
    GtkTreeSelection* sel = gtk_tree_view_get_selection( GTK_TREE_VIEW(ui->path_tree_view) );
    if( gtk_tree_selection_get_selected(sel, NULL, &it) )
        gtk_list_store_remove( ui->path_list_store, &it );
}

static void on_other_file_checkbutton_toggled(GtkToggleButton *btn, FileSearchUI *ui)
{
    gboolean enabled = gtk_toggle_button_get_active(btn);

    gtk_widget_set_sensitive(GTK_WIDGET(ui->other_file_entry), enabled);
}

static void on_name_entry_changed(GtkEditable *editable, FileSearchUI *ui)
{
    char *text, *cpt;
    gint pos;

    text = gtk_editable_get_chars(editable, 0, -1);
    cpt = strchr(text, G_DIR_SEPARATOR);
    if (cpt) /* file basename can never contain separator */
    {
        *cpt = '\0';
        pos = gtk_editable_get_position(editable);
        gtk_entry_set_text(GTK_ENTRY(editable), text);
        gtk_editable_set_position(editable, pos - 1);
    }
    g_free(text);
}

static void file_search_ui_free(gpointer ui)
{
    g_slice_free(FileSearchUI, ui);
}

/**
 * fm_launch_search_simple
 * @parent: (allow-none): window to determine launch screen
 * @ctx: (allow-none): launch context
 * @paths: (allow-none) (element-type FmPath): paths to set initial search paths list
 * @func: callback to launch folder
 * @user_data: data supplied for @func
 *
 * Launches filesystem search. Uses @func to open folder with search
 * results. If @ctx is %NULL then new context on the same screen as
 * @parent will be created for launching.
 *
 * Returns: %TRUE if launch was succesful.
 *
 * Since: 1.0.2
 */
gboolean fm_launch_search_simple(GtkWindow* parent, GAppLaunchContext* ctx,
                                 const GList* paths, FmLaunchFolderFunc func,
                                 gpointer user_data)
{
    FileSearchUI * ui;
    char *c, *expr, *mask;
    FmSavedSearch saved;

    g_return_val_if_fail(func != NULL, FALSE);
    ui = g_slice_new0(FileSearchUI);
    ui->parent = parent;
    ui->ctx = ctx;
    ui->func = func;
    ui->user_data = user_data;

    GtkBuilder* builder = gtk_builder_new();
    gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);
    gtk_builder_add_from_file(builder, PACKAGE_UI_DIR "/filesearch.ui", NULL);

    ui->dlg = GTK_DIALOG(gtk_builder_get_object(builder, "dlg"));
    gtk_dialog_set_alternative_button_order(GTK_DIALOG(ui->dlg), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL);
    ui->path_tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "path_tree_view"));

    ui->name_entry = GTK_ENTRY(gtk_builder_get_object(builder, "name_entry"));
    ui->name_case_insensitive_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "name_case_insensitive_checkbutton"));
    ui->name_regex_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "name_regex_checkbutton"));
    ui->search_recursive_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "search_recursive_checkbutton"));
    ui->search_hidden_files_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "search_hidden_files_checkbutton"));

    ui->text_file_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "text_file_checkbutton"));
    ui->image_file_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "image_file_checkbutton"));
    ui->audio_file_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "audio_file_checkbutton"));
    ui->video_file_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "video_file_checkbutton"));
    ui->doc_file_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "doc_file_checkbutton"));
    ui->dir_file_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "dir_file_checkbutton"));
    ui->other_file_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "other_file_checkbutton"));
    ui->other_file_entry = GTK_ENTRY(gtk_builder_get_object(builder, "other_file_entry"));
    if (ui->other_file_checkbutton)
    {
        gtk_widget_show(GTK_WIDGET(ui->other_file_checkbutton));
        gtk_widget_show(GTK_WIDGET(ui->dir_file_checkbutton));
        gtk_widget_show(GTK_WIDGET(ui->other_file_entry));
        gtk_widget_set_sensitive(GTK_WIDGET(ui->other_file_entry), FALSE);
        g_signal_connect(ui->other_file_checkbutton, "toggled",
                         G_CALLBACK(on_other_file_checkbutton_toggled), ui);
    }

    ui->content_entry = GTK_ENTRY(gtk_builder_get_object(builder, "content_entry"));
    ui->content_case_insensitive_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "content_case_insensitive_checkbutton"));
    ui->content_regex_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "content_regex_checkbutton"));

    ui->bigger_spinbutton = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "bigger_spinbutton"));
    ui->bigger_unit_combo = GTK_COMBO_BOX(gtk_builder_get_object(builder, "bigger_unit_combo"));

    ui->smaller_spinbutton = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "smaller_spinbutton"));
    ui->smaller_unit_combo = GTK_COMBO_BOX(gtk_builder_get_object(builder, "smaller_unit_combo"));

    ui->min_mtime_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "min_mtime_checkbutton"));
    ui->min_mtime_button = GTK_BUTTON(gtk_builder_get_object(builder, "min_mtime_button"));
    ui->max_mtime_checkbutton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "max_mtime_checkbutton"));
    ui->max_mtime_button = GTK_BUTTON(gtk_builder_get_object(builder, "max_mtime_button"));

    ui->path_list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "path_list_store"));

    ui->date_dlg = GTK_DIALOG(gtk_builder_get_object(builder, "date_dlg"));
    gtk_dialog_set_alternative_button_order(GTK_DIALOG(ui->date_dlg), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL);
    ui->calendar = GTK_CALENDAR(gtk_builder_get_object(builder, "calendar"));

    /* check for saved config */
    if (fm_config->saved_search)
    {
        saved = strtoul(fm_config->saved_search, &c, 16);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->name_case_insensitive_checkbutton),
                                    (saved & FM_SAVED_SEARCH_NAME_CI) != 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->name_regex_checkbutton),
                                    (saved & FM_SAVED_SEARCH_NAME_REGEXP) != 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->search_recursive_checkbutton),
                                    (saved & FM_SAVED_SEARCH_RECURSION) != 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->search_hidden_files_checkbutton),
                                    (saved & FM_SAVED_SEARCH_HIDDEN) != 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->text_file_checkbutton),
                                    (saved & FM_SAVED_SEARCH_TYPE_TEXT) != 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->image_file_checkbutton),
                                    (saved & FM_SAVED_SEARCH_TYPE_IMAGE) != 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->audio_file_checkbutton),
                                    (saved & FM_SAVED_SEARCH_TYPE_AUDIO) != 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->video_file_checkbutton),
                                    (saved & FM_SAVED_SEARCH_TYPE_VIDEO) != 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->doc_file_checkbutton),
                                    (saved & FM_SAVED_SEARCH_TYPE_DOCS) != 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->dir_file_checkbutton),
                                    (saved & FM_SAVED_SEARCH_TYPE_DIRS) != 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->content_case_insensitive_checkbutton),
                                    (saved & FM_SAVED_SEARCH_CONTENT_CI) != 0);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->content_regex_checkbutton),
                                    (saved & FM_SAVED_SEARCH_CONTENT_REGEXP) != 0);
        if (*c == '&')
        {
            expr = g_strdup(&c[1]);
            mask = strchr(expr, '&');
            if (mask)
                *mask++ = '\0';
            if (ui->other_file_checkbutton)
            {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->other_file_checkbutton), TRUE);
                gtk_entry_set_text(ui->other_file_entry, expr);
            }
        }
        else
            mask = expr = g_strdup(c);
        if (*mask == '/') /* else corrupted */
        {
            mask++;
            c = strchr(mask, '/');
            if (c)
                *c++ = '\0';
            if (mask[0])
                gtk_entry_set_text(ui->name_entry, mask);
            if (c && c[0])
                gtk_entry_set_text(ui->content_entry, c);
        }
        g_free(expr);
    }

    filesearch_glade_connect_signals(builder, ui);
    g_signal_connect(ui->name_entry, "changed", G_CALLBACK(on_name_entry_changed), ui);
    g_object_unref(builder);

    /* associate the data with the dialog so it can be freed as needed. */
    g_object_set_qdata_full(G_OBJECT(ui->dlg), fm_qdata_id, ui, file_search_ui_free);

    /* add folders to search */
    if(paths)
    {
        const GList* l;
        for(l = paths; l; l = l->next)
        {
            FmPath* folder_path = FM_PATH(l->data);
            char* path_str = fm_path_to_str(folder_path);
            add_path(ui->path_list_store, path_str);
            g_free(path_str);
        }
    }

    if(parent)
        gtk_window_set_transient_for(GTK_WINDOW(ui->dlg), parent);
    gtk_widget_show(GTK_WIDGET(ui->dlg));

    return TRUE;
}

static void on_bigger_than_checkbutton_toggled(GtkToggleButton* btn, gpointer user_data)
{
    FileSearchUI* ui = (FileSearchUI*)user_data;
    gboolean enable = gtk_toggle_button_get_active(btn);
    gtk_widget_set_sensitive(GTK_WIDGET(ui->bigger_spinbutton), enable);
    gtk_widget_set_sensitive(GTK_WIDGET(ui->bigger_unit_combo), enable);
}

static void on_smaller_than_checkbutton_toggled(GtkToggleButton* btn, gpointer user_data)
{
    FileSearchUI* ui = (FileSearchUI*)user_data;
    gboolean enable = gtk_toggle_button_get_active(btn);
    gtk_widget_set_sensitive(GTK_WIDGET(ui->smaller_spinbutton), enable);
    gtk_widget_set_sensitive(GTK_WIDGET(ui->smaller_unit_combo), enable);
}

static gboolean choose_date(GtkButton* btn, FileSearchUI* ui)
{
    const char* label = gtk_button_get_label(btn);
    int res;
    guint month, year, day;

    /* FIXME: we definitely need a better UI design for this part. */

    if(sscanf(label, "%04d-%02d-%02d", &year, &month, &day) == 3)
    {
        gtk_calendar_select_month(ui->calendar, month, year);
        gtk_calendar_select_day(ui->calendar, day);
    }

    res = gtk_dialog_run(ui->date_dlg);
    gtk_widget_hide(GTK_WIDGET(ui->date_dlg));

    if(res == GTK_RESPONSE_OK)
    {
        char str[12];
        gtk_calendar_get_date(ui->calendar, &year, &month, &day);
        ++month; /* month returned from GtkCalendar is 0 based. */
        g_snprintf(str, sizeof(str), "%04d-%02d-%02d", year, month, day);
        gtk_button_set_label(btn, str);
        return TRUE;
    }
    return FALSE;
}

static void on_min_mtime_button_clicked(GtkButton* btn, gpointer user_data)
{
    FileSearchUI* ui = (FileSearchUI*)user_data;
    choose_date(btn, ui);
}

static void on_max_mtime_button_clicked(GtkButton* btn, gpointer user_data)
{
    FileSearchUI* ui = (FileSearchUI*)user_data;
    choose_date(btn, ui);
}

static void on_min_mtime_checkbutton_toggled(GtkToggleButton* btn, gpointer user_data)
{
    FileSearchUI* ui = (FileSearchUI*)user_data;
    gboolean enable = gtk_toggle_button_get_active(btn);
    gtk_widget_set_sensitive(GTK_WIDGET(ui->min_mtime_button), enable);
}

static void on_max_mtime_checkbutton_toggled(GtkToggleButton* btn, gpointer user_data)
{
    FileSearchUI* ui = (FileSearchUI*)user_data;
    gboolean enable = gtk_toggle_button_get_active(btn);
    gtk_widget_set_sensitive(GTK_WIDGET(ui->max_mtime_button), enable);
}

#include "fm-file-search-ui.c" /* file generated with glade-connect-gen */
