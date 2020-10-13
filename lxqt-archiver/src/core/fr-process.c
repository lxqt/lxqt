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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glib.h>
#include "fr-proc-error.h"
#include "fr-process.h"
#include "fr-marshal.h"
#include "glib-utils.h"

#define REFRESH_RATE 20
#define BUFFER_SIZE 16384

enum {
	START,
	DONE,
	STICKY_ONLY,
	LAST_SIGNAL
};

static GObjectClass *parent_class;
static guint fr_process_signals[LAST_SIGNAL] = { 0 };

static void fr_process_class_init (FrProcessClass *class);
static void fr_process_init       (FrProcess      *process);
static void fr_process_finalize   (GObject        *object);


typedef struct {
	GList        *args;              /* command to execute */
	char         *dir;               /* working directory */
	guint         sticky : 1;        /* whether the command must be
					  * executed even if a previous
					  * command has failed. */
	guint         ignore_error : 1;  /* whether to continue to execute
					  * other commands if this command
					  * fails. */
	ContinueFunc  continue_func;
	gpointer      continue_data;
	ProcFunc      begin_func;
	gpointer      begin_data;
	ProcFunc      end_func;
	gpointer      end_data;
} FrCommandInfo;


static FrCommandInfo *
fr_command_info_new (void)
{
	FrCommandInfo *info;

	info = g_new0 (FrCommandInfo, 1);
	info->args = NULL;
	info->dir = NULL;
	info->sticky = FALSE;
	info->ignore_error = FALSE;

	return info;
}


static void
fr_command_info_free (FrCommandInfo *info)
{
	if (info == NULL)
		return;

	if (info->args != NULL) {
		g_list_foreach (info->args, (GFunc) g_free, NULL);
		g_list_free (info->args);
		info->args = NULL;
	}

	if (info->dir != NULL) {
		g_free (info->dir);
		info->dir = NULL;
	}

	g_free (info);
}


static void
fr_channel_data_init (FrChannelData *channel)
{
	channel->source = NULL;
	channel->raw = NULL;
	channel->status = G_IO_STATUS_NORMAL;
	channel->error = NULL;
}


static void
fr_channel_data_close_source (FrChannelData *channel)
{
	if (channel->source != NULL) {
		g_io_channel_shutdown (channel->source, FALSE, NULL);
		g_io_channel_unref (channel->source);
		channel->source = NULL;
	}
}


static GIOStatus
fr_channel_data_read (FrChannelData *channel)
{
	char  *line;
	gsize  length;
	gsize  terminator_pos;

	channel->status = G_IO_STATUS_NORMAL;
	g_clear_error (&channel->error);

	while ((channel->status = g_io_channel_read_line (channel->source,
							  &line,
							  &length,
							  &terminator_pos,
							  &channel->error)) == G_IO_STATUS_NORMAL)
	{
		line[terminator_pos] = 0;
		channel->raw = g_list_prepend (channel->raw, line);
		if (channel->line_func != NULL)
			(*channel->line_func) (line, channel->line_data);
	}

	return channel->status;
}


static GIOStatus
fr_channel_data_flush (FrChannelData *channel)
{
	GIOStatus status;

	while (((status = fr_channel_data_read (channel)) != G_IO_STATUS_ERROR) && (status != G_IO_STATUS_EOF))
		/* void */;
	fr_channel_data_close_source (channel);

	return status;
}


static void
fr_channel_data_reset (FrChannelData *channel)
{
	fr_channel_data_close_source (channel);

	if (channel->raw != NULL) {
		g_list_foreach (channel->raw, (GFunc) g_free, NULL);
		g_list_free (channel->raw);
		channel->raw = NULL;
	}
}


static void
fr_channel_data_free (FrChannelData *channel)
{
	fr_channel_data_reset (channel);
}


static void
fr_channel_data_set_fd (FrChannelData *channel,
			int            fd,
			const char    *charset)
{
	fr_channel_data_reset (channel);

	channel->source = g_io_channel_unix_new (fd);
	g_io_channel_set_flags (channel->source, G_IO_FLAG_NONBLOCK, NULL);
	g_io_channel_set_buffer_size (channel->source, BUFFER_SIZE);
	if (charset != NULL)
		g_io_channel_set_encoding (channel->source, charset, NULL);
}


const char *try_charsets[] = { "UTF-8", "ISO-8859-1", "WINDOW-1252" };
int n_charsets = G_N_ELEMENTS (try_charsets);


