/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2003, 2008 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 */

#ifndef FR_PROCESS_H
#define FR_PROCESS_H

#include <glib.h>
#include <sys/types.h>
#include "typedefs.h"

#define FR_TYPE_PROCESS            (fr_process_get_type ())
#define FR_PROCESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_PROCESS, FrProcess))
#define FR_PROCESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_PROCESS, FrProcessClass))
#define FR_IS_PROCESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_PROCESS))
#define FR_IS_PROCESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_PROCESS))
#define FR_PROCESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_PROCESS, FrProcessClass))

typedef struct _FrProcess        FrProcess;
typedef struct _FrProcessClass   FrProcessClass;
typedef struct _FrProcessPrivate FrProcessPrivate;

typedef void     (*ProcFunc)     (gpointer data);
typedef gboolean (*ContinueFunc) (gpointer data);
typedef void     (*LineFunc)     (char *line, gpointer data);

typedef struct {
	GIOChannel *source;
	GList      *raw;
	LineFunc    line_func;
	gpointer    line_data;
	GIOStatus   status;
	GError     *error;
} FrChannelData;

struct _FrProcess {
	GObject  __parent;

	/*< public >*/

	gboolean          term_on_stop;  /* whether we must terminate the
					  * command when calling
					  * fr_process_stop. */

	/*< public read-only >*/

	FrChannelData     out;
	FrChannelData     err;
	FrProcError       error;

	/*< protected >*/

	gboolean          restart;       /* whether to restart the process
			  		  * after an error. */

	FrProcessPrivate *priv;
};

struct _FrProcessClass {
	GObjectClass __parent_class;

	/* -- Signals -- */

	void (* start)         (FrProcess   *fr_proc);
	void (* done)          (FrProcess   *fr_proc,
				FrProcError *error);
	void (* sticky_only)   (FrProcess   *fr_proc);
};

GType       fr_process_get_type             (void);
FrProcess * fr_process_new                  (void);
void        fr_process_clear                (FrProcess    *fr_proc);
void        fr_process_begin_command        (FrProcess    *fr_proc,
					     const char   *arg);
void        fr_process_begin_command_at     (FrProcess    *fr_proc,
					     const char   *arg,
					     int           index);
void        fr_process_add_arg              (FrProcess    *fr_proc,
					     const char   *arg);
void        fr_process_add_arg_concat       (FrProcess    *fr_proc,
					     const char   *arg,
					     ...) G_GNUC_NULL_TERMINATED;
void        fr_process_add_arg_printf       (FrProcess    *fr_proc,
					     const char   *format,
					     ...) G_GNUC_PRINTF (2, 3);
void        fr_process_set_arg_at           (FrProcess    *fr_proc,
					     int           n_comm,
					     int           n_arg,
					     const char   *arg);
void        fr_process_set_begin_func       (FrProcess    *fr_proc,
					     ProcFunc      func,
					     gpointer      func_data);
void        fr_process_set_end_func         (FrProcess    *fr_proc,
					     ProcFunc      func,
					     gpointer      func_data);
void        fr_process_set_continue_func    (FrProcess    *fr_proc,
					     ContinueFunc  func,
					     gpointer      func_data);
void        fr_process_end_command          (FrProcess    *fr_proc);
void        fr_process_set_working_dir      (FrProcess    *fr_proc,
					     const char   *arg);
void        fr_process_set_sticky           (FrProcess    *fr_proc,
					     gboolean      sticky);
void        fr_process_set_ignore_error     (FrProcess    *fr_proc,
					     gboolean      ignore_error);
void        fr_process_use_standard_locale  (FrProcess    *fr_proc,
					     gboolean      use_stand_locale);
void        fr_process_set_out_line_func    (FrProcess    *fr_proc,
					     LineFunc      func,
					     gpointer      func_data);
void        fr_process_set_err_line_func    (FrProcess    *fr_proc,
					     LineFunc      func,
					     gpointer      func_data);
void        fr_process_start                (FrProcess    *fr_proc);
void        fr_process_stop                 (FrProcess    *fr_proc);

#endif /* FR_PROCESS_H */
