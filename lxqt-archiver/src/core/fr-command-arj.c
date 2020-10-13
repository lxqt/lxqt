/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>

#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-arj.h"

static void fr_command_arj_class_init  (FrCommandArjClass *class);
static void fr_command_arj_init        (FrCommand         *afile);
static void fr_command_arj_finalize    (GObject           *object);

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
		/* warning : this will work until 2075 ;) */
		int y = atoi (fields[0]);
		if (y >= 75)
			tm.tm_year = y;
		else
			tm.tm_year = 100 + y;

		tm.tm_mon = atoi (fields[1]) - 1;
		tm.tm_mday = atoi (fields[2]);
	}
	g_strfreev (fields);

	/* time */

	fields = g_strsplit (time_s, ":", 3);
	if (fields[0] != NULL) {
		tm.tm_hour = atoi (fields[0]);
		if (fields[1] != NULL) {
			tm.tm_min = atoi (fields[1]);
			if (fields[2] != NULL)
				tm.tm_sec = atoi (fields[2]);
		}
	}
	g_strfreev (fields);

	return mktime (&tm);
}


static void
list__process_line (char     *line,
		    gpointer  data)
{
	FrCommand     *comm = FR_COMMAND (data);
	FrCommandArj  *arj_comm = FR_COMMAND_ARJ (comm);

	g_return_if_fail (line != NULL);

	if (! arj_comm->list_started) {
		if (strncmp (line, "--------", 8) == 0) {
			arj_comm->list_started = TRUE;
			arj_comm->line_no = 1;
		}
		return;
	}

	if (strncmp (line, "--------", 8) == 0) {
		arj_comm->list_started = FALSE;
		return;
	}

	if (g_regex_match (arj_comm->filename_line_regex, line, 0, NULL)) { /* Read the filename. */
		FileData   *fdata;
		const char *name_field;

		arj_comm->line_no = 1;

		arj_comm->fdata = fdata = file_data_new ();

		name_field = get_last_field (line, 2);

		if (*name_field == '/') {
			fdata->full_path = g_strdup (name_field);
			fdata->original_path = fdata->full_path;
		}
		else {
			fdata->full_path = g_strconcat ("/", name_field, NULL);
			fdata->original_path = fdata->full_path + 1;
		}

		fdata->link = NULL;

		fdata->name = g_strdup (file_name_from_path (fdata->full_path));
		fdata->path = remove_level_from_path (fdata->full_path);
	}
	else if (arj_comm->line_no == 2) { /* Read file size and date. */
		FileData  *fdata;
		char     **fields;

		fdata = arj_comm->fdata;

		/* read file info. */

		fields = split_line (line, 10);
		fdata->size = g_ascii_strtoull (fields[2], NULL, 10);
		fdata->modified = mktime_from_string (fields[5], fields[6]);
		if ((strcmp (fields[1], "MS-DOS") == 0) || (strcmp (fields[1], "WIN32") == 0))
			fdata->encrypted = (g_ascii_strcasecmp (fields[7], "11") == 0);
		else
			fdata->encrypted = (g_ascii_strcasecmp (fields[9], "11") == 0);
		g_strfreev (fields);

		if (*fdata->name == 0)
			file_data_free (fdata);
		else
			fr_command_add_file (comm, fdata);
		arj_comm->fdata = NULL;
	}

	arj_comm->line_no++;
}


static void
fr_command_arj_list (FrCommand *comm)
{
	fr_process_set_out_line_func (comm->process, list__process_line, comm);

	fr_process_begin_command (comm->process, "arj");
	fr_process_add_arg (comm->process, "v");
	fr_process_add_arg (comm->process, "-y");
	fr_process_add_arg (comm->process, "-");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
	fr_process_start (comm->process);
}


static void
fr_command_arj_add (FrCommand     *comm,
		    const char    *from_file,
		    GList         *file_list,
		    const char    *base_dir,
		    gboolean       update,
		    gboolean       recursive)
{
	GList *scan;

	fr_process_begin_command (comm->process, "arj");

	fr_process_add_arg (comm->process, "a");

	if (base_dir != NULL)
		fr_process_set_working_dir (comm->process, base_dir);

	if (update)
		fr_process_add_arg (comm->process, "-u");

	if (comm->password != NULL)
		fr_process_add_arg_concat (comm->process, "-g/", comm->password, NULL);

	switch (comm->compression) {
	case FR_COMPRESSION_VERY_FAST:
		fr_process_add_arg (comm->process, "-m3"); break;
	case FR_COMPRESSION_FAST:
		fr_process_add_arg (comm->process, "-m2"); break;
	case FR_COMPRESSION_NORMAL:
		fr_process_add_arg (comm->process, "-m1"); break;
	case FR_COMPRESSION_MAXIMUM:
		fr_process_add_arg (comm->process, "-m1"); break;
	}

	fr_process_add_arg (comm->process, "-i");
	fr_process_add_arg (comm->process, "-y");
	fr_process_add_arg (comm->process, "-");

	fr_process_add_arg (comm->process, comm->filename);

	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, (gchar*) scan->data);

	fr_process_end_command (comm->process);
}


