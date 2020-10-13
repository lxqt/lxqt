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

#ifndef __DBUS_UTILS_H__
#define __DBUS_UTILS_H__

#include <glib.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

// char* dbus_get_prop(DBusGProxy* proxy, const char* iface, const char* prop);
GHashTable* dbus_get_all_props(DBusGProxy* proxy, const char* iface, GError** err);

static inline const char* dbus_prop_str(GHashTable* props, const char* name)
{
    GValue* val = (GValue*)g_hash_table_lookup(props, name);
    return val ? g_value_get_string(val) : NULL;
}

static inline const char* dbus_prop_obj_path(GHashTable* props, const char* name)
{
    GValue* val = (GValue*)g_hash_table_lookup(props, name);
    return val ? (char*)g_value_get_boxed(val) : NULL;
}

static inline const char** dbus_prop_strv(GHashTable* props, const char* name)
{
    GValue* val = (GValue*)g_hash_table_lookup(props, name);
    return val ? (const char**)g_value_get_boxed(val) : NULL;
}

static inline char* dbus_prop_dup_str(GHashTable* props, const char* name)
{
    GValue* val = (GValue*)g_hash_table_lookup(props, name);
    return val ? g_value_dup_string(val) : NULL;
}

static inline char* dbus_prop_dup_obj_path(GHashTable* props, const char* name)
{
    GValue* val = (GValue*)g_hash_table_lookup(props, name);
    return val ? g_strdup((char*)g_value_get_boxed(val)) : NULL;
}

static inline char** dbus_prop_dup_strv(GHashTable* props, const char* name)
{
    GValue* val = (GValue*)g_hash_table_lookup(props, name);
    return val ? g_value_dup_boxed(val) : NULL;
}

static inline gboolean dbus_prop_bool(GHashTable* props, const char* name)
{
    GValue* val = (GValue*)g_hash_table_lookup(props, name);
    return val ? g_value_get_boolean(val) : FALSE;
}

static inline gint dbus_prop_int(GHashTable* props, const char* name)
{
    GValue* val = (GValue*)g_hash_table_lookup(props, name);
    return val ? g_value_get_int(val) : 0;
}

static inline guint dbus_prop_uint(GHashTable* props, const char* name)
{
    GValue* val = (GValue*)g_hash_table_lookup(props, name);
    return val ? g_value_get_uint(val) : 0;
}

static inline gint64 dbus_prop_int64(GHashTable* props, const char* name)
{
    GValue* val = (GValue*)g_hash_table_lookup(props, name);
    return val ? g_value_get_int64(val) : 0;
}

static inline guint64 dbus_prop_uint64(GHashTable* props, const char* name)
{
    GValue* val = (GValue*)g_hash_table_lookup(props, name);
    return val ? g_value_get_uint64(val) : 0;
}

// GHashTable* dbus_get_prop_async();
// GHashTable* dbus_get_all_props_async();

GError* g_udisks_error_to_gio_error(GError* error);

G_END_DECLS

#endif /* __DBUS_UTILS_H__ */
