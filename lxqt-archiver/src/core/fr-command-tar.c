/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.
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

#include <config.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <glib.h>
#include "tr-wrapper.h"

#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-tar.h"

#define ACTIVITY_DELAY 20

static void fr_command_tar_class_init  (FrCommandTarClass *class);
static void fr_command_tar_init        (FrCommand         *afile);
static void fr_command_tar_finalize    (GObject           *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;


/* -- list -- */

static time_t
mktime_from_string (char *date_s,
		    char *time_s)
{
	struct tm   tm = {0, };
	char      **fields;

	tm.tm_isdst = -1;

	/* date */

	fields = g_strsplit (date_s, "-", 3);
	if (fields[0] != NULL) {
		tm.tm_year = atoi (fields[0]) - 1900;
		if (fields[1] != NULL) {
			tm.tm_mon = atoi (fields[1]) - 1;
			if (fields[2] != NULL)
				tm.tm_mday = atoi (fields[2]);
		}
	}
	g_strfreev (fields);

	/* time */

	fields = g_strsplit (time_s, ":", 3);
	if (fields[0] != NULL) {
		tm.tm_hour = atoi (fields[0]);
		if (fields[1] != NULL) {
			tm.tm_min  = atoi (fields[1]);
			if (fields[2] != NULL)
				tm.tm_sec  = atoi (fields[2]);
		}
	}
	g_strfreev (fields);

	return mktime (&tm);
}


static char*
tar_get_last_field (const char *line,
		    int         start_from,
		    int         field_n)
{
	const char *f_start, *f_end;

	line = line + start_from;

	f_start = line;
	while ((*f_start == ' ') && (*f_start != *line))
		f_start++;
	f_end = f_start;

	while ((field_n > 0) && (*f_end != 0)) {
		if (*f_end == ' ') {
			field_n--;
			if (field_n != 0) {
				while ((*f_end == ' ') && (*f_end != *line))
					f_end++;
				f_start = f_end;
			}
		} else
			f_end++;
	}

	return g_strdup (f_start);
}


static void
process_line (char     *line,
	      gpointer  data)
{
	FileData    *fdata;
	FrCommand   *comm = FR_COMMAND (data);
	char       **fields;
	int          date_idx;
	char        *field_date, *field_time, *field_size, *field_name;
	char        *name;

	g_return_if_fail (line != NULL);

	date_idx = file_list__get_index_from_pattern (line, "%n%n%n%n-%n%n-%n%n %n%n:%n%n");
	if (date_idx < 0)
		return;

	fdata = file_data_new ();

	field_size = file_list__get_prev_field (line, date_idx, 1);
	fdata->size = g_ascii_strtoull (field_size, NULL, 10);
	g_free (field_size);

	field_date = file_list__get_next_field (line, date_idx, 1);
	field_time = file_list__get_next_field (line, date_idx, 2);
	fdata->modified = mktime_from_string (field_date, field_time);
	g_free (field_date);
	g_free (field_time);

	/* Full path */

	field_name = tar_get_last_field (line, date_idx, 3);
	fields = g_strsplit (field_name, " -> ", 2);

	if (fields[1] == NULL) {
		g_strfreev (fields);
		fields = g_strsplit (field_name, " link to ", 2);
	}

	name = g_strcompress (fields[0]);
	if (*name == '/') {
		fdata->full_path = g_strdup (name);
		fdata->original_path = fdata->full_path;
	} else {
		fdata->full_path = g_strconcat ("/", name, NULL);
		fdata->original_path = fdata->full_path + 1;
	}
	g_free (name);
	name = g_filename_from_utf8 (fdata->original_path, -1, NULL, NULL, NULL);
	if (name)
		fdata->original_path = name;

	if (fields[1] != NULL)
		fdata->link = g_strdup (fields[1]);
	g_strfreev (fields);
	g_free (field_name);

	fdata->dir = line[0] == 'd';
	if (fdata->dir)
		fdata->name = dir_name_from_path (fdata->full_path);
	else
		fdata->name = g_strdup (file_name_from_path (fdata->full_path));
	fdata->path = remove_level_from_path (fdata->full_path);

	if (*fdata->name == 0)
		file_data_free (fdata);
	else
		fr_command_add_file (comm, fdata);
}


static void
add_compress_arg (FrCommand *comm)
{
	if (is_mime_type (comm->mime_type, "application/x-compressed-tar"))
		fr_process_add_arg (comm->process, "-z");

	else if (is_mime_type (comm->mime_type, "application/x-bzip-compressed-tar"))
		if (is_program_in_path ("lbzip2"))
			fr_process_add_arg (comm->process, "--use-compress-program=lbzip2");
		else
			fr_process_add_arg (comm->process, "--use-compress-program=bzip2");

	else if (is_mime_type (comm->mime_type, "application/x-tarz")) {
		if (is_program_in_path ("gzip"))
			fr_process_add_arg (comm->process, "-z");
		else
			fr_process_add_arg (comm->process, "-Z");
	}
	else if (is_mime_type (comm->mime_type, "application/x-lrzip-compressed-tar"))
		fr_process_add_arg (comm->process, "--use-compress-program=lrzip");

	else if (is_mime_type (comm->mime_type, "application/x-lzip-compressed-tar"))
		fr_process_add_arg (comm->process, "--use-compress-program=lzip");

	else if (is_mime_type (comm->mime_type, "application/x-lzma-compressed-tar"))
		fr_process_add_arg (comm->process, "--use-compress-program=lzma");

	else if (is_mime_type (comm->mime_type, "application/x-xz-compressed-tar"))
		if (is_program_in_path ("pixz"))
			fr_process_add_arg (comm->process, "--use-compress-program=pixz");
		else
			fr_process_add_arg (comm->process, "--use-compress-program=xz");

	else if (is_mime_type (comm->mime_type, "application/x-zstd-compressed-tar"))
		fr_process_add_arg (comm->process, "--use-compress-program=zstd");

	else if (is_mime_type (comm->mime_type, "application/x-lzop-compressed-tar"))
		fr_process_add_arg (comm->process, "--use-compress-program=lzop");

	else if (is_mime_type (comm->mime_type, "application/x-7z-compressed-tar")) {
		FrCommandTar *comm_tar = (FrCommandTar*) comm;
		char         *option;

		option = g_strdup_printf ("--use-compress-program=%s", comm_tar->compress_command);
		fr_process_add_arg (comm->process, option);
		g_free (option);
	}
}


static void
begin_tar_command (FrCommand *comm)
{
	char *command = NULL;

	/* In solaris gtar is present under /usr/sfw/bin */

	command = g_find_program_in_path ("gtar");
#if defined (__SVR4) && defined (__sun)
	if (g_file_test ("/usr/sfw/bin/gtar", G_FILE_TEST_IS_EXECUTABLE)) {
		g_free (command);
		command = g_strdup ("/usr/sfw/bin/gtar");
	}
#endif
	if (command != NULL)
		fr_process_begin_command (comm->process, command);
	else
		fr_process_begin_command (comm->process, "tar");
	g_free (command);
}


static void
fr_command_tar_list (FrCommand *comm)
{
	fr_process_set_out_line_func (comm->process, process_line, comm);

	begin_tar_command (comm);
	fr_process_add_arg (comm->process, "--force-local");
	fr_process_add_arg (comm->process, "--no-wildcards");
	fr_process_add_arg (comm->process, "-tvf");
	fr_process_add_arg (comm->process, comm->filename);
	add_compress_arg (comm);
	fr_process_end_command (comm->process);
	fr_process_start (comm->process);
}


static gboolean
can_create_a_compressed_archive (FrCommand *comm)
{
	return comm->creating_archive && ! is_mime_type (comm->mime_type, "application/x-7z-compressed-tar");
}


static void
process_line__generic (char     *line,
		       gpointer  data,
		       char     *action_msg)
{
	FrCommand *comm = FR_COMMAND (data);

	if (line == NULL)
		return;

	if (line[strlen (line) - 1] == '/') /* ignore directories */
		return;

	if (comm->n_files != 0) {
		double fraction = (double) ++comm->n_file / (comm->n_files + 1);
		fr_command_progress (comm, fraction);
	}
	else {
		char *msg = g_strconcat (action_msg, file_name_from_path (line), NULL);
		fr_command_message (comm, msg);
		g_free (msg);
	}
}


static void
process_line__add (char     *line,
		   gpointer  data)
{
	/* Translators: after the colon there is a filename. */
	process_line__generic (line, data, _("Adding file: "));
}


static void
fr_command_tar_add (FrCommand     *comm,
		    const char    *from_file,
		    GList         *file_list,
		    const char    *base_dir,
		    gboolean       update,
		    gboolean       recursive)
{
	FrCommandTar *c_tar = FR_COMMAND_TAR (comm);
	GList        *scan;

	fr_process_set_out_line_func (FR_COMMAND (comm)->process,
				      process_line__add,
				      comm);

	begin_tar_command (comm);
	fr_process_add_arg (comm->process, "--force-local");
	if (! recursive)
		fr_process_add_arg (comm->process, "--no-recursion");
	fr_process_add_arg (comm->process, "--no-wildcards");
	fr_process_add_arg (comm->process, "--no-unquote");
	fr_process_add_arg (comm->process, "-v");
	fr_process_add_arg (comm->process, "-p");

	if (base_dir != NULL) {
		fr_process_add_arg (comm->process, "-C");
		fr_process_add_arg (comm->process, base_dir);
	}

	if (can_create_a_compressed_archive (comm)) {
		fr_process_add_arg (comm->process, "-cf");
		fr_process_add_arg (comm->process, comm->filename);
		add_compress_arg (comm);
	}
	else {
		if (comm->creating_archive)
			fr_process_add_arg (comm->process, "-cf");
		else
			fr_process_add_arg (comm->process, "-rf");
		fr_process_add_arg (comm->process, c_tar->uncomp_filename);
	}

	if (from_file != NULL) {
		fr_process_add_arg (comm->process, "-T");
		fr_process_add_arg (comm->process, from_file);
	}

	fr_process_add_arg (comm->process, "--");

	if (from_file == NULL)
		for (scan = file_list; scan; scan = scan->next)
			fr_process_add_arg (comm->process, scan->data);

	fr_process_end_command (comm->process);
}


static void
process_line__delete (char     *line,
		      gpointer  data)
{
	/* Translators: after the colon there is a filename. */
	process_line__generic (line, data, _("Removing file: "));
}


static void
begin_func__delete (gpointer data)
{
	FrCommand *comm = data;
	fr_command_progress (comm, -1.0);
	fr_command_message (comm, _("Deleting files from archive"));
}


static void
fr_command_tar_delete (FrCommand  *comm,
		       const char *from_file,
		       GList      *file_list)
{
	FrCommandTar *c_tar = FR_COMMAND_TAR (comm);
	GList        *scan;

	fr_process_set_out_line_func (comm->process,
				      process_line__delete,
				      comm);

	begin_tar_command (comm);
	fr_process_set_begin_func (comm->process, begin_func__delete, comm);
	fr_process_add_arg (comm->process, "--force-local");
	fr_process_add_arg (comm->process, "--no-wildcards");
	fr_process_add_arg (comm->process, "--no-unquote");
	fr_process_add_arg (comm->process, "-v");
	fr_process_add_arg (comm->process, "--delete");
	fr_process_add_arg (comm->process, "-f");
	fr_process_add_arg (comm->process, c_tar->uncomp_filename);

	if (from_file != NULL) {
		fr_process_add_arg (comm->process, "-T");
		fr_process_add_arg (comm->process, from_file);
	}

	fr_process_add_arg (comm->process, "--");

	if (from_file == NULL)
		for (scan = file_list; scan; scan = scan->next)
			fr_process_add_arg (comm->process, scan->data);

	fr_process_end_command (comm->process);
}


static void
process_line__extract (char     *line,
		       gpointer  data)
{
	/* Translators: after the colon there is a filename. */
	process_line__generic (line, data, _("Extracting file: "));
}


static void
fr_command_tar_extract (FrCommand  *comm,
		        const char *from_file,
			GList      *file_list,
			const char *dest_dir,
			gboolean    overwrite,
			gboolean    skip_older,
			gboolean    junk_paths)
{
	GList *scan;

	fr_process_set_out_line_func (comm->process,
				      process_line__extract,
				      comm);

	begin_tar_command (comm);
	fr_process_add_arg (comm->process, "--force-local");
	fr_process_add_arg (comm->process, "--no-wildcards");
	fr_process_add_arg (comm->process, "--no-unquote");
	fr_process_add_arg (comm->process, "-v");
	fr_process_add_arg (comm->process, "-p");

	if (! overwrite)
		fr_process_add_arg (comm->process, "-k");
	if (skip_older)
		fr_process_add_arg (comm->process, "--keep-newer-files");

	fr_process_add_arg (comm->process, "-xf");
	fr_process_add_arg (comm->process, comm->filename);
	add_compress_arg (comm);

	if (dest_dir != NULL) {
		fr_process_add_arg (comm->process, "-C");
		fr_process_add_arg (comm->process, dest_dir);
	}

	if (from_file != NULL) {
		fr_process_add_arg (comm->process, "-T");
		fr_process_add_arg (comm->process, from_file);
	}

	fr_process_add_arg (comm->process, "--");

	if (from_file == NULL)
		for (scan = file_list; scan; scan = scan->next)
			fr_process_add_arg (comm->process, scan->data);

	fr_process_end_command (comm->process);
}


static void
begin_func__recompress (gpointer data)
{
	FrCommand *comm = data;
	fr_command_progress (comm, -1.0);
	fr_command_message (comm, _("Recompressing archive"));
}


static gboolean
gzip_continue_func (gpointer user_data)
{
	FrCommand *comm = user_data;

	/* ignore gzip warnings */

	if (comm->process->error.status == 2) {
		comm->process->error.type = FR_PROC_ERROR_NONE;
		comm->process->error.status = 0;
		g_clear_error (&comm->process->error.gerror);
	}

	return comm->process->error.status == 0;
}


static void
fr_command_tar_recompress (FrCommand *comm)
{
	FrCommandTar *c_tar = FR_COMMAND_TAR (comm);
	char         *new_name = NULL;

	if (can_create_a_compressed_archive (comm))
		return;

	if (is_mime_type (comm->mime_type, "application/x-compressed-tar")) {
		fr_process_begin_command (comm->process, "gzip");
		fr_process_set_begin_func (comm->process, begin_func__recompress, comm);
		fr_process_set_continue_func (comm->process, gzip_continue_func, comm);
		switch (comm->compression) {
		case FR_COMPRESSION_VERY_FAST:
			fr_process_add_arg (comm->process, "-1"); break;
		case FR_COMPRESSION_FAST:
			fr_process_add_arg (comm->process, "-3"); break;
		case FR_COMPRESSION_NORMAL:
			fr_process_add_arg (comm->process, "-6"); break;
		case FR_COMPRESSION_MAXIMUM:
			fr_process_add_arg (comm->process, "-9"); break;
		}
		fr_process_add_arg (comm->process, "-f");
		fr_process_add_arg (comm->process, c_tar->uncomp_filename);
		fr_process_end_command (comm->process);

		new_name = g_strconcat (c_tar->uncomp_filename, ".gz", NULL);
	}
	else if (is_mime_type (comm->mime_type, "application/x-bzip-compressed-tar")) {
		fr_process_begin_command (comm->process, "bzip2");
		fr_process_set_begin_func (comm->process, begin_func__recompress, comm);
		switch (comm->compression) {
		case FR_COMPRESSION_VERY_FAST:
			fr_process_add_arg (comm->process, "-1"); break;
		case FR_COMPRESSION_FAST:
			fr_process_add_arg (comm->process, "-3"); break;
		case FR_COMPRESSION_NORMAL:
			fr_process_add_arg (comm->process, "-6"); break;
		case FR_COMPRESSION_MAXIMUM:
			fr_process_add_arg (comm->process, "-9"); break;
		}
		fr_process_add_arg (comm->process, "-f");
		fr_process_add_arg (comm->process, c_tar->uncomp_filename);
		fr_process_end_command (comm->process);

		new_name = g_strconcat (c_tar->uncomp_filename, ".bz2", NULL);
	}
	else if (is_mime_type (comm->mime_type, "application/x-tarz")) {
		fr_process_begin_command (comm->process, "compress");
		fr_process_set_begin_func (comm->process, begin_func__recompress, comm);
		fr_process_add_arg (comm->process, "-f");
		fr_process_add_arg (comm->process, c_tar->uncomp_filename);
		fr_process_end_command (comm->process);

		new_name = g_strconcat (c_tar->uncomp_filename, ".Z", NULL);
	}
	else if (is_mime_type (comm->mime_type, "application/x-lrzip-compressed-tar")) {
		fr_process_begin_command (comm->process, "lrzip");
		fr_process_set_begin_func (comm->process, begin_func__recompress, comm);
		switch (comm->compression) {
		case FR_COMPRESSION_VERY_FAST:
			fr_process_add_arg (comm->process, "-l"); break;
		case FR_COMPRESSION_FAST:
			fr_process_add_arg (comm->process, "-g"); break;
		case FR_COMPRESSION_NORMAL:
			fr_process_add_arg (comm->process, "-b"); break;
		case FR_COMPRESSION_MAXIMUM:
			fr_process_add_arg (comm->process, "-z"); break;
		}
		fr_process_add_arg (comm->process, "-o");
		fr_process_add_arg (comm->process, c_tar->uncomp_filename);
		fr_process_end_command (comm->process);

		new_name = g_strconcat (c_tar->uncomp_filename, ".lrz", NULL);
	}
	else if (is_mime_type (comm->mime_type, "application/x-lzip-compressed-tar")) {
		fr_process_begin_command (comm->process, "lzip");
		fr_process_set_begin_func (comm->process, begin_func__recompress, comm);
		switch (comm->compression) {
		case FR_COMPRESSION_VERY_FAST:
			fr_process_add_arg (comm->process, "-1"); break;
		case FR_COMPRESSION_FAST:
			fr_process_add_arg (comm->process, "-3"); break;
		case FR_COMPRESSION_NORMAL:
			fr_process_add_arg (comm->process, "-6"); break;
		case FR_COMPRESSION_MAXIMUM:
			fr_process_add_arg (comm->process, "-9"); break;
		}
		fr_process_add_arg (comm->process, "-f");
		fr_process_add_arg (comm->process, c_tar->uncomp_filename);
		fr_process_end_command (comm->process);

		new_name = g_strconcat (c_tar->uncomp_filename, ".lz", NULL);
	}
	else if (is_mime_type (comm->mime_type, "application/x-lzma-compressed-tar")) {
		fr_process_begin_command (comm->process, "lzma");
		fr_process_set_begin_func (comm->process, begin_func__recompress, comm);
		switch (comm->compression) {
		case FR_COMPRESSION_VERY_FAST:
			fr_process_add_arg (comm->process, "-1"); break;
		case FR_COMPRESSION_FAST:
			fr_process_add_arg (comm->process, "-3"); break;
		case FR_COMPRESSION_NORMAL:
			fr_process_add_arg (comm->process, "-6"); break;
		case FR_COMPRESSION_MAXIMUM:
			fr_process_add_arg (comm->process, "-9"); break;
		}
		fr_process_add_arg (comm->process, "-f");
		fr_process_add_arg (comm->process, c_tar->uncomp_filename);
		fr_process_end_command (comm->process);

		new_name = g_strconcat (c_tar->uncomp_filename, ".lzma", NULL);
	}
	else if (is_mime_type (comm->mime_type, "application/x-xz-compressed-tar")) {
		fr_process_begin_command (comm->process, "xz");
		fr_process_set_begin_func (comm->process, begin_func__recompress, comm);
		switch (comm->compression) {
		case FR_COMPRESSION_VERY_FAST:
			fr_process_add_arg (comm->process, "-1"); break;
		case FR_COMPRESSION_FAST:
			fr_process_add_arg (comm->process, "-3"); break;
		case FR_COMPRESSION_NORMAL:
			fr_process_add_arg (comm->process, "-6"); break;
		case FR_COMPRESSION_MAXIMUM:
			fr_process_add_arg (comm->process, "-9"); break;
		}
		fr_process_add_arg (comm->process, "-f");
		fr_process_add_arg (comm->process, c_tar->uncomp_filename);
		fr_process_end_command (comm->process);

		new_name = g_strconcat (c_tar->uncomp_filename, ".xz", NULL);
	}
	else if (is_mime_type (comm->mime_type, "application/x-zstd-compressed-tar")) {
		fr_process_begin_command (comm->process, "zstd");
		fr_process_set_begin_func (comm->process, begin_func__recompress, comm);
		switch (comm->compression) {
		case FR_COMPRESSION_VERY_FAST:
			fr_process_add_arg (comm->process, "-1"); break;
		case FR_COMPRESSION_FAST:
			fr_process_add_arg (comm->process, "-2"); break;
		case FR_COMPRESSION_NORMAL:
			fr_process_add_arg (comm->process, "-3"); break;
		case FR_COMPRESSION_MAXIMUM:
			fr_process_add_arg (comm->process, "--ultra");
			fr_process_add_arg (comm->process, "-22");
			break;
		}
		fr_process_add_arg (comm->process, "-f");
		fr_process_add_arg (comm->process, c_tar->uncomp_filename);
		fr_process_end_command (comm->process);

		new_name = g_strconcat (c_tar->uncomp_filename, ".zst", NULL);
	}
	else if (is_mime_type (comm->mime_type, "application/x-lzop-compressed-tar")) {
		fr_process_begin_command (comm->process, "lzop");
		fr_process_set_begin_func (comm->process, begin_func__recompress, comm);
		switch (comm->compression) {
		case FR_COMPRESSION_VERY_FAST:
			fr_process_add_arg (comm->process, "-1"); break;
		case FR_COMPRESSION_FAST:
			fr_process_add_arg (comm->process, "-3"); break;
		case FR_COMPRESSION_NORMAL:
			fr_process_add_arg (comm->process, "-6"); break;
		case FR_COMPRESSION_MAXIMUM:
			fr_process_add_arg (comm->process, "-9"); break;
		}
		fr_process_add_arg (comm->process, "-fU");
		fr_process_add_arg (comm->process, "--no-stdin");
		fr_process_add_arg (comm->process, c_tar->uncomp_filename);
		fr_process_end_command (comm->process);

		new_name = g_strconcat (c_tar->uncomp_filename, ".lzo", NULL);
	}
	else if (is_mime_type (comm->mime_type, "application/x-7z-compressed-tar")) {
		FrCommandTar *comm_tar = (FrCommandTar*) comm;

		fr_process_begin_command (comm->process, comm_tar->compress_command);
		fr_process_set_begin_func (comm->process, begin_func__recompress, comm);
		switch (comm->compression) {
		case FR_COMPRESSION_VERY_FAST:
			fr_process_add_arg (comm->process, "-mx=1"); break;
		case FR_COMPRESSION_FAST:
			fr_process_add_arg (comm->process, "-mx=5"); break;
		case FR_COMPRESSION_NORMAL:
			fr_process_add_arg (comm->process, "-mx=5"); break;
		case FR_COMPRESSION_MAXIMUM:
			fr_process_add_arg (comm->process, "-mx=7"); break;
		}
		fr_process_add_arg (comm->process, "a");
		fr_process_add_arg (comm->process, "-bd");
		fr_process_add_arg (comm->process, "-y");
		fr_process_add_arg (comm->process, "-l");

		new_name = g_strconcat (c_tar->uncomp_filename, ".7z", NULL);
		fr_process_add_arg_concat (comm->process, new_name, NULL);

		fr_process_add_arg (comm->process, c_tar->uncomp_filename);
		fr_process_end_command (comm->process);

		/* remove the uncompressed tar */

		fr_process_begin_command (comm->process, "rm");
		fr_process_add_arg (comm->process, "-f");
		fr_process_add_arg (comm->process, c_tar->uncomp_filename);
		fr_process_end_command (comm->process);
	}

	if (c_tar->name_modified) {
		char *tmp_dir;

		/* Restore original name. */

		fr_process_begin_command (comm->process, "mv");
		fr_process_add_arg (comm->process, "-f");
		fr_process_add_arg (comm->process, new_name);
		fr_process_add_arg (comm->process, comm->filename);
		fr_process_end_command (comm->process);

		tmp_dir = remove_level_from_path (new_name);

		fr_process_begin_command (comm->process, "rm");
		fr_process_set_sticky (comm->process, TRUE);
		fr_process_add_arg (comm->process, "-fr");
		fr_process_add_arg (comm->process, tmp_dir);
		fr_process_end_command (comm->process);

		g_free (tmp_dir);
	}

	g_free (new_name);
	g_free (c_tar->uncomp_filename);
	c_tar->uncomp_filename = NULL;
}


static void
begin_func__uncompress (gpointer data)
{
	FrCommand *comm = data;
	fr_command_progress (comm, -1.0);
	fr_command_message (comm, _("Decompressing archive"));
}


static char *
get_uncompressed_name (FrCommandTar *c_tar,
		       const char   *e_filename)
{
	FrCommand *comm = FR_COMMAND (c_tar);
	char      *new_name = g_strdup (e_filename);
	int        l = strlen (new_name);

	if (is_mime_type (comm->mime_type, "application/x-compressed-tar")) {
		/* X.tgz     -->  X.tar
		 * X.tar.gz  -->  X.tar */
		if (file_extension_is (e_filename, ".tgz")) {
			new_name[l - 2] = 'a';
			new_name[l - 1] = 'r';
		}
		else if (file_extension_is (e_filename, ".tar.gz"))
			new_name[l - 3] = 0;
	}
	else if (is_mime_type (comm->mime_type, "application/x-bzip-compressed-tar")) {
		/* X.tbz2    -->  X.tar
		 * X.tar.bz2 -->  X.tar */
		if (file_extension_is (e_filename, ".tbz2")) {
			new_name[l - 3] = 'a';
			new_name[l - 2] = 'r';
			new_name[l - 1] = 0;
		}
		else if (file_extension_is (e_filename, ".tar.bz2"))
			new_name[l - 4] = 0;
	}
	else if (is_mime_type (comm->mime_type, "application/x-tarz")) {
		/* X.taz   -->  X.tar
		 * X.tar.Z -->  X.tar */
		if (file_extension_is (e_filename, ".taz"))
			new_name[l - 1] = 'r';
		else if (file_extension_is (e_filename, ".tar.Z"))
			new_name[l - 2] = 0;
	}
	else if (is_mime_type (comm->mime_type, "application/x-lrzip-compressed-tar")) {
		/* X.tlrz     -->  X.tar
		 * X.tar.lrz  -->  X.tar */
		if (file_extension_is (e_filename, ".tlrz")) {
			new_name[l - 3] = 'a';
			new_name[l - 2] = 'r';
			new_name[l - 1] = 0;
		}
		else if (file_extension_is (e_filename, ".tar.lrz"))
			new_name[l - 4] = 0;
	}
	else if (is_mime_type (comm->mime_type, "application/x-lzip-compressed-tar")) {
		/* X.tlz     -->  X.tar
		 * X.tar.lz  -->  X.tar */
		if (file_extension_is (e_filename, ".tlz")) {
			new_name[l - 2] = 'a';
			new_name[l - 1] = 'r';
		}
		else if (file_extension_is (e_filename, ".tar.lz"))
			new_name[l - 3] = 0;
	}
	else if (is_mime_type (comm->mime_type, "application/x-lzma-compressed-tar")) {
		/* X.tar.lzma --> X.tar
		 * (There doesn't seem to be a shorthand suffix) */
		if (file_extension_is (e_filename, ".tar.lzma"))
			new_name[l - 5] = 0;
	}
	else if (is_mime_type (comm->mime_type, "application/x-xz-compressed-tar")) {
		/* X.txz     -->  X.tar
		 * X.tar.xz  -->  X.tar */
		if (file_extension_is (e_filename, ".txz")) {
			new_name[l - 2] = 'a';
			new_name[l - 1] = 'r';
		}
		else if (file_extension_is (e_filename, ".tar.xz"))
			new_name[l - 3] = 0;
	}
	else if (is_mime_type (comm->mime_type, "application/x-zstd-compressed-tar")) {
		/* X.tzst    -->  X.tar
		 * X.tar.zst -->  X.tar */
		if (file_extension_is (e_filename, ".tzst")) {
			new_name[l - 3] = 'a';
			new_name[l - 2] = 'r';
			new_name[l - 1] = 0;
		}
		else if (file_extension_is (e_filename, ".tar.zst"))
			new_name[l - 4] = 0;
	}
	else if (is_mime_type (comm->mime_type, "application/x-lzop-compressed-tar")) {
		/* X.tzo     -->  X.tar
		 * X.tar.lzo -->  X.tar */
		if (file_extension_is (e_filename, ".tzo")) {
			new_name[l - 2] = 'a';
			new_name[l - 1] = 'r';
		}
		else if (file_extension_is (e_filename, ".tar.lzo"))
			new_name[l - 4] = 0;
	}
	else if (is_mime_type (comm->mime_type, "application/x-7z-compressed-tar")) {
		/* X.tar.7z -->  X.tar */
		if (file_extension_is (e_filename, ".tar.7z"))
			new_name[l - 3] = 0;
	}

	return new_name;
}


#define MAX_TRIES 50


static char *
get_temp_name (FrCommandTar *c_tar,
	       const char   *filepath)
{
	char *dirname = remove_level_from_path (filepath);
	char *template;
	char *result = NULL;
	char *temp_name = NULL;

	template = g_strconcat (dirname, "/.fr-XXXXXX", NULL);
	result = mkdtemp (template);
	temp_name = g_build_filename (result, file_name_from_path (filepath), NULL);
	g_free (template);

	return temp_name;
}


static void
fr_command_tar_uncompress (FrCommand *comm)
{
	FrCommandTar *c_tar = FR_COMMAND_TAR (comm);
	char         *tmp_name;
	gboolean      archive_exists;

	if (can_create_a_compressed_archive (comm))
		return;

	if (c_tar->uncomp_filename != NULL) {
		g_free (c_tar->uncomp_filename);
		c_tar->uncomp_filename = NULL;
	}

	archive_exists = ! comm->creating_archive;

	c_tar->name_modified = ! is_mime_type (comm->mime_type, "application/x-tar");
	if (c_tar->name_modified) {
		tmp_name = get_temp_name (c_tar, comm->filename);
		if (archive_exists) {
			fr_process_begin_command (comm->process, "mv");
			fr_process_add_arg (comm->process, "-f");
			fr_process_add_arg (comm->process, comm->filename);
			fr_process_add_arg (comm->process, tmp_name);
			fr_process_end_command (comm->process);
		}
	}
	else
		tmp_name = g_strdup (comm->filename);

	if (archive_exists) {
		if (is_mime_type (comm->mime_type, "application/x-compressed-tar")) {
			fr_process_begin_command (comm->process, "gzip");
			fr_process_set_begin_func (comm->process, begin_func__uncompress, comm);
			fr_process_set_continue_func (comm->process, gzip_continue_func, comm);
			fr_process_add_arg (comm->process, "-f");
			fr_process_add_arg (comm->process, "-d");
			fr_process_add_arg (comm->process, tmp_name);
			fr_process_end_command (comm->process);
		}
		else if (is_mime_type (comm->mime_type, "application/x-bzip-compressed-tar")) {
			fr_process_begin_command (comm->process, "bzip2");
			fr_process_set_begin_func (comm->process, begin_func__uncompress, comm);
			fr_process_add_arg (comm->process, "-f");
			fr_process_add_arg (comm->process, "-d");
			fr_process_add_arg (comm->process, tmp_name);
			fr_process_end_command (comm->process);
		}
		else if (is_mime_type (comm->mime_type, "application/x-tarz")) {
			if (is_program_in_path ("gzip")) {
				fr_process_begin_command (comm->process, "gzip");
				fr_process_set_continue_func (comm->process, gzip_continue_func, comm);
			}
			else
				fr_process_begin_command (comm->process, "uncompress");
			fr_process_set_begin_func (comm->process, begin_func__uncompress, comm);
			fr_process_add_arg (comm->process, "-f");
			fr_process_add_arg (comm->process, tmp_name);
			fr_process_end_command (comm->process);
		}
		else if (is_mime_type (comm->mime_type, "application/x-lrzip-compressed-tar")) {
			fr_process_begin_command (comm->process, "lrzip");
			fr_process_set_begin_func (comm->process, begin_func__uncompress, comm);
			fr_process_add_arg (comm->process, "-f");
			fr_process_add_arg (comm->process, "-d");
			fr_process_add_arg (comm->process, tmp_name);
			fr_process_end_command (comm->process);
		}
		else if (is_mime_type (comm->mime_type, "application/x-lzip-compressed-tar")) {
			fr_process_begin_command (comm->process, "lzip");
			fr_process_set_begin_func (comm->process, begin_func__uncompress, comm);
			fr_process_add_arg (comm->process, "-f");
			fr_process_add_arg (comm->process, "-d");
			fr_process_add_arg (comm->process, tmp_name);
			fr_process_end_command (comm->process);
		}
		else if (is_mime_type (comm->mime_type, "application/x-lzma-compressed-tar")) {
			fr_process_begin_command (comm->process, "lzma");
			fr_process_set_begin_func (comm->process, begin_func__uncompress, comm);
			fr_process_add_arg (comm->process, "-f");
			fr_process_add_arg (comm->process, "-d");
			fr_process_add_arg (comm->process, tmp_name);
			fr_process_end_command (comm->process);
		}
		else if (is_mime_type (comm->mime_type, "application/x-xz-compressed-tar")) {
			fr_process_begin_command (comm->process, "xz");
			fr_process_set_begin_func (comm->process, begin_func__uncompress, comm);
			fr_process_add_arg (comm->process, "-f");
			fr_process_add_arg (comm->process, "-d");
			fr_process_add_arg (comm->process, tmp_name);
			fr_process_end_command (comm->process);
		}
		else if (is_mime_type (comm->mime_type, "application/x-zstd-compressed-tar")) {
			fr_process_begin_command (comm->process, "zstd");
			fr_process_set_begin_func (comm->process, begin_func__uncompress, comm);
			fr_process_add_arg (comm->process, "-f");
			fr_process_add_arg (comm->process, "-d");
			fr_process_add_arg (comm->process, tmp_name);
			fr_process_end_command (comm->process);
		}
		else if (is_mime_type (comm->mime_type, "application/x-lzop-compressed-tar")) {
			fr_process_begin_command (comm->process, "lzop");
			fr_process_set_begin_func (comm->process, begin_func__uncompress, comm);
			fr_process_add_arg (comm->process, "-dfU");
			fr_process_add_arg (comm->process, "--no-stdin");
			fr_process_add_arg (comm->process, tmp_name);
			fr_process_end_command (comm->process);
		}
		else if (is_mime_type (comm->mime_type, "application/x-7z-compressed-tar")) {
			FrCommandTar *comm_tar = (FrCommandTar*) comm;

			fr_process_begin_command (comm->process, comm_tar->compress_command);
			fr_process_set_begin_func (comm->process, begin_func__uncompress, comm);
			fr_process_add_arg (comm->process, "e");
			fr_process_add_arg (comm->process, "-bd");
			fr_process_add_arg (comm->process, "-y");
			fr_process_add_arg (comm->process, tmp_name);
			fr_process_end_command (comm->process);

			/* remove the compressed tar */

			fr_process_begin_command (comm->process, "rm");
			fr_process_add_arg (comm->process, "-f");
			fr_process_add_arg (comm->process, tmp_name);
			fr_process_end_command (comm->process);
		}
	}

	c_tar->uncomp_filename = get_uncompressed_name (c_tar, tmp_name);
	g_free (tmp_name);
}


static void
fr_command_tar_handle_error (FrCommand   *comm,
			     FrProcError *error)
{
	if (error->type != FR_PROC_ERROR_NONE) {
		if (error->status <= 1)
			error->type = FR_PROC_ERROR_NONE;
	}
}


const char *tar_mime_types[] = { "application/x-compressed-tar",
				 "application/x-bzip-compressed-tar",
				 "application/x-tar",
				 "application/x-7z-compressed-tar",
				 "application/x-lrzip-compressed-tar",
				 "application/x-lzip-compressed-tar",
			         "application/x-lzma-compressed-tar",
			         "application/x-lzop-compressed-tar",
			         "application/x-tarz",
				 "application/x-xz-compressed-tar",
				 "application/x-zstd-compressed-tar",
			         NULL };


static const char **
fr_command_tar_get_mime_types (FrCommand *comm)
{
	return tar_mime_types;
}


static FrCommandCap
fr_command_tar_get_capabilities (FrCommand  *comm,
			         const char *mime_type,
				 gboolean    check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES;

	/* In solaris gtar is present under /usr/sfw/bin */
	if (! is_program_available ("tar", check_command) && ! is_program_available ("/usr/sfw/bin/gtar", check_command))
		return capabilities;

	if (is_mime_type (mime_type, "application/x-tar")) {
		capabilities |= FR_COMMAND_CAN_READ_WRITE;
	}
	else if (is_mime_type (mime_type, "application/x-compressed-tar")) {
		if (is_program_available ("gzip", check_command))
			capabilities |= FR_COMMAND_CAN_READ_WRITE;
	}
	else if (is_mime_type (mime_type, "application/x-bzip-compressed-tar")) {
		if (is_program_available ("bzip2", check_command))
			capabilities |= FR_COMMAND_CAN_READ_WRITE;
	}
	else if (is_mime_type (mime_type, "application/x-tarz")) {
		if (is_program_available ("compress", check_command) && is_program_available ("uncompress", check_command))
			capabilities |= FR_COMMAND_CAN_READ_WRITE;
		else if (is_program_available ("gzip", check_command))
			capabilities |= FR_COMMAND_CAN_READ;
	}
	else if (is_mime_type (mime_type, "application/x-lrzip-compressed-tar")) {
		if (is_program_available ("lrzip", check_command))
			capabilities |= FR_COMMAND_CAN_READ_WRITE;
	}
	else if (is_mime_type (mime_type, "application/x-lzip-compressed-tar")) {
		if (is_program_available ("lzip", check_command))
			capabilities |= FR_COMMAND_CAN_READ_WRITE;
	}
	else if (is_mime_type (mime_type, "application/x-lzma-compressed-tar")) {
		if (is_program_available ("lzma", check_command))
			capabilities |= FR_COMMAND_CAN_READ_WRITE;
	}
	else if (is_mime_type (mime_type, "application/x-xz-compressed-tar")) {
		if (is_program_available ("xz", check_command))
			capabilities |= FR_COMMAND_CAN_READ_WRITE;
	}
	else if (is_mime_type (mime_type, "application/x-zstd-compressed-tar")) {
		if (is_program_available ("zstd", check_command))
			capabilities |= FR_COMMAND_CAN_READ_WRITE;
	}
	else if (is_mime_type (mime_type, "application/x-lzop-compressed-tar")) {
		if (is_program_available ("lzop", check_command))
			capabilities |= FR_COMMAND_CAN_READ_WRITE;
	}
	else if (is_mime_type (mime_type, "application/x-7z-compressed-tar")) {
		char  *try_command[3] = { "7za", "7zr", "7z" };
		size_t i;

		for (i = 0; i < G_N_ELEMENTS (try_command); i++) {
			if (is_program_available (try_command[i], check_command)) {
				capabilities |= FR_COMMAND_CAN_WRITE;
				break;
			}
		}
	}

	return capabilities;
}


static void
fr_command_tar_set_mime_type (FrCommand  *comm,
		 	      const char *mime_type)
{
	FrCommandTar *comm_tar = FR_COMMAND_TAR (comm);

	FR_COMMAND_CLASS (parent_class)->set_mime_type (comm, mime_type);

	if (is_mime_type (mime_type, "application/x-7z-compressed-tar")) {
		char  *try_command[3] = { "7za", "7zr", "7z" };
		size_t i;

		for (i = 0; i < G_N_ELEMENTS (try_command); i++) {
			if (is_program_in_path (try_command[i])) {
				comm_tar->compress_command = g_strdup (try_command[i]);
				break;
			}
		}
	}
}


static const char *
fr_command_tar_get_packages (FrCommand  *comm,
			     const char *mime_type)
{
	if (is_mime_type (mime_type, "application/x-tar"))
		return PACKAGES ("tar");
	else if (is_mime_type (mime_type, "application/x-compressed-tar"))
		return PACKAGES ("tar,gzip");
	else if (is_mime_type (mime_type, "application/x-bzip-compressed-tar"))
		return PACKAGES ("tar,bzip2");
	else if (is_mime_type (mime_type, "application/x-tarz"))
		return PACKAGES ("tar,gzip,ncompress");
	else if (is_mime_type (mime_type, "application/x-lrzip-compressed-tar"))
		return PACKAGES ("tar,lrzip");
	else if (is_mime_type (mime_type, "application/x-lzip-compressed-tar"))
		return PACKAGES ("tar,lzip");
	else if (is_mime_type (mime_type, "application/x-lzma-compressed-tar"))
		return PACKAGES ("tar,lzma");
	else if (is_mime_type (mime_type, "application/x-xz-compressed-tar"))
		return PACKAGES ("tar,xz");
	else if (is_mime_type (mime_type, "application/x-zstd-compressed-tar"))
		return PACKAGES ("tar,zstd");
	else if (is_mime_type (mime_type, "application/x-lzop-compressed-tar"))
		return PACKAGES ("tar,lzop");
	else if (is_mime_type (mime_type, "application/x-7z-compressed-tar"))
		return PACKAGES ("tar,p7zip");

	return NULL;
}


static void
fr_command_tar_class_init (FrCommandTarClass *class)
{
        GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
        FrCommandClass *afc;

        parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_tar_finalize;

        afc->list             = fr_command_tar_list;
	afc->add              = fr_command_tar_add;
	afc->delete_           = fr_command_tar_delete;
	afc->extract          = fr_command_tar_extract;
	afc->handle_error     = fr_command_tar_handle_error;
	afc->get_mime_types   = fr_command_tar_get_mime_types;
	afc->get_capabilities = fr_command_tar_get_capabilities;
	afc->set_mime_type    = fr_command_tar_set_mime_type;
	afc->recompress       = fr_command_tar_recompress;
	afc->uncompress       = fr_command_tar_uncompress;
	afc->get_packages     = fr_command_tar_get_packages;
}


static void
fr_command_tar_init (FrCommand *comm)
{
	FrCommandTar *comm_tar = (FrCommandTar*) comm;

	comm->propAddCanUpdate              = FALSE;
	comm->propAddCanReplace             = FALSE;
	comm->propAddCanStoreFolders        = TRUE;
	comm->propExtractCanAvoidOverwrite  = FALSE;
	comm->propExtractCanSkipOlder       = TRUE;
	comm->propExtractCanJunkPaths       = FALSE;
	comm->propPassword                  = FALSE;
	comm->propTest                      = FALSE;
	comm->propCanDeleteNonEmptyFolders  = FALSE;
	comm->propCanExtractNonEmptyFolders = FALSE;
	comm->propListFromFile              = TRUE;

	comm_tar->msg = NULL;
	comm_tar->uncomp_filename = NULL;
}


static void
fr_command_tar_finalize (GObject *object)
{
	FrCommandTar *comm_tar;

        g_return_if_fail (object != NULL);
        g_return_if_fail (FR_IS_COMMAND_TAR (object));

	comm_tar = FR_COMMAND_TAR (object);

	if (comm_tar->uncomp_filename != NULL) {
		g_free (comm_tar->uncomp_filename);
		comm_tar->uncomp_filename = NULL;
	}

	if (comm_tar->msg != NULL) {
		g_free (comm_tar->msg);
		comm_tar->msg = NULL;
	}

	if (comm_tar->compress_command != NULL) {
		g_free (comm_tar->compress_command);
		comm_tar->compress_command = NULL;
	}

	/* Chain up */
        if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_tar_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (FrCommandTarClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_tar_class_init,
			NULL,
			NULL,
			sizeof (FrCommandTar),
			0,
			(GInstanceInitFunc) fr_command_tar_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandTar",
					       &type_info,
					       0);
        }

        return type;
}
