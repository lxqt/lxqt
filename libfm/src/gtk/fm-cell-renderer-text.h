/*
 *      fm-cell-renderer-text.h
 *      
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
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


#ifndef __FM_CELL_RENDERER_TEXT_H__
#define __FM_CELL_RENDERER_TEXT_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FM_CELL_RENDERER_TEXT_TYPE				(fm_cell_renderer_text_get_type())
#define FM_CELL_RENDERER_TEXT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			FM_CELL_RENDERER_TEXT_TYPE, FmCellRendererText))
#define FM_CELL_RENDERER_TEXT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			FM_CELL_RENDERER_TEXT_TYPE, FmCellRendererTextClass))
#define FM_IS_CELL_RENDERER_TEXT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			FM_CELL_RENDERER_TEXT_TYPE))
#define FM_IS_CELL_RENDERER_TEXT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			FM_CELL_RENDERER_TEXT_TYPE))

typedef struct _FmCellRendererText			FmCellRendererText;
typedef struct _FmCellRendererTextClass		FmCellRendererTextClass;

struct _FmCellRendererText
{
	GtkCellRendererText parent;
	/* add your public declarations here */
	/*< private >*/
	gpointer _reserved1;
	gint height; /* "max-height" - height in pixels */
};

struct _FmCellRendererTextClass
{
    /*< private >*/
    GtkCellRendererTextClass parent_class;
    gpointer _reserved1;
};

GType		fm_cell_renderer_text_get_type		(void);
GtkCellRenderer*	fm_cell_renderer_text_new			(void);

G_END_DECLS

#endif /* __FM_CELL_RENDERER_TEXT_H__ */
