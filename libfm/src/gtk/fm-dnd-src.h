/*
 *      fm-dnd-src.h
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


#ifndef __FM_DND_SRC_H__
#define __FM_DND_SRC_H__

#include <gtk/gtk.h>
#include "fm-file-info.h"

#include "fm-seal.h"

G_BEGIN_DECLS

#define FM_TYPE_DND_SRC				(fm_dnd_src_get_type())
#define FM_DND_SRC(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			FM_TYPE_DND_SRC, FmDndSrc))
#define FM_DND_SRC_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			FM_TYPE_DND_SRC, FmDndSrcClass))
#define FM_IS_DND_SRC(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			FM_TYPE_DND_SRC))
#define FM_IS_DND_SRC_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			FM_TYPE_DND_SRC))

/**
 * FmDndSrcTarget
 * @FM_DND_SRC_TARGET_FM_LIST: direct pointer of FmList
 * @FM_DND_SRC_TARGET_URI_LIST: "text/uri-list"
 * @FM_DND_SRC_TARGET_TEXT: Gtk+ default text targets
 * @N_FM_DND_SRC_DEFAULT_TARGETS: widget's target indices should start from this
 *
 * default targets of drag source
 */
typedef enum
{
    FM_DND_SRC_TARGET_FM_LIST = 1,
    FM_DND_SRC_TARGET_URI_LIST,
    FM_DND_SRC_TARGET_TEXT,
    /*< private >*/
    FM_DND_SRC_RESERVED2,
    FM_DND_SRC_RESERVED3,
    FM_DND_SRC_RESERVED4,
    FM_DND_SRC_RESERVED5,
    /*< public >*/
    N_FM_DND_SRC_DEFAULT_TARGETS
} FmDndSrcTarget;

typedef struct _FmDndSrc			FmDndSrc;
typedef struct _FmDndSrcClass		FmDndSrcClass;

struct _FmDndSrc
{
	GObject parent;
	GtkWidget* FM_SEAL(widget);
	FmFileInfoList* FM_SEAL(files);
	/*< private >*/
	gpointer _reserved1;
};

/**
 * FmDndSrcClass
 * @parent_class: the parent class
 * @data_get: the class closure for the #FmDndSrc::data-get signal
 */
struct _FmDndSrcClass
{
	GObjectClass parent_class;
	void (*data_get)(FmDndSrc*);
	/*< private >*/
	gpointer _reserved1;
};

GType		fm_dnd_src_get_type		(void);
FmDndSrc*	fm_dnd_src_new			(GtkWidget* w);

void fm_dnd_src_set_widget(FmDndSrc* ds, GtkWidget* w);

/* FmFileInfoList* fm_dnd_src_get_files(FmDndSrc* ds); */
void fm_dnd_src_set_files(FmDndSrc* ds, FmFileInfoList* files);
void fm_dnd_src_set_file(FmDndSrc* ds, FmFileInfo* file);

/**
 * fm_dnd_src_add_targets
 * @widget: #GtkWidget to add targets
 * @targets: pointer to array of #GtkTargetEntry to add
 * @n: number of targets to add
 *
 * Adds drag source targets to existing list for @widget. Convenience API.
 *
 * Since: 1.0.1
 */
#define fm_dnd_src_add_targets(widget,targets,n) \
            gtk_target_list_add_table(gtk_drag_source_get_target_list(widget), \
                                      targets, n)

G_END_DECLS

#endif /* __FM_DND_SRC_H__ */
