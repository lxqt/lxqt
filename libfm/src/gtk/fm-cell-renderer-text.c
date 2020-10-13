/*
 *      fm-cell-renderer-text.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2012-2016 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *      Copyright 2015 Mamoru TASAKA <mtasaka@fedoraproject.org>
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
 * SECTION:fm-cell-renderer-text
 * @short_description: An implementation of cell text renderer.
 * @title: FmCellRendererText
 *
 * @include: libfm/fm-gtk.h
 *
 * The #FmCellRendererText can be used to render text in cell.
 */

#include "fm-cell-renderer-text.h"

static void fm_cell_renderer_text_get_property(GObject *object, guint param_id,
                                               GValue *value, GParamSpec *psec);
static void fm_cell_renderer_text_set_property(GObject *object, guint param_id,
                                               const GValue *value, GParamSpec *psec);

enum
{
    PROP_HEIGHT = 1,
    N_PROPS
};

G_DEFINE_TYPE(FmCellRendererText, fm_cell_renderer_text, GTK_TYPE_CELL_RENDERER_TEXT);

static void fm_cell_renderer_text_get_size(GtkCellRenderer            *cell,
                                           GtkWidget                  *widget,
#if GTK_CHECK_VERSION(3, 0, 0)
                                           const
#endif
                                           GdkRectangle               *rectangle,
                                           gint                       *x_offset,
                                           gint                       *y_offset,
                                           gint                       *width,
                                           gint                       *height);

#if GTK_CHECK_VERSION(3, 0, 0)
static void fm_cell_renderer_text_render(GtkCellRenderer *cell,
                                         cairo_t *cr,
                                         GtkWidget *widget,
                                         const GdkRectangle *background_area,
                                         const GdkRectangle *cell_area,
                                         GtkCellRendererState flags);
#else
static void fm_cell_renderer_text_render(GtkCellRenderer *cell, 
                                         GdkDrawable *window,
                                         GtkWidget *widget,
                                         GdkRectangle *background_area,
                                         GdkRectangle *cell_area,
                                         GdkRectangle *expose_area,
                                         GtkCellRendererState  flags);
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
static void fm_cell_renderer_text_get_preferred_width(GtkCellRenderer *cell,
                                                      GtkWidget *widget,
                                                      gint *minimum_size,
                                                      gint *natural_size);

static void fm_cell_renderer_text_get_preferred_height(GtkCellRenderer *cell,
                                                       GtkWidget *widget,
                                                       gint *minimum_size,
                                                       gint *natural_size);

static void fm_cell_renderer_text_get_preferred_height_for_width(GtkCellRenderer *cell,
                                                                 GtkWidget *widget,
                                                                 gint width,
                                                                 gint *minimum_height,
                                                                 gint *natural_height);
#endif

static void fm_cell_renderer_text_class_init(FmCellRendererTextClass *klass)
{
    GObjectClass *g_object_class = G_OBJECT_CLASS(klass);
    GtkCellRendererClass* render_class = GTK_CELL_RENDERER_CLASS(klass);

    g_object_class->get_property = fm_cell_renderer_text_get_property;
    g_object_class->set_property = fm_cell_renderer_text_set_property;

    render_class->render = fm_cell_renderer_text_render;
    render_class->get_size = fm_cell_renderer_text_get_size;
#if GTK_CHECK_VERSION(3, 0, 0)
    render_class->get_preferred_width = fm_cell_renderer_text_get_preferred_width;
    render_class->get_preferred_height = fm_cell_renderer_text_get_preferred_height;
    render_class->get_preferred_height_for_width = fm_cell_renderer_text_get_preferred_height_for_width;
#endif

    /**
     * FmCellRendererText:max-height:
     *
     * The #FmCellRendererText:max-height property is the maximum height
     * of text that will be rendered. If text doesn't fit in that height
     * then text will be ellipsized. The value of -1 means unlimited
     * height.
     *
     * Since: 1.0.2
     */
    g_object_class_install_property(g_object_class,
                                    PROP_HEIGHT,
                                    g_param_spec_int("max-height",
                                                     "Maximum_height",
                                                     "Maximum height",
                                                     -1, 2048, -1,
                                                     G_PARAM_READWRITE));
}


