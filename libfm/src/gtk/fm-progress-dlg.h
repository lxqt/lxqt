/*
 *      fm-progress-dlg.h
 *      
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */


#ifndef __FM_PROGRESS_DLG_H__
#define __FM_PROGRESS_DLG_H__

#include <gtk/gtk.h>
#include "fm-file-ops-job.h"

G_BEGIN_DECLS

typedef struct _FmProgressDisplay FmProgressDisplay;

/* Run the file operation job with a progress dialog.
 * The returned data structure will be freed in idle handler automatically
 * when it's not needed anymore.
 * NOTE: INCONSISTENCY: it takes a reference from job */
FmProgressDisplay* fm_file_ops_job_run_with_progress(GtkWindow* parent, FmFileOpsJob* job);

G_END_DECLS

#endif /* __FM_PROGRESS_DLG_H__ */
