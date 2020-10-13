/*
 *      fm-file-ops-job-delete.c
 *
 *      Copyright 2009 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2012-2016 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-file-ops-job-delete.h"
#include "fm-file-ops-job-xfer.h"
#include "fm-config.h"
#include "fm-file.h"
#include <glib/gi18n-lib.h>

static const char query[] =  G_FILE_ATTRIBUTE_STANDARD_TYPE","
                               G_FILE_ATTRIBUTE_STANDARD_NAME","
                               G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME;


gboolean _fm_file_ops_job_delete_file(FmJob* job, GFile* gf, GFileInfo* inf,
                                      FmFolder *folder, gboolean only_empty)
{
    GError* err = NULL;
    FmFileOpsJob* fjob = FM_FILE_OPS_JOB(job);
    gboolean is_dir, is_trash_root = FALSE, ok;
    GFileInfo* _inf = NULL;
    FmPath *path;
    FmJobErrorAction act;

    while(!inf)
    {
        _inf = inf = g_file_query_info(gf, query,
                            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                            fm_job_get_cancellable(job), &err);
        if(_inf)
            break;
        act = fm_job_emit_error(job, err, FM_JOB_ERROR_MODERATE);
        g_error_free(err);
        err = NULL;
        if(act == FM_JOB_ABORT)
            return FALSE;
        if(act != FM_JOB_RETRY)
            break;
    }
    if(!inf)
    {
        /* use basename of GFile as display name. */
        char* basename = g_file_get_basename(gf);
        char* disp = basename ? g_filename_display_name(basename) : NULL;
        g_free(basename);
                                                        /* FIXME: translate it */
        fm_file_ops_job_emit_cur_file(fjob, disp ? disp : "(invalid file)");
        g_free(disp);
        ++fjob->finished;
        return FALSE;
    }

    /* currently processed file. */
    fm_file_ops_job_emit_cur_file(fjob, g_file_info_get_display_name(inf));

    /* show progress */
    ++fjob->finished;
    fm_file_ops_job_emit_percent(fjob);

    is_dir = (g_file_info_get_file_type(inf)==G_FILE_TYPE_DIRECTORY);

    if(_inf)
        g_object_unref(_inf);

    if( fm_job_is_cancelled(job) )
        return FALSE;

    if(is_dir)
    {
        /* special handling for trash:/// */
        if(!g_file_is_native(gf))
        {
            char* scheme = g_file_get_uri_scheme(gf);
            if(g_strcmp0(scheme, "trash") == 0)
            {
                /* little trick: basename of trash root is /. */
                char* basename = g_file_get_basename(gf);
                if(basename && basename[0] == G_DIR_SEPARATOR)
                    is_trash_root = TRUE;
                g_free(basename);
            }
            g_free(scheme);
        }
    }

    while(!fm_job_is_cancelled(job))
    {
        if(g_file_delete(gf, fm_job_get_cancellable(job), &err))
        {
            if (folder)
            {
                path = fm_path_new_for_gfile(gf);
                _fm_folder_event_file_deleted(folder, path);
                fm_path_unref(path);
            }
            return TRUE;
        }
        if(err)
        {
            /* if it's non-empty dir then descent into it then try again */
            /* trash root gives G_IO_ERROR_PERMISSION_DENIED */
            if(is_trash_root || /* FIXME: need to refactor this! */
               (is_dir && !only_empty &&
                err->domain == G_IO_ERROR && err->code == G_IO_ERROR_NOT_EMPTY))
            {
                GFileEnumerator* enu;
                FmFolder *sub_folder;

                g_error_free(err);
                err = NULL;
                enu = g_file_enumerate_children(gf, query,
                                            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                            fm_job_get_cancellable(job), &err);
                if(!enu)
                {
                    fm_job_emit_error(job, err, FM_JOB_ERROR_MODERATE);
                    g_error_free(err);
                    return FALSE;
                }

                path = fm_path_new_for_gfile(gf);
                sub_folder = fm_folder_find_by_path(path);
                fm_path_unref(path);
                while( ! fm_job_is_cancelled(job) )
                {
                    inf = g_file_enumerator_next_file(enu, fm_job_get_cancellable(job), &err);
                    if(inf)
                    {
                        GFile* sub = g_file_get_child(gf, g_file_info_get_name(inf));
                        ok = _fm_file_ops_job_delete_file(job, sub, inf, sub_folder, FALSE);
                        g_object_unref(sub);
                        g_object_unref(inf);
                        if (!ok) /* stop the job if error happened */
                            goto _failed;
                    }
                    else
                    {
                        if(err)
                        {
                            fm_job_emit_error(job, err, FM_JOB_ERROR_MODERATE);
                            /* FM_JOB_RETRY is not supported here */
                            g_error_free(err);
_failed:
                            g_object_unref(enu);
                            if (sub_folder)
                                g_object_unref(sub_folder);
                            return FALSE;
                        }
                        else /* EOF */
                            break;
                    }
                }
                g_object_unref(enu);
                if (sub_folder)
                    g_object_unref(sub_folder);

                is_trash_root = FALSE; /* don't go here again! */
                is_dir = FALSE;
                continue;
            }
            else if (is_dir && only_empty)
            {
                /* special case: trying delete directory with skipped files
                   see _fm_file_ops_job_copy_file() for details */
                g_error_free(err);
                return TRUE;
            }
            if(err->domain == G_IO_ERROR && err->code == G_IO_ERROR_PERMISSION_DENIED)
            {
                /* special case for trash:/// */
                /* FIXME: is there any better way to handle this? */
                char* scheme = g_file_get_uri_scheme(gf);
                if(g_strcmp0(scheme, "trash") == 0)
                {
                    g_free(scheme);
                    g_error_free(err);
                    return TRUE;
                }
                g_free(scheme);
            }
            act = fm_job_emit_error(job, err, FM_JOB_ERROR_MODERATE);
            g_error_free(err);
            err = NULL;
            if(act != FM_JOB_RETRY)
                return FALSE;
        }
        else
            return FALSE;
    }

    return FALSE;
}


