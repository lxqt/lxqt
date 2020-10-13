/*
 *      module.c
 *
 *      Copyright 2010 PCMan <pcman.tw@gmail.com>
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

#include <glib.h>
#include <gmodule.h>
#include <gio/gio.h>
#include <glib/gi18n-lib.h>
#include <gio/gdesktopappinfo.h>

#include "fm-app-lookup.h"

void g_io_module_load (GIOModule *module)
{
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    fm_app_lookup_register (module);
}

void g_io_module_unload (GIOModule *module)
{
}

char** g_io_module_query (void)
{
    char** eps = g_new(char*, 2);
    eps[0] = g_strdup(G_DESKTOP_APP_INFO_LOOKUP_EXTENSION_POINT_NAME);
    eps[1] = NULL;
    return eps;
}
