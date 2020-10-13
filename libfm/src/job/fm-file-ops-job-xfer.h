/*
 *      fm-file-ops-xfer.c
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

#ifndef __FM_FILE_OPS_JOB_XFER_H__
#define __FM_FILE_OPS_JOB_XFER_H__

#include <glib.h>
#include <gio/gio.h>
#include "fm-file-ops-job.h"
#include "fm-folder.h"

G_BEGIN_DECLS

/* gboolean _fm_file_ops_job_copy_file(FmFileOpsJob* job, GFile* src, GFileInfo* inf, GFile* dest); */
gboolean _fm_file_ops_job_copy_run(FmFileOpsJob* job);

gboolean _fm_file_ops_job_move_file(FmFileOpsJob* job, GFile* src, GFileInfo* inf, GFile* dest, FmPath *src_path, FmFolder *src_folder, FmFolder *dst_folder);
gboolean _fm_file_ops_job_move_run(FmFileOpsJob* job);

G_END_DECLS

#endif
