/*
 *      fm-file-launcher.c
 *
 *      Copyright 2009 - 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2012 Vadim Ushakov <igeekless@gmail.com>
 *      Copyright 2012-2018 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *      Copyright 2017 Tsu Jan <tsujan2000@gmail.com>
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
 * SECTION:fm-file-launcher
 * @short_description: File launching utilities with callbacks to GUI.
 * @title: Libfm file launchers
 *
 * @include: libfm/fm.h
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-file-launcher.h"
#include "fm-file-info-job.h"
#include "fm-app-info.h"
#include "glib-compat.h"

#include <glib/gi18n-lib.h>
#include <libintl.h>
#include <gio/gdesktopappinfo.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/**
 * fm_launch_desktop_entry
 * @ctx: (allow-none): a launch context
 * @file_or_id: a desktop entry to launch
 * @uris: (element-type char *): files to use in run substitutions
 * @launcher: #FmFileLauncher with callbacks
 * @user_data: data supplied for callbacks
 *
 * Launches a desktop entry with optional files.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 0.1.0
 */
gboolean fm_launch_desktop_entry(GAppLaunchContext* ctx, const char* file_or_id, GList* uris, FmFileLauncher* launcher, gpointer user_data)
{
    gboolean ret = FALSE;
    GAppInfo* app;
    gboolean is_absolute_path = g_path_is_absolute(file_or_id);
    GList* _uris = NULL;
    GError* err = NULL;

    /* Let GDesktopAppInfo try first. */
    if(is_absolute_path)
        app = (GAppInfo*)g_desktop_app_info_new_from_filename(file_or_id);
    else
        app = (GAppInfo*)g_desktop_app_info_new(file_or_id);
    /* we handle Type=Link in FmFileInfo so if GIO failed then
       it cannot be launched in fact */

    if(app) {
        ret = fm_app_info_launch_uris(app, uris, ctx, &err);
        g_object_unref(app);
    }
    else if (launcher->error)
        g_set_error(&err, G_IO_ERROR, G_IO_ERROR_FAILED,
                    _("Invalid desktop entry file: '%s'"), file_or_id);

    if(err)
    {
        if(launcher->error)
            launcher->error(ctx, err, NULL, user_data);
        g_error_free(err);
    }

    if(_uris)
    {
        g_list_foreach(_uris, (GFunc)g_free, NULL);
        g_list_free(_uris);
    }

    return ret;
}

typedef struct
{
    GAppLaunchContext* ctx;
    FmFileLauncher* launcher;
    gpointer user_data;
} QueryErrorData;

static FmJobErrorAction on_query_target_info_error(FmJob* job, GError* err, FmJobErrorSeverity severity, QueryErrorData* data)
{
    if(data->launcher->error
       && !data->launcher->error(data->ctx, err,
                                 fm_file_info_job_get_current(FM_FILE_INFO_JOB(job)),
                                 data->user_data))
        return FM_JOB_RETRY;
    return FM_JOB_CONTINUE;
}

static FmFileInfo *_fetch_file_info_for_shortcut(const char *target,
                                                 GAppLaunchContext* ctx,
                                                 FmFileLauncher* launcher,
                                                 gpointer user_data)
{
    FmFileInfoJob *job;
    QueryErrorData data;
    FmFileInfo *fi;
    FmPath *path;

    job = fm_file_info_job_new(NULL, 0);
    /* bug #3614794: the shortcut target is a commandline argument */
    path = fm_path_new_for_commandline_arg(target);
    fm_file_info_job_add(job, path);
    fm_path_unref(path);
    data.ctx = ctx;
    data.launcher = launcher;
    data.user_data = user_data;
    g_signal_connect(job, "error", G_CALLBACK(on_query_target_info_error), &data);
    fi = NULL;
    /* if we do fm_job_run_sync_with_mainloop() then we should unlock
       GDK threads before main loop but the libfm in GDK independent
       therefore we cannot use the fm_job_run_sync_with_mainloop() here */
    if (fm_job_run_sync(FM_JOB(job))) {
        FmFileInfo* file_info = fm_file_info_list_peek_head(job->file_infos);
        if (file_info) {
            fi = fm_file_info_ref(file_info);
        }
    }
    g_signal_handlers_disconnect_by_func(job, on_query_target_info_error, &data);
    g_object_unref(job);
    return fi;
}

