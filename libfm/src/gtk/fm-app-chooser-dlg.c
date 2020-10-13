/*
 *      fm-app-chooser-dlg.c
 *
 *      Copyright 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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
 * SECTION:fm-app-chooser-dlg
 * @short_description: Dialog for application selection.
 * @title: Application chooser dialog
 *
 * @include: libfm/fm-gtk.h
 *
 * The dialog to choose application from tree of known applications.
 * Also allows user to create custom one.
 * The tree itself is represented by fm_app_menu_view_new().
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "fm.h"
#include "fm-app-chooser-dlg.h"
#include "fm-app-menu-view.h"
#include "fm-gtk-utils.h"
#include <menu-cache.h>
#include <gio/gdesktopappinfo.h>

/* support for libmenu-cache 0.4.x */
#ifndef MENU_CACHE_CHECK_VERSION
# ifdef HAVE_MENU_CACHE_DIR_LIST_CHILDREN
#  define MENU_CACHE_CHECK_VERSION(_a,_b,_c) (_a == 0 && _b < 5) /* < 0.5.0 */
# else
#  define MENU_CACHE_CHECK_VERSION(_a,_b,_c) 0 /* not even 0.4.0 */
# endif
#endif

typedef struct _AppChooserData AppChooserData;
struct _AppChooserData
{
    GtkDialog* dlg;
    GtkNotebook* notebook;
    GtkTreeView* apps_view;
    GtkEntry* cmdline;
    GtkToggleButton* set_default;
    GtkToggleButton* use_terminal;
    GtkToggleButton* keep_open;
    GtkEntry* app_name;
    GtkWidget* browse_btn;
    FmMimeType* mime_type;
};

static void on_temp_appinfo_destroy(gpointer data, GObject *objptr)
{
    char *filename = data;

    if(g_unlink(filename) < 0)
        g_critical("failed to remove %s", filename);
    /* else
        g_debug("temp file %s removed", filename); */
    g_free(filename);
}

static GAppInfo* app_info_create_from_commandline(const char *commandline,
                                               const char *application_name,
                                               const char *bin_name,
                                               const char *mime_type,
                                               gboolean terminal, gboolean keep)
{
    GAppInfo* app = NULL;
    char* dirname = g_build_filename (g_get_user_data_dir (), "applications", NULL);
    const char* app_basename = strrchr(bin_name, '/');

    if(app_basename)
        app_basename++;
    else
        app_basename = bin_name;
    if(g_mkdir_with_parents(dirname, 0700) == 0)
    {
        char *filename = NULL;
        int fd;

#if GLIB_CHECK_VERSION(2, 37, 6)
        if (mime_type && application_name[0])
        {
            /* SF bug #871: new GLib has ids cached so we do a trick here:
               we create a dummy app before really creating the file */
            app = g_app_info_create_from_commandline(commandline,
                                                     app_basename,
                                                     0, NULL);
            if (app)
            {
                g_app_info_remove_supports_type(app, mime_type, NULL);
                filename = g_strdup(g_desktop_app_info_get_filename(G_DESKTOP_APP_INFO(app)));
                g_object_unref(app);
                app = NULL;
            }
        }
        if (filename)
            fd = g_open(filename, O_RDWR, 0);
        else
#endif
        {
            filename = g_strdup_printf ("%s/userapp-%s-XXXXXX.desktop", dirname, app_basename);
            fd = g_mkstemp (filename);
        }
        if(fd != -1)
        {
            GString* content = g_string_sized_new(256);
            g_string_printf(content,
                "[" G_KEY_FILE_DESKTOP_GROUP "]\n"
                G_KEY_FILE_DESKTOP_KEY_TYPE "=" G_KEY_FILE_DESKTOP_TYPE_APPLICATION "\n"
                G_KEY_FILE_DESKTOP_KEY_NAME "=%s\n"
                G_KEY_FILE_DESKTOP_KEY_EXEC "=%s\n"
                G_KEY_FILE_DESKTOP_KEY_CATEGORIES "=Other;\n"
                G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY "=true\n",
                application_name,
                commandline
            );
            if(mime_type)
                g_string_append_printf(content,
                                       G_KEY_FILE_DESKTOP_KEY_MIME_TYPE "=%s\n",
                                       mime_type);
            g_string_append_printf(content,
                                   G_KEY_FILE_DESKTOP_KEY_TERMINAL "=%s\n",
                                   terminal ? "true" : "false");
            if(terminal)
                g_string_append_printf(content, "X-KeepTerminal=%s\n",
                                       keep ? "true" : "false");
            close(fd); /* g_file_set_contents() may fail creating duplicate */
            if(g_file_set_contents(filename, content->str, content->len, NULL))
            {
                char *fbname = g_path_get_basename(filename);
                app = G_APP_INFO(g_desktop_app_info_new(fbname));
                g_free(fbname);
                if (app == NULL)
                {
                    g_warning("failed to load %s as an application", filename);
                    g_unlink(filename);
                }
                /* if there is mime_type set then created application will be
                   saved for the mime type (see fm_choose_app_for_mime_type()
                   below) but if not then we should remove this temp. file */
                else if (!mime_type || !application_name[0])
                    /* save the name so this file will be removed later */
                    g_object_weak_ref(G_OBJECT(app), on_temp_appinfo_destroy,
                                      g_strdup(filename));
            }
            else
                g_unlink(filename);
            g_string_free(content, TRUE);
        }
        g_free(filename);
    }
    g_free(dirname);
    return app;
}

