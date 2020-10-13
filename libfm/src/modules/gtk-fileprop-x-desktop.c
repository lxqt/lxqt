/*
 *      fm-dentry-properties.c
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

/* File properties dialog extension for desktop entry type */

#include <config.h>
#include <glib/gi18n-lib.h>

#include "fm.h"
#include "fm-file-properties.h"

#define GRP_NAME "Desktop Entry"

typedef struct _FmFilePropertiesDEntryData FmFilePropertiesDEntryData;
struct _FmFilePropertiesDEntryData
{
    GFile *file;
    GKeyFile *kf;
    GObject *icon;
    GtkEntry *name;
    GtkEntry *comment;
    GtkEntry *exec;
    GtkEntry *generic_name;
    GtkEntry *path;
    GtkToggleButton *hidden;
    GtkToggleButton *terminal;
    GtkToggleButton *keep_open;
    GtkToggleButton *notification;
    gchar *lang;
    gchar *saved_name;
    gboolean was_hidden;
    gboolean changed;
};

static gboolean exe_filter(const GtkFileFilterInfo *inf, gpointer user_data)
{
    return g_file_test(inf->filename, G_FILE_TEST_IS_EXECUTABLE);
}

static void _dentry_browse_exec_event(GtkButton *button, FmFilePropertiesDEntryData *data)
{
    /* g_debug("browse button pressed"); */
    /* this handler is also taken from lxshortcut */
    GtkWidget *chooser;
    GtkFileFilter *filter;

    chooser = gtk_file_chooser_dialog_new(_("Choose Executable File"),
                                          NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), "/usr/bin");
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(GTK_FILE_FILTER(filter), _("Executable files") );
    gtk_file_filter_add_custom(GTK_FILE_FILTER(filter), GTK_FILE_FILTER_FILENAME,
                               exe_filter, NULL, NULL);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_OK)
    {
        char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        gtk_entry_set_text(data->exec, file);
        g_free(file);
    }
    gtk_widget_destroy(chooser);
}

static void _dentry_name_changed(GtkEditable *editable, FmFilePropertiesDEntryData *data)
{
    /* g_debug("entry content changed"); */
    if (data->lang)
        g_key_file_set_locale_string(data->kf, GRP_NAME, "Name", data->lang,
                                     gtk_entry_get_text(GTK_ENTRY(editable)));
    else
        g_key_file_set_string(data->kf, GRP_NAME, "Name",
                              gtk_entry_get_text(GTK_ENTRY(editable)));
    data->changed = TRUE;
}

static void _dentry_tooltip_changed(GtkEditable *editable, FmFilePropertiesDEntryData *data)
{
    /* g_debug("entry content changed"); */
    if (data->lang)
        g_key_file_set_locale_string(data->kf, GRP_NAME, "Comment", data->lang,
                                     gtk_entry_get_text(GTK_ENTRY(editable)));
    else
        g_key_file_set_string(data->kf, GRP_NAME, "Comment",
                              gtk_entry_get_text(GTK_ENTRY(editable)));
    data->changed = TRUE;
}

static void _dentry_exec_changed(GtkEditable *editable, FmFilePropertiesDEntryData *data)
{
    /* g_debug("entry content changed"); */
    g_key_file_set_string(data->kf, GRP_NAME, "Exec",
                          gtk_entry_get_text(GTK_ENTRY(editable)));
    data->changed = TRUE;
}

static void _dentry_genname_changed(GtkEditable *editable, FmFilePropertiesDEntryData *data)
{
    /* g_debug("entry content changed"); */
    g_key_file_set_string(data->kf, GRP_NAME, "GenericName",
                          gtk_entry_get_text(GTK_ENTRY(editable)));
    data->changed = TRUE;
}

