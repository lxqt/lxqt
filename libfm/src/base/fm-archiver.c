/*
 *      fm-archiver.c
 *
 *      Copyright 2010 PCMan <pcman.tw@gmail.com>
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
 * SECTION:fm-archiver
 * @short_description: Support for packing and unpacking archiver utilities.
 * @title: FmArchiver
 *
 * @include: libfm/fm.h
 *
 * The #FmArchiver represents support for utilities which can pack files
 * into archive and/or extract them.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-config.h"
#include "fm-archiver.h"
#include "fm-app-info.h"
#include "fm-utils.h"
#include <gio/gdesktopappinfo.h>
#include <string.h>

static GList* archivers = NULL;
static FmArchiver* default_archiver = NULL;

static void fm_archiver_free(FmArchiver* archiver)
{
    g_free(archiver->program);
    g_free(archiver->create_cmd);
    g_free(archiver->extract_cmd);
    g_free(archiver->extract_to_cmd);
    g_strfreev(archiver->mime_types);
    g_slice_free(FmArchiver, archiver);
}

gboolean fm_archiver_is_mime_type_supported(FmArchiver* archiver, const char* type)
{
    char** p;
    if(G_UNLIKELY(!type))
        return FALSE;
    for(p=archiver->mime_types; *p; ++p)
    {
        if(strcmp(*p, type) == 0)
            return TRUE;
    }
    return FALSE;
}

/* FIXME: error handling */
static gboolean launch_program(FmArchiver* archiver, GAppLaunchContext* ctx, const char* cmd, FmPathList* files, FmPath* dir)
{
    GAppInfo* app;
    char* _cmd = NULL;
    const char* dir_place_holder;
    GKeyFile* dummy;
    char* tmp;

    if(dir && (dir_place_holder = strstr(cmd, "%d")))
    {
        char* dir_str;
        int len;
        if(strstr(cmd, "%U") || strstr(cmd, "%u")) /* supports URI */
            dir_str = fm_path_to_uri(dir);
        else
        {
            GFile* gf = fm_path_to_gfile(dir);
            /* FIXME: convert dir to fuse-based local path if needed. */
            dir_str = g_file_get_path(gf);
            g_object_unref(gf);
        }

        /* replace all % with %% so encoded URI can be handled correctly when parsing Exec key. */
        tmp = fm_strdup_replace(dir_str, "%", "%%");
        g_free(dir_str);
        dir_str = tmp;

        /* quote the path or URI */
        tmp = g_shell_quote(dir_str);
        g_free(dir_str);
        dir_str = tmp;

        len = strlen(cmd) - 2 + strlen(dir_str) + 1;
        _cmd = g_malloc(len);
        len = (dir_place_holder - cmd);
        strncpy(_cmd, cmd, len);
        strcpy(_cmd + len, dir_str);
        strcat(_cmd, dir_place_holder + 2);
        g_free(dir_str);
        cmd = _cmd;
    }

    /* create a fake key file to cheat GDesktopAppInfo */
    dummy = g_key_file_new();
    g_key_file_set_string(dummy, G_KEY_FILE_DESKTOP_GROUP, "Type", "Application");
    g_key_file_set_string(dummy, G_KEY_FILE_DESKTOP_GROUP, "Name", archiver->program);

    /* replace all % with %% so encoded URI can be handled correctly when parsing Exec key. */
    g_key_file_set_string(dummy, G_KEY_FILE_DESKTOP_GROUP, "Exec", cmd);
    app = (GAppInfo*)g_desktop_app_info_new_from_keyfile(dummy);

    g_key_file_free(dummy);
    g_debug("cmd = %s", cmd);
    if(app)
    {
        GList* uris = NULL, *l;
        for(l = fm_path_list_peek_head_link(files); l; l=l->next)
        {
            FmPath* path = FM_PATH(l->data);
            uris = g_list_prepend(uris, fm_path_to_uri(path));
        }
        fm_app_info_launch_uris(app, uris, ctx, NULL);
        g_list_foreach(uris, (GFunc)g_free, NULL);
        g_list_free(uris);
        g_object_unref(app);
    }
    g_free(_cmd);
    return TRUE;
}

/**
 * fm_archiver_create_archive
 * @archiver: the archiver descriptor
 * @ctx: (allow-none): a launch context
 * @files: files to pack into archive
 *
 * Creates an archive for @files.
 *
 * Returns: %FALSE.
 *
 * Since: 0.1.9
 */
