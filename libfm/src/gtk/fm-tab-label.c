/*
 *      fm-tab-label.c
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

/**
 * SECTION:fm-tab-label
 * @short_description: A tab label widget.
 * @title: FmTabLabel
 *
 * @include: libfm/fm-gtk.h
 *
 * The #FmTabLabel is a widget that can be used as a label of tab in
 * notebook-like folders view.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>

#include "fm-tab-label.h"

G_DEFINE_TYPE(FmTabLabel, fm_tab_label, GTK_TYPE_EVENT_BOX);

#if GTK_CHECK_VERSION(3, 0, 0)
GtkCssProvider *provider;
#endif

static void fm_tab_label_class_init(FmTabLabelClass *klass)
{
    /* special style used by close button */
#if GTK_CHECK_VERSION(3, 0, 0)
    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "#tab-close-btn {\n"
            "-GtkWidget-focus-padding : 0;\n"
            "-GtkWidget-focus-line-width : 0;\n"
            "padding : 0;\n"
        "}\n", -1, NULL);
#else
    gtk_rc_parse_string(
        "style \"close-btn-style\" {\n"
            "GtkWidget::focus-padding = 0\n"
            "GtkWidget::focus-line-width = 0\n"
            "xthickness = 0\n"
            "ythickness = 0\n"
        "}\n"
        "widget \"*.tab-close-btn\" style \"close-btn-style\"");
#endif
}

/* FIXME: add g_object_unref (provider); on class destroy? */

static void on_close_btn_style_set(GtkWidget *btn, GtkRcStyle *prev, gpointer data)
{
    gint w, h;
    gtk_icon_size_lookup_for_settings(gtk_widget_get_settings(btn), GTK_ICON_SIZE_MENU, &w, &h);
    gtk_widget_set_size_request(btn, w + 2, h + 2);
}

static gboolean on_query_tooltip(GtkWidget *widget, gint x, gint y,
                                 gboolean    keyboard_mode,
                                 GtkTooltip *tooltip, gpointer user_data)
{
    /* We should only show the tooltip if the text is ellipsized */
    GtkLabel* label = GTK_LABEL(widget);
    PangoLayout* layout = gtk_label_get_layout(label);
    if(pango_layout_is_ellipsized(layout))
    {
        gtk_tooltip_set_text(tooltip, gtk_label_get_text(label));
        return TRUE;
    }
    return FALSE;
}

static void fm_tab_label_init(FmTabLabel *self)
{
    GtkBox* hbox;
#if GTK_CHECK_VERSION(3, 0, 0)
    GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(self));
#endif

    gtk_event_box_set_visible_window(GTK_EVENT_BOX(self), FALSE);
#if GTK_CHECK_VERSION(3, 2, 0)
    /* FIXME: migrate to GtkGrid */
    hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
#else
    hbox = GTK_BOX(gtk_hbox_new(FALSE, 0));
#endif

    self->label = (GtkLabel*)gtk_label_new("");
    gtk_widget_set_has_tooltip((GtkWidget*)self->label, TRUE);
    gtk_box_pack_start(hbox, (GtkWidget*)self->label, FALSE, FALSE, 4 );
    g_signal_connect(self->label, "query-tooltip", G_CALLBACK(on_query_tooltip), self);

    self->close_btn = (GtkButton*)gtk_button_new();
    gtk_button_set_focus_on_click(self->close_btn, FALSE);
    gtk_button_set_relief(self->close_btn, GTK_RELIEF_NONE );
    gtk_container_add ( GTK_CONTAINER ( self->close_btn ),
                        gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
    gtk_container_set_border_width(GTK_CONTAINER(self->close_btn), 0);
    gtk_widget_set_name((GtkWidget*)self->close_btn, "tab-close-btn");
    g_signal_connect(self->close_btn, "style-set", G_CALLBACK(on_close_btn_style_set), NULL);

    gtk_box_pack_end( hbox, (GtkWidget*)self->close_btn, FALSE, FALSE, 0 );

    gtk_container_add(GTK_CONTAINER(self), (GtkWidget*)hbox);
    gtk_widget_show_all((GtkWidget*)hbox);

#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#endif

/*
    gtk_drag_dest_set ( GTK_WIDGET( evt_box ), GTK_DEST_DEFAULT_ALL,
                        drag_targets,
                        sizeof( drag_targets ) / sizeof( GtkTargetEntry ),
                        GDK_ACTION_DEFAULT | GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK );
    g_signal_connect ( ( gpointer ) evt_box, "drag-motion",
                       G_CALLBACK ( on_tab_drag_motion ),
                       file_browser );
*/
}

/**
 * fm_tab_label_new
 * @text: text to display as a tab label
 *
 * Creates new tab label widget.
 *
 * Returns: (transfer full): a new #FmTabLabel widget.
 *
 * Since: 0.1.10
 */
FmTabLabel *fm_tab_label_new(const char* text)
{
    FmTabLabel* label = (FmTabLabel*)g_object_new(FM_TYPE_TAB_LABEL, NULL);
    AtkObject *obj;
    obj = gtk_widget_get_accessible(GTK_WIDGET(label));
    atk_object_set_description(obj, _("Changes active tab"));
    gtk_label_set_text(label->label, text);
    return label;
}

/**
 * fm_tab_label_set_text
 * @label: a tab label widget
 * @text: text to display as a tab label
 *
 * Changes text on the @label.
 *
 * Since: 0.1.10
 */
void fm_tab_label_set_text(FmTabLabel* label, const char* text)
{
    gtk_label_set_text(label->label, text);
}

/**
 * fm_tab_label_set_tooltip_text
 * @label: a tab label widget
 * @text: text to display in label tooltip
 *
 * Changes text of tooltip on the @label.
 *
 * Since: 1.0.0
 */
void fm_tab_label_set_tooltip_text(FmTabLabel* label, const char* text)
{
    gtk_widget_set_tooltip_text(GTK_WIDGET(label->label), text);
}

/**
 * fm_tab_label_set_icon
 * @label: a tab label widget
 * @icon: (allow-none): an icon to show before text or %NULL
 *
 * Sets an optional @icon to be shown before text in the @label.
 *
 * Since: 1.2.0
 */
void fm_tab_label_set_icon(FmTabLabel *label, FmIcon *icon)
{
    g_return_if_fail(FM_IS_TAB_LABEL(label));
    if (icon)
    {
        gint height, width;
        GdkPixbuf *pixbuf;

        if (!gtk_icon_size_lookup(GTK_ICON_SIZE_BUTTON, &width, &height))
            height = 20; /* fallback size, is that ever needed? */
        pixbuf = fm_pixbuf_from_icon(icon, height);
        if (pixbuf == NULL)
            goto _no_image;
        if (label->image)
        {
            gtk_image_set_from_pixbuf((GtkImage*)label->image, pixbuf);
            gtk_widget_queue_draw(GTK_WIDGET(label));
        }
        else
        {
            /* hbox is only child of the label */
            GtkWidget *hbox = gtk_bin_get_child(GTK_BIN(label));

            label->image = gtk_image_new_from_pixbuf(pixbuf);
            gtk_box_pack_start(GTK_BOX(hbox), label->image, FALSE, FALSE, 0);
            gtk_widget_show(label->image);
        }
        g_object_unref(pixbuf);
        return;
    }
_no_image:
    if (label->image)
    {
        gtk_widget_destroy(label->image);
        label->image = NULL;
    }
}
