/*
 * fm-menu-tool-item.c
 *
 * Copyright (C) 2003 Ricardo Fernandez Pascual
 * Copyright (C) 2004 Paolo Borelli
 *
 * Copyright (C) 2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:fm-menu-tool-item
 * @short_description: A widget with arrow to show a menu in a tollbar.
 * @title: FmMenuToolItem
 *
 * @include: libfm/fm-gtk.h
 *
 * The #FmMenuToolItem shows button with arrow which shows menu when is
 * clicked, similar to #GtkMenuToolButton, but without any actual button,
 * just an arrow for menu.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define FM_DISABLE_SEAL

#include "fm-menu-tool-item.h"

#define FM_MENU_TOOL_ITEM_GET_PRIVATE(object) \
    (G_TYPE_INSTANCE_GET_PRIVATE((object), FM_TYPE_MENU_TOOL_ITEM, FmMenuToolItemPrivate))

struct _FmMenuToolItemPrivate
{
    GtkWidget *arrow;
    GtkWidget *arrow_button;
    GtkMenu   *menu;
};

#if GTK_CHECK_VERSION(3, 0, 0)
static void fm_menu_tool_item_destroy(GtkWidget *object);
#else
static void fm_menu_tool_item_destroy(GtkObject *object);
#endif
static int menu_deactivate_cb(GtkMenuShell *menu_shell, FmMenuToolItem *button);

enum
{
    SHOW_MENU,
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_MENU
};

static gint signals[LAST_SIGNAL];

G_DEFINE_TYPE (FmMenuToolItem, fm_menu_tool_item, GTK_TYPE_TOOL_ITEM)

static void fm_menu_tool_item_construct_contents (FmMenuToolItem *button)
{
    FmMenuToolItemPrivate *priv = button->priv;
    GtkOrientation orientation;

    orientation = gtk_tool_item_get_orientation (GTK_TOOL_ITEM (button));

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_arrow_set (GTK_ARROW (priv->arrow), GTK_ARROW_DOWN, GTK_SHADOW_NONE);
    else
        gtk_arrow_set (GTK_ARROW (priv->arrow), GTK_ARROW_RIGHT, GTK_SHADOW_NONE);

    gtk_button_set_relief (GTK_BUTTON (priv->arrow_button),
                           gtk_tool_item_get_relief_style (GTK_TOOL_ITEM (button)));

    gtk_widget_queue_resize (GTK_WIDGET (button));
}

static void fm_menu_tool_item_toolbar_reconfigured (GtkToolItem *toolitem)
{
    fm_menu_tool_item_construct_contents (FM_MENU_TOOL_ITEM (toolitem));
}

static void fm_menu_tool_item_state_changed(GtkWidget *widget,
                                            GtkStateType previous_state)
{
    FmMenuToolItem *button = FM_MENU_TOOL_ITEM (widget);
    FmMenuToolItemPrivate *priv = button->priv;

    if (!gtk_widget_is_sensitive(widget) && priv->menu)
    {
        gtk_menu_shell_deactivate (GTK_MENU_SHELL (priv->menu));
    }
}

static void fm_menu_tool_item_set_property(GObject *object, guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec)
{
    FmMenuToolItem *button = FM_MENU_TOOL_ITEM (object);

    switch (prop_id)
    {
    case PROP_MENU:
        fm_menu_tool_item_set_menu (button, g_value_get_object (value));
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void fm_menu_tool_item_get_property(GObject *object, guint prop_id,
                                           GValue *value, GParamSpec *pspec)
{
    FmMenuToolItem *button = FM_MENU_TOOL_ITEM (object);

    switch (prop_id)
    {
    case PROP_MENU:
        g_value_set_object (value, button->priv->menu);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void fm_menu_tool_item_class_init (FmMenuToolItemClass *klass)
{
    GObjectClass *object_class;
#if !GTK_CHECK_VERSION(3, 0, 0)
    GtkObjectClass *gtk_object_class;
#endif
    GtkWidgetClass *widget_class;
    GtkToolItemClass *toolitem_class;

    object_class = (GObjectClass *)klass;
#if !GTK_CHECK_VERSION(3, 0, 0)
    gtk_object_class = (GtkObjectClass *)klass;
#endif
    widget_class = (GtkWidgetClass *)klass;
    toolitem_class = (GtkToolItemClass *)klass;

    object_class->set_property = fm_menu_tool_item_set_property;
    object_class->get_property = fm_menu_tool_item_get_property;
#if GTK_CHECK_VERSION(3, 0, 0)
    widget_class->destroy = fm_menu_tool_item_destroy;
#else
    gtk_object_class->destroy = fm_menu_tool_item_destroy;
#endif
    widget_class->state_changed = fm_menu_tool_item_state_changed;
    toolitem_class->toolbar_reconfigured = fm_menu_tool_item_toolbar_reconfigured;

    /**
     * FmMenuToolItem::show-menu:
     * @button: the object on which the signal is emitted
     *
     * The ::show-menu signal is emitted before the menu is shown.
     *
     * It can be used to populate the menu on demand, using
     * fm_menu_tool_item_get_menu().
     *
     * Note that even if you populate the menu dynamically in this way,
     * you must set an empty menu on the #FmMenuToolItem beforehand,
     * since the arrow is made insensitive if the menu is not set.
     */
    signals[SHOW_MENU] =
        g_signal_new ("show-menu",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (FmMenuToolItemClass, show_menu),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    g_object_class_install_property (object_class,
                                     PROP_MENU,
                                     g_param_spec_object ("menu",
                                                          "Menu",
                                                          "The dropdown menu",
                                                          GTK_TYPE_MENU,
                                                          G_PARAM_READWRITE));

    g_type_class_add_private (object_class, sizeof (FmMenuToolItemPrivate));
}

