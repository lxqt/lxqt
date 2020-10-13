/*
 *      fm-file-ops-job-change-attr.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>

#include "fm-file-ops-job-change-attr.h"
#include "fm-folder.h"

static const char query[] =  G_FILE_ATTRIBUTE_STANDARD_TYPE","
                               G_FILE_ATTRIBUTE_STANDARD_NAME","
                               G_FILE_ATTRIBUTE_UNIX_GID","
                               G_FILE_ATTRIBUTE_UNIX_UID","
                               G_FILE_ATTRIBUTE_UNIX_MODE","
                               G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME;

static gboolean _fm_file_ops_job_change_attr_file(FmFileOpsJob* job, GFile* gf,
                                                  GFileInfo* inf, FmFolder *folder)
{
    GError* err = NULL;
    FmJob* fmjob = FM_JOB(job);
    GCancellable* cancellable = fm_job_get_cancellable(fmjob);
    FmPath *path;
    GFileType type;
    gboolean ret = TRUE;
    gboolean changed = FALSE;

    if( !inf)
    {
_retry_query_info:
        inf = g_file_query_info(gf, query,
                            G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                            cancellable, &err);
        if(!inf)
        {
            FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
            g_error_free(err);
            err = NULL;
            if(act == FM_JOB_RETRY)
                goto _retry_query_info;
        }
    }
    else
        g_object_ref(inf);

    type = g_file_info_get_file_type(inf);

    /* change owner */
    if( !fm_job_is_cancelled(fmjob) && job->uid != -1 )
    {
_retry_change_owner:
        if(!g_file_set_attribute_uint32(gf, G_FILE_ATTRIBUTE_UNIX_UID,
                                                  job->uid, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                                  cancellable, &err) )
        {
            FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
            g_error_free(err);
            err = NULL;
            if(act == FM_JOB_RETRY)
                goto _retry_change_owner;
        }
        else
            changed = TRUE;
    }

    /* change group */
    if( !fm_job_is_cancelled(fmjob) && job->gid != -1 )
    {
_retry_change_group:
        if(!g_file_set_attribute_uint32(gf, G_FILE_ATTRIBUTE_UNIX_GID,
                                                  job->gid, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                                  cancellable, &err) )
        {
            FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
            g_error_free(err);
            err = NULL;
            if(act == FM_JOB_RETRY)
                goto _retry_change_group;
        }
        else
            changed = TRUE;
    }

    /* change mode */
    if( !fm_job_is_cancelled(fmjob) && job->new_mode_mask )
    {
        guint32 mode = g_file_info_get_attribute_uint32(inf, G_FILE_ATTRIBUTE_UNIX_MODE);
        mode &= ~job->new_mode_mask;
        mode |= (job->new_mode & job->new_mode_mask);

        /* FIXME: this behavior should be optional. */
        /* treat dirs with 'r' as 'rx' */
        if(type == G_FILE_TYPE_DIRECTORY)
        {
            if((job->new_mode_mask & S_IRUSR) && (mode & S_IRUSR))
                mode |= S_IXUSR;
            if((job->new_mode_mask & S_IRGRP) && (mode & S_IRGRP))
                mode |= S_IXGRP;
            if((job->new_mode_mask & S_IROTH) && (mode & S_IROTH))
                mode |= S_IXOTH;
        }

        /* new mode */
_retry_chmod:
        if( !g_file_set_attribute_uint32(gf, G_FILE_ATTRIBUTE_UNIX_MODE,
                                         mode, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                         cancellable, &err) )
        {
            FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
            g_error_free(err);
            err = NULL;
            if(act == FM_JOB_RETRY)
                goto _retry_chmod;
        }
        else
            changed = TRUE;
    }
    /* change display name, icon, hidden, target */
    if (!fm_job_is_cancelled(fmjob) && job->display_name)
    {
        GFile *renamed;
_retry_disp_name:
        renamed = g_file_set_display_name(gf, job->display_name, cancellable, &err);
        if (renamed == NULL)
        {
            FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
            g_clear_error(&err);
            if(act == FM_JOB_RETRY)
                goto _retry_disp_name;
        }
        else
        {
            g_object_unref(renamed);
            changed = TRUE;
        }
    }
    if (!fm_job_is_cancelled(fmjob) && job->icon)
    {
_retry_change_icon:
        if (!g_file_set_attribute(gf, G_FILE_ATTRIBUTE_STANDARD_ICON,
                                  G_FILE_ATTRIBUTE_TYPE_OBJECT, job->icon,
                                  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                  cancellable, &err))
        {
            FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
            g_clear_error(&err);
            if(act == FM_JOB_RETRY)
                goto _retry_change_icon;
        }
        else
            changed = TRUE;
    }
    if (!fm_job_is_cancelled(fmjob) && job->set_hidden >= 0)
    {
        gboolean hidden = job->set_hidden > 0;
_retry_change_hidden:
        if (!g_file_set_attribute(gf, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN,
                                  G_FILE_ATTRIBUTE_TYPE_BOOLEAN, &hidden,
                                  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                  cancellable, &err))
        {
            FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
            g_clear_error(&err);
            if(act == FM_JOB_RETRY)
                goto _retry_change_hidden;
        }
        else
            changed = TRUE;
    }
    if (!fm_job_is_cancelled(fmjob) && job->target)
    {
_retry_change_target:
        if (!g_file_set_attribute_string(gf, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI,
                                         job->target, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                         cancellable, &err))
        {
            FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
            g_clear_error(&err);
            if(act == FM_JOB_RETRY)
                goto _retry_change_target;
        }
        else
            changed = TRUE;
    }

    /* currently processed file. */
    if(inf)
    {
        fm_file_ops_job_emit_cur_file(job, g_file_info_get_display_name(inf));
        g_object_unref(inf);
        inf = NULL;
    }
    else
    {
        char* basename = g_file_get_basename(gf);
        char* disp = basename ? g_filename_display_name(basename) : NULL;
                                                        /* FIXME: translate it */
        fm_file_ops_job_emit_cur_file(job, disp ? disp : "(invalid file)");
        g_free(disp);
        g_free(basename);
    }

    ++job->finished;
    fm_file_ops_job_emit_percent(job);

    path = fm_path_new_for_gfile(gf);
    if( !fm_job_is_cancelled(fmjob) && job->recursive && type == G_FILE_TYPE_DIRECTORY)
    {
        GFileEnumerator* enu;
        FmFolder *sub_folder;
_retry_enum_children:
        enu = g_file_enumerate_children(gf, query,
                                    G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                    cancellable, &err);
        if(!enu)
        {
            FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
            g_error_free(err);
            err = NULL;
            if(act == FM_JOB_RETRY)
                goto _retry_enum_children;
            fm_path_unref(path);
            return FALSE;
        }

        sub_folder = fm_folder_find_by_path(path);
        while( ! fm_job_is_cancelled(fmjob) )
        {
            inf = g_file_enumerator_next_file(enu, cancellable, &err);
            if(inf)
            {
                GFile* sub = g_file_get_child(gf, g_file_info_get_name(inf));
                ret = _fm_file_ops_job_change_attr_file(job, sub, inf, sub_folder);
                g_object_unref(sub);
                g_object_unref(inf);
                if(!ret) /* _fm_file_ops_job_change_attr_file() failed */
                    break;
            }
            else
            {
                if(err)
                {
                    fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
                    g_error_free(err);
                    err = NULL;
                    /* FM_JOB_RETRY is not supported here */
                }
                else /* EOF */
                    break;
            }
        }
        g_object_unref(enu);
        if (sub_folder)
            g_object_unref(sub_folder);
        if(!folder || !_fm_folder_event_file_changed(folder, path))
            fm_path_unref(path);
    }
    else if (!changed || !folder || !_fm_folder_event_file_changed(folder, path))
        fm_path_unref(path);
    return ret;
}

