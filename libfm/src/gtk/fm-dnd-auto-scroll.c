/*
 *      fm-dnd-auto-scroll.c
 *
 *      Copyright 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

/**
 * SECTION:fm-dnd-auto-scroll
 * @short_description: Autoscroll when dragged out of widget.
 * @title: Drag and drop autoscrolling
 *
 * @include: libfm/fm-gtk.h
 *
 */
#include "fm-dnd-auto-scroll.h"
#include "gtk-compat.h"

#define SCROLL_EDGE_SIZE 15

typedef struct _FmDndAutoScroll FmDndAutoScroll;
struct _FmDndAutoScroll
{
    GtkWidget* widget;
    guint timeout;
    GtkAdjustment* hadj;
    GtkAdjustment* vadj;
};

static GQuark data_id = 0;

static gboolean on_auto_scroll(gpointer user_data)
{
    FmDndAutoScroll* as = (FmDndAutoScroll*)user_data;
    int x, y;
    GtkAdjustment* va = as->vadj;
    GtkAdjustment* ha = as->hadj;
    GtkWidget* widget = as->widget;
    GtkAllocation allocation;

    if(g_source_is_destroyed(g_main_current_source()))
        return FALSE;

    gdk_window_get_device_position (gtk_widget_get_window(widget),
                                    gdk_device_manager_get_client_pointer(
                                        gdk_display_get_device_manager(
                                            gdk_window_get_display(
                                                gtk_widget_get_window(widget)))),
                                    &x, &y, NULL);
    gtk_widget_get_allocation(widget, &allocation);

    /*
       HACK.
       Sometimes we do not get drag-leave signal. (Why?)
       This check prevents infinite scrolling.
    */
    if (y < 0 || y > allocation.height ||
        x < 0 || x > allocation.width)
    {
        as->timeout = 0;
        return FALSE;
    }

    if(va)
    {
        if(y < SCROLL_EDGE_SIZE) /* scroll up */
        {
            gdouble value = gtk_adjustment_get_value(va);
            gdouble lower = gtk_adjustment_get_lower(va);
            if(value > lower)
            {
                value -= gtk_adjustment_get_step_increment(va);
                if(value < lower)
                    value = lower;
                gtk_adjustment_set_value(va, value);
            }
        }
        else if(y > (allocation.height - SCROLL_EDGE_SIZE))
        {
            gdouble value = gtk_adjustment_get_value(va);
            gdouble upper = gtk_adjustment_get_upper(va) - gtk_adjustment_get_page_size(va);
            /* scroll down */
            if(value < upper)
            {
                value += gtk_adjustment_get_step_increment(va);
                if(value > upper)
                    value = upper;
                gtk_adjustment_set_value(va, value);
            }
        }
        gtk_adjustment_value_changed(va);
    }

    if(ha)
    {
        if(x < SCROLL_EDGE_SIZE) /* scroll to left */
        {
            gdouble value = gtk_adjustment_get_value(ha);
            gdouble lower = gtk_adjustment_get_lower(ha);
            if(value > lower)
            {
                value -= gtk_adjustment_get_step_increment(ha);
                if(value < lower)
                    value = lower;
                gtk_adjustment_set_value(ha, value);
            }
        }
        else if(x > (allocation.width - SCROLL_EDGE_SIZE))
        {
            gdouble value = gtk_adjustment_get_value(ha);
            gdouble upper = gtk_adjustment_get_upper(ha) - gtk_adjustment_get_page_size(ha);
            /* scroll to right */
            if(value < upper)
            {
                value += gtk_adjustment_get_step_increment(ha);
                if(value > upper)
                    value = upper;
                gtk_adjustment_set_value(ha, value);
            }
        }
        gtk_adjustment_value_changed(ha);
    }
    return TRUE;
}