static void
fr_command_arj_delete (FrCommand  *comm,
		       const char *from_file,
		       GList      *file_list)
{
	GList *scan;

	fr_process_begin_command (comm->process, "arj");
	fr_process_add_arg (comm->process, "d");

	fr_process_add_arg (comm->process, "-i");
	fr_process_add_arg (comm->process, "-y");
	fr_process_add_arg (comm->process, "-");

	fr_process_add_arg (comm->process, comm->filename);

	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, scan->data);
	fr_process_end_command (comm->process);
}


static void
fr_command_arj_extract (FrCommand  *comm,
			const char *from_file,
			GList      *file_list,
			const char *dest_dir,
			gboolean    overwrite,
			gboolean    skip_older,
			gboolean    junk_paths)
{
	GList *scan;

	fr_process_begin_command (comm->process, "arj");

	if (junk_paths)
		fr_process_add_arg (comm->process, "e");
	else
		fr_process_add_arg (comm->process, "x");

	if (dest_dir != NULL)
		fr_process_add_arg_concat (comm->process, "-ht/", dest_dir, NULL);

	if (! overwrite)
		fr_process_add_arg (comm->process, "-n");

	if (skip_older)
		fr_process_add_arg (comm->process, "-u");

	if (comm->password != NULL)
		fr_process_add_arg_concat (comm->process, "-g/", comm->password, NULL);
	else
 		fr_process_add_arg (comm->process, "-g/");

	fr_process_add_arg (comm->process, "-i");
	fr_process_add_arg (comm->process, "-y");
	fr_process_add_arg (comm->process, "-");

	fr_process_add_arg (comm->process, comm->filename);

	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, scan->data);

	fr_process_end_command (comm->process);
}


static void
fr_command_arj_test (FrCommand   *comm)
{
	fr_process_begin_command (comm->process, "arj");
	fr_process_add_arg (comm->process, "t");
	if (comm->password != NULL)
		fr_process_add_arg_concat (comm->process, "-g/", comm->password, NULL);
	fr_process_add_arg (comm->process, "-i");
	fr_process_add_arg (comm->process, "-y");
	fr_process_add_arg (comm->process, "-");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
}


static void
fr_command_arj_handle_error (FrCommand   *comm,
			     FrProcError *error)
{
	if (error->type != FR_PROC_ERROR_NONE) {
 		if (error->status <= 1)
 			error->type = FR_PROC_ERROR_NONE;
		else if (error->status == 3)
 			error->type = FR_PROC_ERROR_ASK_PASSWORD;
 	}
}


const char *arj_mime_type[] = { "application/x-arj", NULL };


static const char **
fr_command_arj_get_mime_types (FrCommand *comm)
{
	return arj_mime_type;
}


static FrCommandCap
fr_command_arj_get_capabilities (FrCommand  *comm,
			         const char *mime_type,
				 gboolean    check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES | FR_COMMAND_CAN_ENCRYPT;
	if (is_program_available ("arj", check_command))
		capabilities |= FR_COMMAND_CAN_READ_WRITE;

	return capabilities;
}


static const char *
fr_command_arj_get_packages (FrCommand  *comm,
			     const char *mime_type)
{
	return PACKAGES ("arj");
}


static void
fr_command_arj_class_init (FrCommandArjClass *class)
{
	GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
	FrCommandClass *afc;

	parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_arj_finalize;

	afc->list             = fr_command_arj_list;
	afc->add              = fr_command_arj_add;
	afc->delete_           = fr_command_arj_delete;
	afc->extract          = fr_command_arj_extract;
	afc->test             = fr_command_arj_test;
	afc->handle_error     = fr_command_arj_handle_error;
	afc->get_mime_types   = fr_command_arj_get_mime_types;
	afc->get_capabilities = fr_command_arj_get_capabilities;
	afc->get_packages     = fr_command_arj_get_packages;
}


static void
fr_command_arj_init (FrCommand *comm)
{
	FrCommandArj *arj_comm;

	comm->propAddCanUpdate             = TRUE;
	comm->propAddCanReplace            = TRUE;
	comm->propAddCanStoreFolders       = FALSE;
	comm->propExtractCanAvoidOverwrite = TRUE;
	comm->propExtractCanSkipOlder      = TRUE;
	comm->propExtractCanJunkPaths      = TRUE;
	comm->propPassword                 = TRUE;
	comm->propTest                     = TRUE;

	arj_comm = FR_COMMAND_ARJ (comm);
	arj_comm->list_started = FALSE;
	arj_comm->fdata = FALSE;
	arj_comm->filename_line_regex = g_regex_new ("[0-9]+\\) ", G_REGEX_OPTIMIZE, 0, NULL);
}


static void
fr_command_arj_finalize (GObject *object)
{
	FrCommandArj *arj_comm;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FR_IS_COMMAND_ARJ (object));

	arj_comm = FR_COMMAND_ARJ (object);
	g_regex_unref (arj_comm->filename_line_regex);

	/* Chain up */
	if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_arj_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FrCommandArjClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_arj_class_init,
			NULL,
			NULL,
			sizeof (FrCommandArj),
			0,
			(GInstanceInitFunc) fr_command_arj_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandArj",
					       &type_info,
					       0);
	}

	return type;
}
