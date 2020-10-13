/*
 *      fm-cell-renderer-pixbuf.c
 *      
 *      Copyright 2010 PCMan <pcman.tw@gmail.com>
 *      Copyright 2013-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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
 * SECTION:fm-cell-renderer-pixbuf
 * @short_description: Extended pixbuf cell renderer.
 * @title: FmCellRendererPixbuf
 *
 * @include: libfm/fm-gtk.h
 *
 * The #FmCellRendererPixbuf is extended version of #GtkCellRendererPixbuf
 * which adds small link picture if corresponding file is symbolic link.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define FM_DISABLE_SEAL

#include "fm-config.h"

#include "fm-cell-renderer-pixbuf.h"

static void fm_cell_renderer_pixbuf_dispose  (GObject *object);

static void fm_cell_renderer_pixbuf_get_size   (GtkCellRenderer            *cell,
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
static void fm_cell_renderer_pixbuf_render(GtkCellRenderer *cell,
                                           cairo_t *cr,
                                           GtkWidget *widget,
                                           const GdkRectangle *background_area,
                                           const GdkRectangle *cell_area,
                                           GtkCellRendererState flags);
#else
static void fm_cell_renderer_pixbuf_render     (GtkCellRenderer            *cell,
						 GdkDrawable                *window,
						 GtkWidget                  *widget,
						 GdkRectangle               *background_area,
						 GdkRectangle               *cell_area,
						 GdkRectangle               *expose_area,
						 GtkCellRendererState        flags);
#endif

static void fm_cell_renderer_pixbuf_get_property ( GObject *object,
                                      guint param_id,
                                      GValue *value,
                                      GParamSpec *pspec );

static void fm_cell_renderer_pixbuf_set_property ( GObject *object,
                                      guint param_id,
                                      const GValue *value,
                                      GParamSpec *pspec );

enum
{
    PROP_INFO = 1,
    N_PROPS
};

G_DEFINE_TYPE(FmCellRendererPixbuf, fm_cell_renderer_pixbuf, GTK_TYPE_CELL_RENDERER_PIXBUF);

static GdkPixbuf* link_icon = NULL;

/* GdkPixbuf RGBA C-Source image dump */
#ifdef __SUNPRO_C
#pragma align 4 (link_icon_data)
#endif
#ifdef __GNUC__
static const guint8 link_icon_data[] __attribute__ ((__aligned__ (4))) =
#else
static const guint8 link_icon_data[] =
#endif
    { ""
      /* Pixbuf magic (0x47646b50) */
      "GdkP"
      /* length: header (24) + pixel_data (400) */
      "\0\0\1\250"
      /* pixdata_type (0x1010002) */
      "\1\1\0\2"
      /* rowstride (40) */
      "\0\0\0("
      /* width (10) */
      "\0\0\0\12"
      /* height (10) */
      "\0\0\0\12"
      /* pixel_data: */
      "\200\200\200\377\200\200\200\377\200\200\200\377\200\200\200\377\200"
      "\200\200\377\200\200\200\377\200\200\200\377\200\200\200\377\200\200"
      "\200\377\0\0\0\377\200\200\200\377\377\377\377\377\377\377\377\377\377"
      "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
      "\377\377\377\377\377\377\0\0\0\377\200\200\200\377\377\377\377\377\0"
      "\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\377\377\377\377\377"
      "\377\377\377\0\0\0\377\200\200\200\377\377\377\377\377\0\0\0\377\0\0"
      "\0\377\0\0\0\377\0\0\0\377\377\377\377\377\377\377\377\377\377\377\377"
      "\377\0\0\0\377\200\200\200\377\377\377\377\377\0\0\0\377\0\0\0\377\0"
      "\0\0\377\0\0\0\377\0\0\0\377\377\377\377\377\377\377\377\377\0\0\0\377"
      "\200\200\200\377\377\377\377\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0"
      "\377\0\0\0\377\0\0\0\377\377\377\377\377\0\0\0\377\200\200\200\377\377"
      "\377\377\377\0\0\0\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0"
      "\0\377\0\0\0\377\377\377\377\377\0\0\0\377\200\200\200\377\377\377\377"
      "\377\377\377\377\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0"
      "\377\0\0\0\377\377\377\377\377\0\0\0\377\200\200\200\377\377\377\377"
      "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
      "\377\377\377\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\377"
      "\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377"
      "\0\0\0\377\0\0\0\377"
    };


