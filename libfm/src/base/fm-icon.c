/*
 *      fm-icon.c
 *
 *      Copyright 2009 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2013-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This file is a part of the Libfm library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:fm-icon
 * @short_description: A simple icons cache.
 * @title: FmIcon
 *
 * @include: libfm/fm.h
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm.h"

#include <string.h>

static GHashTable* hash = NULL;
G_LOCK_DEFINE_STATIC(hash);

static GDestroyNotify destroy_func = NULL;

void _fm_icon_init()
{
    if(G_UNLIKELY(hash))
        return;
    hash = g_hash_table_new_full(g_icon_hash, (GEqualFunc)g_icon_equal,
                                 g_object_unref, NULL);
}

void _fm_icon_finalize()
{
    g_hash_table_destroy(hash);
    hash = NULL;
}

/**
 * fm_icon_from_gicon
 * @gicon: a #GIcon object
 *
 * Retrives a #FmIcon corresponding to @gicon from cache inserting new
 * one if there was no such icon there yet.
 *
 * Returns: (transfer full): a #FmIcon object.
 *
 * Since: 0.1.0
 */
FmIcon* fm_icon_from_gicon(GIcon* gicon)
{
    FmIcon* icon;
    G_LOCK(hash);
    icon = (FmIcon*)g_hash_table_lookup(hash, gicon);
    if(G_UNLIKELY(!icon))
    {
        icon = (FmIcon*)g_object_ref(gicon);
        g_hash_table_insert(hash, gicon, icon);
    }
    G_UNLOCK(hash);
    return g_object_ref(icon);
}

/**
 * fm_icon_from_name
 * @name: a name for icon
 *
 * Retrives a #FmIcon corresponding to @name from cache inserting new
 * one if there was no such icon there yet.
 *
 * Returns: (transfer full): a #FmIcon object.
 *
 * Since: 0.1.0
 */
FmIcon* fm_icon_from_name(const char* name)
{
    if(G_LIKELY(name))
    {
        FmIcon* icon;
        GIcon* gicon;
        gchar *dot;
        if(g_path_is_absolute(name))
        {
            GFile* gicon_file = g_file_new_for_path(name);
            gicon = g_file_icon_new(gicon_file);
            g_object_unref(gicon_file);
        }
        else if(G_UNLIKELY((dot = strrchr(name, '.')) != NULL && dot > name &&
                (g_ascii_strcasecmp(&dot[1], "png") == 0
                 || g_ascii_strcasecmp(&dot[1], "svg") == 0
                 || g_ascii_strcasecmp(&dot[1], "xpm") == 0)))
        {
            /* some desktop entries have invalid icon name which contains
               suffix so let strip the suffix from such invalid name */
            dot = g_strndup(name, dot - name);
            gicon = g_themed_icon_new_with_default_fallbacks(dot);
            g_free(dot);
        }
        else
            gicon = g_themed_icon_new_with_default_fallbacks(name);

        if(G_LIKELY(gicon))
        {
            icon = fm_icon_from_gicon(gicon);
            g_object_unref(gicon);
            return icon;
        }
    }
    return NULL;
}

/**
 * fm_icon_ref
 * @icon: an existing #FmIcon object
 *
 * Increases reference count on @icon.
 *
 * Returns: @icon.
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.2.0: Use g_object_ref() instead.
 */
FmIcon* fm_icon_ref(FmIcon* icon)
{
    return g_object_ref(icon);
}

/**
 * fm_icon_unref
 * @icon: a #FmIcon object
 *
 * Decreases reference count on @icon. If refernce count went to 0 then
 * removes @icon from cache.
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.2.0: Use g_object_unref() instead.
 */
void fm_icon_unref(FmIcon* icon)
{
    g_object_unref(icon);
}

/**
 * fm_icon_unload_cache
 *
 * Flushes cache.
 *
 * Since: 0.1.0
 */
void fm_icon_unload_cache(void)
{
    G_LOCK(hash);
    g_hash_table_remove_all(hash);
    G_UNLOCK(hash);
}

static void unload_user_data_cache(GIcon* key, FmIcon* icon, gpointer quark)
{
    g_object_set_qdata(G_OBJECT(icon), (guint32)(gulong)quark, NULL);
}

/**
 * fm_icon_unload_user_data_cache
 *
 * Flushes all user data in cache.
 *
 * See also: fm_icon_set_user_data().
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.2.0: Use fm_icon_reset_user_data_cache() instead.
 */
void fm_icon_unload_user_data_cache(void)
{
    G_LOCK(hash);
    g_hash_table_foreach(hash, (GHFunc)unload_user_data_cache, (gpointer)(gulong)fm_qdata_id);
    G_UNLOCK(hash);
}

/**
 * fm_icon_reset_user_data_cache
 * @quark: the associated key for user data
 *
 * Flushes all user data by @quark in cache.
 *
 * Since: 1.2.0
 */
void fm_icon_reset_user_data_cache(GQuark quark)
{
    G_LOCK(hash);
    g_hash_table_foreach(hash, (GHFunc)unload_user_data_cache, (gpointer)(gulong)quark);
    G_UNLOCK(hash);
}

/**
 * fm_icon_get_user_data
 * @icon: a #FmIcon object
 *
 * Retrieves user data that was set via fm_icon_set_user_data().
 *
 * Returns: user data.
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.2.0: Use g_object_get_qdata() instead.
 */
gpointer fm_icon_get_user_data(FmIcon* icon)
{
    return g_object_get_qdata(G_OBJECT(icon), fm_qdata_id);
}

/**
 * fm_icon_set_user_data
 * @icon: a #FmIcon object
 * @user_data: data pointer to set
 *
 * Sets @user_data to be associated with @icon.
 *
 * See also: fm_icon_get_user_data(), fm_icon_unload_user_data_cache().
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.2.0: Use g_object_set_qdata_full() instead.
 */
void fm_icon_set_user_data(FmIcon* icon, gpointer user_data)
{
    /* this old API meant to replace data not freeing it */
    g_object_steal_qdata(G_OBJECT(icon), fm_qdata_id);
    g_object_set_qdata_full(G_OBJECT(icon), fm_qdata_id, user_data, destroy_func);
}

/**
 * fm_icon_set_user_data_destroy
 * @func: function for user data
 *
 * Sets @func to be used by fm_icon_unload_user_data_cache() to destroy
 * user data that was set by fm_icon_set_user_data().
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.2.0:
 */
static gboolean reload_user_data_cache(GIcon* key, FmIcon* icon, gpointer unused)
{
    /* reset destroy_func for data -- compatibility */
    gpointer user_data = g_object_steal_qdata(G_OBJECT(icon), fm_qdata_id);
    if (user_data)
        g_object_set_qdata_full(G_OBJECT(icon), fm_qdata_id, user_data, destroy_func);
    return TRUE;
}

void fm_icon_set_user_data_destroy(GDestroyNotify func)
{
    G_LOCK(hash);
    destroy_func = func;
    g_hash_table_foreach(hash, (GHFunc)reload_user_data_cache, NULL);
    G_UNLOCK(hash);
}
