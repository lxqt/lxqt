/*
 *      fm-app-chooser-combo-box.h
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


#ifndef __FM_APP_CHOOSER_COMBO_BOX_H__
#define __FM_APP_CHOOSER_COMBO_BOX_H__

#include <gtk/gtk.h>
#include <gio/gio.h>
#include "fm-mime-type.h"

G_BEGIN_DECLS

void fm_app_chooser_combo_box_setup(GtkComboBox* combo, FmMimeType* mime_type, GList* apps, GAppInfo* sel);

/**
 * fm_app_chooser_combo_box_setup_for_mime_type
 * @combo: a #GtkComboBox
 * @mime_type: a #FmMimeType to select application
 *
 * Setups a combobox for selecting default application for specified mime-type.
 * after set up, the combobox will contain a list of available applications for
 * this mime-type, and the default application of the mime-type will get selected.
 *
 * Since: 0.1.5
 */
#define fm_app_chooser_combo_box_setup_for_mime_type(combo, mime_type)  \
    fm_app_chooser_combo_box_setup(combo, mime_type, NULL, NULL)

/**
 * fm_app_chooser_combo_box_setup_custom
 * @combo: a #GtkComboBox
 * @apps: (element-type GAppInfo): custom #GList of applications
 * @sel: (allow-none): a selected application in @apps
 *
 * Setups a combobox for selecting from a list of pre-defined applications.
 * after set up, the combobox will contain a list of available applications the caller
 * provides, and if @sel if found in the list, it will be selected.
 *
 * Since: 0.1.5
 */
#define fm_app_chooser_combo_box_setup_custom(combo, apps, sel) \
    fm_app_chooser_combo_box_setup(combo, NULL, apps, sel)

/* returns the currently selected app. is_sel_changed (can be NULL) will retrive a
 * boolean value which tells you if the currently selected app is different from the one
 * initially selected in the combobox.
 * the returned GAppInfo needs to be freed with g_object_unref() */
GAppInfo* fm_app_chooser_combo_box_dup_selected_app(GtkComboBox* combo, gboolean* is_sel_changed);

/* get a list of custom apps added with app-chooser.
 * the returned GList is owned by the combo box and shouldn't be freed. */
const GList* fm_app_chooser_combo_box_get_custom_apps(GtkComboBox* combo);

G_END_DECLS

#endif /* __FM_APP_CHOOSER_COMBO_BOX_H__ */