static void _dentry_path_changed(GtkEditable *editable, FmFilePropertiesDEntryData *data)
{
    /* g_debug("entry content changed"); */
    g_key_file_set_string(data->kf, GRP_NAME, "Path",
                          gtk_entry_get_text(GTK_ENTRY(editable)));
    data->changed = TRUE;
}

static void _dentry_terminal_toggled(GtkToggleButton *togglebutton,
                                     FmFilePropertiesDEntryData *data)
{
    gboolean active = gtk_toggle_button_get_active(togglebutton);

    /* g_debug("run in terminal toggled"); */
    g_key_file_set_boolean(data->kf, GRP_NAME, "Terminal", active);
    gtk_widget_set_sensitive(GTK_WIDGET(data->keep_open), active);
    if (!active) /* it is not reasonable to keep the key if no Terminal is set */
        g_key_file_remove_key(data->kf, GRP_NAME, "X-KeepTerminal", NULL);
    data->changed = TRUE;
}

static void _dentry_keepterm_toggled(GtkToggleButton *togglebutton,
                                     FmFilePropertiesDEntryData *data)
{
    /* g_debug("keep terminal open toggled"); */
    g_key_file_set_boolean(data->kf, GRP_NAME, "X-KeepTerminal",
                           gtk_toggle_button_get_active(togglebutton));
    data->changed = TRUE;
}

static void _dentry_notification_toggled(GtkToggleButton *togglebutton,
                                         FmFilePropertiesDEntryData *data)
{
    /* g_debug("startup notification toggled"); */
    g_key_file_set_boolean(data->kf, GRP_NAME, "StartupNotify",
                           gtk_toggle_button_get_active(togglebutton));
    data->changed = TRUE;
}

static void _dentry_hidden_toggled(GtkToggleButton *togglebutton,
                                   FmFilePropertiesDEntryData *data)
{
    /* g_debug("no display toggled"); */
    g_key_file_set_boolean(data->kf, GRP_NAME, "Hidden",
                           gtk_toggle_button_get_active(togglebutton));
    data->changed = TRUE;
}