gboolean _fm_file_ops_job_change_attr_run(FmFileOpsJob* job)
{
    GList* l;

    /* prepare the job, count total work needed with FmDeepCountJob */
    if(job->recursive)
    {
        FmDeepCountJob* dc = fm_deep_count_job_new(job->srcs, FM_DC_JOB_DEFAULT);
        fm_job_run_sync(FM_JOB(dc));
        job->total = dc->count;
        g_object_unref(dc);
    }
    else
        job->total = fm_path_list_get_length(job->srcs);

    g_debug("total number of files to change attribute: %llu", (long long unsigned int)job->total);

    fm_file_ops_job_emit_prepared(job);

    l = fm_path_list_peek_head_link(job->srcs);
    /* check if we trying to set display name for more than one file and fail */
    if (!fm_job_is_cancelled(FM_JOB(job)) && l->next &&
        (job->display_name || job->target))
    {
        GError *error;

        if (job->display_name)
            error =  g_error_new(G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                                 _("Setting display name can be done only for single file"));
        else
            error =  g_error_new(G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                                 _("Setting target can be done only for single file"));
        fm_job_emit_error(FM_JOB(job), error, FM_JOB_ERROR_CRITICAL);
        g_error_free(error);
        return FALSE;
    }
    for(; ! fm_job_is_cancelled(FM_JOB(job)) && l;l=l->next)
    {
        gboolean ret;
        GFile* src = fm_path_to_gfile(FM_PATH(l->data));
        FmFolder *folder = fm_folder_find_by_path(l->data);

        ret = _fm_file_ops_job_change_attr_file(job, src, NULL, folder);
        g_object_unref(src);
        if (folder)
            g_object_unref(folder);

        if(!ret) /* error! */
            return FALSE;
    }
    return TRUE;
}