static void on_dlg_destroy(AppChooserData* data, GObject* dlg)
{
    if(data->mime_type)
        fm_mime_type_unref(data->mime_type);
    g_slice_free(AppChooserData, data);
}

static void on_switch_page(GtkNotebook* nb, GtkWidget* page, gint num, AppChooserData* data)
{
    if(num == 0) /* list of installed apps */
    {
        gtk_dialog_set_response_sensitive(data->dlg, GTK_RESPONSE_OK,
                        fm_app_menu_view_is_app_selected(data->apps_view));
    }
    else /* custom app */
    {
        const char* cmd = gtk_entry_get_text(data->cmdline);
        gtk_dialog_set_response_sensitive(data->dlg, GTK_RESPONSE_OK, (cmd && cmd[0]));
    }
}

static void on_apps_view_sel_changed(GtkTreeSelection* tree_sel, AppChooserData* data)
{
    if(gtk_notebook_get_current_page(data->notebook) == 0)
    {
        gtk_dialog_set_response_sensitive(data->dlg, GTK_RESPONSE_OK,
                        fm_app_menu_view_is_app_selected(data->apps_view));
    }
}

static void on_cmdline_changed(GtkEditable* cmdline, AppChooserData* data)
{
    if(gtk_notebook_get_current_page(data->notebook) == 1)
    {
        const char* cmd = gtk_entry_get_text(data->cmdline);
        gtk_dialog_set_response_sensitive(data->dlg, GTK_RESPONSE_OK, (cmd && cmd[0]));
    }
}

static gboolean exec_filter_func(const GtkFileFilterInfo *filter_info, gpointer data)
{
    if(g_content_type_can_be_executable(filter_info->mime_type))
        return TRUE;
    return FALSE;
}

static void on_browse_btn_clicked(GtkButton* btn, AppChooserData* data)
{
    FmPath* file;
    GtkFileFilter* filter = gtk_file_filter_new();
    char* binary;
    gtk_file_filter_add_custom(filter,
        GTK_FILE_FILTER_FILENAME|GTK_FILE_FILTER_MIME_TYPE, exec_filter_func, NULL, NULL);
    /* gtk_file_filter_set_name(filter, _("Executable files")); */
    file = fm_select_file(GTK_WINDOW(data->dlg), NULL, "/usr/bin", TRUE, FALSE, filter, NULL);

    if (file == NULL)
        return;
    binary = fm_path_to_str(file);
    if (g_str_has_suffix(fm_path_get_basename(file), ".desktop"))
    {
        GKeyFile *kf = g_key_file_new();
        GDesktopAppInfo *info;
        if (g_key_file_load_from_file(kf, binary, 0, NULL) &&
            (info = g_desktop_app_info_new_from_keyfile(kf)) != NULL)
            /* it is a valid desktop entry */
        {
            /* FIXME: it will duplicate the file, how to avoid that? */
            gtk_entry_set_text(data->cmdline,
                               g_app_info_get_commandline(G_APP_INFO(info)));
            gtk_entry_set_text(data->app_name,
                               g_app_info_get_name(G_APP_INFO(info)));
            gtk_toggle_button_set_active(data->use_terminal,
                                         g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                                                G_KEY_FILE_DESKTOP_KEY_TERMINAL,
                                                                NULL));
            gtk_toggle_button_set_active(data->keep_open,
                                         g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                                                "X-KeepTerminal",
                                                                NULL));
            g_object_unref(info);
            g_key_file_free(kf);
            fm_path_unref(file);
            return;
        }
        g_key_file_free(kf);
    }
    gtk_entry_set_text(data->cmdline, binary);
    g_free(binary);
    fm_path_unref(file);
}