struct _FrProcessPrivate {
	GPtrArray   *comm;                /* FrCommandInfo elements. */
	gint         n_comm;              /* total number of commands */
	gint         current_comm;        /* currenlty editing command. */

	GPid         command_pid;
	guint        check_timeout;

	FrProcError  first_error;

	gboolean     running;
	gboolean     stopping;
	gint         current_command;
	gint         error_command;       /* command that coused an error. */

	gboolean     use_standard_locale;
	gboolean     sticky_only;         /* whether to execute only sticky
			 		   * commands. */
	int          current_charset;
};


GType
fr_process_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FrProcessClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_process_class_init,
			NULL,
			NULL,
			sizeof (FrProcess),
			0,
			(GInstanceInitFunc) fr_process_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "FRProcess",
					       &type_info,
					       0);
	}

	return type;
}


static void
fr_process_class_init (FrProcessClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	fr_process_signals[START] =
		g_signal_new ("start",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrProcessClass, start),
			      NULL, NULL,
			      fr_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	fr_process_signals[DONE] =
		g_signal_new ("done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrProcessClass, done),
			      NULL, NULL,
			      fr_marshal_VOID__BOXED,
			      G_TYPE_NONE, 1,
			      FR_TYPE_PROC_ERROR);
	fr_process_signals[STICKY_ONLY] =
		g_signal_new ("sticky_only",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrProcessClass, sticky_only),
			      NULL, NULL,
			      fr_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	gobject_class->finalize = fr_process_finalize;

	class->start = NULL;
	class->done  = NULL;
}


static void
fr_process_init (FrProcess *process)
{
	process->priv = g_new0 (FrProcessPrivate, 1);

	process->term_on_stop = TRUE;

	process->priv->comm = g_ptr_array_new ();
	process->priv->n_comm = -1;
	process->priv->current_comm = -1;

	process->priv->command_pid = 0;
	fr_channel_data_init (&process->out);
	fr_channel_data_init (&process->err);

	process->error.gerror = NULL;
	process->priv->first_error.gerror = NULL;

	process->priv->check_timeout = 0;
	process->priv->running = FALSE;
	process->priv->stopping = FALSE;
	process->restart = FALSE;

	process->priv->current_charset = -1;

	process->priv->use_standard_locale = FALSE;
}


FrProcess *
fr_process_new (void)
{
	return FR_PROCESS (g_object_new (FR_TYPE_PROCESS, NULL));
}


static void fr_process_stop_priv (FrProcess *process, gboolean emit_signal);


static void
fr_process_finalize (GObject *object)
{
	FrProcess *process;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FR_IS_PROCESS (object));

	process = FR_PROCESS (object);

	fr_process_stop_priv (process, FALSE);
	fr_process_clear (process);

	g_ptr_array_free (process->priv->comm, FALSE);

	fr_channel_data_free (&process->out);
	fr_channel_data_free (&process->err);

	g_clear_error (&process->error.gerror);
	g_clear_error (&process->priv->first_error.gerror);

	g_free (process->priv);

	/* Chain up */

	if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


void
fr_process_begin_command (FrProcess  *process,
			  const char *arg)
{
	FrCommandInfo *info;

	g_return_if_fail (process != NULL);

	info = fr_command_info_new ();
	info->args = g_list_prepend (NULL, g_strdup (arg));

	g_ptr_array_add (process->priv->comm, info);

	process->priv->n_comm++;
	process->priv->current_comm = process->priv->n_comm;
}


void
fr_process_begin_command_at (FrProcess  *process,
			     const char *arg,
			     int         index)
{
	FrCommandInfo *info, *old_c_info;

	g_return_if_fail (process != NULL);
	g_return_if_fail (index >= 0 && index <= process->priv->n_comm);

	process->priv->current_comm = index;

	old_c_info = g_ptr_array_index (process->priv->comm, index);

	if (old_c_info != NULL)
		fr_command_info_free (old_c_info);

	info = fr_command_info_new ();
	info->args = g_list_prepend (NULL, g_strdup (arg));

	g_ptr_array_index (process->priv->comm, index) = info;
}


void
fr_process_set_working_dir (FrProcess  *process,
			    const char *dir)
{
	FrCommandInfo *info;

	g_return_if_fail (process != NULL);
	g_return_if_fail (process->priv->current_comm >= 0);

	info = g_ptr_array_index (process->priv->comm, process->priv->current_comm);
	if (info->dir != NULL)
		g_free (info->dir);
	info->dir = g_strdup (dir);
}


