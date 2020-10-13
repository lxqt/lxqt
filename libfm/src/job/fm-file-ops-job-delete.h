/*
 *      fm-file-ops-job-delete.h
 *
 *      Copyright 2009 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef __FM_FILE_OPS_JOB_DELETE_H__
#define __FM_FILE_OPS_JOB_DELETE_H__

#include <glib.h>
#include <gio/gio.h>
#include "fm-file-ops-job.h"
#include "fm-folder.h"

G_BEGIN_DECLS

gboolean _fm_file_ops_job_delete_file(FmJob* job, GFile* gf, GFileInfo* inf,
                                      FmFolder *folder, gboolean only_empty);
gboolean _fm_file_ops_job_delete_run(FmFileOpsJob* job);

/* gboolean _fm_file_ops_job_trash_file(FmJob* job, GFile* gf, GFileInfo* inf); */
gboolean _fm_file_ops_job_trash_run(FmFileOpsJob* job);

/* gboolean _fm_file_ops_job_untrash_file(FmJob* job, GFile* gf, GFileInfo* inf); */
gboolean _fm_file_ops_job_untrash_run(FmFileOpsJob* job);

G_END_DECLS

#endif