static gboolean on_drag_motion(GtkWidget *widget, GdkDragContext *drag_context,
                               gint x, gint y, guint time, gpointer user_data)
{
    FmDndAutoScroll* as = (FmDndAutoScroll*)user_data;
    /* FIXME: this is a dirty hack for GTK_TREE_MODEL_ROW. When dragging GTK_TREE_MODEL_ROW
     * we cannot receive "drag-leave" message. So weied! Is it a gtk+ bug? */
    GdkAtom target = gtk_drag_dest_find_target(widget, drag_context, NULL);
    if(target == GDK_NONE)
        return FALSE;
    if(0 == as->timeout) /* install a scroll timeout if needed */
    {
        as->timeout = gdk_threads_add_timeout(150, on_auto_scroll, as);
    }
    return FALSE;
}

static void on_drag_leave(GtkWidget *widget, GdkDragContext *drag_context,
                          guint time, gpointer user_data)
{
    FmDndAutoScroll* as = (FmDndAutoScroll*)user_data;
    if(as->timeout)
    {
        g_source_remove(as->timeout);
        as->timeout = 0;
    }
}

static void fm_dnd_auto_scroll_free(gpointer user_data)
{
    FmDndAutoScroll* as = (FmDndAutoScroll*)user_data;
    if(as->timeout)
        g_source_remove(as->timeout);
    if(as->hadj)
        g_object_unref(as->hadj);
    if(as->vadj)
        g_object_unref(as->vadj);

    g_signal_handlers_disconnect_by_func(as->widget, on_drag_motion, as);
    g_signal_handlers_disconnect_by_func(as->widget, on_drag_leave, as);
    g_slice_free(FmDndAutoScroll, as);
}

/**
 * fm_dnd_set_dest_auto_scroll
 * @drag_dest_widget: a drag destination widget
 * @hadj: horizontal GtkAdjustment
 * @vadj: vertical GtkAdjustment
 *
 * This function installs a "drag-motion" handler to the dest widget
 * to support auto-scroll when the dragged item is near the margin
 * of the destination widget. For example, when a user drags an item
 * over the bottom of a GtkTreeView, the desired behavior should be
 * to scroll up the content of the tree view and to expose the items
 * below currently visible region. So the user can drop on them.
 */
void fm_dnd_set_dest_auto_scroll(GtkWidget* drag_dest_widget,
                                 GtkAdjustment* hadj, GtkAdjustment* vadj)
{
    FmDndAutoScroll* as;
    if(G_UNLIKELY(data_id == 0))
        data_id = g_quark_from_static_string("FmDndAutoScroll");

    if(G_UNLIKELY(!hadj && !vadj))
    {
        g_object_set_qdata_full(G_OBJECT(drag_dest_widget), data_id, NULL, NULL);
        return;
    }

    as = g_slice_new(FmDndAutoScroll);
    as->widget = drag_dest_widget; /* no g_object_ref is needed here */
    as->timeout = 0;
    as->hadj = hadj ? GTK_ADJUSTMENT(g_object_ref(hadj)) : NULL;
    as->vadj = vadj ? GTK_ADJUSTMENT(g_object_ref(vadj)) : NULL;

    g_object_set_qdata_full(G_OBJECT(drag_dest_widget), data_id,
                            as, fm_dnd_auto_scroll_free);

    g_signal_connect(drag_dest_widget, "drag-motion",
                     G_CALLBACK(on_drag_motion), as);
    g_signal_connect(drag_dest_widget, "drag-leave",
                     G_CALLBACK(on_drag_leave), as);
}

/**
 * fm_dnd_unset_dest_auto_scroll
 * @drag_dest_widget: drag destination widget.
 *
 * Unsets what has been done by fm_dnd_set_dest_auto_scroll()
 */
void fm_dnd_unset_dest_auto_scroll(GtkWidget* drag_dest_widget)
{
    if(G_UNLIKELY(data_id == 0))
        data_id = g_quark_from_static_string("FmDndAutoScroll");
    g_object_set_qdata_full(G_OBJECT(drag_dest_widget), data_id, NULL, NULL);
}
