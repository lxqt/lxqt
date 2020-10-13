/*
 *      lxshortcut.c
 *
 *      Copyright 2008  <pcman.tw@gmail.com>
 *      Copyright 2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fm-gtk.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

static gboolean no_display = FALSE;
static char* ifile = NULL;
static char* ofile = NULL;

static GOptionEntry opt_entries[] =
{
    {"no-display", 'n', 0, G_OPTION_ARG_NONE, &no_display, NULL, NULL},
    {"input", 'i', 0, G_OPTION_ARG_FILENAME, &ifile, N_("source file name or desktop id"), N_("SOURCE")},
    {"output", 'o', 0, G_OPTION_ARG_FILENAME, &ofile, N_("result file name"), N_("RESULT")},
    { NULL }
};

static gpointer dlg_init(GtkBuilder *ui, gpointer uidata, FmFileInfoList *files)
{
    gpointer widget;

    g_debug("dlg_init");
    /* FIXME: handle NoDisplay */
#define HIDE_WIDGET(x) widget = gtk_builder_get_object(ui, x); \
    if (widget) \
        gtk_widget_hide(widget)
    /* remove permissions tab */
    HIDE_WIDGET("permissions_tab");
    /* remove unwanted texts (times, target, etc.) */
    HIDE_WIDGET("target");
    HIDE_WIDGET("target_label");
    HIDE_WIDGET("total_size");
    HIDE_WIDGET("total_size_label");
    HIDE_WIDGET("size_on_disk");
    HIDE_WIDGET("size_on_disk_label");
    HIDE_WIDGET("mtime");
    HIDE_WIDGET("mtime_label");
    HIDE_WIDGET("atime");
    HIDE_WIDGET("atime_label");
    HIDE_WIDGET("ctime");
    HIDE_WIDGET("ctime_label");
#undef HIDE_WIDGET
    return (gpointer)-1;
}

static void dlg_finish(gpointer data, gboolean cancelled)
{
    /* FIXME: handle NoDisplay */
}

static FmFilePropertiesExtensionInit ext_table = {
    .init = &dlg_init,
    .finish = &dlg_finish
};

int main(int argc, char **argv)
{
    GError *err = NULL;
    FmConfig *config;
    GtkDialog *dlg;
    char *sfile = NULL, *tfile = NULL;
    const char * const *strv;
    char *bl[] = {"*", NULL};
    char *wl[] = {"gtk_file_prop:application/x-desktop", NULL};
    char *contents;
    gsize len;
    FmPath *path;
    FmFileInfo *fi;
    FmFileInfoList *files;

#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    if (argc == 1) {
        /* FIXME: we should just call 'lxshortcut --help' */
        g_print("Error: You need at least one option\n");
        g_print("for help run 'lxshortcut --help\n");
        return 1;
    }

    /* initialize GTK+ and parse the command line arguments */
    if (G_UNLIKELY(!gtk_init_with_args(&argc, &argv, " ", opt_entries, GETTEXT_PACKAGE, &err)))
    {
        g_print("Error: %s\n", err->message);
        return 1;
    }

    if (!ifile && !ofile)
        return 1;

    /* setup LibFM for its default config */
    config = fm_config_new();
    fm_config_load_from_file(config, "/dev/null");
    fm_gtk_init(config);
    /* set modules blacklist and allow only application/x-desktop */
    fm_config->modules_blacklist = g_strdupv(bl);
    fm_config->modules_whitelist = g_strdupv(wl);

    if (ofile)
        tfile = g_strdup(ofile);

    if (ifile)
    {
        // FIXME: this won't work for files without full path
        if (strchr(ifile, '/'))  /* if this is a file path */
        {
            if (!ofile) /* store to the same file if not otherwise specified. */
                tfile = g_strdup(ifile);
            else
                sfile = g_strdup(ifile);
        }
        else /* otherwise treat it as desktop id */
        {
            /* find in g_get_user_data_dir() */
            sfile = g_build_filename(g_get_user_data_dir(), "applications", ifile, NULL);
            if (!ofile)
                tfile = g_strdup(sfile);
            if (!g_file_test(sfile, G_FILE_TEST_IS_REGULAR))
            {
                /* otherwise find in g_get_system_data_dirs() */
                g_free(sfile);
                sfile = NULL;
                for (strv = g_get_system_data_dirs(); *strv; strv++)
                {
                    sfile = g_build_filename(*strv, "applications", ifile, NULL);
                    if (g_file_test(sfile, G_FILE_TEST_IS_REGULAR))
                        break;
                    g_free(sfile);
                    sfile = NULL;
                }
            }
        }
    }

    /* if we got sfile then copy found sfile into tfile */
    if (sfile && g_file_get_contents(sfile, &contents, &len, NULL))
    {
        g_file_set_contents(tfile, contents, len, NULL);
        /* FIXME: check errors */
        g_free(contents);
    }
    else if (!g_file_test(tfile, G_FILE_TEST_EXISTS))
        /* if no sfile is set then create an empty tfile */
        g_file_set_contents(tfile, "[Desktop Entry]\n"
                                   "Type=Application", -1, NULL);
    g_free(sfile);

    /* we have existing target file now with actual contents, let edit it */
    path = fm_path_new_for_path(tfile);
    /* get file info of it */
    fi = fm_file_info_new();
    fm_file_info_set_path(fi, path);
    fm_path_unref(path);
    fm_file_info_set_from_native_file(fi, tfile, &err);
    if (!fm_file_info_is_desktop_entry(fi))
    {
        g_print("Error: file %s is not a desktop entry file\n", tfile);
        g_free(tfile);
        fm_file_info_unref(fi);
        return 1;
    }
    g_free(tfile);
    files = fm_file_info_list_new();
    fm_file_info_list_push_tail_noref(files, fi);

    /* add own extension for it */
    fm_file_properties_add_for_mime_type("application/x-desktop", &ext_table);

    dlg = fm_file_properties_widget_new(files, TRUE);
    fm_file_info_list_unref(files);

    gtk_dialog_run(dlg);

    gtk_widget_destroy(GTK_WIDGET(dlg));

    fm_gtk_finalize();
    return 0;
}
