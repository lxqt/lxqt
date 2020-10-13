/*
 *      gtk-compat.h
 *
 *      Copyright 2011 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef __GTK_COMPAT_H__
#define __GTK_COMPAT_H__
#include <gtk/gtk.h>
#include "glib-compat.h"

G_BEGIN_DECLS

/* for gtk+ 3.0 migration */
#if !GTK_CHECK_VERSION(3, 0, 0)
    #define gdk_display_get_app_launch_context(dpy) gdk_app_launch_context_new()
#  define gdk_window_get_device_position(win,dev,xptr,yptr,mptr) \
        gdk_window_get_pointer(win,xptr,yptr,mptr)
#else
#  define gdk_cursor_unref(obj) g_object_unref(obj)
#endif

#if !GTK_CHECK_VERSION(2, 21, 0)
#  define   GDK_KEY_Left    GDK_Left
#  define   GDK_KEY_Right   GDK_Right
#  define   GDK_KEY_Tab     GDK_Tab
#  define   GDK_KEY_Escape  GDK_Escape
#  define   GDK_KEY_a       GDK_a
#  define   GDK_KEY_space   GDK_space
#  define   GDK_KEY_Return  GDK_Return
#  define   GDK_KEY_ISO_Enter GDK_ISO_Enter
#  define   GDK_KEY_KP_Enter GDK_KP_Enter
#  define   GDK_KEY_f       GDK_f
#  define   GDK_KEY_F       GDK_F
#  define   GDK_KEY_g       GDK_g
#  define   GDK_KEY_G       GDK_G
#  define   GDK_KEY_Up      GDK_Up
#  define   GDK_KEY_KP_Up   GDK_KP_Up
#  define   GDK_KEY_Down    GDK_Down
#  define   GDK_KEY_KP_Down GDK_KP_Down
#  define   GDK_KEY_p       GDK_p
#  define   GDK_KEY_n       GDK_n
#  define   GDK_KEY_Home    GDK_Home
#  define   GDK_KEY_KP_Home GDK_KP_Home
#  define   GDK_KEY_End     GDK_End
#  define   GDK_KEY_KP_End  GDK_KP_End
#  define   GDK_KEY_Page_Up GDK_Page_Up
#  define   GDK_KEY_KP_Page_Up GDK_KP_Page_Up
#  define   GDK_KEY_Page_Down GDK_Page_Down
#  define   GDK_KEY_KP_Page_Down GDK_KP_Page_Down
#  define   GDK_KEY_KP_Right GDK_KP_Right
#  define   GDK_KEY_KP_Left GDK_KP_Left
#  define   GDK_KEY_F10     GDK_F10
#  define   GDK_KEY_Menu    GDK_Menu
#endif

#if !GTK_CHECK_VERSION(3, 0, 0)
    #define gtk_widget_in_destruction(widget) \
        (GTK_OBJECT_FLAGS(GTK_OBJECT(widget)) & GTK_IN_DESTRUCTION)
#endif

#if !GTK_CHECK_VERSION(3, 0, 0)
#  define gtk_selection_data_get_data_with_length(sel_data,length) \
        gtk_selection_data_get_data(sel_data); \
        *(length) = gtk_selection_data_get_length(sel_data)
#endif

#if !GTK_CHECK_VERSION(2, 24, 0)
#  define gdk_window_get_screen(window) gdk_drawable_get_screen(window)
#endif

#if !GTK_CHECK_VERSION(2, 22, 0)
#  define gdk_drag_context_get_source_window(drag_context) \
        drag_context->source_window
#  define gdk_drag_context_get_selected_action(drag_context) \
        drag_context->action
#  define gdk_drag_context_get_actions(drag_context) \
        drag_context->actions
#  define gdk_drag_context_get_suggested_action(drag_context) \
        drag_context->suggested_action
#  define gtk_accessible_get_widget(accessible) accessible->widget
#endif

#if !GTK_CHECK_VERSION(2, 20, 0)
#  define gtk_widget_get_realized(widget) GTK_WIDGET_REALIZED(widget)
#endif

#ifndef ATK_CHECK_VERSION
#  define ATK_CHECK_VERSION(_a,_b,_c) (_a < 2 || (_a == 2 && _b < 8)) /* < 2.8 */
#endif

G_END_DECLS

#endif