gboolean _fm_file_ops_job_delete_run(FmFileOpsJob* job)
{
    GList* l;
    gboolean ret = TRUE;
    /* prepare the job, count total work needed with FmDeepCountJob */
    FmDeepCountJob* dc = fm_deep_count_job_new(job->srcs, FM_DC_JOB_PREPARE_DELETE);
    FmJob* fmjob = FM_JOB(job);
    FmPath *path, *parent = NULL;
    FmFolder *parent_folder = NULL;

    /* let the deep count job share the same cancellable */
    fm_job_set_cancellable(FM_JOB(dc), fm_job_get_cancellable(fmjob));
    fm_job_run_sync(FM_JOB(dc));
    job->total = dc->count;
    g_object_unref(dc);

    if(fm_job_is_cancelled(fmjob))
    {
        g_debug("delete job is cancelled");
        return FALSE;
    }

    g_debug("total number of files to delete: %llu", (long long unsigned int)job->total);

    fm_file_ops_job_emit_prepared(job);

    l = fm_path_list_peek_head_link(job->srcs);
    for(; ! fm_job_is_cancelled(fmjob) && l;l=l->next)
    {
        GFile* src;

        path = FM_PATH(l->data);
        if (fm_path_get_parent(path) != parent && fm_path_get_parent(path) != NULL)
        {
            FmFolder *pf = fm_folder_find_by_path(fm_path_get_parent(path));
            if (pf != parent_folder)
            {
                if (parent_folder)
                {
                    fm_folder_unblock_updates(parent_folder);
                    g_object_unref(parent_folder);
                }
                if (pf)
                    fm_folder_block_updates(pf);
                parent_folder = pf;
            }
            else if (pf)
                g_object_unref(pf);
        }
        parent = fm_path_get_parent(path);
        src = fm_path_to_gfile(path);

        ret = _fm_file_ops_job_delete_file(fmjob, src, NULL, parent_folder, FALSE);
        g_object_unref(src);
    }
    if (parent_folder)
    {
        fm_folder_unblock_updates(parent_folder);
        g_object_unref(parent_folder);
    }
    return ret;
}