static void fm_cell_renderer_text_init(FmCellRendererText *self)
{
    self->height = -1;
}

/**
 * fm_cell_renderer_text_new
 *
 * Creates new #GtkCellRenderer object for #FmCellRendererText.
 *
 * Returns: a new #GtkCellRenderer object.
 *
 * Since: 0.1.0
 */
GtkCellRenderer *fm_cell_renderer_text_new(void)
{
    return (GtkCellRenderer*)g_object_new(FM_CELL_RENDERER_TEXT_TYPE, NULL);
}

static void fm_cell_renderer_text_get_property(GObject *object, guint param_id,
                                               GValue *value, GParamSpec *psec)
{
    FmCellRendererText *self = FM_CELL_RENDERER_TEXT(object);
    switch(param_id)
    {
    case PROP_HEIGHT:
        g_value_set_int(value, self->height);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, psec);
        break;
    }
}

static void fm_cell_renderer_text_set_property(GObject *object, guint param_id,
                                               const GValue *value, GParamSpec *psec)
{
    FmCellRendererText *self = FM_CELL_RENDERER_TEXT(object);
    switch(param_id)
    {
    case PROP_HEIGHT:
        self->height = g_value_get_int(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, psec);
        break;
    }
}

static void _get_size(GtkCellRenderer *cell, GtkWidget *widget,
                      PangoLayout *layout, gchar *text,
                      const GdkRectangle *cell_area,
                      gint *text_width, gint *text_height,
                      gint *xpad, gint *ypad,
                      gint *x_offset, gint *y_offset,
                      gint *x_align_offset)
{
    FmCellRendererText *self = FM_CELL_RENDERER_TEXT(cell);
    PangoWrapMode wrap_mode;
    gint wrap_width;
    PangoAlignment alignment;
    gfloat xalign, yalign;
    gint a_width, a_height;
    gint a_xpad, a_ypad;

    if (layout)
        g_object_ref(layout);
    else
        layout = pango_layout_new(gtk_widget_get_pango_context(widget));

    g_object_get(G_OBJECT(cell),
                 "wrap-mode" , &wrap_mode,
                 "wrap-width", &wrap_width,
                 "alignment" , &alignment,
                 NULL);

    pango_layout_set_alignment(layout, alignment);

    /* Setup the wrapping. */
    if (wrap_width < 0)
    {
        pango_layout_set_width(layout, -1);
        pango_layout_set_wrap(layout, PANGO_WRAP_CHAR);
    }
    else
    {
        pango_layout_set_width(layout, wrap_width * PANGO_SCALE);
        pango_layout_set_wrap(layout, wrap_mode);
        if(self->height > 0)
        {
            /* FIXME: add custom ellipsize from object? */
            pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
            pango_layout_set_height(layout, self->height * PANGO_SCALE);
        }
        else
            pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
    }

    pango_layout_set_text(layout, text, -1);

    pango_layout_set_auto_dir(layout, TRUE);

    if (!text_width)
        text_width = &a_width;
    if (!text_height)
        text_height = &a_height;
    pango_layout_get_pixel_size(layout, text_width, text_height);

    if (wrap_width > 0)
        *text_width = wrap_width;

    gtk_cell_renderer_get_alignment(cell, &xalign, &yalign);
    if (!xpad)
        xpad = &a_xpad;
    if (!ypad)
        ypad = &a_ypad;
    gtk_cell_renderer_get_padding(cell, xpad, ypad);
    /* Calculate the real x and y offsets. */
    if (x_offset)
    {
        *x_offset = ((gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL) ? (1.0 - xalign) : xalign)
                 * (cell_area->width - *text_width - (2 * *xpad));
        *x_offset = MAX(*x_offset, 0);
    }

    if (y_offset)
    {
        *y_offset = yalign * (cell_area->height - *text_height - (2 * *ypad));
        *y_offset = MAX (*y_offset, 0);
    }

    /* FIXME: this hack is ugly, need to rewrite this later */
    if (x_align_offset)
        *x_align_offset = (alignment == PANGO_ALIGN_CENTER) ? (wrap_width - *text_width) / 2 : 0;

    g_object_unref(layout);
}

