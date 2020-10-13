/*
 *      gtk-fileprop-x-shortcut.c
 *
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

/* File properties dialog extension for desktop entry shortcut type */

#include <config.h>

#include "fm.h"
#include "fm-file-properties.h"

#include <string.h>

typedef struct _FmFilePropertiesShortcutData FmFilePropertiesShortcutData;
struct _FmFilePropertiesShortcutData
{
    GFile *file;
    GKeyFile *kf;
    GObject *icon;
    GtkEntry *name;
    GtkWidget *hidden;
    gchar *lang;
    gchar *saved_name;
    gboolean was_hidden;
    gboolean changed;
};

static void _shortcut_name_changed(GtkEditable *editable, FmFilePropertiesShortcutData *data)
{
    /* g_debug("entry content changed"); */
    if (data->lang)
        g_key_file_set_locale_string(data->kf, G_KEY_FILE_DESKTOP_GROUP, "Name", data->lang,
                                     gtk_entry_get_text(GTK_ENTRY(editable)));
    else
        g_key_file_set_string(data->kf, G_KEY_FILE_DESKTOP_GROUP, "Name",
                              gtk_entry_get_text(GTK_ENTRY(editable)));
    data->changed = TRUE;
}

static void _shortcut_hidden_toggled(GtkToggleButton *togglebutton,
                                     FmFilePropertiesShortcutData *data)
{
    gboolean active = gtk_toggle_button_get_active(togglebutton);

    g_key_file_set_boolean(data->kf, G_KEY_FILE_DESKTOP_GROUP, "Hidden", active);
    data->changed = TRUE;
}

static gpointer _shortcut_ui_init(GtkBuilder *ui, gpointer uidata, FmFileInfoList *files)
{
    GObject *widget;
    FmFilePropertiesShortcutData *data;
    GtkTable *table;
    GFile *gf;
    gchar *txt;
    gsize length;
    GKeyFile *kf;
    const gchar * const *langs;
    gboolean ok;

    /* test the file if it is really an desktop entry */
    /* we will do the thing only for single file! */
    if (fm_file_info_list_get_length(files) != 1)
        return NULL;
    gf = fm_path_to_gfile(fm_file_info_get_path(fm_file_info_list_peek_head(files)));
    if (!g_file_load_contents(gf, NULL, &txt, &length, NULL, NULL))
    {
        g_warning("file properties dialog: cannot access shortcut file");
        g_object_unref(gf);
        return NULL;
    }
    kf = g_key_file_new();
    ok = g_key_file_load_from_data(kf, txt, length,
                                   G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                                   NULL);
    g_free(txt);
    if (!ok || !g_key_file_has_group(kf, G_KEY_FILE_DESKTOP_GROUP))
        goto _not_the_file;
    txt = g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                G_KEY_FILE_DESKTOP_KEY_TYPE, NULL);
    if (txt == NULL)
        goto _not_the_file;
    ok = (strcmp(txt, G_KEY_FILE_DESKTOP_TYPE_LINK) == 0);
    g_free(txt);
    if (!ok)
    {
_not_the_file:
        g_key_file_free(kf);
        g_object_unref(gf);
        return NULL;
    }
    /* disable open_with in any case */
#define HIDE_WIDGET(x) widget = gtk_builder_get_object(ui, x); \
        gtk_widget_hide(GTK_WIDGET(widget))
    /* HIDE_WIDGET("permissions_tab");
       TODO: made visibility of permissions_tab configurable */
    HIDE_WIDGET("open_with");
    HIDE_WIDGET("open_with_label");
    table = GTK_TABLE(gtk_builder_get_object(ui, "general_table"));
    gtk_table_set_row_spacing(table, 5, 0);
    data = g_slice_new(FmFilePropertiesShortcutData);
    data->changed = FALSE;
    data->file = gf;
    data->kf = kf;
    /* get locale name */
    data->lang = NULL;
    langs = g_get_language_names();
    if (strcmp(langs[0], "C") != 0)
    {
        /* remove encoding from locale name */
        char *sep = strchr(langs[0], '.');
        if (sep)
            data->lang = g_strndup(langs[0], sep - langs[0]);
        else
            data->lang = g_strdup(langs[0]);
    }
    /* enable events for icon */
    widget = gtk_builder_get_object(ui, "icon_eventbox");
    data->icon = gtk_builder_get_object(ui, "icon");
    gtk_widget_set_can_focus(GTK_WIDGET(widget), TRUE);
    /* disable Name event handler in the widget */
    widget = gtk_builder_get_object(ui, "name");
    g_signal_handlers_block_matched(widget, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, uidata);
    g_signal_connect(widget, "changed", G_CALLBACK(_shortcut_name_changed), data);
    data->name = GTK_ENTRY(widget);
    data->saved_name = g_strdup(gtk_entry_get_text(data->name));
    /* make Name field editable */
    gtk_widget_set_can_focus(GTK_WIDGET(widget), TRUE);
    gtk_editable_set_editable(GTK_EDITABLE(widget), TRUE);
    /* Name is set from "Name" by libfm already so don't touch it */
    widget = gtk_builder_get_object(ui, "hidden");
    data->hidden = NULL;
    if (widget && GTK_IS_TOGGLE_BUTTON(widget))
    {
        data->hidden = GTK_WIDGET(widget);
        data->was_hidden = fm_file_info_is_hidden(fm_file_info_list_peek_head(files));
        gtk_widget_set_can_focus(data->hidden, TRUE);
        gtk_widget_set_sensitive(data->hidden, TRUE);
        g_signal_connect(widget, "toggled", G_CALLBACK(_shortcut_hidden_toggled), data);
        gtk_widget_show(data->hidden);
    }
    /* FIXME: make Target changeable - external little window probably? */
#undef HIDE_WIDGET
    return data;
}

static void _shortcut_ui_finish(gpointer pdata, gboolean cancelled)
{
    FmFilePropertiesShortcutData *data = pdata;
    gsize len;
    char *text;

    if (data == NULL)
        return;
    if (!cancelled)
    {
        text = g_object_get_qdata(data->icon, fm_qdata_id);
        if (text)
        {
            g_key_file_set_string(data->kf, G_KEY_FILE_DESKTOP_GROUP, "Icon", text);
            /* disable default handler for icon change since we'll do it below */
            g_object_set_qdata(data->icon, fm_qdata_id, NULL);
            data->changed = TRUE;
        }
        if (data->changed)
        {
            text = g_key_file_to_data(data->kf, &len, NULL);
            g_file_replace_contents(data->file, text, len, NULL, FALSE, 0, NULL,
                                    NULL, NULL);
            /* FIXME: handle errors */
            g_free(text);
        }
    }
    g_object_unref(data->file);
    g_key_file_free(data->kf);
    /* disable own handler on data->name */
    g_signal_handlers_disconnect_by_func(data->name, _shortcut_name_changed, data);
    /* restore the field so properties dialog will not do own processing */
    gtk_entry_set_text(data->name, data->saved_name);
    if (data->hidden)
    {
        /* disable own handler on data->hidden */
        g_signal_handlers_disconnect_by_func(data->hidden,
                                             _shortcut_hidden_toggled, data);
        /* disable default handler returning previous value */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->hidden),
                                     data->was_hidden);
    }
    g_free(data->saved_name);
    g_free(data->lang);
    g_slice_free(FmFilePropertiesShortcutData, data);
}

FM_DEFINE_MODULE(gtk_file_prop, inode/x-shortcut)

FmFilePropertiesExtensionInit fm_module_init_gtk_file_prop = {
    .init = &_shortcut_ui_init,
    .finish = &_shortcut_ui_finish
};