gboolean _fm_file_ops_job_trash_run(FmFileOpsJob* job)
{
    gboolean ret = TRUE;
    GList* l;
    FmPathList* unsupported = fm_path_list_new();
    GError* err = NULL;
    FmJob* fmjob = FM_JOB(job);
    FmPath *path, *parent = NULL;
    FmFolder *parent_folder = NULL;

    g_debug("total number of files to delete: %u", fm_path_list_get_length(job->srcs));
    job->total = fm_path_list_get_length(job->srcs);

    fm_file_ops_job_emit_prepared(job);

    /* FIXME: we shouldn't trash a file already in trash:/// */

    l = fm_path_list_peek_head_link(job->srcs);
    for(; !fm_job_is_cancelled(fmjob) && l;l=l->next)
    {
        GFile* gf = fm_path_to_gfile(FM_PATH(l->data));
        GFileInfo* inf;

        path = FM_PATH(l->data);
        if (fm_path_get_parent(path) != parent && fm_path_get_parent(path) != NULL)
        {
            FmFolder *pf = fm_folder_find_by_path(fm_path_get_parent(path));
            if (pf != parent_folder)
            {
                if (parent_folder)
                {
                    fm_folder_unblock_updates(parent_folder);
                    g_object_unref(parent_folder);
                }
                if (pf)
                    fm_folder_block_updates(pf);
                parent_folder = pf;
            }
            else if (pf)
                g_object_unref(pf);
        }
        parent = fm_path_get_parent(path);
_retry_trash:
        inf = g_file_query_info(gf, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, 0,
                                fm_job_get_cancellable(fmjob), &err);
        if(inf)
        {
            /* currently processed file. */
            fm_file_ops_job_emit_cur_file(job, g_file_info_get_display_name(inf));
            g_object_unref(inf);
        }
        else
        {
            char* basename = g_file_get_basename(gf);
            char* disp = basename ? g_filename_display_name(basename) : NULL;
            g_free(basename);
            ret = FALSE;
                                                        /* FIXME: translate it */
            fm_file_ops_job_emit_cur_file(job, disp ? disp : "(invalid file)");
            g_free(disp);
            goto _on_error;
        }
        ret = FALSE;
        if(fm_config->no_usb_trash)
        {
            GMount *mnt = g_file_find_enclosing_mount(gf, NULL, &err);

            if(mnt)
            {
                ret = g_mount_can_unmount(mnt); /* TRUE if it's removable media */
                g_object_unref(mnt);
                if(ret)
                    fm_path_list_push_tail(unsupported, FM_PATH(l->data));
            }
            else
            {
                g_error_free(err);
                err = NULL;
            }
        }

        if(!ret)
        {
            ret = g_file_trash(gf, fm_job_get_cancellable(fmjob), &err);
            if (ret && parent_folder)
                _fm_folder_event_file_deleted(parent_folder, path);
            /* FIXME: signal trash:/// that file added there */
        }
        if(!ret)
        {
_on_error:
            /* if trashing is not supported by the file system */
            if( err->domain == G_IO_ERROR && err->code == G_IO_ERROR_NOT_SUPPORTED)
                fm_path_list_push_tail(unsupported, FM_PATH(l->data));
            else
            {
                FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MODERATE);
                g_error_free(err);
                err = NULL;
                if(act == FM_JOB_RETRY)
                    goto _retry_trash;
                else if(act == FM_JOB_ABORT)
                {
                    g_object_unref(gf);
                    fm_path_list_unref(unsupported);
                    return FALSE;
                }
            }
            g_error_free(err);
            err = NULL;
        }
        g_object_unref(gf);
        ++job->finished;
        fm_file_ops_job_emit_percent(job);
    }
    if (parent_folder)
    {
        fm_folder_unblock_updates(parent_folder);
        g_object_unref(parent_folder);
    }

    /* these files cannot be trashed due to lack of support from
     * underlying file systems. */
    if(fm_path_list_is_empty(unsupported))
        fm_path_list_unref(unsupported);
    else
    {
        /* FIXME: this is a dirty hack to fallback to delete if trash is not available.
         * The API must be re-designed later. */
        g_object_set_data_full(G_OBJECT(job), "trash-unsupported", unsupported, (GDestroyNotify)fm_path_list_unref);
    }
    return TRUE;
}

static const char trash_query[]=
    G_FILE_ATTRIBUTE_STANDARD_TYPE","
    G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME","
    G_FILE_ATTRIBUTE_STANDARD_NAME","
    G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL","
    G_FILE_ATTRIBUTE_STANDARD_SIZE","
    G_FILE_ATTRIBUTE_UNIX_BLOCKS","
    G_FILE_ATTRIBUTE_UNIX_BLOCK_SIZE","
    G_FILE_ATTRIBUTE_ID_FILESYSTEM","
    "trash::orig-path";

