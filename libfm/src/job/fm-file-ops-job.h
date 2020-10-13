/*
 *      fm-file-ops-job.h
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


#ifndef __FM_FILE_OPS_JOB_H__
#define __FM_FILE_OPS_JOB_H__

#include "fm-job.h"
#include "fm-deep-count-job.h"
#include "fm-file-info.h"

G_BEGIN_DECLS

#define FM_FILE_OPS_JOB_TYPE                (fm_file_ops_job_get_type())
#define FM_FILE_OPS_JOB(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            FM_FILE_OPS_JOB_TYPE, FmFileOpsJob))
#define FM_FILE_OPS_JOB_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST((klass),\
            FM_FILE_OPS_JOB_TYPE, FmFileOpsJobClass))
#define FM_IS_FILE_OPS_JOB(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            FM_FILE_OPS_JOB_TYPE))
#define FM_IS_FILE_OPS_JOB_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),\
            FM_FILE_OPS_JOB_TYPE))

typedef struct _FmFileOpsJob            FmFileOpsJob;
typedef struct _FmFileOpsJobClass        FmFileOpsJobClass;

/**
 * FmFileOpType:
 * @FM_FILE_OP_NONE: dummy
 * @FM_FILE_OP_MOVE: move file
 * @FM_FILE_OP_COPY: copy file
 * @FM_FILE_OP_TRASH: move file to trash can
 * @FM_FILE_OP_UNTRASH: restore file from trash can
 * @FM_FILE_OP_DELETE: erase file
 * @FM_FILE_OP_LINK: create symbolic link
 * @FM_FILE_OP_CHANGE_ATTR: change file owner and attributes
 *
 * Operation for #FmFileOpsJob
 */
typedef enum {
    FM_FILE_OP_NONE,
    FM_FILE_OP_MOVE,
    FM_FILE_OP_COPY,
    FM_FILE_OP_TRASH,
    FM_FILE_OP_UNTRASH,
    FM_FILE_OP_DELETE,
    FM_FILE_OP_LINK,
    FM_FILE_OP_CHANGE_ATTR
} FmFileOpType;

/**
 * FmFileOpOption:
 * @FM_FILE_OP_CANCEL: cancel operation
 * @FM_FILE_OP_OVERWRITE: overwrite existing file
 * @FM_FILE_OP_RENAME: change name and continue
 * @FM_FILE_OP_SKIP: skip this file
 * @FM_FILE_OP_SKIP_ERROR: not supported
 *
 * Operation selection on error.
 */
typedef enum {
    FM_FILE_OP_CANCEL = 0,
    FM_FILE_OP_OVERWRITE = 1<<0,
    FM_FILE_OP_RENAME = 1<<1,
    FM_FILE_OP_SKIP = 1<<2,
    FM_FILE_OP_SKIP_ERROR = 1<<3
} FmFileOpOption;

/* FIXME: maybe we should create derived classes for different kind
 * of file operations rather than use one class to handle all kinds of
 * file operations. */

struct _FmFileOpsJob
{
    FmJob parent;
    FmFileOpType type;
    FmPathList* srcs;
    FmPath* dest;
    const char* dest_fs_id;

    goffset total;
    goffset finished;
    goffset current_file_finished;
    /* goffset rate; */
    guint percent;
/*
    time_t started_time;
    time_t elapsed_time;
    time_t remaining_time;
    FmFileOpOption default_option;
*/

    union
    {
        gboolean recursive; /* used by chmod/chown only */
        gboolean skip_dir_content; /* used by _fm_file_ops_job_copy_file */
    };

    /* for chmod and chown */
    gint32 uid;
    gint32 gid;
    mode_t new_mode;
    mode_t new_mode_mask;

    /* for change attributes */
    char *display_name;
    GIcon *icon;
    char *target;
    gint8 set_hidden;

    FmFileOpOption supported_options;

    /*< private >*/
    gpointer _reserved1;
    gpointer _reserved2;
};

/**
 * FmFileOpsJobClass
 * @parent_class: the parent class
 * @prepared: the class closure for the #FmFileOpsJob::prepared signal
 * @cur_file: the class closure for the #FmFileOpsJob::cur-file signal
 * @percent: the class closure for the #FmFileOpsJob::percent signal
 * @ask_rename: the class closure for the #FmFileOpsJob::ask-rename signal
 */
struct _FmFileOpsJobClass
{
    FmJobClass parent_class;
    void (*prepared)(FmFileOpsJob* job);
    void (*cur_file)(FmFileOpsJob* job, const char* file);
    void (*percent)(FmFileOpsJob* job, guint percent);
    FmFileOpOption (*ask_rename)(FmFileOpsJob* job, FmFileInfo* src, FmFileInfo* dest, char** new_name);
};

GType fm_file_ops_job_get_type        (void);
FmFileOpsJob* fm_file_ops_job_new(FmFileOpType type, FmPathList* files);
void fm_file_ops_job_set_dest(FmFileOpsJob* job, FmPath* dest);
FmPath* fm_file_ops_job_get_dest(FmFileOpsJob* job);

/* This only work for change attr jobs. */
void fm_file_ops_job_set_recursive(FmFileOpsJob* job, gboolean recursive);

void fm_file_ops_job_set_chmod(FmFileOpsJob* job, mode_t new_mode, mode_t new_mode_mask);
void fm_file_ops_job_set_chown(FmFileOpsJob* job, gint uid, gint gid);

/* supported attributes are: display name, icon, hidden, target */
void fm_file_ops_job_set_display_name(FmFileOpsJob *job, const char *name);
void fm_file_ops_job_set_icon(FmFileOpsJob *job, GIcon *icon);
void fm_file_ops_job_set_hidden(FmFileOpsJob *job, gboolean hidden);
void fm_file_ops_job_set_target(FmFileOpsJob *job, const char *url);

void fm_file_ops_job_emit_prepared(FmFileOpsJob* job);
void fm_file_ops_job_emit_cur_file(FmFileOpsJob* job, const char* cur_file);
void fm_file_ops_job_emit_percent(FmFileOpsJob* job);
FmFileOpOption fm_file_ops_job_ask_rename(FmFileOpsJob* job, GFile* src, GFileInfo* src_inf, GFile* dest, GFile** new_dest);
FmFileOpOption _fm_file_ops_job_ask_new_name(FmFileOpsJob* job, GFile* src,
                                             GFile* dest, GFile** new_dest,
                                             gboolean dest_exists);
FmFileOpOption fm_file_ops_job_get_options(FmFileOpsJob* job);

G_END_DECLS

#endif /* __FM_FILE_OPS_JOB_H__ */