static gpointer _dentry_ui_init(GtkBuilder *ui, gpointer uidata, FmFileInfoList *files)
{
    GObject *widget;
    GtkWidget *new_widget;
    FmFilePropertiesDEntryData *data;
    GtkTable *table;
    GtkLabel *label;
    GError *err = NULL;
    FmFileInfo *fi;
    GFile *gf;
    gchar *txt;
    gsize length;
    const gchar * const *langs;
    gboolean tmp_bool;

    /* disable permissions tab and open_with in any case */
#define HIDE_WIDGET(x) widget = gtk_builder_get_object(ui, x); \
        gtk_widget_hide(GTK_WIDGET(widget))
    /* HIDE_WIDGET("permissions_tab");
       TODO: made visibility of permissions_tab configurable */
    table = GTK_TABLE(gtk_builder_get_object(ui, "general_table"));
    HIDE_WIDGET("open_with");
    HIDE_WIDGET("open_with_label");
    gtk_table_set_row_spacing(table, 5, 0);
    /* we will do the thing only for single file! */
    if (fm_file_info_list_get_length(files) != 1)
        return NULL;
    fi = fm_file_info_list_peek_head(files);
    gf = fm_path_to_gfile(fm_file_info_get_path(fi));
    if (!g_file_load_contents(gf, NULL, &txt, &length, NULL, NULL))
    {
        g_warning("file properties dialog: cannot access desktop entry file");
        g_object_unref(gf);
        return NULL;
    }
    data = g_slice_new(FmFilePropertiesDEntryData);
    data->changed = FALSE;
    data->file = gf;
    data->kf = g_key_file_new();
    g_key_file_load_from_data(data->kf, txt, length,
                              G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                              NULL);
    g_free(txt);
    /* FIXME: handle errors, also do g_key_file_has_group() */
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
    g_signal_connect(widget, "changed", G_CALLBACK(_dentry_name_changed), data);
    data->name = GTK_ENTRY(widget);
    data->saved_name = g_strdup(gtk_entry_get_text(data->name));
    /* FIXME: two lines below is temporary workaround on FIXME in widget */
    gtk_widget_set_can_focus(GTK_WIDGET(widget), TRUE);
    gtk_editable_set_editable(GTK_EDITABLE(widget), TRUE);
    /* Name is set from "Name" by libfm already so don't touch it */
    /* support 'hidden' option */
    data->hidden = NULL;
    widget = gtk_builder_get_object(ui, "hidden");
    if (widget && GTK_IS_TOGGLE_BUTTON(widget) && fm_file_info_is_native(fi))
    {
        data->hidden = (GtkToggleButton*)widget;
        data->was_hidden = fm_file_info_is_hidden(fi);
        g_signal_connect(widget, "toggled", G_CALLBACK(_dentry_hidden_toggled), data);
        gtk_widget_set_can_focus(GTK_WIDGET(data->hidden), TRUE);
        /* set sensitive since it can be toggled for desktop entry */
        gtk_widget_set_sensitive(GTK_WIDGET(widget), TRUE);
        gtk_widget_show(GTK_WIDGET(data->hidden));
    }
#undef HIDE_WIDGET
    /* FIXME: migrate to GtkGrid */
    table = GTK_TABLE(gtk_table_new(8, 2, FALSE));
    gtk_table_set_row_spacings(table, 4);
    gtk_table_set_col_spacings(table, 12);
    gtk_container_set_border_width(GTK_CONTAINER(table), 4);
    /* row 0: "Exec" GtkHBox: GtkEntry+GtkButton */
    new_widget = gtk_label_new(NULL);
    label = GTK_LABEL(new_widget);
    gtk_misc_set_alignment(GTK_MISC(new_widget), 0.0, 0.0);
    gtk_label_set_markup_with_mnemonic(label, _("<b>Co_mmand:</b>"));
    gtk_table_attach(table, new_widget, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
#if GTK_CHECK_VERSION(3, 2, 0)
    /* FIXME: migrate to GtkGrid */
    widget = G_OBJECT(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6));
#else
    widget = G_OBJECT(gtk_hbox_new(FALSE, 6));
#endif
    new_widget = gtk_button_new_with_mnemonic(_("_Browse..."));
    gtk_box_pack_end(GTK_BOX(widget), new_widget, FALSE, FALSE, 0);
    g_signal_connect(new_widget, "clicked",
                     G_CALLBACK(_dentry_browse_exec_event), data);
    new_widget = gtk_entry_new();
    data->exec = GTK_ENTRY(new_widget);
    txt = g_key_file_get_locale_string(data->kf, GRP_NAME, "Exec", NULL, NULL);
    if (txt)
    {
        gtk_entry_set_text(data->exec, txt);
        g_free(txt);
    }
    gtk_widget_set_tooltip_text(new_widget,
                                _("Command to execute when the application icon is activated"));
    gtk_box_pack_start(GTK_BOX(widget), new_widget, TRUE, TRUE, 0);
    g_signal_connect(new_widget, "changed", G_CALLBACK(_dentry_exec_changed), data);
    gtk_table_attach(table, GTK_WIDGET(widget), 1, 2, 0, 1, GTK_FILL|GTK_EXPAND, 0, 0, 0);
    gtk_label_set_mnemonic_widget(label, new_widget);
    /* row 1: "Terminal" GtkCheckButton */
    new_widget = gtk_check_button_new_with_mnemonic(_("_Execute in terminal emulator"));
    data->terminal = GTK_TOGGLE_BUTTON(new_widget);
    tmp_bool = g_key_file_get_boolean(data->kf, GRP_NAME, "Terminal", &err);
    if (err) /* no such key present */
    {
        tmp_bool = FALSE;
        g_clear_error(&err);
    }
    gtk_toggle_button_set_active(data->terminal, tmp_bool);
    g_signal_connect(new_widget, "toggled", G_CALLBACK(_dentry_terminal_toggled), data);
    gtk_table_attach(table, new_widget, 0, 2, 1, 2, GTK_FILL, 0, 18, 0);
    /* row 2: "X-KeepTerminal" GtkCheckButton */
    new_widget = gtk_check_button_new_with_mnemonic(_("_Keep terminal window open after command execution"));
    data->keep_open = GTK_TOGGLE_BUTTON(new_widget);
    gtk_widget_set_sensitive(new_widget, tmp_bool); /* disable if not in terminal */
    tmp_bool = g_key_file_get_boolean(data->kf, GRP_NAME, "X-KeepTerminal", &err);
    if (err) /* no such key present */
    {
        tmp_bool = FALSE;
        g_clear_error(&err);
    }
    gtk_toggle_button_set_active(data->keep_open, tmp_bool);
    g_signal_connect(new_widget, "toggled", G_CALLBACK(_dentry_keepterm_toggled), data);
    gtk_table_attach(table, new_widget, 0, 2, 2, 3, GTK_FILL, 0, 27, 0);
    /* row 4: "GenericName" GtkEntry */
    new_widget = gtk_label_new(NULL);
    label = GTK_LABEL(new_widget);
    gtk_misc_set_alignment(GTK_MISC(new_widget), 0.0, 0.0);
    gtk_label_set_markup_with_mnemonic(label, _("<b>D_escription:</b>"));
    gtk_table_attach(table, new_widget, 0, 1, 4, 5, GTK_FILL, 0, 0, 0);
    new_widget = gtk_entry_new();
    data->generic_name = GTK_ENTRY(new_widget);
    txt = g_key_file_get_locale_string(data->kf, GRP_NAME, "GenericName", NULL, NULL);
    if (txt)
    {
        gtk_entry_set_text(data->generic_name, txt);
        g_free(txt);
    }
    gtk_widget_set_tooltip_text(new_widget, _("Generic name of the application"));
    g_signal_connect(new_widget, "changed", G_CALLBACK(_dentry_genname_changed), data);
    gtk_table_attach(table, new_widget, 1, 2, 4, 5, GTK_FILL|GTK_EXPAND, 0, 0, 0);
    gtk_label_set_mnemonic_widget(label, new_widget);
    /* row 3: "Path" GtkEntry */
    new_widget = gtk_label_new(NULL);
    label = GTK_LABEL(new_widget);
    gtk_misc_set_alignment(GTK_MISC(new_widget), 0.0, 0.0);
    gtk_label_set_markup_with_mnemonic(label, _("<b>_Working directory:</b>"));
    gtk_table_attach(table, new_widget, 0, 1, 3, 4, GTK_FILL, 0, 0, 0);
    new_widget = gtk_entry_new();
    data->path = GTK_ENTRY(new_widget);
    txt = g_key_file_get_locale_string(data->kf, GRP_NAME, "Path", NULL, NULL);
    if (txt)
    {
        gtk_entry_set_text(data->path, txt);
        g_free(txt);
    }
    gtk_widget_set_tooltip_text(new_widget,
                                _("The working directory to run the program in"));
    g_signal_connect(new_widget, "changed", G_CALLBACK(_dentry_path_changed), data);
    gtk_table_attach(table, new_widget, 1, 2, 3, 4, GTK_FILL|GTK_EXPAND, 0, 0, 0);
    gtk_label_set_mnemonic_widget(label, new_widget);
    /* row 5: "Comment" GtkEntry */
    new_widget = gtk_label_new(NULL);
    label = GTK_LABEL(new_widget);
    gtk_misc_set_alignment(GTK_MISC(new_widget), 0.0, 0.0);
    gtk_label_set_markup_with_mnemonic(label, _("<b>_Tooltip:</b>"));
    gtk_table_attach(table, new_widget, 0, 1, 5, 6, GTK_FILL, 0, 0, 0);
    new_widget = gtk_entry_new();
    data->comment = GTK_ENTRY(new_widget);
    txt = g_key_file_get_locale_string(data->kf, GRP_NAME, "Comment", NULL, NULL);
    if (txt)
    {
        gtk_entry_set_text(data->comment, txt);
        g_free(txt);
    }
    gtk_widget_set_tooltip_text(new_widget, _("Tooltip to show on application"));
    g_signal_connect(new_widget, "changed", G_CALLBACK(_dentry_tooltip_changed), data);
    gtk_table_attach(table, new_widget, 1, 2, 5, 6, GTK_FILL|GTK_EXPAND, 0, 0, 0);
    gtk_label_set_mnemonic_widget(label, new_widget);
    /* TODO: handle "TryExec" field ? */
    /* row 7: "StartupNotify" GtkCheckButton */
    new_widget = gtk_check_button_new_with_mnemonic(_("_Use startup notification"));
    data->notification = GTK_TOGGLE_BUTTON(new_widget);
    tmp_bool = g_key_file_get_boolean(data->kf, GRP_NAME, "StartupNotify", &err);
    if (err) /* no such key present */
    {
        tmp_bool = FALSE;
        g_clear_error(&err);
    }
    gtk_toggle_button_set_active(data->notification, tmp_bool);
    g_signal_connect(new_widget, "toggled", G_CALLBACK(_dentry_notification_toggled), data);
    gtk_table_attach(table, new_widget, 0, 2, 7, 8, GTK_FILL, 0, 0, 0);
    /* put the table into third tab and enable it */
    widget = gtk_builder_get_object(ui, "extra_tab_label");
    gtk_label_set_markup_with_mnemonic(GTK_LABEL(widget), _("_Desktop Entry"));
    widget = gtk_builder_get_object(ui, "extra_tab");
    gtk_container_add(GTK_CONTAINER(widget), GTK_WIDGET(table));
    gtk_widget_show_all(GTK_WIDGET(widget));
    return data;
}

