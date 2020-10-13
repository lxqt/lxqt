/*
 *      fm-deep-count-job.h
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
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


#ifndef __FM_DEEP_COUNT_JOB_H__
#define __FM_DEEP_COUNT_JOB_H__

#include "fm-job.h"
#include "fm-path.h"
#include <gio/gio.h>
#include <sys/types.h>

G_BEGIN_DECLS

#define FM_DEEP_COUNT_JOB_TYPE              (fm_deep_count_job_get_type())
#define FM_DEEP_COUNT_JOB(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            FM_DEEP_COUNT_JOB_TYPE, FmDeepCountJob))
#define FM_DEEP_COUNT_JOB_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),\
            FM_DEEP_COUNT_JOB_TYPE, FmDeepCountJobClass))
#define FM_IS_DEEP_COUNT_JOB(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            FM_DEEP_COUNT_JOB_TYPE))
#define FM_IS_DEEP_COUNT_JOB_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass),\
            FM_DEEP_COUNT_JOB_TYPE))

typedef struct _FmDeepCountJob            FmDeepCountJob;
typedef struct _FmDeepCountJobClass        FmDeepCountJobClass;

/**
 * FmDeepCountJobFlags
 * @FM_DC_JOB_DEFAULT: do deep count for all files/folders
 * @FM_DC_JOB_FOLLOW_LINKS: don't follow symlinks
 * @FM_DC_JOB_SAME_FS: only do deep count for files on the same devices. what's the use case of this?
 * @FM_DC_JOB_PREPARE_MOVE: special handling for moving files. only do deep count for files on different devices
 * @FM_DC_JOB_PREPARE_DELETE: special handling for deleting files
 */
typedef enum {
    FM_DC_JOB_DEFAULT = 0,
    FM_DC_JOB_FOLLOW_LINKS = 1<<0,
    FM_DC_JOB_SAME_FS = 1<<1,
    FM_DC_JOB_PREPARE_MOVE = 1<<2,
    FM_DC_JOB_PREPARE_DELETE = 1 <<3
} FmDeepCountJobFlags;

/**
 * FmDeepCountJob
 * @parent: the parent object
 * @paths: list of paths to count
 * @flags: flags for counting
 * @total_size: counted total file size
 * @total_ondisk_size: counted total file size on disk
 * @count: number of files to be moved between devices
 */
struct _FmDeepCountJob
{
    /*< public >*/
    FmJob parent;
    FmPathList* paths;
    FmDeepCountJobFlags flags;
    goffset total_size;
    goffset total_ondisk_size;
    guint count;

    /*< private >*/
    gpointer _reserved1;
    gpointer _reserved2;
    /* used to count total size used when moving files */
    dev_t dest_dev;
    const char* dest_fs_id;
};

struct _FmDeepCountJobClass
{
    /*< private >*/
    FmJobClass parent_class;
};

GType fm_deep_count_job_get_type(void);
FmDeepCountJob* fm_deep_count_job_new(FmPathList* paths, FmDeepCountJobFlags flags);

/* dev is UNIX device ID.
 * fs_id is filesystem id in gio format (can be NULL).
 */
void fm_deep_count_job_set_dest(FmDeepCountJob* dc, dev_t dev, const char* fs_id);

G_END_DECLS

#endif /* __FM_DEEP_COUNT_JOB_H__ */