static void fm_cell_renderer_pixbuf_class_init(FmCellRendererPixbufClass *klass)
{
    GObjectClass *g_object_class = G_OBJECT_CLASS(klass);
    GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS(klass);

    g_object_class->dispose = fm_cell_renderer_pixbuf_dispose;
    g_object_class->get_property = fm_cell_renderer_pixbuf_get_property;
    g_object_class->set_property = fm_cell_renderer_pixbuf_set_property;

    cell_class->get_size = fm_cell_renderer_pixbuf_get_size;
    cell_class->render = fm_cell_renderer_pixbuf_render;

    /**
     * FmCellRendererPixbuf:info:
     *
     * The #FmCellRendererPixbuf:info property is the #FmFileInfo
     * of file that this object corresponds to.
     */
    g_object_class_install_property ( g_object_class,
                                      PROP_INFO,
                                      g_param_spec_pointer ( "info",
                                                             "File info",
                                                             "File info",
                                                             G_PARAM_READWRITE ) );
}


static void fm_cell_renderer_pixbuf_dispose(GObject *object)
{
    FmCellRendererPixbuf *self;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_CELL_RENDERER_PIXBUF(object));

    self = FM_CELL_RENDERER_PIXBUF(object);
    if( self->fi )
    {
        fm_file_info_unref(self->fi);
        self->fi = NULL;
    }
    if(self->icon)
    {
        g_object_unref(self->icon);
        self->icon = NULL;
    }

    G_OBJECT_CLASS(fm_cell_renderer_pixbuf_parent_class)->dispose(object);
}


static void fm_cell_renderer_pixbuf_init(FmCellRendererPixbuf *self)
{
    if( !link_icon )
    {
        link_icon = gdk_pixbuf_new_from_inline(
                            sizeof(link_icon_data),
                            link_icon_data,
                            FALSE, NULL );
        g_object_add_weak_pointer(G_OBJECT(link_icon), (gpointer)&link_icon);
        self->icon = link_icon;
    }
    else
        self->icon = g_object_ref(link_icon);
}

/**
 * fm_cell_renderer_pixbuf_new
 *
 * Creates new #FmCellRendererPixbuf object.
 *
 * Returns: (transfer full): a new #FmCellRendererPixbuf object.
 *
 * Since: 0.1.0
 */
FmCellRendererPixbuf *fm_cell_renderer_pixbuf_new(void)
{
    return g_object_new(FM_TYPE_CELL_RENDERER_PIXBUF, NULL);
}

static void fm_cell_renderer_pixbuf_get_property ( GObject *object,
                                      guint param_id,
                                      GValue *value,
                                      GParamSpec *psec )
{
    FmCellRendererPixbuf* renderer = FM_CELL_RENDERER_PIXBUF(object);
    switch( param_id )
    {
    case PROP_INFO:
        g_value_set_pointer(value, renderer->fi ? fm_file_info_ref(renderer->fi) : NULL);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, param_id, psec );
        break;
    }
}

static void fm_cell_renderer_pixbuf_set_property ( GObject *object,
                                      guint param_id,
                                      const GValue *value,
                                      GParamSpec *psec )
{
    FmCellRendererPixbuf* renderer = FM_CELL_RENDERER_PIXBUF(object);
    FmFileInfo *fi;
    switch ( param_id )
    {
    case PROP_INFO:
        if( renderer->fi )
            fm_file_info_unref(renderer->fi);
        fi = FM_FILE_INFO(g_value_get_pointer(value));
        if (fi)
            fm_file_info_ref(fi);
        renderer->fi = fi;
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, param_id, psec );
        break;
    }
}