static void on_use_terminal_changed(GtkToggleButton* btn, AppChooserData* data)
{
    if(data->keep_open)
        gtk_widget_set_sensitive(GTK_WIDGET(data->keep_open),
                                 gtk_toggle_button_get_active(btn));
}

/**
 * fm_app_chooser_dlg_new
 * @mime_type: (allow-none): MIME type for list creation
 * @can_set_default: %TRUE if widget can set selected item as default for @mime_type
 *
 * Creates a widget for choosing an application either from tree of
 * existing ones or also allows to set up own command for it.
 *
 * Returns: (transfer full): a widget.
 *
 * Since: 0.1.0
 */
GtkDialog *fm_app_chooser_dlg_new(FmMimeType* mime_type, gboolean can_set_default)
{
    GtkContainer* scroll;
    GtkLabel *file_type, *file_type_header;
    GtkTreeSelection* tree_sel;
    GtkBuilder* builder = gtk_builder_new();
    AppChooserData* data = g_slice_new0(AppChooserData);

    gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);
    gtk_builder_add_from_file(builder, PACKAGE_UI_DIR "/app-chooser.ui", NULL);
    data->dlg = GTK_DIALOG(gtk_builder_get_object(builder, "dlg"));
    data->notebook = GTK_NOTEBOOK(gtk_builder_get_object(builder, "notebook"));
    scroll = GTK_CONTAINER(gtk_builder_get_object(builder, "apps_scroll"));
    file_type = GTK_LABEL(gtk_builder_get_object(builder, "file_type"));
    file_type_header = GTK_LABEL(gtk_builder_get_object(builder, "file_type_header"));
    data->cmdline = GTK_ENTRY(gtk_builder_get_object(builder, "cmdline"));
    data->set_default = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "set_default"));
    data->use_terminal = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "use_terminal"));
    data->keep_open = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "keep_open"));
    data->browse_btn = GTK_WIDGET(gtk_builder_get_object(builder, "browse_btn"));
    data->app_name = GTK_ENTRY(gtk_builder_get_object(builder, "app_name"));
    /* FIXME: shouldn't verify if app-chooser.ui was correct? */
    if(mime_type)
        data->mime_type = fm_mime_type_ref(mime_type);

    gtk_dialog_set_alternative_button_order(data->dlg, GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

    if(!can_set_default)
        gtk_widget_hide(GTK_WIDGET(data->set_default));

    if(mime_type && fm_mime_type_get_desc(mime_type))
    {
        if (file_type_header)
        {
            char *text = g_strdup_printf(_("<b>Select an application to open \"%s\" files</b>"),
                                         fm_mime_type_get_desc(mime_type));
            gtk_label_set_markup(file_type_header, text);
            g_free(text);
        }
        else
            gtk_label_set_text(file_type, fm_mime_type_get_desc(mime_type));
    }
    else
    {
        GtkWidget* hbox = GTK_WIDGET(gtk_builder_get_object(builder, "file_type_hbox"));
        gtk_widget_destroy(hbox);
        gtk_widget_hide(GTK_WIDGET(data->set_default));
    }

    data->apps_view = fm_app_menu_view_new();
    gtk_tree_view_set_headers_visible(data->apps_view, FALSE);
    gtk_widget_show(GTK_WIDGET(data->apps_view));
    gtk_container_add(scroll, GTK_WIDGET(data->apps_view));
    gtk_widget_grab_focus(GTK_WIDGET(data->apps_view));

    g_object_unref(builder);

    g_signal_connect(data->browse_btn, "clicked", G_CALLBACK(on_browse_btn_clicked), data);

    g_object_set_qdata_full(G_OBJECT(data->dlg), fm_qdata_id, data, (GDestroyNotify)on_dlg_destroy);
    g_signal_connect(data->notebook, "switch-page", G_CALLBACK(on_switch_page), data);
    on_switch_page(data->notebook, NULL, 0, data);
    tree_sel = gtk_tree_view_get_selection(data->apps_view);
    g_signal_connect(tree_sel, "changed", G_CALLBACK(on_apps_view_sel_changed), data);
    g_signal_connect(data->cmdline, "changed", G_CALLBACK(on_cmdline_changed), data);
    g_signal_connect(data->use_terminal, "toggled", G_CALLBACK(on_use_terminal_changed), data);
    gtk_dialog_set_response_sensitive(data->dlg, GTK_RESPONSE_OK, FALSE);

    return data->dlg;
}

