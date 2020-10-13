/*
 *      fm-app-lookup.h
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


#ifndef __FM_APP_LOOKUP_H__
#define __FM_APP_LOOKUP_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define FM_TYPE_APP_LOOKUP				(fm_app_lookup_get_type())
#define FM_APP_LOOKUP(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			FM_TYPE_APP_LOOKUP, FmAppLookup))
#define FM_APP_LOOKUP_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			FM_TYPE_APP_LOOKUP, FmAppLookupClass))
#define FM_IS_APP_LOOKUP(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			FM_TYPE_APP_LOOKUP))
#define FM_IS_APP_LOOKUP_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			FM_TYPE_APP_LOOKUP))

typedef struct _FmAppLookup			FmAppLookup;
typedef struct _FmAppLookupClass		FmAppLookupClass;

struct _FmAppLookup
{
	GObject parent;
};

struct _FmAppLookupClass
{
	GObjectClass parent_class;
};

GType fm_app_lookup_get_type(void);
void fm_app_lookup_register(GIOModule *module);

G_END_DECLS

#endif /* __FM_APP_LOOKUP_H__ */
