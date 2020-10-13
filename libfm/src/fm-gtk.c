/*
 *      fm-gtk.c
 *      
 *      Copyright 2009 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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
 * SECTION:fm-gtk
 * @short_description: Libfm-gtk initialization
 * @title: Libfm-Gtk
 *
 * @include: libfm/fm-gtk.h
 *
 */

#include "fm-gtk.h"

static volatile gint gtk_initialized = 0;

/**
 * fm_gtk_init
 * @config: configuration file data
 *
 * Initializes libfm-gtk data. This API should be always called before any
 * other libfm-gtk function is called. It is idempotent.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 0.1.0
 */
gboolean fm_gtk_init(FmConfig* config)
{
#if GLIB_CHECK_VERSION(2, 30, 0)
    if (g_atomic_int_add(&gtk_initialized, 1) != 0 ||
#else
    if (g_atomic_int_exchange_and_add(&gtk_initialized, 1) != 0 ||
#endif
        G_UNLIKELY(!fm_init(config)))
        return FALSE;

    /* see http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=734691
       if no theme was selected and GTK fallback isn't available then no icons
       are shown - we should add folder and file icons as fallbacks theme */
    gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), PACKAGE_THEME_DIR);
    _fm_icon_pixbuf_init();
    _fm_thumbnail_init();
    _fm_file_properties_init();
    _fm_folder_model_init();
    _fm_folder_view_init();
    _fm_file_menu_init();

    return TRUE;
}

/**
 * fm_gtk_finalize
 *
 * Frees libfm data.
 *
 * This API should be called exactly that many times the fm_gtk_init()
 * was called before.
 *
 * Since: 0.1.0
 */
void fm_gtk_finalize(void)
{
    if (!g_atomic_int_dec_and_test(&gtk_initialized))
        return;

    _fm_icon_pixbuf_finalize();
    _fm_thumbnail_finalize();
    _fm_file_properties_finalize();
    _fm_folder_model_finalize();
    _fm_folder_view_finalize();
    _fm_file_menu_finalize();

    fm_finalize();
}

