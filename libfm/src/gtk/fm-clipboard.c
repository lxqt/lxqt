/*
 *      fm-clipboard.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2012-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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
 * SECTION:fm-clipboard
 * @short_description: Clipboard operations handler for files.
 * @title: Clipboard operations
 *
 * @include: libfm/fm-gtk.h
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gtk-compat.h"
#include "fm-clipboard.h"
#include "fm-gtk-utils.h"

enum {
    URI_LIST = 1,
    GNOME_COPIED_FILES,
    KDE_CUT_SEL,
    UTF8_STRING,
    N_CLIPBOARD_TARGETS
};

static GtkTargetEntry targets[]=
{
    {"text/uri-list", 0, URI_LIST},
    {"x-special/gnome-copied-files", 0, GNOME_COPIED_FILES},
    {"application/x-kde-cutselection", 0, KDE_CUT_SEL}/*,
    { "UTF8_STRING", 0, UTF8_STRING }*/
};

static GdkAtom target_atom[N_CLIPBOARD_TARGETS];

static gboolean got_atoms = FALSE;

static gboolean is_cut = FALSE;

static inline void check_atoms(void)
{
    if(!got_atoms)
    {
        guint i;

        for(i = 0; i < N_CLIPBOARD_TARGETS; i++)
            target_atom[i] = GDK_NONE;
        for(i = 0; i < G_N_ELEMENTS(targets); i++)
            target_atom[targets[i].info] = gdk_atom_intern_static_string(targets[i].target);
        got_atoms = TRUE;
    }
}

static void get_data(GtkClipboard *clip, GtkSelectionData *sel, guint info, gpointer user_data)
{
    FmPathList* files = (FmPathList*)user_data;
    GString* uri_list;
    GList *l;
    GdkAtom target = gtk_selection_data_get_target(sel);

    if(info == KDE_CUT_SEL)
    {
        /* set application/kde-cutselection data */
        if(is_cut)
            gtk_selection_data_set(sel, target, 8, (guchar*)"1", 2);
        return;
    }

    uri_list = g_string_sized_new(4096);
    if(info == GNOME_COPIED_FILES)
    {
        g_string_append(uri_list, is_cut ? "cut\n" : "copy\n");
        fm_path_list_write_uri_list(files, uri_list);
    }
    /* FIXME: this is invalid, UTF8_STRING means this is plain text not file list */
    else if(info == UTF8_STRING)
    {
        for (l = fm_path_list_peek_head_link(files); l; l = l->next)
        {
            FmPath* path = (FmPath*)l->data;
            char* str = fm_path_to_str(path);
            g_string_append(uri_list, str);
            g_string_append_c(uri_list, '\n');
            g_free(str);
        }
    }
    else/* text/uri-list format */
    {
        for (l = fm_path_list_peek_head_link(files); l; l = l->next)
        {
            FmPath *path = (FmPath*)l->data;
            char *str = fm_path_to_uri(path);
            g_string_append(uri_list, str);
            g_free(str);
            if (l->next)
                g_string_append(uri_list, "\r\n");
        }
    }
    gtk_selection_data_set(sel, target, 8, (guchar*)uri_list->str, uri_list->len + 1);
    g_string_free(uri_list, TRUE);
    if(is_cut && info == GNOME_COPIED_FILES)
    {
        /* info about is_cut is already on clipboard so reset it */
        gtk_clipboard_clear(clip);
        is_cut = FALSE;
    }
}

static void clear_data(GtkClipboard* clip, gpointer user_data)
{
    FmPathList* files = (FmPathList*)user_data;
    fm_path_list_unref(files);
    is_cut = FALSE;
}

/**
 * fm_clipboard_cut_or_copy_files
 * @src_widget: widget where files were taken
 * @files: files to place on clipboard
 * @_is_cut: %TRUE if operation is 'Cut', %FALSE if 'Copy'
 *
 * Places files onto system clipboard.
 *
 * Returns: %TRUE if operation was successful.
 *
 * Since: 0.1.0
 */
gboolean fm_clipboard_cut_or_copy_files(GtkWidget* src_widget, FmPathList* files, gboolean _is_cut)
{
    GdkDisplay* dpy = src_widget ? gtk_widget_get_display(src_widget) : gdk_display_get_default();
    GtkClipboard* clip = gtk_clipboard_get_for_display(dpy, GDK_SELECTION_CLIPBOARD);
    gboolean ret;

    ret = gtk_clipboard_set_with_data(clip, targets, G_N_ELEMENTS(targets),
                                      get_data, clear_data, fm_path_list_ref(files));
    is_cut = _is_cut;
    return ret;
}

