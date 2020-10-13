/*
 *      libfm-prefapps.c
 *
 *      Copyright 2010 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "fm-gtk.h"
#include "../glib-compat.h"

static GtkDialog* dlg;
static GtkComboBox* browser;
static GtkComboBox* mail_client;

int main(int argc, char** argv)
{
    GtkBuilder* b;
    FmMimeType *mt;

#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain( GETTEXT_PACKAGE );
#endif

    gtk_init(&argc, &argv);
    fm_gtk_init(NULL);

#if GTK_CHECK_VERSION(3, 10, 0)
    b = gtk_builder_new_from_file(PACKAGE_UI_DIR "/preferred-apps.ui");
#else
    b = gtk_builder_new();
    gtk_builder_add_from_file(b, PACKAGE_UI_DIR "/preferred-apps.ui", NULL);
#endif
    dlg = GTK_DIALOG(gtk_builder_get_object(b, "dlg"));
    browser = GTK_COMBO_BOX(gtk_builder_get_object(b, "browser"));
    mail_client = GTK_COMBO_BOX(gtk_builder_get_object(b, "mail_client"));
    g_object_unref(b);

    /* Set icon name for main (dlg) window so it displays in the panel. LP #737274 */
    gtk_window_set_icon_name(GTK_WINDOW(dlg), "preferences-desktop");

    /* make sure we're using menu from lxmenu-data */
    g_setenv("XDG_MENU_PREFIX", "lxde-", TRUE);

    mt = fm_mime_type_from_name("x-scheme-handler/http");
    fm_app_chooser_combo_box_setup_for_mime_type(browser, mt);
    fm_mime_type_unref(mt);
    mt = fm_mime_type_from_name("x-scheme-handler/mailto");
    fm_app_chooser_combo_box_setup_for_mime_type(mail_client, mt);
    fm_mime_type_unref(mt);

    if(gtk_dialog_run(dlg) == GTK_RESPONSE_OK)
    {
        gboolean is_changed;
        GAppInfo* app;
        const GList* custom_apps, *l;

        /* get currently selected web browser */
        app = fm_app_chooser_combo_box_dup_selected_app(browser, &is_changed);
        if(app)
        {
            if(is_changed)
            {
                g_app_info_set_as_default_for_type(app, "x-scheme-handler/http", NULL);
                g_app_info_set_as_default_for_type(app, "x-scheme-handler/https", NULL);
            }
            g_object_unref(app);
        }
        custom_apps = fm_app_chooser_combo_box_get_custom_apps(browser);
        if(custom_apps)
        {
            for(l=custom_apps;l;l=l->next)
            {
                app = G_APP_INFO(l->data);
                /* x-scheme-handler/http is set by chooser already */
#if GLIB_CHECK_VERSION(2, 27, 6)
                g_app_info_set_as_last_used_for_type(app, "x-scheme-handler/https", NULL);
#else
                g_app_info_add_supports_type(app, "x-scheme-handler/https", NULL);
#endif
            }
            /* custom_apps is owned by the combobox and shouldn't be freed. */
        }

        /* get selected mail client */
        app = fm_app_chooser_combo_box_dup_selected_app(mail_client, &is_changed);
        if(app)
        {
            if(is_changed)
                g_app_info_set_as_default_for_type(app, "x-scheme-handler/mailto", NULL);
            g_object_unref(app);
        }
    }
    gtk_widget_destroy((GtkWidget*)dlg);

    fm_gtk_finalize();

    return 0;
}