void
fr_process_set_sticky (FrProcess *process,
		       gboolean   sticky)
{
	FrCommandInfo *info;

	g_return_if_fail (process != NULL);
	g_return_if_fail (process->priv->current_comm >= 0);

	info = g_ptr_array_index (process->priv->comm, process->priv->current_comm);
	info->sticky = sticky;
}


void
fr_process_set_ignore_error (FrProcess *process,
			     gboolean   ignore_error)
{
	FrCommandInfo *info;

	g_return_if_fail (process != NULL);
	g_return_if_fail (process->priv->current_comm >= 0);

	info = g_ptr_array_index (process->priv->comm, process->priv->current_comm);
	info->ignore_error = ignore_error;
}


void
fr_process_add_arg (FrProcess  *process,
		    const char *arg)
{
	FrCommandInfo *info;

	g_return_if_fail (process != NULL);
	g_return_if_fail (process->priv->current_comm >= 0);

	info = g_ptr_array_index (process->priv->comm, process->priv->current_comm);
	info->args = g_list_prepend (info->args, g_strdup (arg));
}


void
fr_process_add_arg_concat (FrProcess  *process,
			   const char *arg1,
			   ...)
{
	GString *arg;
	va_list  args;
	char    *s;

	arg = g_string_new (arg1);

	va_start (args, arg1);
	while ((s = va_arg (args, char*)) != NULL)
		g_string_append (arg, s);
	va_end (args);

	fr_process_add_arg (process, arg->str);
	g_string_free (arg, TRUE);
}


void
fr_process_add_arg_printf (FrProcess    *fr_proc,
			   const char   *format,
			   ...)
{
	va_list  args;
	char    *arg;

	va_start (args, format);
	arg = g_strdup_vprintf (format, args);
	va_end (args);

	fr_process_add_arg (fr_proc, arg);

	g_free (arg);
}


void
fr_process_set_arg_at (FrProcess  *process,
		       int         n_comm,
		       int         n_arg,
		       const char *arg_value)
{
	FrCommandInfo *info;
	GList         *arg;

	g_return_if_fail (process != NULL);

	info = g_ptr_array_index (process->priv->comm, n_comm);
	arg = g_list_nth (info->args, n_arg);
	g_return_if_fail (arg != NULL);

	g_free (arg->data);
	arg->data = g_strdup (arg_value);
}


void
fr_process_set_begin_func (FrProcess    *process,
			   ProcFunc      func,
			   gpointer      func_data)
{
	FrCommandInfo *info;

	g_return_if_fail (process != NULL);

	info = g_ptr_array_index (process->priv->comm, process->priv->current_comm);
	info->begin_func = func;
	info->begin_data = func_data;
}


void
fr_process_set_end_func (FrProcess    *process,
			 ProcFunc      func,
			 gpointer      func_data)
{
	FrCommandInfo *info;

	g_return_if_fail (process != NULL);

	info = g_ptr_array_index (process->priv->comm, process->priv->current_comm);
	info->end_func = func;
	info->end_data = func_data;
}


void
fr_process_set_continue_func (FrProcess    *process,
			      ContinueFunc  func,
			      gpointer      func_data)
{
	FrCommandInfo *info;

	g_return_if_fail (process != NULL);

	if (process->priv->current_comm < 0)
		return;

	info = g_ptr_array_index (process->priv->comm, process->priv->current_comm);
	info->continue_func = func;
	info->continue_data = func_data;
}


void
fr_process_end_command (FrProcess *process)
{
	FrCommandInfo *info;

	g_return_if_fail (process != NULL);

	info = g_ptr_array_index (process->priv->comm, process->priv->current_comm);
	info->args = g_list_reverse (info->args);
}


void
fr_process_clear (FrProcess *process)
{
	gint i;

	g_return_if_fail (process != NULL);

	for (i = 0; i <= process->priv->n_comm; i++) {
		FrCommandInfo *info;

		info = g_ptr_array_index (process->priv->comm, i);
		fr_command_info_free (info);
		g_ptr_array_index (process->priv->comm, i) = NULL;
	}

	for (i = 0; i <= process->priv->n_comm; i++)
		g_ptr_array_remove_index_fast (process->priv->comm, 0);

	process->priv->n_comm = -1;
	process->priv->current_comm = -1;
}


