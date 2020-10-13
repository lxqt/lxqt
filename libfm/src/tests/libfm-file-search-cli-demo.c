/*
 * libfm-file-search-cli-demo.c
 * 
 * Copyright 2010 Shae Smittle <starfall87@gmail.com>
 * Copyright 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include <stdio.h>
#include <gio/gio.h>
#include <glib.h>

#include "fm.h"

#include <string.h>

static char * target = NULL;
static char * target_contains = NULL;
static char * path_list = NULL;
static char * target_type = NULL;
static gboolean not_recursive = FALSE;
static gboolean show_hidden = FALSE;
static gboolean regex_target = FALSE;
static gboolean regex_content = FALSE;
static gboolean exact_target = FALSE;
static gboolean exact_content = FALSE;
static gboolean case_insensitive_target = FALSE;
static gboolean case_insensitive_content = FALSE;
static gint64 min_size = -1;
static gint64 max_size = -1;

static GOptionEntry entries[] =
{
    {"target", 't', 0, G_OPTION_ARG_STRING, &target, "phrase to search for in file names", NULL },
    {"contains", 'c', 0, G_OPTION_ARG_STRING, &target_contains, "phrase to search for in file contents", NULL},
    {"paths", 'p', 0, G_OPTION_ARG_STRING, &path_list, "paths to search through i.e. /usr/share/local:/usr/share", NULL},
    {"type", 'y', 0, G_OPTION_ARG_STRING, &target_type, "system string representation of type of files to search", NULL},
    {"norecurse", 'r', 0, G_OPTION_ARG_NONE, &not_recursive, "disables recursively searching directories", NULL},
    {"showhidden", 's', 0, G_OPTION_ARG_NONE, &show_hidden, "enables searching hidden files", NULL},
    {"regextarget", 'e', 0, G_OPTION_ARG_NONE, &regex_target, "enables regex target searching", NULL},
    {"regexcontent", 'g', 0, G_OPTION_ARG_NONE, &regex_content, "enables regex target searching", NULL},
    {"exacttarget", 'x', 0, G_OPTION_ARG_NONE, &exact_target, "enables regex target searching", NULL},
    {"exactcontent", 'a', 0, G_OPTION_ARG_NONE, &exact_content, "enables regex target searching", NULL},
    {"name-ci", 'n', 0, G_OPTION_ARG_NONE, &case_insensitive_target, "enables case insensitive target searching", NULL},
    {"content-ci", 'i', 0, G_OPTION_ARG_NONE, &case_insensitive_content, "enables case insensitive content searching", NULL},
    {"min-size", 'u', 0,G_OPTION_ARG_INT64, &min_size, "minimum size of file that is a match", NULL},
    {"max-size", 'w', 0, G_OPTION_ARG_INT64, &max_size, "maximum size of file taht is a match", NULL},
    {NULL}
};

static void on_files_added(FmFolder* folder, GSList* files, gpointer user_data)
{
    GSList* l;
    for(l = files; l; l = l->next)
    {
        FmFileInfo* file = FM_FILE_INFO(l->data);
        FmPath* path = fm_file_info_get_path(file);
        char* path_str = fm_path_display_name(path, FALSE);
        g_printf("file found: %s\n", path_str);
    }
}

#if 0
static gboolean on_timeout(gpointer user_data)
{
    g_printf("timeout!\n");
    gtk_main_quit();
    return FALSE;
}
#endif

static GMainLoop *loop;

static void on_finish_loading(FmFolder* folder)
{
    g_printf("finished\n");
    g_main_loop_quit(loop);
}

int main(int argc, char** argv)
{
    GOptionContext * context;

#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init();
#endif
    fm_init(NULL);

    context = g_option_context_new(" - test for libfm file search");
    g_option_context_add_main_entries(context, entries, NULL);
    if(g_option_context_parse(context, &argc, &argv, NULL) == FALSE)
		return 1;

    GString* search_uri = g_string_sized_new(1024);
    g_string_append(search_uri, "search:/");
    g_string_append(search_uri, path_list);

    g_string_append_c(search_uri, '?');

    g_string_append_printf(search_uri, "recursive=%d", not_recursive ? 0 : 1);
    g_string_append_printf(search_uri, "&show_hidden=%d", show_hidden ? 1 : 0);

    if(target)
    {
        g_string_append_printf(search_uri, "&name=%s", target);
        if(case_insensitive_target)
            g_string_append_printf(search_uri, "&name_ci=%d", case_insensitive_target ? 0 : 1);
    }

    if(target_contains)
    {
        if(regex_content)
            g_string_append_printf(search_uri, "&content_regex=%s", target_contains);
        else
            g_string_append_printf(search_uri, "&content=%s", target_contains);

        if(case_insensitive_content)
            g_string_append_printf(search_uri, "&content_ci=%d", case_insensitive_target ? 0 : 1);
    }

    if(target_type)
        g_string_append_printf(search_uri, "&types=%s", target_type);

    if(min_size > 0)
        g_string_append_printf(search_uri, "&min_size=%llu", (long long unsigned int)min_size);

    if(max_size > 0)
        g_string_append_printf(search_uri, "&max_size=%llu", (long long unsigned int)max_size);

    // g_string_append(search_uri, "search://usr/share?recursive=1&name=*.mo&name_mode=widecard&show_hidden=0&name_case_sensitive=1");

    g_print("URI: %s\n", search_uri->str);

    FmFolder* folder = fm_folder_from_uri(search_uri->str);
    g_string_free(search_uri, TRUE);
    g_signal_connect(folder, "files-added", G_CALLBACK(on_files_added), NULL);
    g_signal_connect(folder, "finish-loading", G_CALLBACK(on_finish_loading), NULL);

    /* g_timeout_add_seconds(30, on_timeout, NULL); */

    loop = g_main_loop_new(NULL, TRUE);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    g_object_unref(folder);

    fm_finalize();
    return 0;
}
