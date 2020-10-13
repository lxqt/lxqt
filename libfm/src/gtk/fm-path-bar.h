/*
 *      fm-path-bar.h
 *
 *      Copyright 2011 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef __FM_PATH_BAR_H__
#define __FM_PATH_BAR_H__

#include <gtk/gtk.h>
#include "fm-path.h"

#include "fm-seal.h"

G_BEGIN_DECLS


#define FM_TYPE_PATH_BAR                (fm_path_bar_get_type())
#define FM_PATH_BAR(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            FM_TYPE_PATH_BAR, FmPathBar))
#define FM_PATH_BAR_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST((klass),\
            FM_TYPE_PATH_BAR, FmPathBarClass))
#define FM_IS_PATH_BAR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            FM_TYPE_PATH_BAR))
#define FM_IS_PATH_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),\
            FM_TYPE_PATH_BAR))
#define FM_PATH_BAR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj),\
            FM_TYPE_PATH_BAR, FmPathBarClass))

typedef struct _FmPathBar            FmPathBar;
typedef struct _FmPathBarClass        FmPathBarClass;

struct _FmPathBar
{
    /*< private >*/
    GtkHBox parent;
    GtkWidget* FM_SEAL(viewport); /* viewport to make hbox scrollable */
    GtkWidget* FM_SEAL(btn_box); /* hbox containing path element buttons */

    GtkWidget* FM_SEAL(left_scroll);    /* left scroll button */
    GtkWidget* FM_SEAL(right_scroll);   /* right scroll button */
    FmPath* FM_SEAL(cur_path);   /* currently active path */
    FmPath* FM_SEAL(full_path);  /* full path shown in the bar */
    gpointer _reserved1;
    gpointer _reserved2;
};

/**
 * FmPathBarClass:
 * @parent_class: the parent class
 * @chdir: the class closure for the #FmPathBar::chdir signal
 */
struct _FmPathBarClass
{
    GtkHBoxClass parent_class;
    void (*chdir)(FmPathBar* bar, FmPath* path);
    /*< private >*/
    gpointer _reserved1;
};


GType        fm_path_bar_get_type        (void);
FmPathBar*    fm_path_bar_new            (void);

FmPath* fm_path_bar_get_path(FmPathBar* bar);
void fm_path_bar_set_path(FmPathBar* bar, FmPath* path);

G_END_DECLS

#endif /* __FM_PATH_BAR_H__ */