/**
 * fm_launch_files
 * @ctx: (allow-none): a launch context
 * @file_infos: (element-type FmFileInfo): files to launch
 * @launcher: #FmFileLauncher with callbacks
 * @user_data: data supplied for callbacks
 *
 * Launches files using callbacks in @launcher.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 0.1.0
 */
gboolean fm_launch_files(GAppLaunchContext* ctx, GList* file_infos, FmFileLauncher* launcher, gpointer user_data)
{
    GList* l;
    GHashTable* hash = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    GList *folders = NULL;
    FmFileInfo* fi;
    GList *targets = NULL;
    GError* err = NULL;
    GAppInfo* app;
    const char* type;

    for(l = file_infos; l; l=l->next)
    {
        GList* fis;
        char *filename, *scheme;
        const char *target = NULL;

        fi = (FmFileInfo*)l->data;
        /* special handling for shortcuts */
        if (!fm_file_info_is_symlink(fi))
            /* symlinks also has fi->target, but we only handle shortcuts here. */
            target = fm_file_info_get_target(fi);
        if (launcher->open_folder &&
            (fm_file_info_is_dir(fi) || fm_file_info_is_mountable(fi)))
        {
            /* special handling for shortcuts */
            if(target)
            {
                fi = _fetch_file_info_for_shortcut(fm_file_info_get_target(fi),
                                                   ctx, launcher, user_data);
                if (fi == NULL)
                    /* error was shown by job already */
                    continue;
                targets = g_list_prepend(targets, fi);
            }
            folders = g_list_prepend(folders, fi);
        }
        else if (fm_file_info_is_desktop_entry(fi))
        {
_launch_desktop_entry:
            /* treat desktop entries as executables */
            if(fm_file_info_is_executable_type(fi))
            {
                switch(launcher->exec_file(fi, user_data))
                {
                case FM_FILE_LAUNCHER_EXEC_IN_TERMINAL:
                case FM_FILE_LAUNCHER_EXEC:
                {
                    if (!target)
                        filename = fm_path_to_str(fm_file_info_get_path(fi));
                    fm_launch_desktop_entry(ctx, target ? target : filename, NULL,
                                            launcher, user_data);
                    if (!target)
                        g_free(filename);
                    continue;
                }
                case FM_FILE_LAUNCHER_EXEC_OPEN:
                    break;
                case FM_FILE_LAUNCHER_EXEC_CANCEL:
                    continue;
               }
            }
            /* make exception for desktop entries under menu */
            else if (fm_file_info_is_native(fi) /* an exception */ ||
                     fm_path_is_xdg_menu(fm_file_info_get_path(fi)))
            {
                if (!target)
                    filename = fm_path_to_str(fm_file_info_get_path(fi));
                fm_launch_desktop_entry(ctx, target ? target : filename, NULL,
                                        launcher, user_data);
                if (!target)
                    g_free(filename);
                continue;
            }

            FmMimeType* mime_type = fm_file_info_get_mime_type(fi);
            if(mime_type && (type = fm_mime_type_get_type(mime_type)))
            {
                fis = g_hash_table_lookup(hash, type);
                fis = g_list_prepend(fis, fi);
                g_hash_table_insert(hash, (gpointer)type, fis);
            }
            else if (launcher->error)
            {
                g_set_error(&err, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("Could not determine content type of file '%s' to launch it"),
                            fm_file_info_get_disp_name(fi));
                launcher->error(ctx, err, NULL, user_data);
                g_clear_error(&err);
            }
        }
        else
        {
            FmPath* path = fm_file_info_get_path(fi);
            FmMimeType* mime_type = NULL;
            if(fm_path_is_native(path))
            {
                /* special handling for shortcuts */
                if (target)
                {
                    if (fm_file_info_get_mime_type(fi) == _fm_mime_type_get_inode_x_shortcut())
                    /* if we already know MIME type then use it instead */
                    {
                      scheme = g_uri_parse_scheme(target);
                      if (scheme)
                      {
                        /* FIXME: this is rough! */
                        if (strcmp(scheme, "file") != 0 &&
                            strcmp(scheme, "trash") != 0 &&
                            strcmp(scheme, "network") != 0 &&
                            strcmp(scheme, "computer") != 0 &&
                            strcmp(scheme, "menu") != 0)
                        {
                            /* we don't support this URI internally, try GIO */
                            app = g_app_info_get_default_for_uri_scheme(scheme);
                            if (app)
                            {
                                fis = g_list_prepend(NULL, (char *)target);
                                fm_app_info_launch_uris(app, fis, ctx, &err);
                                g_list_free(fis);
                                g_object_unref(app);
                            }
                            else if (launcher->error)
                                g_set_error(&err, G_IO_ERROR, G_IO_ERROR_FAILED,
                                            _("No default application is set to launch URIs %s://"),
                                            scheme);
                            if (err)
                            {
                                launcher->error(ctx, err, NULL, user_data);
                                g_clear_error(&err);
                            }
                            g_free(scheme);
                            continue;
                        }
                        g_free(scheme);
                      }
                    }
                    else
                        mime_type = fm_file_info_get_mime_type(fi);
                    /* retrieve file info for target otherwise and handle it */
                    fi = _fetch_file_info_for_shortcut(target, ctx, launcher, user_data);
                    if (fi == NULL)
                        /* error was shown by job already */
                        continue;
                    targets = g_list_prepend(targets, fi);
                    path = fm_file_info_get_path(fi);
                    /* special handling for desktop entries */
                    if (fm_file_info_is_desktop_entry(fi))
                        goto _launch_desktop_entry;
                }
                if(fm_file_info_is_executable_type(fi) && !fm_file_info_is_desktop_entry(fi))
                {
                    /* if it's an executable file, directly execute it. */
                    filename = fm_path_to_str(path);

                    /* FIXME: we need to use eaccess/euidaccess here. */
                    if(g_file_test(filename, G_FILE_TEST_IS_EXECUTABLE))
                    {
                        if(launcher->exec_file)
                        {
                            FmFileLauncherExecAction act = launcher->exec_file(fi, user_data);
                            GAppInfoCreateFlags flags = 0;
                            switch(act)
                            {
                            case FM_FILE_LAUNCHER_EXEC_IN_TERMINAL:
                                flags |= G_APP_INFO_CREATE_NEEDS_TERMINAL;
                                /* NOTE: no break here */
                            case FM_FILE_LAUNCHER_EXEC:
                            {
                                /* filename may contain spaces. Fix #3143296 */
                                char* quoted = g_shell_quote(filename);
                                app = fm_app_info_create_from_commandline(quoted, NULL, flags, NULL);
                                g_free(quoted);
                                if(app)
                                {
                                    char* run_path = g_path_get_dirname(filename);
                                    char* cwd = NULL;
                                    /* bug #3589641: scripts are ran from $HOME.
                                       since GIO launcher is kinda ugly - it has
                                       no means to set running directory so we
                                       do workaround - change directory to it */
                                    if(run_path && strcmp(run_path, "."))
                                    {
                                        cwd = g_get_current_dir();
                                        if(chdir(run_path) != 0)
                                        {
                                            g_free(cwd);
                                            cwd = NULL;
                                            if (launcher->error)
                                            {
                                                g_set_error(&err, G_IO_ERROR,
                                                            g_io_error_from_errno(errno),
                                                            _("Cannot set working directory to '%s': %s"),
                                                            run_path, g_strerror(errno));
                                                launcher->error(ctx, err, NULL, user_data);
                                                g_clear_error(&err);
                                            }
                                        }
                                    }
                                    g_free(run_path);
                                    if(!fm_app_info_launch(app, NULL, ctx, &err))
                                    {
                                        if(launcher->error)
                                            launcher->error(ctx, err, NULL, user_data);
                                        g_error_free(err);
                                        err = NULL;
                                    }
                                    if(cwd) /* return back */
                                    {
                                        if(chdir(cwd) != 0)
                                            g_warning("fm_launch_files(): chdir() failed");
                                        g_free(cwd);
                                    }
                                    g_object_unref(app);
                                    continue;
                                }
                                break;
                            }
                            case FM_FILE_LAUNCHER_EXEC_OPEN:
                                break;
                            case FM_FILE_LAUNCHER_EXEC_CANCEL:
                                continue;
                            }
                        }
                    }
                    g_free(filename);
                }
            }
            if (mime_type == NULL)
                mime_type = fm_file_info_get_mime_type(fi);
            if(mime_type && (type = fm_mime_type_get_type(mime_type)))
            {
                fis = g_hash_table_lookup(hash, type);
                fis = g_list_prepend(fis, fi);
                g_hash_table_insert(hash, (gpointer)type, fis);
            }
            else if (launcher->error)
            {
                g_set_error(&err, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("Could not determine content type of file '%s' to launch it"),
                            fm_file_info_get_disp_name(fi));
                launcher->error(ctx, err, NULL, user_data);
                g_clear_error(&err);
            }
        }
    }

    if(g_hash_table_size(hash) > 0)
    {
        GHashTableIter it;
        GList* fis;
        g_hash_table_iter_init(&it, hash);
        while(g_hash_table_iter_next(&it, (void**)&type, (void**)&fis))
        {
            GAppInfo* app = g_app_info_get_default_for_type(type, FALSE);
            if(!app)
            {
                if(launcher->get_app)
                {
                    FmMimeType* mime_type = fm_file_info_get_mime_type((FmFileInfo*)fis->data);
                    app = launcher->get_app(fis, mime_type, user_data, NULL);
                }
            }
            if(app)
            {
                for(l=fis; l; l=l->next)
                {
                    char* uri;
                    fi = (FmFileInfo*)l->data;
                    /* special handling for shortcuts */
                    if (fm_file_info_is_shortcut(fi))
                        uri = g_strdup(fm_file_info_get_target(fi));
                    else
                        uri = fm_path_to_uri(fm_file_info_get_path(fi));
                    l->data = uri;
                }
                fis = g_list_reverse(fis);
                fm_app_info_launch_uris(app, fis, ctx, &err);
                /* free URI strings */
                g_list_foreach(fis, (GFunc)g_free, NULL);
                g_object_unref(app);
            }
            else if (launcher->error && !launcher->get_app)
                /* SF#837: don't show an error window after user pressed
                   'Cancel' in the selection window, it'll annoy user */
                g_set_error(&err, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("No default application is set for MIME type %s"),
                            type);
            if (err)
            {
                launcher->error(ctx, err, NULL, user_data);
                g_clear_error(&err);
            }
            g_list_free(fis);
        }
    }
    g_hash_table_destroy(hash);

    if(folders)
    {
        folders = g_list_reverse(folders);
        if(launcher->open_folder)
        {
            launcher->open_folder(ctx, folders, user_data, &err);
            if(err)
            {
                if(launcher->error)
                    launcher->error(ctx, err, NULL, user_data);
                g_error_free(err);
                err = NULL;
            }
        }
        g_list_free(folders);
    }
    g_list_free_full(targets, (GDestroyNotify)fm_file_info_unref);
    return TRUE;
}