inline static char* get_binary(const char* cmdline, gboolean* arg_found)
{
    /* see if command line contains %f, %F, %u, or %U. */
    const char* p = strstr(cmdline, " %");
    if(p)
    {
        if( !strchr("fFuU", *(p + 2)) )
            p = NULL;
    }
    if(arg_found)
        *arg_found = (p != NULL);
    if(p)
        return g_strndup(cmdline, p - cmdline);
    else
        return g_strdup(cmdline);
}

/**
 * fm_app_chooser_dlg_dup_selected_app
 * @dlg: a widget
 * @set_default: location to get value that was used for fm_app_chooser_dlg_new()
 *
 * Retrieves a currently selected application from @dlg.
 *
 * Before 1.0.0 this call had name fm_app_chooser_dlg_get_selected_app.
 *
 * Returns: (transfer full): selected application.
 *
 * Since: 0.1.0
 */
GAppInfo* fm_app_chooser_dlg_dup_selected_app(GtkDialog* dlg, gboolean* set_default)
{
    GAppInfo* app = NULL;
    AppChooserData* data = (AppChooserData*)g_object_get_qdata(G_OBJECT(dlg), fm_qdata_id);
    switch( gtk_notebook_get_current_page(data->notebook) )
    {
    case 0: /* all applications */
        app = fm_app_menu_view_dup_selected_app(data->apps_view);
        break;
    case 1: /* custom cmd line */
        {
            const char* cmdline = gtk_entry_get_text(data->cmdline);
            const char* app_name = gtk_entry_get_text(data->app_name);
            if(cmdline && cmdline[0])
            {
                char* _cmdline = NULL;
                gboolean arg_found = FALSE;
                char* bin1 = get_binary(cmdline, &arg_found);
                g_debug("bin1 = %s", bin1);
                /* see if command line contains %f, %F, %u, or %U. */
                if(!arg_found)  /* append %f if no %f, %F, %u, or %U was found. */
                    cmdline = _cmdline = g_strconcat(cmdline, " %f", NULL);

                /* FIXME: is there any better way to do this?
                   this is quite dirty, whole cmdline should be tested instead */
                /* We need to ensure that no duplicated items are added */
                if (app_name && app_name[0] && data->mime_type)
                {
                    MenuCache* menu_cache;
                    /* see if the command is already in the list of known apps for this mime-type */
                    GList* apps = g_app_info_get_all_for_type(fm_mime_type_get_type(data->mime_type));
                    GList* l;
                    for(l=apps;l;l=l->next)
                    {
                        GAppInfo* app2 = G_APP_INFO(l->data);
                        const char* cmd = g_app_info_get_commandline(app2);
                        char* bin2 = get_binary(cmd, NULL);
                        if(g_strcmp0(bin1, bin2) == 0)
                        {
                            app = G_APP_INFO(g_object_ref(app2));
                            g_debug("found in app list");
                            g_free(bin2);
                            break;
                        }
                        g_free(bin2);
                    }
                    g_list_foreach(apps, (GFunc)g_object_unref, NULL);
                    g_list_free(apps);
                    if(app)
                        goto _out;

                    /* see if this command can be found in menu cache */
                    menu_cache = menu_cache_lookup_sync("applications.menu");
                    if(menu_cache)
                    {
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
                        MenuCacheDir *root_dir = menu_cache_dup_root_dir(menu_cache);
                        if(root_dir)
#else
                        if(menu_cache_get_root_dir(menu_cache))
#endif
                        {
                            GSList* all_apps = menu_cache_list_all_apps(menu_cache);
                            GSList* l;
                            for(l=all_apps;l;l=l->next)
                            {
                                MenuCacheApp* ma = MENU_CACHE_APP(l->data);
                                const char *exec = menu_cache_app_get_exec(ma);
                                char* bin2;
                                if (exec == NULL)
                                {
                                    g_warning("application %s has no Exec statement", menu_cache_item_get_id(MENU_CACHE_ITEM(ma)));
                                    continue;
                                }
                                bin2 = get_binary(exec, NULL);
                                if(g_strcmp0(bin1, bin2) == 0)
                                {
                                    app = G_APP_INFO(g_desktop_app_info_new(menu_cache_item_get_id(MENU_CACHE_ITEM(ma))));
                                    g_debug("found in menu cache");
                                    menu_cache_item_unref(MENU_CACHE_ITEM(ma));
                                    g_free(bin2);
                                    break;
                                }
                                menu_cache_item_unref(MENU_CACHE_ITEM(ma));
                                g_free(bin2);
                            }
                            g_slist_free(all_apps);
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
                            menu_cache_item_unref(MENU_CACHE_ITEM(root_dir));
#endif
                        }
                        menu_cache_unref(menu_cache);
                    }
                    if(app)
                        goto _out;
                }

                /* FIXME: g_app_info_create_from_commandline force the use of %f or %u, so this is not we need */
                app = app_info_create_from_commandline(cmdline,
                            app_name ? app_name : "", bin1,
                            data->mime_type ? fm_mime_type_get_type(data->mime_type) : NULL,
                            gtk_toggle_button_get_active(data->use_terminal),
                            data->keep_open && gtk_toggle_button_get_active(data->keep_open));
            _out:
                g_free(bin1);
                g_free(_cmdline);
            }
        }
        break;
    }

    if(set_default)
        *set_default = gtk_toggle_button_get_active(data->set_default);
    return app;
}

