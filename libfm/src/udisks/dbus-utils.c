/*
 *      dbus-utils.h
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

#include "dbus-utils.h"
#include <gio/gio.h>

GHashTable* dbus_get_all_props(DBusGProxy* proxy, const char* iface, GError** err)
{
    GHashTable* props = NULL;
    dbus_g_proxy_call(proxy, "GetAll", err,
                      G_TYPE_STRING, iface, G_TYPE_INVALID,
                      dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), &props, G_TYPE_INVALID);
    return props;
}


GError* g_udisks_error_to_gio_error(GError* error)
{
    if(error)
    {
        int code = G_IO_ERROR_FAILED;
        error = g_error_new_literal(G_IO_ERROR, code, error->message);
        return error;
    }
    return NULL;
}