void
fr_process_set_out_line_func (FrProcess *process,
			      LineFunc   func,
			      gpointer   data)
{
	g_return_if_fail (process != NULL);

	process->out.line_func = func;
	process->out.line_data = data;
}


void
fr_process_set_err_line_func (FrProcess *process,
			      LineFunc   func,
			      gpointer   data)
{
	g_return_if_fail (process != NULL);

	process->err.line_func = func;
	process->err.line_data = data;
}


static gboolean check_child (gpointer data);


static void
child_setup (gpointer user_data)
{
	FrProcess *process = user_data;

	if (process->priv->use_standard_locale)
		putenv ("LC_MESSAGES=C");

	/* detach from the tty */

	setsid ();

	/* create a process group to kill all the child processes when
	 * canceling the operation. */

	setpgid (0, 0);
}


static const char *
fr_process_get_charset (FrProcess *process)
{
	const char *charset = NULL;

	if (process->priv->current_charset >= 0)
		charset = try_charsets[process->priv->current_charset];
	else if (g_get_charset (&charset))
		charset = NULL;

	return charset;
}


static void
start_current_command (FrProcess *process)
{
	FrCommandInfo  *info;
	GList          *scan;
	char          **argv;
	int             out_fd, err_fd;
	int             i = 0;
	char           *commandline = "";
	gboolean        fixname = FALSE;

	debug (DEBUG_INFO, "%d/%d) ", process->priv->current_command, process->priv->n_comm);

	info = g_ptr_array_index (process->priv->comm, process->priv->current_command);

	argv = g_new (char *, g_list_length (info->args) + 1);

	for (scan = info->args; scan; scan = scan->next) {
		argv[i++] = scan->data;

		if (g_str_has_prefix(commandline, "mv")) {

			if ((i==3) && (!g_file_test(argv[2], G_FILE_TEST_EXISTS)) && (!fixname)) {
				char	rarfile[strlen(argv[2])+7];

				strcpy(rarfile, argv[2]);
				rarfile[strlen(rarfile)-3]=0;
				strcat(rarfile, "part1.rar");

				if (g_str_has_suffix(argv[2], ".7z")) {
					commandline = g_strconcat(commandline, " ", g_shell_quote(argv[2]), ".*", NULL);
					fixname = TRUE;
				}
				else if (g_str_has_suffix(argv[2], ".rar")) {
					rarfile[strlen(rarfile)-5]=0;
					commandline = g_strconcat(commandline, " ", g_shell_quote(rarfile), "*.rar", NULL);
					fixname = TRUE;
				}
			}
			else if ((i==4) && (fixname))
				commandline = g_strconcat(commandline, " \"$(dirname ", g_shell_quote(argv[3]), ")\"", NULL);
			else
				commandline = g_strconcat(commandline, " ", argv[(i-1)], NULL);
		}
		else if (g_str_has_prefix(argv[0], "mv")) {
			commandline = g_strconcat(commandline, "mv", NULL);
		}
	}

	argv[i] = NULL;

#ifdef DEBUG
	{
		int j;

		if (process->priv->use_standard_locale)
			g_print ("\tLC_MESSAGES=C\n");

		if (info->dir != NULL)
			g_print ("\tcd %s\n", info->dir);

		g_print ("\t");
		for (j = 0; j < i; j++)
			g_print ("%s ", argv[j]);
		g_print ("\n");
	}
#endif

	if ((fixname) && (system(commandline) != 0)) {
		g_warning ("The files could not be move: %s\n", commandline);
		return;
	}

	if (info->begin_func != NULL)
		(*info->begin_func) (info->begin_data);

	if (! g_spawn_async_with_pipes (info->dir,
					argv,
					NULL,
					(G_SPAWN_LEAVE_DESCRIPTORS_OPEN
					 | G_SPAWN_SEARCH_PATH
					 | G_SPAWN_DO_NOT_REAP_CHILD),
					child_setup,
					process,
					&process->priv->command_pid,
					NULL,
					&out_fd,
					&err_fd,
					&process->error.gerror))
	{
		process->error.type = FR_PROC_ERROR_SPAWN;
		g_signal_emit (G_OBJECT (process),
			       fr_process_signals[DONE],
			       0,
			       &process->error);
		g_free (argv);
		return;
	}

	g_free (argv);

	fr_channel_data_set_fd (&process->out, out_fd, fr_process_get_charset (process));
	fr_channel_data_set_fd (&process->err, err_fd, fr_process_get_charset (process));

	process->priv->check_timeout = g_timeout_add (REFRESH_RATE,
					              check_child,
					              process);
}


