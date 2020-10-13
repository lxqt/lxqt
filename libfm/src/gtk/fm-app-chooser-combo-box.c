/*
 *      fm-app-chooser-combo-box.c
 *      
 *      Copyright 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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
 * SECTION:fm-app-chooser-combo-box
 * @short_description: Combo box for application selection dialogs.
 * @title: Application chooser combobox
 *
 * @include: libfm/fm-gtk.h
 *
 * The fm_app_chooser_combo_box_setup() allows to create a widget where
 * applications are represented as a tree to choose from it. The dialog
 * itself is represented by fm_app_chooser_dlg_new().
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include "fm-app-chooser-combo-box.h"
#include "fm-app-chooser-dlg.h"
#include "fm.h"

typedef struct _FmAppChooserComboBoxData FmAppChooserComboBoxData;
struct _FmAppChooserComboBoxData
{
    FmMimeType* mime_type;
    GtkTreeIter initial_sel_iter;
    GtkTreeIter prev_sel_iter;
    GAppInfo* initial_sel_app;
    GtkTreeIter separator_iter;
    GtkTreeIter other_apps_iter;
    GList* custom_apps;
};

static gboolean is_row_separator(GtkTreeModel* model, GtkTreeIter* it, gpointer user_data)
{
    FmAppChooserComboBoxData* data = (FmAppChooserComboBoxData*)user_data;
    /* FIXME: this is dirty but it works! */
    return data->separator_iter.user_data == it->user_data;
}

static void on_app_selected(GtkComboBox* cb, FmAppChooserComboBoxData* data)
{
    GtkTreeIter it;
    GtkTreeModel* model;

    if( !gtk_combo_box_get_active_iter(cb, &it) )
        return;

    model = gtk_combo_box_get_model(cb);
    /* if the user chooses "Customize..." */
    if(it.user_data == data->other_apps_iter.user_data)
    {
        /* let the user choose an app or add custom actions here. */
        GtkWidget* parent = gtk_widget_get_toplevel(GTK_WIDGET(cb));
        GAppInfo* app = fm_choose_app_for_mime_type(GTK_WINDOW(parent), data->mime_type, FALSE);
        if(app)
        {
            gboolean found = FALSE;
            /* see if it's already in the list to prevent duplication */
            if(gtk_tree_model_get_iter_first(model, &it))
            {
                GAppInfo *_app;
                do
                {
                    gtk_tree_model_get(model, &it, 2, &_app, -1);
                    if(_app)
                    {
                        found = g_app_info_equal(app, _app);
                        g_object_unref(_app);
                    }
                    if(found)
                        break;
                }while(gtk_tree_model_iter_next(model, &it));
            }

            /* if it's already in the list, select it */
            if(found)
            {
                gtk_combo_box_set_active_iter(cb, &it);
            }
            else /* if it's not found, add it to the list */
            {
                gtk_list_store_insert_before(GTK_LIST_STORE(model), &it, &data->separator_iter);
                gtk_list_store_set(GTK_LIST_STORE(model), &it,
                                   0, g_app_info_get_icon(app),
                                   1, g_app_info_get_name(app),
                                   2, app, -1);
                data->prev_sel_iter = it;
                gtk_combo_box_set_active_iter(cb, &it);
                /* add to custom apps list */
                data->custom_apps = g_list_prepend(data->custom_apps, g_object_ref(app));
            }
            g_object_unref(app);
        }
        else
        {
            if(!data->prev_sel_iter.user_data)
                gtk_tree_model_get_iter_first(model, &data->prev_sel_iter);
            gtk_combo_box_set_active_iter(cb, &data->prev_sel_iter);
        }
    }
    else /* an application in the list is selected */
        data->prev_sel_iter = it;
}

static void free_data(gpointer user_data)
{
    FmAppChooserComboBoxData* data = (FmAppChooserComboBoxData*)user_data;
    if(data->initial_sel_app)
        g_object_unref(data->initial_sel_app);

    if(data->mime_type)
        fm_mime_type_unref(data->mime_type);

    if(data->custom_apps)
    {
        g_list_foreach(data->custom_apps, (GFunc)g_object_unref, NULL);
        g_list_free(data->custom_apps);
    }
    g_slice_free(FmAppChooserComboBoxData, data);
}

/**
 * fm_app_chooser_combo_box_setup
 * @combo: a #GtkComboBox
 * @mime_type: (allow-none): a #FmMimeType to select application
 * @apps: (allow-none) (element-type GAppInfo): custom list of applications
 * @sel: (allow-none): a selected application in @apps
 *
 * Setups a combobox for selecting default application either for
 * specified mime-type or from a list of pre-defined applications.
 * If @mime_type is %NULL, and @sel is provided and found in the @apps,
 * then it will be selected. If @mime_type is not %NULL then default
 * application for the @mime_type will be selected.
 * When set up, the combobox will contain a list of available applications.
 *
 * Since: 0.1.5
 */