static void _dentry_ui_finish(gpointer pdata, gboolean cancelled)
{
    FmFilePropertiesDEntryData *data = pdata;
    gsize len;
    char *text;

    if (data == NULL)
        return;
    if (!cancelled)
    {
        text = g_object_get_qdata(data->icon, fm_qdata_id);
        if (text)
        {
            g_key_file_set_string(data->kf, GRP_NAME, "Icon", text);
            /* disable default handler for icon change since we'll do it below */
            g_object_set_qdata(data->icon, fm_qdata_id, NULL);
            data->changed = TRUE;
        }
    }
    if (!cancelled && data->changed)
    {
        text = g_key_file_to_data(data->kf, &len, NULL);
        g_file_replace_contents(data->file, text, len, NULL, FALSE, 0, NULL,
                                NULL, NULL);
        /* FIXME: handle errors */
        g_free(text);
    }
    g_object_unref(data->file);
    g_key_file_free(data->kf);
    /* disable own handler on data->name */
    g_signal_handlers_disconnect_by_func(data->name, _dentry_name_changed, data);
    /* restore the field so properties dialog will not do own processing */
    gtk_entry_set_text(data->name, data->saved_name);
    if (data->hidden)
    {
        /* disable own handler on data->hidden */
        g_signal_handlers_disconnect_by_func(data->hidden,
                                             _dentry_hidden_toggled, data);
        /* disable default handler returning previous value */
        gtk_toggle_button_set_active(data->hidden, data->was_hidden);
    }
    g_free(data->saved_name);
    g_free(data->lang);
    g_slice_free(FmFilePropertiesDEntryData, data);
}

FM_DEFINE_MODULE(gtk_file_prop, application/x-desktop)

FmFilePropertiesExtensionInit fm_module_init_gtk_file_prop = {
    &_dentry_ui_init,
    &_dentry_ui_finish
};