static gboolean ensure_parent_dir(FmJob* job, GFile* orig_path)
{
    GFile* parent = g_file_get_parent(orig_path);
    gboolean ret = g_file_query_exists(parent, fm_job_get_cancellable(job));
    if(!ret)
    {
        GError* err = NULL;
_retry_mkdir:
        if(!g_file_make_directory_with_parents(parent, fm_job_get_cancellable(job), &err))
        {
            if(!fm_job_is_cancelled(job))
            {
                FmJobErrorAction act = fm_job_emit_error(job, err, FM_JOB_ERROR_MODERATE);
                g_error_free(err);
                err = NULL;
                if(act == FM_JOB_RETRY)
                    goto _retry_mkdir;
            }
        }
        else
            ret = TRUE;
    }
    g_object_unref(parent);
    return ret;
}

gboolean _fm_file_ops_job_untrash_run(FmFileOpsJob* job)
{
    gboolean ret = TRUE;
    GList* l;
    GError* err = NULL;
    FmJob* fmjob = FM_JOB(job);
    job->total = fm_path_list_get_length(job->srcs);
    fm_file_ops_job_emit_prepared(job);

    l = fm_path_list_peek_head_link(job->srcs);
    for(; !fm_job_is_cancelled(fmjob) && l;l=l->next)
    {
        GFile* gf;
        GFileInfo* inf;
        FmPath* path = FM_PATH(l->data);
        if(!fm_path_is_trash(path))
            continue;
        gf = fm_path_to_gfile(path);
_retry_get_orig_path:
        inf = g_file_query_info(gf, trash_query, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, fm_job_get_cancellable(fmjob), &err);
        if(inf)
        {
            const char* orig_path_str = g_file_info_get_attribute_byte_string(inf, "trash::orig-path");
            fm_file_ops_job_emit_cur_file(job, g_file_info_get_display_name(inf));

            if(orig_path_str)
            {
                /* FIXME: what if orig_path_str is a relative path?
                 * This is actually allowed by the horrible trash spec. */
                GFile* orig_path = fm_file_new_for_commandline_arg(orig_path_str);
                FmFolder *src_folder = fm_folder_find_by_path(fm_path_get_parent(path));
                FmPath *orig_fm_path = fm_path_new_for_gfile(orig_path);
                FmFolder *dst_folder = fm_folder_find_by_path(fm_path_get_parent(orig_fm_path));
                fm_path_unref(orig_fm_path);
                /* ensure the existence of parent folder. */
                if(ensure_parent_dir(fmjob, orig_path))
                    ret = _fm_file_ops_job_move_file(job, gf, inf, orig_path, path, src_folder, dst_folder);
                if (src_folder)
                    g_object_unref(src_folder);
                if (dst_folder)
                    g_object_unref(dst_folder);
                g_object_unref(orig_path);
            }
            else
            {
                FmJobErrorAction act;

                g_set_error(&err, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("Cannot untrash file '%s': original path not known"),
                            g_file_info_get_display_name(inf));
                act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MODERATE);
                g_clear_error(&err);
                if(act == FM_JOB_ABORT)
                {
                    g_object_unref(inf);
                    g_object_unref(gf);
                    return FALSE;
                }
            }
            g_object_unref(inf);
        }
        else
        {
            char* basename = g_file_get_basename(gf);
            char* disp = basename ? g_filename_display_name(basename) : NULL;
            g_free(basename);
                                                        /* FIXME: translate it */
            fm_file_ops_job_emit_cur_file(job, disp ? disp : "(invalid file)");
            g_free(disp);

            if(err)
            {
                FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MODERATE);
                g_error_free(err);
                err = NULL;
                if(act == FM_JOB_RETRY)
                    goto _retry_get_orig_path;
                else if(act == FM_JOB_ABORT)
                {
                    g_object_unref(gf);
                    return FALSE;
                }
            }
        }
        g_object_unref(gf);
        ++job->finished;
        fm_file_ops_job_emit_percent(job);
    }

    return ret;
}