static void menu_position_func(GtkMenu *menu, int *x, int *y,
                               gboolean *push_in, FmMenuToolItem *button)
{
    FmMenuToolItemPrivate *priv = button->priv;
    GtkWidget *widget = GTK_WIDGET (button);
    GtkAllocation arrow_allocation;
    GtkRequisition menu_req;
    GtkOrientation orientation;
    GtkTextDirection direction;
    GdkRectangle monitor;
    gint monitor_num;
    GdkScreen *screen;
    GdkWindow *window;

#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_widget_get_preferred_size (GTK_WIDGET (priv->menu), &menu_req, NULL);
#else
    gtk_widget_size_request (GTK_WIDGET (priv->menu), &menu_req);
#endif

    orientation = gtk_tool_item_get_orientation (GTK_TOOL_ITEM (button));
    direction = gtk_widget_get_direction (widget);
    window = gtk_widget_get_window (widget);

    screen = gtk_widget_get_screen (GTK_WIDGET (menu));
    monitor_num = gdk_screen_get_monitor_at_window (screen, window);
    if (monitor_num < 0)
        monitor_num = 0;
    gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        GtkAllocation allocation;

        gtk_widget_get_allocation (widget, &allocation);
        gtk_widget_get_allocation (priv->arrow_button, &arrow_allocation);

        gdk_window_get_origin (window, x, y);
        *x += allocation.x;
        *y += allocation.y;

        if (direction == GTK_TEXT_DIR_LTR)
            *x += MAX (allocation.width - menu_req.width, 0);
        else if (menu_req.width > allocation.width)
            *x -= menu_req.width - allocation.width;

        if ((*y + arrow_allocation.height + menu_req.height) <= monitor.y + monitor.height)
            *y += arrow_allocation.height;
        else if ((*y - menu_req.height) >= monitor.y)
            *y -= menu_req.height;
        else if (monitor.y + monitor.height - (*y + arrow_allocation.height) > *y)
            *y += arrow_allocation.height;
        else
            *y -= menu_req.height;
    }
    else
    {
#if GTK_CHECK_VERSION(2, 22, 0)
        gdk_window_get_origin (gtk_button_get_event_window (GTK_BUTTON (priv->arrow_button)), x, y);
#else
        gdk_window_get_origin (GTK_BUTTON (priv->arrow_button)->event_window, x, y);
#endif

        gtk_widget_get_allocation (priv->arrow_button, &arrow_allocation);

        if (direction == GTK_TEXT_DIR_LTR)
            *x += arrow_allocation.width;
        else
            *x -= menu_req.width;

        if (*y + menu_req.height > monitor.y + monitor.height &&
            *y + arrow_allocation.height - monitor.y > monitor.y + monitor.height - *y)
            *y += arrow_allocation.height - menu_req.height;
    }

    *push_in = FALSE;
}

static void popup_menu_under_arrow(FmMenuToolItem *button, GdkEventButton *event)
{
    FmMenuToolItemPrivate *priv = button->priv;

    g_signal_emit (button, signals[SHOW_MENU], 0);

    if (!priv->menu)
        return;

    gtk_menu_popup (priv->menu, NULL, NULL,
                    (GtkMenuPositionFunc) menu_position_func,
                    button,
                    event ? event->button : 0,
                    event ? event->time : gtk_get_current_event_time ());
}

static void arrow_button_toggled_cb(GtkToggleButton *togglebutton,
                                    FmMenuToolItem *button)
{
    FmMenuToolItemPrivate *priv = button->priv;

    if (!priv->menu)
        return;

    if (gtk_toggle_button_get_active (togglebutton) &&
        !gtk_widget_get_visible(GTK_WIDGET(priv->menu)))
    {
        /* we get here only when the menu is activated by a key
         * press, so that we can select the first menu item */
        popup_menu_under_arrow (button, NULL);
        gtk_menu_shell_select_first (GTK_MENU_SHELL (priv->menu), FALSE);
    }
}

