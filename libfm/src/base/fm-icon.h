/*
 *      fm-icon.h
 *
 *      Copyright 2009 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This file is a part of the Libfm library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __FM_ICON_H__
#define __FM_ICON_H__

#include <gio/gio.h>

G_BEGIN_DECLS

/**
 * FmIcon:
 *
 * Opaque structure used in Libfm icon cache. Before version 1.2.0 it had
 * own data structure. Since 1.2.0 it is derived from GIcon therefore it
 * can be used basicly the same way.
 */
typedef struct _FmIcon			FmIcon;

/* must be called before using FmIcon */
void _fm_icon_init();
void _fm_icon_finalize();

FmIcon* fm_icon_from_gicon(GIcon* gicon);
FmIcon* fm_icon_from_name(const char* name);

#ifndef FM_DISABLE_DEPRECATED
FmIcon* fm_icon_ref(FmIcon* icon);
void fm_icon_unref(FmIcon* icon);

/* Those APIs are used by GUI toolkits to cache loaded icon pixmaps
 * or GdkPixbuf in the FmIcon objects. Fox example, libfm-gtk stores
 * a list of GdkPixbuf objects in FmIcon::user_data.
 * It shouldn't be used in other ways by application developers. */
gpointer fm_icon_get_user_data(FmIcon* icon);
void fm_icon_set_user_data(FmIcon* icon, gpointer user_data);
void fm_icon_set_user_data_destroy(GDestroyNotify func);

void fm_icon_unload_user_data_cache();
#endif

/**
 * fm_icon_get_gicon:
 * @icon: a #FmIcon
 *
 * The macro to access GIcon instead of old direct access.
 * For older versions applications can define it as (icon->gicon)
 *
 * Returns: cached #GIcon object.
 *
 * Since: 1.2.0
 */
#define fm_icon_get_gicon(icon) G_ICON(icon)

void fm_icon_reset_user_data_cache(GQuark quark);

void fm_icon_unload_cache();

G_END_DECLS

#endif /* __FM_ICON_H__ */
