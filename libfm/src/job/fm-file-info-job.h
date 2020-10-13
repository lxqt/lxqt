/*
 *      fm-file-info-job.h
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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


#ifndef __FM_FILE_INFO_JOB_H__
#define __FM_FILE_INFO_JOB_H__

#include "fm-job.h"
#include "fm-file-info.h"

G_BEGIN_DECLS

#define FM_TYPE_FILE_INFO_JOB               (fm_file_info_job_get_type())
#define FM_FILE_INFO_JOB(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj),\
			FM_TYPE_FILE_INFO_JOB, FmFileInfoJob))
#define FM_FILE_INFO_JOB_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST((klass),\
			FM_TYPE_FILE_INFO_JOB, FmFileInfoJobClass))
#define FM_IS_FILE_INFO_JOB(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			FM_TYPE_FILE_INFO_JOB))
#define FM_IS_FILE_INFO_JOB_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),\
			FM_TYPE_FILE_INFO_JOB))

typedef struct _FmFileInfoJob           FmFileInfoJob;
typedef struct _FmFileInfoJobClass      FmFileInfoJobClass;

/**
 * FmFileInfoJobFlags:
 * @FM_FILE_INFO_JOB_NONE: default
 * @FM_FILE_INFO_JOB_FOLLOW_SYMLINK: not yet implemented
 * @FM_FILE_INFO_JOB_EMIT_FOR_EACH_FILE: emit #FmFileInfoJob::got-info for each file
 */
typedef enum {
    FM_FILE_INFO_JOB_NONE = 0,
    FM_FILE_INFO_JOB_FOLLOW_SYMLINK = 1 << 0, /* FIXME: not yet implemented */
    FM_FILE_INFO_JOB_EMIT_FOR_EACH_FILE = 1 << 1
} FmFileInfoJobFlags;

/**
 * FmFileInfoJob
 * @parent: the parent object
 * @flags: flags for the job
 * @file_infos: gathered data
 */
struct _FmFileInfoJob
{
    /*< public >*/
    FmJob parent;
    FmFileInfoJobFlags flags;
    FmFileInfoList* file_infos;
    /*< private >*/
    FmPath* current;
};

/**
 * FmFileInfoJobClass:
 * @got_info: the class closure for the #FmFileInfoJob::got-info signal.
 */
struct _FmFileInfoJobClass
{
    /*< private >*/
    FmJobClass parent_class;
    /*< public >*/
    void (*got_info)(FmFileInfoJob *job, FmFileInfo *info);
    /*< private >*/
    gpointer _reserved1;
};

GType fm_file_info_job_get_type(void);
FmFileInfoJob* fm_file_info_job_new(FmPathList* files_to_query, FmFileInfoJobFlags flags);

/* this can only be called before running the job. */
void fm_file_info_job_add(FmFileInfoJob* job, FmPath* path);
void fm_file_info_job_add_gfile(FmFileInfoJob* job, GFile* gf);

/* This API should only be called in error handler */
FmPath* fm_file_info_job_get_current(FmFileInfoJob* job);

#ifndef __GTK_DOC_IGNORE__
/* useful inline routines for FmJob classes */
static inline gboolean
_fm_file_info_job_get_info_for_native_file(FmJob* job, FmFileInfo* fi, const char* path, GError** err)
{
    if( ! fm_job_is_cancelled(job) )
        return fm_file_info_set_from_native_file(fi, path, err);
    return TRUE;
}

extern const char gfile_info_query_attribs[];

static inline gboolean
_fm_file_info_job_update_fs_readonly(GFile *gf, GFileInfo *inf, GCancellable *cancellable, GError **error)
{
    /* check if FS is R/O and set attr. into inf */
    GFileInfo *fs_inf = g_file_query_filesystem_info(gf, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY,
                                                     cancellable, error);
    if (fs_inf)
    {
        if (g_file_info_has_attribute(fs_inf, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY))
            g_file_info_set_attribute_boolean(inf, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY,
                    g_file_info_get_attribute_boolean(fs_inf, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY));
        g_object_unref(fs_inf);
        return TRUE;
    }
    return FALSE;
}

static inline gboolean
_fm_file_info_job_get_info_for_gfile(FmJob* job, FmFileInfo* fi, GFile* gf, GError** err)
{
    GFileInfo *inf;
    inf = g_file_query_info(gf, gfile_info_query_attribs, (GFileQueryInfoFlags)0, fm_job_get_cancellable(job), err);
    if( !inf )
        return FALSE;
    _fm_file_info_job_update_fs_readonly(gf, inf, NULL, NULL);
    fm_file_info_set_from_g_file_data(fi, gf, inf);
    g_object_unref(inf);

    return TRUE;
}
#endif /* __GTK_DOC_IGNORE__ */

G_END_DECLS

#endif /* __FM_FILE_INFO_JOB_H__ */