/**
 * fm_choose_app_for_mime_type
 * @parent: (allow-none): a parent window
 * @mime_type: (allow-none): MIME type for list creation
 * @can_set_default: %TRUE if widget can set selected item as default for @mime_type
 *
 * Creates a dialog to choose application for @mime_type, lets user to
 * choose then returns the chosen application.
 *
 * If user creates custom application and @mime_type isn't %NULL then this
 * custom application will be added to list of supporting the @mime_type.
 * Otherwise that custom application file will be deleted after usage.
 *
 * Returns: user choise.
 *
 * Since: 0.1.0
 */
GAppInfo* fm_choose_app_for_mime_type(GtkWindow* parent, FmMimeType* mime_type, gboolean can_set_default)
{
    GAppInfo* app = NULL;
    GtkDialog* dlg = fm_app_chooser_dlg_new(mime_type, can_set_default);
    if(parent)
        gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
    if(gtk_dialog_run(dlg) == GTK_RESPONSE_OK)
    {
        gboolean set_default;
        app = fm_app_chooser_dlg_dup_selected_app(dlg, &set_default);

        if(app && mime_type && fm_mime_type_get_type(mime_type) &&
           g_app_info_get_name(app)[0]) /* don't add empty name */
        {
            GError* err = NULL;
            /* add this app to the mime-type */

#if GLIB_CHECK_VERSION(2, 27, 6)
            if(!g_app_info_set_as_last_used_for_type(app,
#else
            if(!g_app_info_add_supports_type(app,
#endif
                                        fm_mime_type_get_type(mime_type), &err))
            {
                g_debug("error: %s", err->message);
                g_error_free(err);
            }
            /* if need to set default */
            if(set_default)
                g_app_info_set_as_default_for_type(app,
                                        fm_mime_type_get_type(mime_type), NULL);
        }
    }
    gtk_widget_destroy(GTK_WIDGET(dlg));
    return app;
}