void fm_app_chooser_combo_box_setup(GtkComboBox* combo, FmMimeType* mime_type, GList* apps, GAppInfo* sel)
{
    FmAppChooserComboBoxData* data = g_slice_new0(FmAppChooserComboBoxData);
    GtkListStore* store = gtk_list_store_new(3, G_TYPE_ICON, G_TYPE_STRING, G_TYPE_APP_INFO);
    GtkTreeIter it;
    GList* l;
    GtkCellRenderer* render;

    gtk_cell_layout_clear(GTK_CELL_LAYOUT(combo));

    render = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), render, FALSE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combo), render, "gicon", 0);

    render = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), render, FALSE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combo), render, "text", 1);

    if(mime_type)
    {
        data->mime_type = fm_mime_type_ref(mime_type);
        apps = g_app_info_get_all_for_type(fm_mime_type_get_type(data->mime_type));
        sel =  g_app_info_get_default_for_type(fm_mime_type_get_type(data->mime_type), FALSE);
    }

    for(l = apps; l; l = l->next)
    {
        GAppInfo* app = G_APP_INFO(l->data);
        gtk_list_store_insert_with_values(store, &it, -1,
                           0, g_app_info_get_icon(app),
                           1, g_app_info_get_name(app),
                           2, app, -1);
        if(sel && g_app_info_equal(app, sel))
        {
            /* this is the initially selected app */
            data->initial_sel_iter = it;
            data->initial_sel_app = (GAppInfo*)g_object_ref(app);
        }
    }

    if(mime_type) /* if this list is retrived with g_app_info_get_all_for_type() */
    {
        if(apps)
        {
            g_list_foreach(apps, (GFunc)g_object_unref, NULL);
            g_list_free(apps);
        }
        if(sel)
            g_object_unref(sel);
    }

    gtk_list_store_append(store, &it); /* separator */
    data->separator_iter = it;

    /* other applications */
    gtk_list_store_insert_with_values(store, &it, -1,
                       0, NULL,
                       1, _("Customize"), // FIXME: should be "Customize..."
                       2, NULL, -1);
    data->other_apps_iter = it;
    gtk_combo_box_set_model(combo, GTK_TREE_MODEL(store));

    if(data->initial_sel_iter.user_data) /* intital selection is set */
    {
        data->prev_sel_iter = data->initial_sel_iter;
        gtk_combo_box_set_active_iter(combo, &data->initial_sel_iter);
    }
    gtk_combo_box_set_row_separator_func(combo, is_row_separator, data, NULL);
    g_object_unref(store);

    g_signal_connect(combo, "changed", G_CALLBACK(on_app_selected), data);
    g_object_set_qdata_full(G_OBJECT(combo), fm_qdata_id, data, free_data);
}

/**
 * fm_app_chooser_combo_box_dup_selected_app
 * @combo: a #GtkComboBox
 * @is_sel_changed: (out) (allow-none): location to store %TRUE if selection was changed
 *
 * Retrieves the currently selected app. @is_sel_changed (can be %NULL) will get a
 * boolean value which tells you if the currently selected app is different from the one
 * initially selected in the combobox.
 * The returned #GAppInfo needs to be freed with g_object_unref()
 *
 * Before 1.0.0 this call had name fm_app_chooser_combo_box_get_selected.
 *
 * Returns: selected application.
 *
 * Since: 0.1.5
 */
GAppInfo* fm_app_chooser_combo_box_dup_selected_app(GtkComboBox* combo, gboolean* is_sel_changed)
{
    GtkTreeIter it;
    if(gtk_combo_box_get_active_iter(combo, &it))
    {
        GAppInfo* app;
        GtkTreeModel* model = gtk_combo_box_get_model(combo);
        gtk_tree_model_get(model, &it, 2, &app, -1);
        if(is_sel_changed)
        {
            FmAppChooserComboBoxData* data = (FmAppChooserComboBoxData*)g_object_get_qdata(G_OBJECT(combo), fm_qdata_id);
            *is_sel_changed = (it.user_data != data->initial_sel_iter.user_data);
        }
        return app;
    }
    return NULL;
}

/**
 * fm_app_chooser_combo_box_get_custom_apps
 * @combo: a #GtkComboBox
 *
 * Retrieves a list of custom apps added with app-chooser.
 * The returned #GList is owned by the combo box and shouldn't be freed.
 *
 * Returns: (transfer none) (element-type GAppInfo): list of applications
 *
 * Since: 0.1.5
 */
const GList* fm_app_chooser_combo_box_get_custom_apps(GtkComboBox* combo)
{
    FmAppChooserComboBoxData* data = (FmAppChooserComboBoxData*)g_object_get_qdata(G_OBJECT(combo), fm_qdata_id);
    return data->custom_apps;
}