/**
 * fm_cell_renderer_pixbuf_set_fixed_size
 * @render: the renderer object
 * @w: new fixed width
 * @h: new fixed height
 *
 * Sets fixed width and height for rendered object.
 *
 * Since: 0.1.0
 */
void fm_cell_renderer_pixbuf_set_fixed_size(FmCellRendererPixbuf* render, gint w, gint h)
{
    render->fixed_w = w;
    render->fixed_h = h;
}

static void fm_cell_renderer_pixbuf_get_size   (GtkCellRenderer            *cell,
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
    FmCellRendererPixbuf* render = FM_CELL_RENDERER_PIXBUF(cell);
    if(render->fixed_w > 0 && render->fixed_h > 0)
    {
        *width = render->fixed_w;
        *height = render->fixed_h;
    }
    else
    {
        GTK_CELL_RENDERER_CLASS(fm_cell_renderer_pixbuf_parent_class)->get_size(cell, widget, rectangle, x_offset, y_offset, width, height);
    }
}

#if GTK_CHECK_VERSION(3, 0, 0)
static void fm_cell_renderer_pixbuf_render(GtkCellRenderer *cell,
                                           cairo_t *cr,
                                           GtkWidget *widget,
                                           const GdkRectangle *background_area,
                                           const GdkRectangle *cell_area,
                                           GtkCellRendererState flags)
#else
static void fm_cell_renderer_pixbuf_render     (GtkCellRenderer            *cell,
						 GdkDrawable                *window,
						 GtkWidget                  *widget,
						 GdkRectangle               *background_area,
						 GdkRectangle               *cell_area,
						 GdkRectangle               *expose_area,
						 GtkCellRendererState        flags)
#endif
{
    FmCellRendererPixbuf* render = FM_CELL_RENDERER_PIXBUF(cell);
    FmFileInfo *info = NULL;
    if(fm_config->shadow_hidden)
    {
        g_object_get(render, "info", &info, NULL); // FIXME: is info certainly FmFileInfo?
        gtk_cell_renderer_set_sensitive(cell, !(info && fm_file_info_is_hidden(info)));
    }
#if GTK_CHECK_VERSION(3, 0, 0)
    GTK_CELL_RENDERER_CLASS(fm_cell_renderer_pixbuf_parent_class)->render(cell, cr, widget, background_area, cell_area, flags);
#else
    /* we don't need to follow state for prelit items */
    if(flags & GTK_CELL_RENDERER_PRELIT)
        flags &= ~GTK_CELL_RENDERER_PRELIT;
    GTK_CELL_RENDERER_CLASS(fm_cell_renderer_pixbuf_parent_class)->render(cell, window, widget, background_area, cell_area, expose_area, flags);
#endif

    /* FIXME: this should be handled by fm_pixbuf_* and the model instead */
    if(render->fi &&
       G_UNLIKELY(fm_file_info_is_symlink(render->fi) || fm_file_info_is_shortcut(render->fi)))
    {
        GdkPixbuf* pix;
        g_object_get(render, "pixbuf", &pix, NULL);
        if(pix)
        {
#if !GTK_CHECK_VERSION(3, 0, 0)
            cairo_t *cr = gdk_cairo_create(window);
#endif
            int x = cell_area->x + (cell_area->width - gdk_pixbuf_get_width(pix))/2;
            int y = cell_area->y + (cell_area->height - gdk_pixbuf_get_height(pix))/2;

            if (cell_area->width >= 20 && render->fixed_w >= 20)
                gdk_cairo_set_source_pixbuf(cr, link_icon, x, y);
            else
            {
                gint scale_width = MIN(cell_area->width, render->fixed_w) / 2 + 1;
                GdkPixbuf *scaled = gdk_pixbuf_scale_simple(link_icon,
                                                            scale_width,
                                                            scale_width,
                                                            GDK_INTERP_TILES);
                gdk_cairo_set_source_pixbuf(cr, scaled, x, y);
                g_object_unref(scaled);
            }
            cairo_paint(cr);
#if !GTK_CHECK_VERSION(3, 0, 0)
            cairo_destroy(cr);
#endif
            g_object_unref(pix);
        }
    }
}
