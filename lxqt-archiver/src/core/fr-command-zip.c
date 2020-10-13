/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>

#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-zip.h"

#define EMPTY_ARCHIVE_WARNING        "Empty zipfile."
#define ZIP_SPECIAL_CHARACTERS       "[]*?!^-\\"

static void fr_command_zip_class_init  (FrCommandZipClass *class);
static void fr_command_zip_init        (FrCommand         *afile);
static void fr_command_zip_finalize    (GObject           *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;


/* -- list -- */

static time_t
mktime_from_string (char *datetime_s)
{
	struct tm  tm = {0, };
	char      *date;
	char      *time;
	char      *year;
	char      *month;
	char      *day;
	char      *hour;
	char      *min;
	char      *sec;

	tm.tm_isdst = -1;

	/* date */

	date = datetime_s;
	year = g_strndup (date, 4);
	month = g_strndup (date + 4, 2);
	day = g_strndup (date + 6, 2);
	tm.tm_year = atoi (year) - 1900;
	tm.tm_mon = atoi (month) - 1;
	tm.tm_mday = atoi (day);
	g_free (year);
	g_free (month);
	g_free (day);

	/* time */

	time = datetime_s + 9;
	hour = g_strndup (time, 2);
	min = g_strndup (time + 2, 2);
	sec = g_strndup (time + 4, 2);
	tm.tm_hour = atoi (hour);
	tm.tm_min = atoi (min);
	tm.tm_sec = atoi (sec);
	g_free(hour);
	g_free(min);
	g_free(sec);

	return mktime (&tm);
}


static void
list__process_line (char     *line,
		    gpointer  data)
{
	FileData    *fdata;
	FrCommand   *comm = FR_COMMAND (data);
	char       **fields;
	const char  *name_field;
	gint         line_l;

	g_return_if_fail (line != NULL);

	/* check whether unzip gave the empty archive warning. */

	if (FR_COMMAND_ZIP (comm)->is_empty)
		return;

	line_l = strlen (line);

	if (line_l == 0)
		return;

	if (strcmp (line, EMPTY_ARCHIVE_WARNING) == 0) {
		FR_COMMAND_ZIP (comm)->is_empty = TRUE;
		return;
	}

	/* ignore lines that do not describe a file or a
	 * directory. */
	if ((line[0] != '?') && (line[0] != 'd') && (line[0] != '-'))
		return;

	/**/

	fdata = file_data_new ();

	fields = split_line (line, 7);
	fdata->size = g_ascii_strtoull (fields[3], NULL, 10);
	fdata->modified = mktime_from_string (fields[6]);
	fdata->encrypted = (*fields[4] == 'B') || (*fields[4] == 'T');
	g_strfreev (fields);

	/* Full path */

	name_field = get_last_field (line, 8);

	if (*name_field == '/') {
		fdata->full_path = g_strdup (name_field);
		fdata->original_path = fdata->full_path;
	} else {
		fdata->full_path = g_strconcat ("/", name_field, NULL);
		fdata->original_path = fdata->full_path + 1;
	}

	fdata->link = NULL;

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
add_password_arg (FrCommand  *comm,
		  const char *password)
{
	if ((password != NULL) && (password[0] != '\0')) {
		fr_process_add_arg (comm->process, "-P");
		fr_process_add_arg (comm->process, password);
	}
}


static void
list__begin (gpointer data)
{
	FrCommandZip *comm = data;

	comm->is_empty = FALSE;
}


static void
fr_command_zip_list (FrCommand  *comm)
{
	fr_process_set_out_line_func (comm->process, list__process_line, comm);

	fr_process_begin_command (comm->process, "unzip");
	fr_process_set_begin_func (comm->process, list__begin, comm);
	fr_process_add_arg (comm->process, "-ZTs");
	fr_process_add_arg (comm->process, "--");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
	fr_process_start (comm->process);
}


static void
process_line__common (char     *line,
		      gpointer  data)
{
	FrCommand  *comm = FR_COMMAND (data);

	if (line == NULL)
		return;

	if (comm->n_files != 0) {
		double fraction = (double) ++comm->n_file / (comm->n_files + 1);
		fr_command_progress (comm, fraction);
	}
	else
		fr_command_message (comm, line);
}


static void
fr_command_zip_add (FrCommand     *comm,
		    const char    *from_file,
		    GList         *file_list,
		    const char    *base_dir,
		    gboolean       update,
		    gboolean       recursive)
{
	GList *scan;

	fr_process_set_out_line_func (FR_COMMAND (comm)->process,
				      process_line__common,
				      comm);

	fr_process_begin_command (comm->process, "zip");

	if (base_dir != NULL)
		fr_process_set_working_dir (comm->process, base_dir);

	/* preserve links. */
	fr_process_add_arg (comm->process, "-y");

	if (update)
		fr_process_add_arg (comm->process, "-u");

	add_password_arg (comm, comm->password);

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

	fr_process_add_arg (comm->process, comm->filename);
	fr_process_add_arg (comm->process, "--");

	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, scan->data);

	fr_process_end_command (comm->process);
}


static void
fr_command_zip_delete (FrCommand  *comm,
		       const char *from_file,
		       GList      *file_list)
{
	GList *scan;

	fr_process_set_out_line_func (FR_COMMAND (comm)->process,
				      process_line__common,
				      comm);

	fr_process_begin_command (comm->process, "zip");
	fr_process_add_arg (comm->process, "-d");

	fr_process_add_arg (comm->process, comm->filename);
	fr_process_add_arg (comm->process, "--");

	for (scan = file_list; scan; scan = scan->next) {
		char *escaped;

 		escaped = escape_str (scan->data, ZIP_SPECIAL_CHARACTERS);
 		fr_process_add_arg (comm->process, escaped);
 		g_free (escaped);
	}

	fr_process_end_command (comm->process);
}


static void
fr_command_zip_extract (FrCommand  *comm,
			const char *from_file,
			GList      *file_list,
			const char *dest_dir,
			gboolean    overwrite,
			gboolean    skip_older,
			gboolean    junk_paths)
{
	GList *scan;

	fr_process_set_out_line_func (FR_COMMAND (comm)->process,
				      process_line__common,
				      comm);

	fr_process_begin_command (comm->process, "unzip");

	if (dest_dir != NULL) {
		fr_process_add_arg (comm->process, "-d");
		fr_process_add_arg (comm->process, dest_dir);
	}
	if (overwrite)
		fr_process_add_arg (comm->process, "-o");
	else
		fr_process_add_arg (comm->process, "-n");
	if (skip_older)
		fr_process_add_arg (comm->process, "-u");
	if (junk_paths)
		fr_process_add_arg (comm->process, "-j");
	add_password_arg (comm, comm->password);

	fr_process_add_arg (comm->process, "--");
	fr_process_add_arg (comm->process, comm->filename);

	for (scan = file_list; scan; scan = scan->next) {
		char *escaped;

 		escaped = escape_str (scan->data, ZIP_SPECIAL_CHARACTERS);
 		fr_process_add_arg (comm->process, escaped);
 		g_free (escaped);
	}

	fr_process_end_command (comm->process);
}


static void
fr_command_zip_test (FrCommand   *comm)
{
	fr_process_begin_command (comm->process, "unzip");
	fr_process_add_arg (comm->process, "-t");
	add_password_arg (comm, comm->password);
	fr_process_add_arg (comm->process, "--");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
}


static void
fr_command_zip_handle_error (FrCommand   *comm,
			     FrProcError *error)
{
	if (error->type != FR_PROC_ERROR_NONE) {
		if (error->status <= 1)
			error->type = FR_PROC_ERROR_NONE;
		else if ((error->status == 82) || (error->status == 5))
			error->type = FR_PROC_ERROR_ASK_PASSWORD;
		else {
			GList *output;
			GList *scan;

			if (comm->action == FR_ACTION_TESTING_ARCHIVE)
				output = comm->process->out.raw;
			else
				output = comm->process->err.raw;

			for (scan = g_list_last (output); scan; scan = scan->prev) {
				char *line = scan->data;

				if (strstr (line, "incorrect password") != NULL) {
					error->type = FR_PROC_ERROR_ASK_PASSWORD;
					break;
				}
			}
		}
	}
}


const char *zip_mime_type[] = { "application/x-cbz",
				"application/x-ear",
				"application/x-ms-dos-executable",
				"application/x-war",
				"application/zip",
				NULL };


static const char **
fr_command_zip_get_mime_types (FrCommand *comm)
{
	return zip_mime_type;
}


static FrCommandCap
fr_command_zip_get_capabilities (FrCommand  *comm,
			         const char *mime_type,
				 gboolean    check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES | FR_COMMAND_CAN_ENCRYPT;
	if (is_program_available ("zip", check_command)) {
		if (strcmp (mime_type, "application/x-ms-dos-executable") == 0)
			capabilities |= FR_COMMAND_CAN_READ;
		else
			capabilities |= FR_COMMAND_CAN_WRITE;
	}
	if (is_program_available ("unzip", check_command))
		capabilities |= FR_COMMAND_CAN_READ;

	return capabilities;
}


static const char *
fr_command_zip_get_packages (FrCommand  *comm,
			     const char *mime_type)
{
	return PACKAGES ("zip,unzip");
}


static void
fr_command_zip_class_init (FrCommandZipClass *class)
{
	GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
	FrCommandClass *afc;

	parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_zip_finalize;

	afc->list             = fr_command_zip_list;
	afc->add              = fr_command_zip_add;
	afc->delete_           = fr_command_zip_delete;
	afc->extract          = fr_command_zip_extract;
	afc->test             = fr_command_zip_test;
	afc->handle_error     = fr_command_zip_handle_error;
	afc->get_mime_types   = fr_command_zip_get_mime_types;
	afc->get_capabilities = fr_command_zip_get_capabilities;
	afc->get_packages     = fr_command_zip_get_packages;
}


static void
fr_command_zip_init (FrCommand *comm)
{
	comm->propAddCanUpdate             = TRUE;
	comm->propAddCanReplace            = TRUE;
	comm->propAddCanStoreFolders       = TRUE;
	comm->propExtractCanAvoidOverwrite = TRUE;
	comm->propExtractCanSkipOlder      = TRUE;
	comm->propExtractCanJunkPaths      = TRUE;
	comm->propPassword                 = TRUE;
	comm->propTest                     = TRUE;

	FR_COMMAND_ZIP (comm)->is_empty = FALSE;
}


static void
fr_command_zip_finalize (GObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (FR_IS_COMMAND_ZIP (object));

	/* Chain up */
	if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_zip_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FrCommandZipClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_zip_class_init,
			NULL,
			NULL,
			sizeof (FrCommandZip),
			0,
			(GInstanceInitFunc) fr_command_zip_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandZip",
					       &type_info,
					       0);
	}

	return type;
}