#if GTK_CHECK_VERSION(3, 0, 0)
static void fm_cell_renderer_text_render(GtkCellRenderer *cell,
                                         cairo_t *cr,
                                         GtkWidget *widget,
                                         const GdkRectangle *background_area,
                                         const GdkRectangle *cell_area,
                                         GtkCellRendererState flags)
#else
static void fm_cell_renderer_text_render(GtkCellRenderer *cell,
                                         GdkDrawable *window,
                                         GtkWidget *widget,
                                         GdkRectangle *background_area,
                                         GdkRectangle *cell_area,
                                         GdkRectangle *expose_area,
                                         GtkCellRendererState flags)
#endif
{
#if GTK_CHECK_VERSION(3, 0, 0)
    GtkStyleContext* style;
    GtkStateFlags state;
#else
    GtkStyle* style;
    GtkStateType state;
#endif
    gchar* text;
    gint text_width;
    gint text_height;
    gint x_offset;
    gint y_offset;
    gint x_align_offset;
    GdkRectangle rect;
    gint xpad, ypad;

    /* FIXME: this is time-consuming since it invokes pango_layout.
     *        if we want to fix this, we must implement the whole cell
     *        renderer ourselves instead of derived from GtkCellRendererText. */
    PangoContext* context = gtk_widget_get_pango_context(widget);

    PangoLayout* layout = pango_layout_new(context);

    g_object_get(G_OBJECT(cell),
                 "text", &text,
                 NULL);

    _get_size(cell, widget, layout, text, cell_area, &text_width, &text_height,
              &xpad, &ypad, &x_offset, &y_offset, &x_align_offset);

    if(flags & (GTK_CELL_RENDERER_SELECTED|GTK_CELL_RENDERER_FOCUSED))
    {
        rect.x = cell_area->x + x_offset;
        rect.y = cell_area->y + y_offset;
        rect.width = text_width + (2 * xpad);
        rect.height = text_height + (2 * ypad);
    }

#if GTK_CHECK_VERSION(3, 0, 0)
    style = gtk_widget_get_style_context(widget);

    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_VIEW);
#else
    style = gtk_widget_get_style(widget);
#endif
    if(flags & GTK_CELL_RENDERER_SELECTED) /* item is selected */
    {
#if GTK_CHECK_VERSION(3, 0, 0)
        GdkRGBA clr;

        if(flags & GTK_CELL_RENDERER_INSENSITIVE) /* insensitive */
            state = GTK_STATE_FLAG_INSENSITIVE;
        else
            state = GTK_STATE_FLAG_SELECTED;

        gtk_style_context_get_background_color(style, state, &clr);
        gdk_cairo_rectangle(cr, &rect);
        gdk_cairo_set_source_rgba(cr, &clr);
#else
        cairo_t *cr = gdk_cairo_create (window);
        GdkColor clr;

        if(flags & GTK_CELL_RENDERER_INSENSITIVE) /* insensitive */
            state = GTK_STATE_INSENSITIVE;
        else
            state = GTK_STATE_SELECTED;

        clr = style->bg[state];

        /* paint the background */
        if(expose_area)
        {
            gdk_cairo_rectangle(cr, expose_area);
            cairo_clip(cr);
        }
        gdk_cairo_rectangle(cr, &rect);

        cairo_set_source_rgb(cr, clr.red / 65535., clr.green / 65535., clr.blue / 65535.);
#endif
        cairo_fill (cr);

#if !GTK_CHECK_VERSION(3, 0, 0)
        cairo_destroy (cr);
#endif
    }
