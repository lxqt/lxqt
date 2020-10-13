/*
 *      fm-folder-config.c
 *
 *      This file is a part of the LibFM project.
 *
 *      Copyright 2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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
 * SECTION:fm-folder-config
 * @short_description: Folder specific settings cache.
 * @title: FmFolderConfig
 *
 * @include: libfm/fm.h
 *
 * This API represents access to folder-specific configuration settings.
 * Each setting is a key/value pair. To use it the descriptor should be
 * opened first, then required operations performed, then closed. Each
 * opened descriptor holds a lock on the cache so it is not adviced to
 * keep it somewhere.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-folder-config.h"

#include "fm-utils.h"

#include <errno.h>

struct _FmFolderConfig
{
    GKeyFile *kf;
    char *group; /* allocated if not in cache */
    char *filepath; /* NULL if in cache */
    gboolean changed;
};

static GKeyFile *fc_cache = NULL;

static gboolean fc_cache_changed = FALSE;

G_LOCK_DEFINE_STATIC(cache);

/**
 * fm_folder_config_open
 * @path: path to get config
 *
 * Searches for settings in the cache that are specific to @path. Locks
 * the cache. Returned descriptor can be used for access to settings.
 *
 * Returns: (transfer full): new configuration descriptor.
 *
 * Since: 1.2.0
 */
FmFolderConfig *fm_folder_config_open(FmPath *path)
{
    FmFolderConfig *fc = g_slice_new(FmFolderConfig);
    FmPath *sub_path;

    fc->changed = FALSE;
    /* clear .directory file first */
    sub_path = fm_path_new_child(path, ".directory");
    fc->filepath = fm_path_to_str(sub_path);
    fm_path_unref(sub_path);
    if (g_file_test(fc->filepath, G_FILE_TEST_EXISTS))
    {
        fc->kf = g_key_file_new();
        if (g_key_file_load_from_file(fc->kf, fc->filepath,
                                      G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                                      NULL) &&
            g_key_file_has_group(fc->kf, "File Manager"))
        {
            fc->group = "File Manager";
            return fc;
        }
        g_key_file_free(fc->kf);
    }
    g_free(fc->filepath);
    fc->filepath = NULL;
    fc->group = fm_path_to_str(path);
    G_LOCK(cache);
    fc->kf = fc_cache;
    return fc;
}

/**
 * fm_folder_config_close
 * @fc: a configuration descriptor
 * @error: (out) (allow-none): location to save error
 *
 * Unlocks the cache and releases any data related to @fc.
 *
 * Returns: %FALSE if any error happened on data save.
 *
 * Since: 1.2.0
 */
gboolean fm_folder_config_close(FmFolderConfig *fc, GError **error)
{
    gboolean ret = TRUE;

    if (fc->filepath)
    {
        if (fc->changed)
        {
            char *out;
            gsize len;

            out = g_key_file_to_data(fc->kf, &len, error);
            if (!out || !g_file_set_contents(fc->filepath, out, len, error))
                ret = FALSE;
            g_free(out);
        }
        g_free(fc->filepath);
        g_key_file_free(fc->kf);
    }
    else
    {
        g_free(fc->group);
        G_UNLOCK(cache);
        if (fc->changed)
            fc_cache_changed = TRUE;
    }

    g_slice_free(FmFolderConfig, fc);
    return ret;
}

/**
 * fm_folder_config_is_empty
 * @fc: a configuration descriptor
 *
 * Checks if there is no data associated with the folder.
 *
 * Returns: %TRUE if the folder has no settings.
 *
 * Since: 1.2.0
 */
gboolean fm_folder_config_is_empty(FmFolderConfig *fc)
{
    return !g_key_file_has_group(fc->kf, fc->group);
}

/**
 * fm_folder_config_get_integer
 * @fc: a configuration descriptor
 * @key: a key to search
 * @val: (out): location to save the value
 *
 * Returns the value associated with @key as an integer.
 *
 * Returns: %TRUE if key was found and parsed succesfully.
 *
 * Since: 1.2.0
 */
gboolean fm_folder_config_get_integer(FmFolderConfig *fc, const char *key,
                                      gint *val)
{
    return fm_key_file_get_int(fc->kf, fc->group, key, val);
}

/**
 * fm_folder_config_get_uint64
 * @fc: a configuration descriptor
 * @key: a key to search
 * @val: (out): location to save the value
 *
 * Returns the value associated with @key as an unsigned integer.
 *
 * Returns: %TRUE if key was found and value is an unsigned integer.
 *
 * Since: 1.2.0
 */