static gboolean check_kde_curselection(GtkClipboard* clip)
{
    /* Check application/x-kde-cutselection:
     * If the content of this format is string "1", that means the
     * file is cut in KDE (Dolphin). */
    gboolean ret = FALSE;
    GtkSelectionData* data;

    check_atoms();
    data = gtk_clipboard_wait_for_contents(clip, target_atom[KDE_CUT_SEL]);
    if(data)
    {
        gint length, format;
        const gchar* pdata;

        pdata = (const gchar*)gtk_selection_data_get_data_with_length(data, &length);
        format = gtk_selection_data_get_format(data);
        if(length > 0 && format == 8 && pdata[0] == '1')
            ret = TRUE;
        gtk_selection_data_free(data);
    }
    return ret;
}

/**
 * fm_clipboard_paste_files
 * @dest_widget: widget where to paste files
 * @dest_dir: directory to place files
 *
 * Copies or moves files from system clipboard into @dest_dir.
 *
 * Returns: %TRUE if operation was successful.
 *
 * Since: 0.1.0
 */
gboolean fm_clipboard_paste_files(GtkWidget* dest_widget, FmPath* dest_dir)
{
    GdkDisplay* dpy;
    GtkClipboard* clip;
    FmPathList* files;
    char** uris;
    int type = 0;
    GdkAtom *avail_targets;
    int n, i;
    gboolean _is_cut;

    /* safeguard this API call */
    if (dest_dir == NULL)
    {
        g_warning("fm_clipboard_paste_files() for NULL destination");
        return FALSE;
    }

    /* get all available targets currently in the clipboard. */
    dpy = dest_widget ? gtk_widget_get_display(dest_widget) : gdk_display_get_default();
    clip = gtk_clipboard_get_for_display(dpy, GDK_SELECTION_CLIPBOARD);
    if( !gtk_clipboard_wait_for_targets(clip, &avail_targets, &n) )
        return FALSE;

    /* check gnome and xfce compatible format first */
    check_atoms();
    for(i = 0; i < n; ++i)
    {
        if(avail_targets[i] == target_atom[GNOME_COPIED_FILES])
        {
            type = GNOME_COPIED_FILES;
            break;
        }
    }
    if( 0 == type ) /* x-special/gnome-copied-files is not found. */
    {
        /* check uri-list */
        for(i = 0; i < n; ++i)
        {
            if(avail_targets[i] == target_atom[URI_LIST])
            {
                type = URI_LIST;
                break;
            }
        }
        if( 0 == type ) /* text/uri-list is not found. */
        {
            /* finally, fallback to UTF-8 string */
            for(i = 0; i < n; ++i)
            {
                if(avail_targets[i] == target_atom[UTF8_STRING])
                {
                    type = UTF8_STRING;
                    break;
                }
            }
        }
    }
    g_free(avail_targets);

    if( type )
    {
        GtkSelectionData* data;
        const gchar* pdata;
        gint length;

        data = gtk_clipboard_wait_for_contents(clip, target_atom[type]);
        pdata = (const gchar*)gtk_selection_data_get_data_with_length(data, &length);
        _is_cut = FALSE;

        switch(type)
        {
        case GNOME_COPIED_FILES:
            _is_cut = g_str_has_prefix(pdata, "cut\n");
            while(length)
            {
                register gchar ch = *pdata++;
                length--;
                if(ch == '\n')
                    break;
            }
            uris = g_uri_list_extract_uris(pdata);
            break;
        case URI_LIST:
            uris = g_uri_list_extract_uris(pdata);
            _is_cut = check_kde_curselection(clip);
            break;
        case UTF8_STRING:
        default:
            /* FIXME: how should we treat UTF-8 strings? URIs or filenames? */
            /* FIXME: this is invalid, UTF8_STRING means this is plain text
               not file list. We should save text into file instead. */
            uris = g_uri_list_extract_uris(pdata);
            break;
        }
        gtk_selection_data_free(data);

        if(uris)
        {
            GtkWidget* parent;
            if(dest_widget)
                parent = gtk_widget_get_toplevel(dest_widget);
            else
                parent = NULL;

            files = fm_path_list_new_from_uris(uris);
            g_strfreev(uris);

            if(!fm_path_list_is_empty(files))
            {
                if( _is_cut )
                    fm_move_files(GTK_WINDOW(parent), files, dest_dir);
                else
                    fm_copy_files(GTK_WINDOW(parent), files, dest_dir);
            }
            fm_path_list_unref(files);
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * fm_clipboard_have_files
 * @dest_widget: (allow-none): widget to paste files
 *
 * Checks if clipboard have data available for paste.
 *
 * Returns: %TRUE if the clipboard have data that can be handled.
 *
 * Since: 1.0.1
 */
gboolean fm_clipboard_have_files(GtkWidget* dest_widget)
{
    GdkDisplay* dpy = dest_widget ? gtk_widget_get_display(dest_widget) : gdk_display_get_default();
    GtkClipboard* clipboard = gtk_clipboard_get_for_display(dpy, GDK_SELECTION_CLIPBOARD);
    guint i;

    check_atoms();
    for(i = 1; i < N_CLIPBOARD_TARGETS; i++)
        if(target_atom[i] != GDK_NONE
           && gtk_clipboard_wait_is_target_available(clipboard, target_atom[i]))
            return TRUE;
    return FALSE;
}
