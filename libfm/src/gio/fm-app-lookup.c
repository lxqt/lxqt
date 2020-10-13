/*
 *      fm-app-lookup.c
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

#include "fm-app-lookup.h"
#include <gio/gdesktopappinfo.h>

#ifndef G_IMPLEMENT_INTERFACE_DYNAMIC /* this macro is only provided in glib > 2.24 */
    #define G_IMPLEMENT_INTERFACE_DYNAMIC(TYPE_IFACE, iface_init)       { \
      const GInterfaceInfo g_implement_interface_info = { \
        (GInterfaceInitFunc) iface_init, NULL, NULL      \
      }; \
      g_type_module_add_interface (type_module, g_define_type_id, TYPE_IFACE, &g_implement_interface_info); \
    }
#endif

static void app_lookup_iface_init(GDesktopAppInfoLookupIface *iface);
static GObject* fm_app_lookup_constructor(GType type, guint n_props, GObjectConstructParam *props);
static void fm_app_lookup_finalize              (GObject *object);
static GAppInfo *get_default_for_uri_scheme(GDesktopAppInfoLookup *lookup, const char *scheme);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(FmAppLookup, fm_app_lookup, G_TYPE_OBJECT, 0,
    G_IMPLEMENT_INTERFACE_DYNAMIC(G_TYPE_DESKTOP_APP_INFO_LOOKUP, app_lookup_iface_init))


static void fm_app_lookup_class_init(FmAppLookupClass *klass)
{
    GObjectClass *g_object_class;
    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->constructor = fm_app_lookup_constructor;
    g_object_class->finalize = fm_app_lookup_finalize;
}

static void fm_app_lookup_class_finalize(FmAppLookupClass *klass)
{
}

GObject* fm_app_lookup_constructor(GType type, guint n_props, GObjectConstructParam *props)
{
    GObject* obj;
    /* call parent constructor. */
    obj = G_OBJECT_CLASS(fm_app_lookup_parent_class)->constructor(type, n_props, props);
    return obj;
}

static void fm_app_lookup_finalize(GObject *object)
{
    FmAppLookup *self;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_APP_LOOKUP(object));

    self = FM_APP_LOOKUP(object);
    G_OBJECT_CLASS(fm_app_lookup_parent_class)->finalize(object);
}


static void fm_app_lookup_init(FmAppLookup *self)
{

}

void fm_app_lookup_register(GIOModule *module)
{
    gint priority;
    fm_app_lookup_register_type(G_TYPE_MODULE (module));
    /* check if we're in gnome, if true, use lower priority.
     * otherwise, use a high priority to override gvfs gconf module.
     * priority of the gconf module of gvfs is 10. */
    if(G_UNLIKELY(g_getenv("GNOME_DESKTOP_SESSION_ID"))) /* we're in Gnome */
        priority = 9;
    else /* we're in other desktop envionments */
        priority = 90;

    g_io_extension_point_implement(G_DESKTOP_APP_INFO_LOOKUP_EXTENSION_POINT_NAME,
                                         FM_TYPE_APP_LOOKUP, "libfm-applookup", priority);

    /* TODO: implement our own G_NATIVE_VOLUME_MONITOR_EXTENSION_POINT_NAME */
}

static void app_lookup_iface_init(GDesktopAppInfoLookupIface *iface)
{
    iface->get_default_for_uri_scheme = get_default_for_uri_scheme;
}

/* FIXME: implement our own browser lookup method not depending on gconf. */
GAppInfo *get_default_for_uri_scheme(GDesktopAppInfoLookup *lookup, const char *scheme)
{
    GAppInfo* app;
    /* use a way compatible with glib >= 2.27 */
    char* mime_type = g_strconcat("x-scheme-handler/", scheme, NULL);
    app = g_app_info_get_default_for_type(mime_type, FALSE);
    g_free(mime_type);
    return app;
}