gboolean fm_folder_config_get_uint64(FmFolderConfig *fc, const char *key,
                                     guint64 *val)
{
    GError *error = NULL;
#if GLIB_CHECK_VERSION(2, 26, 0)
    guint64 ret = g_key_file_get_uint64(fc->kf, fc->group, key, &error);
#else
    gchar *s, *end;
    guint64 ret;

    s = g_key_file_get_value(fc->kf, fc->group, key, &error);
#endif
    if (error)
    {
        g_error_free(error);
        return FALSE;
    }
#if !GLIB_CHECK_VERSION(2, 26, 0)
    ret = g_ascii_strtoull(s, &end, 10);
    if (*s == '\0' || *end != '\0')
    {
        g_free(s);
        return FALSE;
    }
    g_free(s);
#endif
    *val = ret;
    return TRUE;
}

/**
 * fm_folder_config_get_double
 * @fc: a configuration descriptor
 * @key: a key to search
 * @val: (out): location to save the value
 *
 * Returns the value associated with @key as a double.
 *
 * Returns: %TRUE if key was found and value can be parsed as double.
 *
 * Since: 1.2.0
 */
gboolean fm_folder_config_get_double(FmFolderConfig *fc, const char *key,
                                     gdouble *val)
{
    GError *error = NULL;
    gdouble ret = g_key_file_get_double(fc->kf, fc->group, key, &error);
    if (error)
    {
        g_error_free(error);
        return FALSE;
    }
    *val = ret;
    return TRUE;
}

/**
 * fm_folder_config_get_boolean
 * @fc: a configuration descriptor
 * @key: a key to search
 * @val: (out): location to save the value
 *
 * Returns the value associated with @key as a boolean.
 *
 * Returns: %TRUE if key was found and parsed succesfully.
 *
 * Since: 1.2.0
 */
gboolean fm_folder_config_get_boolean(FmFolderConfig *fc, const char *key,
                                      gboolean *val)
{
    return fm_key_file_get_bool(fc->kf, fc->group, key, val);
}

/**
 * fm_folder_config_get_string
 * @fc: a configuration descriptor
 * @key: a key to search
 *
 * Returns the value associated with @key as a string. This function
 * handles escape sequences like \s.
 *
 * Returns: a newly allocated string or %NULL if the specified key cannot be found.
 *
 * Since: 1.2.0
 */
char *fm_folder_config_get_string(FmFolderConfig *fc, const char *key)
{
    return g_key_file_get_string(fc->kf, fc->group, key, NULL);
}

/**
 * fm_folder_config_get_string_list
 * @fc: a configuration descriptor
 * @key: a key to search
 * @length: (out) (allow-none): location for the number of returned strings
 *
 * Returns the values associated with @key. If the specified key cannot
 * be found then returns %NULL. Returned data array should be freed with
 * g_strfreev() after usage.
 *
 * Returns: a %NULL-terminated string array.
 *
 * Since: 1.2.0
 */
char **fm_folder_config_get_string_list(FmFolderConfig *fc,
                                        const char *key, gsize *length)
{
    return g_key_file_get_string_list(fc->kf, fc->group, key, length, NULL);
}

/**
 * fm_folder_config_set_integer
 * @fc: a configuration descriptor
 * @key: a key to search
 * @val: data to set
 *
 * Associates integer @val with @key for given folder configuration.
 *
 * Since: 1.2.0
 */
void fm_folder_config_set_integer(FmFolderConfig *fc, const char *key, gint val)
{
    fc->changed = TRUE;
    g_key_file_set_integer(fc->kf, fc->group, key, val);
}

/**
 * fm_folder_config_set_uint64
 * @fc: a configuration descriptor
 * @key: a key to search
 * @val: data to set
 *
 * Associates unsigned integer @val with @key for given folder configuration.
 *
 * Since: 1.2.0
 */
void fm_folder_config_set_uint64(FmFolderConfig *fc, const char *key, guint64 val)
{
    fc->changed = TRUE;
#if GLIB_CHECK_VERSION(2, 26, 0)
    g_key_file_set_uint64(fc->kf, fc->group, key, val);
#else
    gchar *result = g_strdup_printf("%" G_GUINT64_FORMAT, val);
    g_key_file_set_value(fc->kf, fc->group, key, result);
    g_free(result);
#endif
}

/**
 * fm_folder_config_set_double
 * @fc: a configuration descriptor
 * @key: a key to search
 * @val: data to set
 *
 * Associates double @val with @key for given folder configuration.
 *
 * Since: 1.2.0
 */
void fm_folder_config_set_double(FmFolderConfig *fc, const char *key, gdouble val)
{
    fc->changed = TRUE;
    g_key_file_set_double(fc->kf, fc->group, key, val);
}

