/*
 *      fm-deep-count-job.c
 *
 *      Copyright 2009 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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
 * SECTION:fm-deep-count-job
 * @short_description: Job to gather information about file sizes.
 * @title: FmDeepCountJob
 *
 * @include: libfm/fm.h
 *
 * The #FmDeepCountJob can be used to recursively gather information about
 * some files and directories content before copy or move. It counts total
 * size of all given files and directories, and size on disk for them.
 * If flags for the job include FM_DC_JOB_PREPARE_MOVE then also count of
 * files to move between volumes will be counted as well.
 */

#include "fm-deep-count-job.h"
#include <glib/gstdio.h>
#include <errno.h>

static void fm_deep_count_job_dispose              (GObject *object);
G_DEFINE_TYPE(FmDeepCountJob, fm_deep_count_job, FM_TYPE_JOB);

static gboolean fm_deep_count_job_run(FmJob* job);

static gboolean deep_count_posix(FmDeepCountJob* job, const char* path);
static gboolean deep_count_gio(FmDeepCountJob* job, GFileInfo* inf, GFile* gf);

static const char query_str[] =
                G_FILE_ATTRIBUTE_STANDARD_TYPE","
                G_FILE_ATTRIBUTE_STANDARD_NAME","
                G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL","
                G_FILE_ATTRIBUTE_STANDARD_SIZE","
                G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE","
                G_FILE_ATTRIBUTE_ID_FILESYSTEM;

static void fm_deep_count_job_class_init(FmDeepCountJobClass *klass)
{
    GObjectClass *g_object_class;
    FmJobClass* job_class;
    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->dispose = fm_deep_count_job_dispose;
    /* use finalize from parent class */

    job_class = FM_JOB_CLASS(klass);
    job_class->run = fm_deep_count_job_run;
}


static void fm_deep_count_job_dispose(GObject *object)
{
    FmDeepCountJob *self;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_DEEP_COUNT_JOB(object));

    self = (FmDeepCountJob*)object;

    if(self->paths)
    {
        fm_path_list_unref(self->paths);
        self->paths = NULL;
    }
    G_OBJECT_CLASS(fm_deep_count_job_parent_class)->dispose(object);
}


static void fm_deep_count_job_init(FmDeepCountJob *self)
{
    fm_job_init_cancellable(FM_JOB(self));
}

/**
 * fm_deep_count_job_new
 * @paths: list of files and directories to count sizes
 * @flags: flags of the counting behavior
 *
 * Creates a new #FmDeepCountJob which can be ran via #FmJob API.
 *
 * Returns: (transfer full): a new #FmDeepCountJob object.
 *
 * Since: 0.1.0
 */
FmDeepCountJob *fm_deep_count_job_new(FmPathList* paths, FmDeepCountJobFlags flags)
{
    FmDeepCountJob* job = (FmDeepCountJob*)g_object_new(FM_DEEP_COUNT_JOB_TYPE, NULL);
    job->paths = fm_path_list_ref(paths);
    job->flags = flags;
    return job;
}

static gboolean fm_deep_count_job_run(FmJob* job)
{
    FmDeepCountJob* dc = (FmDeepCountJob*)job;
    GList* l;

    l = fm_path_list_peek_head_link(dc->paths);
    for(; !fm_job_is_cancelled(job) && l; l=l->next)
    {
        FmPath* path = FM_PATH(l->data);
        if(fm_path_is_native(path)) /* if it's a native file, use posix APIs */
        {
            char *path_str = fm_path_to_str(path);
            deep_count_posix( dc, path_str );
            g_free(path_str);
        }
        else
        {
            GFile* gf = fm_path_to_gfile(path);
            deep_count_gio( dc, NULL, gf );
            g_object_unref(gf);
        }
    }
    return TRUE;
}

static gboolean deep_count_posix(FmDeepCountJob* job, const char *path)
{
    FmJob* fmjob = FM_JOB(job);
    struct stat st;
    int ret;

_retry_stat:
    if( G_UNLIKELY(job->flags & FM_DC_JOB_FOLLOW_LINKS) )
        ret = stat(path, &st);
    else
        ret = lstat(path, &st);

    if( ret == 0 )
    {
        ++job->count;
        /* SF bug #892: dir file size is not relevant in the summary */
        if (!S_ISDIR(st.st_mode))
            job->total_size += (goffset)st.st_size;
        job->total_ondisk_size += (st.st_blocks * 512);

        /* NOTE: if job->dest_dev is 0, that means our destination
         * folder is not on native UNIX filesystem. Hence it's not
         * on the same device. Our st.st_dev will always be non-zero
         * since our file is on a native UNIX filesystem. */

        /* only descends into files on the same filesystem */
        if( job->flags & FM_DC_JOB_SAME_FS )
        {
            if( st.st_dev != job->dest_dev )
                return TRUE;
        }
        /* only descends into files on the different filesystem */
        else if( job->flags & FM_DC_JOB_PREPARE_MOVE )
        {
            if( st.st_dev == job->dest_dev )
                return TRUE;
        }
    }
    else
    {
        GError* err = g_error_new(G_IO_ERROR, g_io_error_from_errno(errno), "%s", g_strerror(errno));
        FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
        g_error_free(err);
        err = NULL;
        if(act == FM_JOB_RETRY)
            goto _retry_stat;
        return FALSE;
    }
    if(fm_job_is_cancelled(fmjob))
        return FALSE;

    if( S_ISDIR(st.st_mode) ) /* if it's a dir */
    {
        GDir* dir_ent = g_dir_open(path, 0, NULL);
        if(dir_ent)
        {
            const char* basename;
            while( !fm_job_is_cancelled(fmjob)
                && (basename = g_dir_read_name(dir_ent)) )
            {
                char *sub = g_build_filename(path, basename, NULL);
                if(!fm_job_is_cancelled(fmjob))
                {
                    if(deep_count_posix(job, sub))
                    {
                        /* for moving across different devices, an additional 'delete'
                         * for source file is needed. so let's +1 for the delete.*/
                        if(job->flags & FM_DC_JOB_PREPARE_MOVE)
                        {
                            ++job->total_size;
                            ++job->total_ondisk_size;
                            ++job->count;
                        }
                    }
                }
                g_free(sub);
            }
            g_dir_close(dir_ent);
        }
    }
    return TRUE;
}

