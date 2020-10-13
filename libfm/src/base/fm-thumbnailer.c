/*
 *      fm-thumbnailer.c
 *
 *      Copyright 2012-2013 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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
 * SECTION:fm-thumbnailer
 * @short_description: External thumbnailers handling.
 * @title: FmThumbnailer
 *
 * @include: libfm/fm.h
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-thumbnailer.h"
#include "fm-mime-type.h"
#include "glib-compat.h"

#include <glib/gi18n-lib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

struct _FmThumbnailer
{
    char* id;
    char* try_exec; /* FIXME: is this useful? */
    char* exec;
    GList* mime_types;
    gint n_ref;
};

time_t last_loaded_time = 0;
GList* all_thumbnailers = NULL;
G_LOCK_DEFINE_STATIC(all_thumbnailers);

/**
 * fm_thumbnailer_ref
 * @thumbnailer: thumbnailer descriptor
 *
 * Increases reference count for @thumbnailer object.
 *
 * Returns: @thumbnailer.
 *
 * Since: 1.2.0
 */
FmThumbnailer* fm_thumbnailer_ref(FmThumbnailer* thumbnailer)
{
    g_return_val_if_fail(thumbnailer, NULL);
        g_atomic_int_inc(&thumbnailer->n_ref);
    return thumbnailer;
}

/**
 * fm_thumbnailer_free
 * @thumbnailer: thumbnailer descriptor
 *
 * Frees @thumbnailer object.
 *
 * Since: 1.0.0
 *
 * Deprecated: 1.2.0: Use fm_thumbnailer_unref() instead.
 */
void fm_thumbnailer_free(FmThumbnailer* thumbnailer)
{
    GList* l;
    g_return_if_fail(thumbnailer);
    g_free(thumbnailer->id);
    g_free(thumbnailer->try_exec);
    g_free(thumbnailer->exec);
    for(l = thumbnailer->mime_types; l; l = l->next)
    {
        FmMimeType* mime_type = (FmMimeType*)l->data;
        /* remove ourself from FmMimeType */
        fm_mime_type_remove_thumbnailer(mime_type, thumbnailer);
        fm_mime_type_unref(mime_type);
    }
    g_list_free(thumbnailer->mime_types);
    g_slice_free(FmThumbnailer, thumbnailer);
}

/**
 * fm_thumbnailer_unref
 * @thumbnailer: thumbnailer descriptor
 *
 * Decreases reference count for @thumbnailer object. When last reference
 * is dropped, allocated resources will be freed and @thumbnailer becomes
 * invalid.
 *
 * Since: 1.2.0
 */
void fm_thumbnailer_unref(FmThumbnailer* thumbnailer)
{
    if (g_atomic_int_dec_and_test(&thumbnailer->n_ref))
        fm_thumbnailer_free(thumbnailer);
}

/**
 * fm_thumbnailer_new_from_keyfile
 * @id: desktop entry Id
 * @kf: content of @id
 *
 * Creates new @thumbnailer object.
 *
 * Returns: (transfer full): a new #FmThumbnailer or %NULL in case of error.
 *
 * Since: 1.0.0
 */
FmThumbnailer* fm_thumbnailer_new_from_keyfile(const char* id, GKeyFile* kf)
{
    FmThumbnailer* thumbnailer = NULL;
    char* exec = g_key_file_get_string(kf, "Thumbnailer Entry", "Exec", NULL);
    if(exec)
    {
        char** mime_types = g_key_file_get_string_list(kf, "Thumbnailer Entry", "MimeType", NULL, NULL);
        if(mime_types)
        {
            char** mime_type_name;
            thumbnailer = g_slice_new0(FmThumbnailer);
            thumbnailer->id = g_strdup(id);
            thumbnailer->exec = exec;
            thumbnailer->try_exec = g_key_file_get_string(kf, "Thumbnailer Entry", "TryExec", NULL);
            thumbnailer->n_ref = 1;

            for(mime_type_name = mime_types; *mime_type_name; ++mime_type_name)
            {
                FmMimeType* mime_type = fm_mime_type_from_name(*mime_type_name);
                if(mime_type)
                {
                    /* here we only add items to mime_type->thumbnailers list and do not
                     * add reference to FmThumbnailer. FmMimeType does not own it
                     * and will not unref FmThumbnailer when FmMimeType object is
                     * freed. We need to do it this way. Otherwise, mutual reference
                     * of FmMimeType and FmThumbnailer objects will cause cyclic
                     * reference. */
                    fm_mime_type_add_thumbnailer(mime_type, thumbnailer);

                    /* Do not call fm_mime_type_unref() here so we own a reference
                     * to the FmMimeType object */
                    thumbnailer->mime_types = g_list_prepend(thumbnailer->mime_types, mime_type);
                }
            }
            g_strfreev(mime_types);
        }
        else
            g_free(exec);
    }
    return thumbnailer;
}