/**
 * fm_launch_paths
 * @ctx: (allow-none): a launch context
 * @paths: (element-type FmPath): files to launch
 * @launcher: #FmFileLauncher with callbacks
 * @user_data: data supplied for callbacks
 *
 * Launches files using callbacks in @launcher.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 0.1.0
 */
gboolean fm_launch_paths(GAppLaunchContext* ctx, GList* paths, FmFileLauncher* launcher, gpointer user_data)
{
    FmFileInfoJob* job = fm_file_info_job_new(NULL, 0);
    GList* l;
    QueryErrorData data;
    gboolean ret;
    for(l=paths;l;l=l->next)
        fm_file_info_job_add(job, (FmPath*)l->data);
    data.ctx = ctx;
    data.launcher = launcher;
    data.user_data = user_data;
    g_signal_connect(job, "error", G_CALLBACK(on_query_target_info_error), &data);
    /* if we do fm_job_run_sync_with_mainloop() then we should unlock
       GDK threads before main loop but the libfm in GDK independent
       therefore we cannot use the fm_job_run_sync_with_mainloop() here */
    ret = fm_job_run_sync(FM_JOB(job));
    g_signal_handlers_disconnect_by_func(job, on_query_target_info_error, &data);
    if(ret)
    {
        GList* file_infos = fm_file_info_list_peek_head_link(job->file_infos);
        if(file_infos)
            ret = fm_launch_files(ctx, file_infos, launcher, user_data);
        else
            ret = FALSE;
    }
    g_object_unref(job);
    return ret;
}
