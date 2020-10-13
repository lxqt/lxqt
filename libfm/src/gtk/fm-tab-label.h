/*
 *      fm-tab-label.h
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


#ifndef __FM_TAB_LABEL_H__
#define __FM_TAB_LABEL_H__

#include <gtk/gtk.h>

#include "fm-icon-pixbuf.h"

G_BEGIN_DECLS

#define FM_TYPE_TAB_LABEL				(fm_tab_label_get_type())
#define FM_TAB_LABEL(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			FM_TYPE_TAB_LABEL, FmTabLabel))
#define FM_TAB_LABEL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			FM_TYPE_TAB_LABEL, FmTabLabelClass))
#define FM_IS_TAB_LABEL(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			FM_TYPE_TAB_LABEL))
#define FM_IS_TAB_LABEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			FM_TYPE_TAB_LABEL))

typedef struct _FmTabLabel			FmTabLabel;
typedef struct _FmTabLabelClass		FmTabLabelClass;

struct _FmTabLabel
{
    GtkEventBox parent;
    GtkLabel* label;
    GtkButton* close_btn;
    /*< private >*/
    GtkWidget *image;
    gpointer _reserved1;
};

struct _FmTabLabelClass
{
    /*< private >*/
    GtkEventBoxClass parent_class;
    gpointer _reserved1;
};

GType fm_tab_label_get_type(void);
FmTabLabel* fm_tab_label_new(const char* text);

void fm_tab_label_set_text(FmTabLabel* label, const char* text);
void fm_tab_label_set_tooltip_text(FmTabLabel* label, const char* text);

void fm_tab_label_set_icon(FmTabLabel *label, FmIcon *icon);

G_END_DECLS

#endif /* __FM_TAB_LABEL_H__ */