/**
 * fm_thumbnailer_command_for_uri
 * @thumbnailer: thumbnailer descriptor
 * @uri: a file to create thumbnail for
 * @output_file: the target file name
 * @size: size of thumbnail to generate
 *
 * Tries to generate command line that can be used to generate a thumbnail
 * for given @uri. Returned string is allocated and should be freed with
 * g_free() after usage.
 *
 * Returns: (transfer full): a newly allocated string or %NULL.
 *
 * Since: 1.2.0
 */
char* fm_thumbnailer_command_for_uri(FmThumbnailer* thumbnailer, const char* uri, const char* output_file, guint size)
{
    if(thumbnailer && thumbnailer->exec)
    {
        /* FIXME: how to handle TryExec? */

        /* parse the command line and do required substitutions according to:
         * http://developer.gnome.org/integration-guide/stable/thumbnailer.html.en
         */
        GString* cmd_line = g_string_sized_new(1024);
        const char* p;
        for(p = thumbnailer->exec; *p; ++p)
        {
            if(G_LIKELY(*p != '%'))
                g_string_append_c(cmd_line, *p);
            else
            {
                char* quoted;
                ++p;
                switch(*p)
                {
                case '\0':
                    break;
                case 's':
                    g_string_append_printf(cmd_line, "%d", size);
                    break;
                case 'i':
                {
                    char* src_path = g_filename_from_uri(uri, NULL, NULL);
                    if(src_path)
                    {
                        quoted = g_shell_quote(src_path);
                        g_string_append(cmd_line, quoted);
                        g_free(quoted);
                        g_free(src_path);
                    }
                    break;
                }
                case 'u':
                    quoted = g_shell_quote(uri);
                    g_string_append(cmd_line, quoted);
                    g_free(quoted);
                    break;
                case 'o':
                    g_string_append(cmd_line, output_file);
                    break;
                default:
                    g_string_append_c(cmd_line, '%');
                    if(*p != '%')
                        g_string_append_c(cmd_line, *p);
                }
            }
        }
        return g_string_free(cmd_line, FALSE);
    }
    return NULL;
}

/**
 * fm_thumbnailer_launch_for_uri_async
 * @thumbnailer: thumbnailer descriptor
 * @uri: a file to create thumbnail for
 * @output_file: the target file name
 * @size: size of thumbnail to generate
 * @error: (allow-none) (out): location to save error
 *
 * Tries to spawn thumbnailer to generate new thumbnail for given @uri.
 *
 * Returns: thumbnailer process ID or -1 in case of failure.
 *
 * Since: 1.2.0
 */
GPid fm_thumbnailer_launch_for_uri_async(FmThumbnailer* thumbnailer,
                                         const char* uri,
                                         const char* output_file, guint size,
                                         GError** error)
{
    GPid pid = -1;
    char* cmd_line = fm_thumbnailer_command_for_uri(thumbnailer, uri, output_file, size);
    /* FIXME: check for mandatory arguments */
    if(cmd_line)
    {
        int argc;
        char** argv;
        if(g_shell_parse_argv(cmd_line, &argc, &argv, NULL))
        {
            g_spawn_async("/", argv, NULL,
                G_SPAWN_SEARCH_PATH|G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_DO_NOT_REAP_CHILD,
                NULL, NULL, &pid, error);
            g_strfreev(argv);
        }
        /* g_print("pid = %d, %s", pid, cmd_line); */
        g_free(cmd_line);
    }
    else
        g_set_error_literal(error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                            _("Invalid description of thumbnailer application"));
    return pid;
}

/**
 * fm_thumbnailer_launch_for_uri
 * @thumbnailer: thumbnailer descriptor
 * @uri: a file to create thumbnail for
 * @output_file: the target file name
 * @size: size of thumbnail to generate
 *
 * Tries to generate new thumbnail for given @uri.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 1.0.0
 *
 * Deprecated: 1.2.0: Use fm_thumbnailer_launch_for_uri_async() instead.
 */
gboolean fm_thumbnailer_launch_for_uri(FmThumbnailer* thumbnailer, const char* uri,  const char* output_file, guint size)
{
    char* cmd_line = fm_thumbnailer_command_for_uri(thumbnailer, uri, output_file, size);
    if(cmd_line)
    {
        int status;
        /* TODO: this call is blocking. Do we have a better way to make it async? */
        g_spawn_command_line_sync(cmd_line, NULL, NULL, &status, NULL);
        /* g_debug("launch thumbnailer: %s", cmd_line->str); */
        g_free(cmd_line);
        return (status == 0);
    }
    return FALSE;
}