/**
 * fm_folder_config_set_boolean
 * @fc: a configuration descriptor
 * @key: a key to search
 * @val: data to set
 *
 * Associates boolean @val with @key for given folder configuration.
 *
 * Since: 1.2.0
 */
void fm_folder_config_set_boolean(FmFolderConfig *fc, const char *key,
                                  gboolean val)
{
    fc->changed = TRUE;
    g_key_file_set_boolean(fc->kf, fc->group, key, val);
}

/**
 * fm_folder_config_set_string
 * @fc: a configuration descriptor
 * @key: a key to search
 * @string: data to set
 *
 * Associates @string with @key for given folder configuration. This
 * function handles characters that need escaping, such as newlines.
 *
 * Since: 1.2.0
 */
void fm_folder_config_set_string(FmFolderConfig *fc, const char *key,
                                 const char *string)
{
    fc->changed = TRUE;
    g_key_file_set_string(fc->kf, fc->group, key, string);
}

/**
 * fm_folder_config_set_string_list
 * @fc: a configuration descriptor
 * @key: a key to search
 * @list: a string list to set
 * @length: number of elements in @list
 *
 * Associates NULL-terminated @list with @key for given folder configuration.
 *
 * Since: 1.2.0
 */
void fm_folder_config_set_string_list(FmFolderConfig *fc, const char *key,
                                      const gchar * const list[], gsize length)
{
    fc->changed = TRUE;
    g_key_file_set_string_list(fc->kf, fc->group, key, list, length);
}

/**
 * fm_folder_config_remove_key
 * @fc: a configuration descriptor
 * @key: a key to search
 *
 * Removes the key and associated data from the cache.
 *
 * Since: 1.2.0
 */
void fm_folder_config_remove_key(FmFolderConfig *fc, const char *key)
{
    fc->changed = TRUE;
    g_key_file_remove_key(fc->kf, fc->group, key, NULL);
}

/**
 * fm_folder_config_purge
 * @fc: a configuration descriptor
 *
 * Clears all the data for the folder from the configuration.
 *
 * Since: 1.2.0
 */
void fm_folder_config_purge(FmFolderConfig *fc)
{
    fc->changed = TRUE;
    g_key_file_remove_group(fc->kf, fc->group, NULL);
}

/**
 * fm_folder_config_save_cache
 *
 * Saves current data into the cache file.
 *
 * Since: 1.2.0
 */
void fm_folder_config_save_cache(void)
{
    char *out;
    char *path, *path2, *path3;
    GError *error = NULL;
    gsize len;

    G_LOCK(cache);
    /* if per-directory cache was changed since last invocation then save it */
    if (fc_cache_changed && (out = g_key_file_to_data(fc_cache, &len, NULL)))
    {
        /* FIXME: create dir */
        /* create temp file with settings */
        path = g_build_filename(g_get_user_config_dir(), "libfm/dir-settings.conf", NULL);
        path2 = g_build_filename(g_get_user_config_dir(), "libfm/dir-settings.tmp", NULL);
        path3 = g_build_filename(g_get_user_config_dir(), "libfm/dir-settings.backup", NULL);
        /* do safe replace now, the file is important enough to be lost */
        if (g_file_set_contents(path2, out, len, &error))
        {
            /* backup old cache file */
            g_unlink(path3);
            if (!g_file_test(path, G_FILE_TEST_EXISTS) ||
                g_rename(path, path3) == 0)
            {
                /* rename temp file */
                if (g_rename(path2, path) == 0)
                {
                    /* success! remove the old cache file */
                    g_unlink(path3);
                    /* reset the 'changed' flag */
                    fc_cache_changed = FALSE;
                }
                else
                    g_warning("cannot rename %s to %s: %s", path2, path,
                              g_strerror(errno));
            }
            else
                g_warning("cannot rename %s to %s: %s", path, path3,
                          g_strerror(errno));
        }
        else
        {
            g_warning("cannot save %s: %s", path2, error->message);
            g_error_free(error);
        }
        g_free(path);
        g_free(path2);
        g_free(path3);
        g_free(out);
    }
    G_UNLOCK(cache);
}

void _fm_folder_config_finalize(void)
{
    fm_folder_config_save_cache();
    g_key_file_free(fc_cache);
    fc_cache = NULL;
}

void _fm_folder_config_init(void)
{
    char *path = g_build_filename(g_get_user_config_dir(),
                                  "libfm/dir-settings.conf", NULL);
    fc_cache = g_key_file_new();
    g_key_file_load_from_file(fc_cache, path, 0, NULL);
    g_free(path);
}