static gboolean arrow_button_button_press_event_cb(GtkWidget *widget,
                                                   GdkEventButton *event,
                                                   FmMenuToolItem *button)
{
    if (event->button == 1)
    {
        popup_menu_under_arrow (button, event);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void fm_menu_tool_item_init(FmMenuToolItem *button)
{
    GtkWidget *arrow;
    GtkWidget *arrow_button;

    button->priv = FM_MENU_TOOL_ITEM_GET_PRIVATE (button);

    gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (button), FALSE);

    arrow_button = gtk_toggle_button_new ();
    arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
    gtk_container_add (GTK_CONTAINER (arrow_button), arrow);

    /* the arrow button is insentive until we set a menu */
    gtk_widget_set_sensitive (arrow_button, FALSE);

    gtk_widget_show_all (arrow_button);

    gtk_container_add (GTK_CONTAINER (button), arrow_button);

    button->priv->arrow = arrow;
    button->priv->arrow_button = arrow_button;

    g_signal_connect (arrow_button, "toggled",
                      G_CALLBACK (arrow_button_toggled_cb), button);
    g_signal_connect (arrow_button, "button-press-event",
                      G_CALLBACK (arrow_button_button_press_event_cb), button);
}

#if GTK_CHECK_VERSION(3, 0, 0)
static void fm_menu_tool_item_destroy(GtkWidget *object)
#else
static void fm_menu_tool_item_destroy (GtkObject *object)
#endif
{
    FmMenuToolItem *button;

    button = FM_MENU_TOOL_ITEM (object);

    if (button->priv->menu)
    {
        g_signal_handlers_disconnect_by_func (button->priv->menu,
                                              menu_deactivate_cb,
                                              button);
        gtk_menu_detach (button->priv->menu);

        g_signal_handlers_disconnect_by_func (button->priv->arrow_button,
                                              arrow_button_toggled_cb,
                                              button);
        g_signal_handlers_disconnect_by_func (button->priv->arrow_button,
                                              arrow_button_button_press_event_cb,
                                              button);
    }

#if GTK_CHECK_VERSION(3, 0, 0)
    GTK_WIDGET_CLASS (fm_menu_tool_item_parent_class)->destroy (object);
#else
    GTK_OBJECT_CLASS (fm_menu_tool_item_parent_class)->destroy (object);
#endif
}

/**
 * fm_menu_tool_item_new:
 *
 * Creates a new #FmMenuToolItem.
 *
 * Return value: (transfer full): the new #FmMenuToolItem
 *
 * Since: 1.2.0
 **/
GtkToolItem *fm_menu_tool_item_new(void)
{
    return (GtkToolItem *)g_object_new (FM_TYPE_MENU_TOOL_ITEM, NULL);
}

/* Callback for the "deactivate" signal on the pop-up menu.
 * This is used so that we unset the state of the toggle button
 * when the pop-up menu disappears.
 */
static int menu_deactivate_cb(GtkMenuShell *menu_shell, FmMenuToolItem *button)
{
    FmMenuToolItemPrivate *priv = button->priv;

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->arrow_button), FALSE);

    return TRUE;
}

static void menu_detacher(GtkWidget *widget, GtkMenu *menu)
{
    FmMenuToolItemPrivate *priv = FM_MENU_TOOL_ITEM (widget)->priv;

    g_return_if_fail (priv->menu == menu);

    priv->menu = NULL;
}

/**
 * fm_menu_tool_item_set_menu:
 * @button: a #FmMenuToolItem
 * @menu: the #GtkMenu associated with #FmMenuToolItem
 *
 * Sets the #GtkMenu that is popped up when the user clicks on the arrow.
 * If @menu is NULL, the arrow button becomes insensitive.
 *
 * Since: 1.2.0
 **/
void fm_menu_tool_item_set_menu(FmMenuToolItem *button, GtkWidget *menu)
{
    FmMenuToolItemPrivate *priv;

    g_return_if_fail (FM_IS_MENU_TOOL_ITEM (button));
    g_return_if_fail (GTK_IS_MENU (menu) || menu == NULL);

    priv = button->priv;

    if (priv->menu != GTK_MENU (menu))
    {
        if (priv->menu && gtk_widget_get_visible(GTK_WIDGET(priv->menu)))
        gtk_menu_shell_deactivate (GTK_MENU_SHELL (priv->menu));

        if (priv->menu)
        {
            g_signal_handlers_disconnect_by_func (priv->menu,
                                                  menu_deactivate_cb,
                                                  button);
            gtk_menu_detach (priv->menu);
        }

        priv->menu = GTK_MENU (menu);

        if (priv->menu)
        {
            gtk_menu_attach_to_widget (priv->menu, GTK_WIDGET (button),
                                       menu_detacher);

            gtk_widget_set_sensitive (priv->arrow_button, TRUE);

            g_signal_connect (priv->menu, "deactivate",
                              G_CALLBACK (menu_deactivate_cb), button);
        }
        else
            gtk_widget_set_sensitive (priv->arrow_button, FALSE);
    }

    g_object_notify (G_OBJECT (button), "menu");
}

/**
 * fm_menu_tool_item_get_menu:
 * @button: a #FmMenuToolItem
 *
 * Gets the #GtkMenu associated with #FmMenuToolItem.
 *
 * Return value: (transfer none): the #GtkMenu associated with #FmMenuToolItem
 *
 * Since: 1.2.0
 **/
GtkWidget *fm_menu_tool_item_get_menu(FmMenuToolItem *button)
{
    g_return_val_if_fail (FM_IS_MENU_TOOL_ITEM (button), NULL);

    return GTK_WIDGET (button->priv->menu);
}