gboolean fm_archiver_create_archive(FmArchiver* archiver, GAppLaunchContext* ctx, FmPathList* files)
{
    if(archiver->create_cmd && files)
        launch_program(archiver, ctx, archiver->create_cmd, files, NULL);
    return FALSE;
}

/**
 * fm_archiver_extract_archives
 * @archiver: the archiver descriptor
 * @ctx: (allow-none): a launch context
 * @files: archives to unpack
 *
 * Extracts files from archives.
 *
 * Returns: %FALSE.
 *
 * Since: 0.1.9
 */
gboolean fm_archiver_extract_archives(FmArchiver* archiver, GAppLaunchContext* ctx, FmPathList* files)
{
    if(archiver->extract_cmd && files)
        launch_program(archiver, ctx, archiver->extract_cmd, files, NULL);
    return FALSE;
}

/**
 * fm_archiver_extract_archives_to
 * @archiver: archiver descriptor
 * @ctx: (allow-none): a launch context
 * @files: archives to unpack
 * @dest_dir: directory where files should be extracted to
 *
 * Extracts files from archives into @dest_dir.
 *
 * Returns: %FALSE.
 *
 * Since: 0.1.9
 */
gboolean fm_archiver_extract_archives_to(FmArchiver* archiver, GAppLaunchContext* ctx, FmPathList* files, FmPath* dest_dir)
{
    if(archiver->extract_to_cmd && files)
        launch_program(archiver, ctx, archiver->extract_to_cmd, files, dest_dir);
    return FALSE;
}

/**
 * fm_archiver_get_default
 *
 * Retrieves default GUI archiver used by libfm.
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: archiver descriptor.
 *
 * Since: 0.1.9
 */
FmArchiver* fm_archiver_get_default(void)
{
    if(!default_archiver)
    {
        GList* l;
        if(fm_config->archiver)
        {
            for(l = archivers; l; l=l->next)
            {
                FmArchiver* archiver = (FmArchiver*)l->data;
                if( g_strcmp0(fm_config->archiver, archiver->program) == 0 )
                {
                    default_archiver = archiver;
                    break;
                }
            }
        }
        else if(archivers)
        {
            for(l = archivers; l; l=l->next)
            {
                FmArchiver* archiver = (FmArchiver*)l->data;
                char* tmp = g_find_program_in_path(archiver->program);
                if( tmp )
                {
                    g_free(tmp);
                    default_archiver = archiver;
                    g_free(fm_config->archiver);
                    fm_config->archiver = g_strdup(archiver->program);
                    break;
                }
            }
        }
    }
    return default_archiver;
}

/**
 * fm_archiver_set_default
 * @archiver: archiver descriptor
 *
 * Sets default GUI archiver used by libfm.
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Since: 0.1.9
 */
void fm_archiver_set_default(FmArchiver* archiver)
{
    if(archiver)
        default_archiver = archiver;
}

/**
 * fm_archiver_get_all
 *
 * Retrieves a list of #FmArchiver of all GUI archivers known to libfm.
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: list of archivers.
 *
 * Since: 0.1.9
 */
const GList* fm_archiver_get_all(void)
{
    return archivers;
}

void _fm_archiver_init()
{
    GKeyFile *kf = g_key_file_new();
    if(g_key_file_load_from_file(kf, PACKAGE_DATA_DIR "/archivers.list", 0, NULL))
    {
        gsize n_archivers;
        gchar** programs = g_key_file_get_groups(kf, &n_archivers);
        if(programs)
        {
            gsize i;
            for(i = 0; i < n_archivers; ++i)
            {
                FmArchiver* archiver = g_slice_new0(FmArchiver);
                archiver->program = programs[i];
                archiver->create_cmd = g_key_file_get_string(kf, programs[i], "create", NULL);
                archiver->extract_cmd = g_key_file_get_string(kf, programs[i], "extract", NULL);
                archiver->extract_to_cmd = g_key_file_get_string(kf, programs[i], "extract_to", NULL);
                archiver->mime_types = g_key_file_get_string_list(kf, programs[i], "mime_types", NULL, NULL);
                archivers = g_list_append(archivers, archiver);
            }
            g_free(programs); /* strings in the vector are stolen by FmArchiver. */
        }
    }
    g_key_file_free(kf);
}

void _fm_archiver_finalize()
{
    g_list_foreach(archivers, (GFunc)fm_archiver_free, NULL);
    g_list_free(archivers);
    archivers = NULL;
    default_archiver = NULL;
}