static gboolean deep_count_gio(FmDeepCountJob* job, GFileInfo* inf, GFile* gf)
{
    FmJob* fmjob = FM_JOB(job);
    GError* err = NULL;
    GFileType type;
    const char* fs_id;
    gboolean descend;

    if(inf)
        g_object_ref(inf);
    else
    {
_retry_query_info:
        inf = g_file_query_info(gf, query_str,
                    (job->flags & FM_DC_JOB_FOLLOW_LINKS) ? 0 : G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                    fm_job_get_cancellable(fmjob), &err);
        if(!inf)
        {
            FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
            g_error_free(err);
            err = NULL;
            if(act == FM_JOB_RETRY)
                goto _retry_query_info;
            return FALSE;
        }
    }
    if(fm_job_is_cancelled(fmjob))
    {
        g_object_unref(inf);
        return FALSE;
    }

    type = g_file_info_get_file_type(inf);
    descend = TRUE;

    ++job->count;
    /* SF bug #892: dir file size is not relevant in the summary */
    if (type != G_FILE_TYPE_DIRECTORY)
        job->total_size += g_file_info_get_size(inf);
    job->total_ondisk_size += g_file_info_get_attribute_uint64(inf, G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE);

    /* prepare for moving across different devices */
    if( job->flags & FM_DC_JOB_PREPARE_MOVE )
    {
        fs_id = g_file_info_get_attribute_string(inf, G_FILE_ATTRIBUTE_ID_FILESYSTEM);
        if( g_strcmp0(fs_id, job->dest_fs_id) != 0 )
        {
            /* files on different device requires an additional 'delete' for the source file. */
            ++job->total_size; /* this is for the additional delete */
            ++job->total_ondisk_size;
            ++job->count;
        }
        else
            descend = FALSE;
    }

    if( type == G_FILE_TYPE_DIRECTORY )
    {
        FmPath* fm_path = fm_path_new_for_gfile(gf);
        /* check if we need to decends into the dir. */
        /* trash:/// doesn't support deleting files recursively */
        if(job->flags & FM_DC_JOB_PREPARE_DELETE && fm_path_is_trash(fm_path) && ! fm_path_is_trash_root(fm_path))
            descend = FALSE;
        else
        {
            /* only descends into files on the same filesystem */
            if( job->flags & FM_DC_JOB_SAME_FS )
            {
                fs_id = g_file_info_get_attribute_string(inf, G_FILE_ATTRIBUTE_ID_FILESYSTEM);
                descend = (g_strcmp0(fs_id, job->dest_fs_id) == 0);
            }
        }
        fm_path_unref(fm_path);
        g_object_unref(inf);
        inf = NULL;

        if(descend)
        {
            GFileEnumerator* enu;
        _retry_enum_children:
            enu = g_file_enumerate_children(gf, query_str,
                                G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                fm_job_get_cancellable(fmjob), &err);
            if(enu)
            {
                while( !fm_job_is_cancelled(fmjob) )
                {
                    inf = g_file_enumerator_next_file(enu, fm_job_get_cancellable(fmjob), &err);
                    if(inf)
                    {
                        GFile* child = g_file_get_child(gf, g_file_info_get_name(inf));
                        deep_count_gio(job, inf, child);
                        g_object_unref(child);
                        g_object_unref(inf);
                        inf = NULL;
                    }
                    else
                    {
                        if(err) /* error! */
                        {
                            /* FM_JOB_RETRY is not supported */
                            /*FmJobErrorAction act = */
                            fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
                            g_error_free(err);
                            err = NULL;
                        }
                        else
                        {
                            /* EOF is reached, do nothing. */
                            break;
                        }
                    }
                }
                g_file_enumerator_close(enu, NULL, NULL);
                g_object_unref(enu);
            }
            else
            {
                FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
                g_error_free(err);
                err = NULL;
                if(act == FM_JOB_RETRY)
                    goto _retry_enum_children;
            }
        }
    }
    else
        g_object_unref(inf);

    return TRUE;
}

/**
 * fm_deep_count_job_set_dest
 * @dc: a job to set the destination
 * @dev: UNIX device ID
 * @fs_id: (allow-none): filesystem id in gio format
 *
 * Sets destination for the job @dc that will be used when the job is
 * ran with any of flags FM_DC_JOB_SAME_FS or FM_DC_JOB_PREPARE_MOVE.
 * If @dev is 0 (and @fs_id is NULL) then destination device is non on
 * native filesystem.
 *
 * Since: 0.1.0
 */
void fm_deep_count_job_set_dest(FmDeepCountJob* dc, dev_t dev, const char* fs_id)
{
    dc->dest_dev = dev;
    if(fs_id)
        dc->dest_fs_id = g_intern_string(fs_id);
}