#if !GTK_CHECK_VERSION(3, 0, 0)
    else
        state = GTK_STATE_NORMAL;
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_render_layout(style, cr,
                      cell_area->x + x_offset + xpad - x_align_offset,
                      cell_area->y + y_offset + ypad, layout);
#else
    gtk_paint_layout(style, window, state, TRUE,
                     expose_area, widget, "cellrenderertext",
                     cell_area->x + x_offset + xpad - x_align_offset,
                     cell_area->y + y_offset + ypad, layout);
#endif

    g_object_unref(layout);

    if(G_UNLIKELY( flags & GTK_CELL_RENDERER_FOCUSED) ) /* focused */
    {
#if GTK_CHECK_VERSION(3, 0, 0)
        gtk_render_focus(style, cr, rect.x, rect.y, rect.width, rect.height);
#else
        gtk_paint_focus(style, window, state, background_area,
                        widget, "cellrenderertext", rect.x, rect.y,
                        rect.width, rect.height);
#endif
    }

    if(flags & GTK_CELL_RENDERER_PRELIT) /* hovered */
        g_object_set(G_OBJECT(widget), "tooltip-text", text, NULL);
    else
        g_object_set(G_OBJECT(widget), "tooltip-text", NULL, NULL);
    g_free(text);

#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_style_context_restore(style);
#endif
}

static void fm_cell_renderer_text_get_size(GtkCellRenderer            *cell,
                                           GtkWidget                  *widget,
#if GTK_CHECK_VERSION(3, 0, 0)
                                           const
#endif
                                           GdkRectangle               *rectangle,
                                           gint                       *x_offset,
                                           gint                       *y_offset,
                                           gint                       *width,
                                           gint                       *height)
{
    char *text;

    g_object_get(G_OBJECT(cell), "text", &text, NULL);
    _get_size(cell, widget, NULL, text, rectangle, width, height, NULL, NULL,
              x_offset, y_offset, NULL);
    g_free(text);
}

#if GTK_CHECK_VERSION(3, 0, 0)
static void fm_cell_renderer_text_get_preferred_width(GtkCellRenderer *cell,
                                                      GtkWidget *widget,
                                                      gint *minimum_size,
                                                      gint *natural_size)
{
    gint wrap_width;

    g_object_get(G_OBJECT(cell), "wrap-width", &wrap_width, NULL);
    if(wrap_width > 0)
    {
        if(minimum_size)
            *minimum_size = wrap_width;
        if(natural_size)
            *natural_size = wrap_width;
    }
    else
        GTK_CELL_RENDERER_CLASS(fm_cell_renderer_text_parent_class)->get_preferred_width(cell, widget, minimum_size, natural_size);
}

static void fm_cell_renderer_text_get_preferred_height(GtkCellRenderer *cell,
                                                       GtkWidget *widget,
                                                       gint *minimum_size,
                                                       gint *natural_size)
{
    char *text;
    gint height;

    g_object_get(G_OBJECT(cell), "text", &text, NULL);
    _get_size(cell, widget, NULL, text, NULL, NULL, &height, NULL, NULL, NULL, NULL, NULL);
    g_free(text);

    if(natural_size && *natural_size > height)
        *natural_size = height;
    if(minimum_size && *minimum_size > height)
        *minimum_size = height;
}

static void fm_cell_renderer_text_get_preferred_height_for_width(GtkCellRenderer *cell,
                                                                 GtkWidget *widget,
                                                                 gint width,
                                                                 gint *minimum_height,
                                                                 gint *natural_height)
{
    fm_cell_renderer_text_get_preferred_height(cell, widget, minimum_height, natural_height);
}
#endif