static void find_thumbnailers_in_data_dir(GHashTable* hash, const char* data_dir)
{
    char* dir_path = g_build_filename(data_dir, "thumbnailers", NULL);
    GDir* dir = g_dir_open(dir_path, 0, NULL);
    if(dir)
    {
        const char* basename;
        while((basename = g_dir_read_name(dir)) != NULL)
        {
            /* we only want filenames with .thumbnailer extension */
            if (G_LIKELY(g_str_has_suffix(basename, ".thumbnailer")) &&
                g_hash_table_lookup(hash, basename) == NULL)
                g_hash_table_replace(hash, g_strdup(basename), g_strdup(dir_path));
        }
        g_dir_close(dir);
    }
    g_free(dir_path);
}

static void load_thumbnailers_from_data_dir(const char* basename, const char* dir_path, gpointer null)
{
    char* file_path = g_build_filename(dir_path, basename, NULL);
    GKeyFile* kf = g_key_file_new();
    if(g_key_file_load_from_file(kf, file_path, 0, NULL))
    {
        FmThumbnailer* thumbnailer = fm_thumbnailer_new_from_keyfile(basename, kf);
        if (thumbnailer != NULL)
        {
            G_LOCK(all_thumbnailers);
            all_thumbnailers = g_list_prepend(all_thumbnailers, thumbnailer);
            G_UNLOCK(all_thumbnailers);
            /* register the thumbnailer */
        }
        else
            g_warning("thumbnailer '%s' is unusable", file_path);
    }
    g_key_file_free(kf);
    g_free(file_path);
}

static void load_thumbnailers()
{
    const gchar * const *data_dirs = g_get_system_data_dirs();
    const gchar * const *data_dir;

    /* use a temporary hash table to collect thumbnailer basenames
     * key: basename of thumbnailer entry file
     * value: data dir the thumbnailer entry file is in */
    GHashTable* tmp_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    /* load user-specific thumbnailers */
    find_thumbnailers_in_data_dir(tmp_hash, g_get_user_data_dir());

    /* load system-wide thumbnailers */
    for(data_dir = data_dirs; *data_dir; ++data_dir)
        find_thumbnailers_in_data_dir(tmp_hash, *data_dir);

    /* load all found thumbnailers */
    g_hash_table_foreach(tmp_hash, (GHFunc)load_thumbnailers_from_data_dir, NULL);
    /* all_thumbnailers = g_list_reverse(all_thumbnailers); */
    g_hash_table_destroy(tmp_hash); /* we don't need the hash table anymore */

    /* record current time which will be used to compare with
     *  mtime of thumbnailer dirs later */
    last_loaded_time = time(NULL);
}

static void unload_thumbnailers()
{
    G_LOCK(all_thumbnailers);
    g_list_free_full(all_thumbnailers, (GDestroyNotify)fm_thumbnailer_unref);
    all_thumbnailers = NULL;
    G_UNLOCK(all_thumbnailers);
}

static gboolean check_data_dir(const char* data_dir)
{
    gboolean ret = FALSE;
    char* dir_path = g_build_filename(data_dir, "thumbnailers", NULL);
    struct stat statbuf;
    if(stat(dir_path, &statbuf) == 0)
    {
        if(statbuf.st_mtime > last_loaded_time)
            ret = TRUE;
    }
    g_free(dir_path);
    return ret;
}

/**
 * fm_thumbnailer_check_update
 *
 * Checks new thumbnailers and reloads if needed.
 *
 * Since: 1.0.0
 */
void fm_thumbnailer_check_update()
{
    gboolean need_reload = FALSE;
    // check system-wide thumbnailers
    const gchar * const *data_dirs = g_get_system_data_dirs();
    const gchar * const *data_dir;
    for(data_dir = data_dirs; *data_dir; ++data_dir)
    {
        need_reload = check_data_dir(*data_dir);
        if(need_reload)
            break;
    }

    /* check user-specific thumbnailers     */
    if(FALSE == need_reload)
        need_reload = check_data_dir(g_get_user_data_dir());

    if(need_reload)
    {
        /* unload all thumbnailers */
        unload_thumbnailers();

        /* reload thumbnailers */
        load_thumbnailers();
    }
}

void _fm_thumbnailer_init()
{
    load_thumbnailers();
}

void _fm_thumbnailer_finalize()
{
    unload_thumbnailers();
}
