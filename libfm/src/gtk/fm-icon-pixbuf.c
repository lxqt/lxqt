/*
 *      fm-icon-pixbuf.c
 *      
 *      Copyright 2009 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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
 * SECTION:fm-icon-pixbuf
 * @short_description: An icon image creator.
 * @title: Icon image
 *
 * @include: libfm/fm-gtk.h
 *
 */

#include "fm-icon-pixbuf.h"
#include "fm.h"

static guint changed_handler = 0;

typedef struct _PixEntry
{
    int size;
    GdkPixbuf* pix;
}PixEntry;

static void destroy_pixbufs(gpointer data)
{
    GSList* pixs = (GSList*)data;
    GSList* l;
    for(l = pixs; l; l=l->next)
    {
        PixEntry* ent = (PixEntry*)l->data;
        if(G_LIKELY(ent->pix))
            g_object_unref(ent->pix);
        g_slice_free(PixEntry, ent);
    }
    g_slist_free(pixs);
}

/**
 * fm_pixbuf_from_icon
 * @icon: icon descriptor
 * @size: size in pixels
 *
 * Creates a #GdkPixbuf and draws icon there.
 *
 * Before 1.0.0 this call had name fm_icon_get_pixbuf.
 *
 * Returns: (transfer full): an image.
 *
 * Since: 0.1.0
 */
GdkPixbuf* fm_pixbuf_from_icon(FmIcon* icon, int size)
{
    return fm_pixbuf_from_icon_with_fallback(icon, size, NULL);
}

/**
 * fm_pixbuf_from_icon_with_fallback
 * @icon: icon descriptor
 * @size: size in pixels
 * @fallback: (allow-none): name of fallback icon
 *
 * Creates a #GdkPixbuf and draws icon there. If icon cannot be found then
 * icon with name @fallback will be loaded instead.
 *
 * Returns: (transfer full): an image.
 *
 * Since: 1.2.0
 */
GdkPixbuf* fm_pixbuf_from_icon_with_fallback(FmIcon* icon, int size, const char *fallback)
{
    GtkIconInfo* ii;
    GdkPixbuf* pix = NULL;
    GSList *pixs, *l;
    PixEntry* ent;

    /* FIXME:
        1) get/add GQuark for emblem type: no emblem is fm_qdata_id
        2) get/add GQuark by GQuark in the theme to support multi GdkScreen */
    pixs = (GSList*)g_object_steal_qdata(G_OBJECT(icon), fm_qdata_id);
    for( l = pixs; l; l=l->next )
    {
        ent = (PixEntry*)l->data;
        if(ent->size == size) /* cached pixbuf is found! */
        {
            /* return stealed data back */
            g_object_set_qdata_full(G_OBJECT(icon), fm_qdata_id, pixs, destroy_pixbufs);
            return ent->pix ? GDK_PIXBUF(g_object_ref(ent->pix)) : NULL;
        }
    }

    /* not found! load the icon from disk */
    ii = gtk_icon_theme_lookup_by_gicon(gtk_icon_theme_get_default(), G_ICON(icon), size, GTK_ICON_LOOKUP_FORCE_SIZE);
    if(ii)
    {
        pix = gtk_icon_info_load_icon(ii, NULL);
        gtk_icon_info_free(ii);

        /* increase ref_count to keep this pixbuf in memory
           even when no one is using it. */
        if(pix)
            g_object_ref(pix);
    }
    if (pix == NULL)
    {
        char* str = g_icon_to_string(G_ICON(icon));
        g_debug("unable to load icon %s", str);
        if(fallback)
            pix = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), fallback,
                    size, GTK_ICON_LOOKUP_USE_BUILTIN|GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
        if(pix == NULL) /* still unloadable */
            pix = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "unknown",
                    size, GTK_ICON_LOOKUP_USE_BUILTIN|GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
        if(G_LIKELY(pix))
            g_object_ref(pix);
        g_free(str);
    }

    /* cache this! */
    ent = g_slice_new(PixEntry);
    ent->size = size;
    ent->pix = pix;

    /* FIXME: maybe we should unload icons that nobody is using to reduce memory usage. */
    /* g_object_weak_ref(); */
    pixs = g_slist_prepend(pixs, ent);
    g_object_set_qdata_full(G_OBJECT(icon), fm_qdata_id, pixs, destroy_pixbufs);

    return pix;
}

static void on_icon_theme_changed(GtkIconTheme* theme, gpointer user_data)
{
    g_debug("icon theme changed!");
    /* unload pixbufs cached in FmIcon's hash table. */
    fm_icon_reset_user_data_cache(fm_qdata_id);
    /* FIXME: gtk_icon_theme_has_icon()+gtk_icon_theme_add_builtin_icon() for symlink emblem */
}

void _fm_icon_pixbuf_init()
{
    /* FIXME: GtkIconTheme object is different on different GdkScreen */
    GtkIconTheme* theme = gtk_icon_theme_get_default();
    changed_handler = g_signal_connect(theme, "changed", G_CALLBACK(on_icon_theme_changed), NULL);
}

void _fm_icon_pixbuf_finalize()
{
    GtkIconTheme* theme = gtk_icon_theme_get_default();
    g_signal_handler_disconnect(theme, changed_handler);
}