static gboolean
command_is_sticky (FrProcess *process,
		   int        i)
{
	FrCommandInfo *info;

	info = g_ptr_array_index (process->priv->comm, i);
	return info->sticky;
}


static void
allow_sticky_processes_only (FrProcess *process,
			     gboolean   emit_signal)
{
	if (! process->priv->sticky_only) {
		/* Remember the first error. */
		process->priv->error_command = process->priv->current_command;
		process->priv->first_error.type = process->error.type;
		process->priv->first_error.status = process->error.status;
		g_clear_error (&process->priv->first_error.gerror);
		if (process->error.gerror != NULL)
			process->priv->first_error.gerror = g_error_copy (process->error.gerror);
	}

	process->priv->sticky_only = TRUE;
	if (emit_signal)
		g_signal_emit (G_OBJECT (process),
			       fr_process_signals[STICKY_ONLY],
			       0);
}


static void
fr_process_set_error (FrProcess       *process,
		      FrProcErrorType  type,
		      int              status,
		      GError          *gerror)
{
	process->error.type = type;
	process->error.status = status;
	if (gerror != process->error.gerror) {
		g_clear_error (&process->error.gerror);
		if (gerror != NULL)
			process->error.gerror = g_error_copy (gerror);
	}
}


static gint
check_child (gpointer data)
{
	FrProcess      *process = data;
	FrCommandInfo  *info;
	pid_t           pid;
	int             status;
	gboolean        continue_process;
	gboolean        channel_error = FALSE;

	info = g_ptr_array_index (process->priv->comm, process->priv->current_command);

	/* Remove check. */

	g_source_remove (process->priv->check_timeout);
	process->priv->check_timeout = 0;

	if (fr_channel_data_read (&process->out) == G_IO_STATUS_ERROR) {
		fr_process_set_error (process, FR_PROC_ERROR_IO_CHANNEL, 0, process->out.error);
		channel_error = TRUE;
	}
	else if (fr_channel_data_read (&process->err) == G_IO_STATUS_ERROR) {
		fr_process_set_error (process, FR_PROC_ERROR_IO_CHANNEL, 0, process->err.error);
		channel_error = TRUE;
	}
	else {
		pid = waitpid (process->priv->command_pid, &status, WNOHANG);
		if (pid != process->priv->command_pid) {
			/* Add check again. */
			process->priv->check_timeout = g_timeout_add (REFRESH_RATE,
							              check_child,
							              process);
			return FALSE;
		}
	}

	if (info->ignore_error) {
		process->error.type = FR_PROC_ERROR_NONE;
		debug (DEBUG_INFO, "[ignore error]\n");
	}
	else if (! channel_error && (process->error.type != FR_PROC_ERROR_STOPPED)) {
		if (WIFEXITED (status)) {
			if (WEXITSTATUS (status) == 0)
				process->error.type = FR_PROC_ERROR_NONE;
			else if (WEXITSTATUS (status) == 255)
				process->error.type = FR_PROC_ERROR_COMMAND_NOT_FOUND;
			else {
				process->error.type = FR_PROC_ERROR_COMMAND_ERROR;
				process->error.status = WEXITSTATUS (status);
			}
		}
		else {
			process->error.type = FR_PROC_ERROR_EXITED_ABNORMALLY;
			process->error.status = 255;
		}
	}

	process->priv->command_pid = 0;

	if (fr_channel_data_flush (&process->out) == G_IO_STATUS_ERROR) {
		fr_process_set_error (process, FR_PROC_ERROR_IO_CHANNEL, 0, process->out.error);
		channel_error = TRUE;
	}
	else if (fr_channel_data_flush (&process->err) == G_IO_STATUS_ERROR) {
		fr_process_set_error (process, FR_PROC_ERROR_IO_CHANNEL, 0, process->err.error);
		channel_error = TRUE;
	}

	if (info->end_func != NULL)
		(*info->end_func) (info->end_data);

	/**/

	if (channel_error
	    && (process->error.type == FR_PROC_ERROR_IO_CHANNEL)
	    && g_error_matches (process->error.gerror, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE))
	{
		if (process->priv->current_charset < n_charsets - 1) {
			/* try with another charset */
			process->priv->current_charset++;
			process->priv->running = FALSE;
			process->restart = TRUE;
			fr_process_start (process);
			return FALSE;
		}
		/*fr_process_set_error (process, FR_PROC_ERROR_NONE, 0, NULL);*/
		fr_process_set_error (process, FR_PROC_ERROR_BAD_CHARSET, 0, process->error.gerror);
	}

	/* Check whether to continue or stop the process */

	continue_process = TRUE;
	if (info->continue_func != NULL)
		continue_process = (*info->continue_func) (info->continue_data);

	/* Execute next command. */
	if (continue_process) {
		if (process->error.type != FR_PROC_ERROR_NONE) {
			allow_sticky_processes_only (process, TRUE);
#ifdef DEBUG
			{
				GList *scan;

				g_print ("** ERROR **\n");
				for (scan = process->err.raw; scan; scan = scan->next)
					g_print ("%s\n", (char *)scan->data);
			}
#endif
		}

		if (process->priv->sticky_only) {
			do {
				process->priv->current_command++;
			} while ((process->priv->current_command <= process->priv->n_comm)
				 && ! command_is_sticky (process, process->priv->current_command));
		}
		else
			process->priv->current_command++;

		if (process->priv->current_command <= process->priv->n_comm) {
			start_current_command (process);
			return FALSE;
		}
	}

	/* Done */

	process->priv->current_command = -1;
	process->priv->use_standard_locale = FALSE;

	if (process->out.raw != NULL)
		process->out.raw = g_list_reverse (process->out.raw);
	if (process->err.raw != NULL)
		process->err.raw = g_list_reverse (process->err.raw);

	process->priv->running = FALSE;
	process->priv->stopping = FALSE;

	if (process->priv->sticky_only) {
		/* Restore the first error. */
		fr_process_set_error (process,
				      process->priv->first_error.type,
				      process->priv->first_error.status,
				      process->priv->first_error.gerror);
	}

	g_signal_emit (G_OBJECT (process),
		       fr_process_signals[DONE],
		       0,
		       &process->error);

	return FALSE;
}


