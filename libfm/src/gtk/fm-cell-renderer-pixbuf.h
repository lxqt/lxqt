/*
 *      fm-cell-renderer-pixbuf.h
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


#ifndef __FM_CELL_RENDERER_PIXBUF_H__
#define __FM_CELL_RENDERER_PIXBUF_H__

#include <gtk/gtk.h>
#include "fm-file-info.h"

#include "fm-seal.h"

G_BEGIN_DECLS

#define FM_TYPE_CELL_RENDERER_PIXBUF				(fm_cell_renderer_pixbuf_get_type())
#define FM_CELL_RENDERER_PIXBUF(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			FM_TYPE_CELL_RENDERER_PIXBUF, FmCellRendererPixbuf))
#define FM_CELL_RENDERER_PIXBUF_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			FM_TYPE_CELL_RENDERER_PIXBUF, FmCellRendererPixbufClass))
#define FM_IS_CELL_RENDERER_PIXBUF(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			FM_TYPE_CELL_RENDERER_PIXBUF))
#define FM_IS_CELL_RENDERER_PIXBUF_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			FM_TYPE_CELL_RENDERER_PIXBUF))

typedef struct _FmCellRendererPixbuf			FmCellRendererPixbuf;
typedef struct _FmCellRendererPixbufClass		FmCellRendererPixbufClass;

struct _FmCellRendererPixbuf
{
    /*< private >*/
    GtkCellRendererPixbuf parent;
    FmFileInfo* FM_SEAL(fi);
    GdkPixbuf* FM_SEAL(icon);
    gint FM_SEAL(fixed_w);
    gint FM_SEAL(fixed_h);
    gint _reserved1;
    gint _reserved2;
};

struct _FmCellRendererPixbufClass
{
    /*< private >*/
    GtkCellRendererPixbufClass parent_class;
    gpointer _reserved1;
};

GType		fm_cell_renderer_pixbuf_get_type		(void);
FmCellRendererPixbuf* fm_cell_renderer_pixbuf_new		(void);

void fm_cell_renderer_pixbuf_set_fixed_size(FmCellRendererPixbuf* render, gint w, gint h);

G_END_DECLS

#endif /* __FM_CELL_RENDERER_PIXBUF_H__ */