void
fr_process_use_standard_locale (FrProcess *process,
				gboolean   use_stand_locale)
{
	g_return_if_fail (process != NULL);
	process->priv->use_standard_locale = use_stand_locale;
}


void
fr_process_start (FrProcess *process)
{
	g_return_if_fail (process != NULL);

	if (process->priv->running)
		return;

	fr_channel_data_reset (&process->out);
	fr_channel_data_reset (&process->err);

	process->priv->sticky_only = FALSE;
	process->priv->current_command = 0;
	fr_process_set_error (process, FR_PROC_ERROR_NONE, 0, NULL);

	if (! process->restart) {
		process->priv->current_charset = -1;
		g_signal_emit (G_OBJECT (process),
			       fr_process_signals[START],
			       0);
	}

	process->priv->stopping = FALSE;

	if (process->priv->n_comm == -1) {
		process->priv->running = FALSE;
		g_signal_emit (G_OBJECT (process),
			       fr_process_signals[DONE],
			       0,
			       &process->error);
	}
	else {
		process->priv->running = TRUE;
		start_current_command (process);
	}
}


static void
fr_process_stop_priv (FrProcess *process,
		      gboolean   emit_signal)
{
	g_return_if_fail (process != NULL);

	if (! process->priv->running)
		return;

	if (process->priv->stopping)
		return;

	process->priv->stopping = TRUE;
	process->error.type = FR_PROC_ERROR_STOPPED;

	if (command_is_sticky (process, process->priv->current_command))
		allow_sticky_processes_only (process, emit_signal);

	else if (process->term_on_stop && (process->priv->command_pid > 0))
		killpg (process->priv->command_pid, SIGTERM);

	else {
		if (process->priv->check_timeout != 0) {
			g_source_remove (process->priv->check_timeout);
			process->priv->check_timeout = 0;
		}

		process->priv->command_pid = 0;
		fr_channel_data_close_source (&process->out);
		fr_channel_data_close_source (&process->err);

		process->priv->running = FALSE;

		if (emit_signal)
			g_signal_emit (G_OBJECT (process),
				       fr_process_signals[DONE],
				       0,
				       &process->error);
	}
}


void
fr_process_stop (FrProcess *process)
{
	fr_process_stop_priv (process, TRUE);
}
