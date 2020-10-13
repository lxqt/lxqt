/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2012 The Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "tr-wrapper.h"
#include <json-glib/json-glib.h>
#include "file-data.h"
#include "file-utils.h"
#include "gio-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-unarchiver.h"
#include "fr-error.h"

#define LSAR_SUPPORTED_FORMAT 2
#define LSAR_DATE_FORMAT "%Y-%m-%d %H:%M:%S %z"

static void fr_command_unarchiver_class_init  (FrCommandUnarchiverClass *class);
static void fr_command_unarchiver_init        (FrCommand         *afile);
static void fr_command_unarchiver_finalize    (GObject           *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;


/* -- list -- */


static void
process_line (char     *line,
	      gpointer  data)
{
	FrCommandUnarchiver *unar_comm = FR_COMMAND_UNARCHIVER (data);
	g_memory_input_stream_add_data (G_MEMORY_INPUT_STREAM (unar_comm->stream), line, -1, NULL);
}


static time_t
mktime_from_string (const char *time_s)
{
	struct tm tm = {0, };
	tm.tm_isdst = -1;
	strptime (time_s, LSAR_DATE_FORMAT, &tm);
	return mktime (&tm);
}

static void
list_command_completed (gpointer data)
{
	FrCommandUnarchiver *unar_comm = FR_COMMAND_UNARCHIVER (data);
	JsonParser          *parser;
	GError              *error = NULL;

	parser = json_parser_new ();
	if (json_parser_load_from_stream (parser, unar_comm->stream, NULL, &error)) {
		JsonObject *root;

		root = json_node_get_object (json_parser_get_root (parser));

		if (json_object_get_int_member (root, "lsarFormatVersion") == LSAR_SUPPORTED_FORMAT) {
			JsonArray *content;
			int        i;

			content = json_object_get_array_member (root, "lsarContents");
			for (i = 0; i < json_array_get_length (content); i++) {
				JsonObject *entry;
				FileData   *fdata;
				const char *filename;

				entry = json_array_get_object_element (content, i);
				fdata = file_data_new ();
				fdata->size = json_object_get_int_member (entry, "XADFileSize");
				fdata->modified = mktime_from_string (json_object_get_string_member (entry, "XADLastModificationDate"));
				if (json_object_has_member (entry, "XADIsEncrypted"))
					fdata->encrypted = json_object_get_int_member (entry, "XADIsEncrypted") == 1;

				filename = json_object_get_string_member (entry, "XADFileName");
				if (*filename == '/') {
					fdata->full_path = g_strdup (filename);
					fdata->original_path = fdata->full_path;
				}
				else {
					fdata->full_path = g_strconcat ("/", filename, NULL);
					fdata->original_path = fdata->full_path + 1;
				}

				fdata->link = NULL;
				if (json_object_has_member (entry, "XADIsDirectory"))
					fdata->dir = json_object_get_int_member (entry, "XADIsDirectory") == 1;
				if (fdata->dir)
					fdata->name = dir_name_from_path (fdata->full_path);
				else
					fdata->name = g_strdup (file_name_from_path (fdata->full_path));
				fdata->path = remove_level_from_path (fdata->full_path);

				fr_command_add_file (FR_COMMAND (unar_comm), fdata);
			}
		}
	}

	g_object_unref (parser);
}


static void
fr_command_unarchiver_list (FrCommand  *comm)
{
	FrCommandUnarchiver *unar_comm = FR_COMMAND_UNARCHIVER (comm);

	_g_object_unref (unar_comm->stream);
	unar_comm->stream = g_memory_input_stream_new ();

	fr_process_set_out_line_func (comm->process, process_line, comm);

	fr_process_begin_command (comm->process, "lsar");
	fr_process_set_end_func (comm->process, list_command_completed, comm);
	fr_process_add_arg (comm->process, "-j");
	if ((comm->password != NULL) && (comm->password[0] != '\0'))
		fr_process_add_arg_concat (comm->process, "-password=", comm->password, NULL);
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);

	fr_process_start (comm->process);
}


static void
process_line__extract (char     *line,
		       gpointer  data)
{
	FrCommand           *comm = FR_COMMAND (data);
	FrCommandUnarchiver *unar_comm = FR_COMMAND_UNARCHIVER (comm);

	if (line == NULL)
		return;

	unar_comm->n_line++;

	/* the first line is the name of the archive */
	if (unar_comm->n_line == 1)
		return;

	if (comm->n_files > 1) {
		double fraction = (double) ++comm->n_file / (comm->n_files + 1);
		fr_command_progress (comm, CLAMP (fraction, 0.0, 1.0));
	}
	else
		fr_command_message (comm, line);
}


static void
fr_command_unarchiver_extract (FrCommand  *comm,
			       const char *from_file,
			       GList      *file_list,
			       const char *dest_dir,
			       gboolean    overwrite,
			       gboolean    skip_older,
			       gboolean    junk_paths)
{
	FrCommandUnarchiver *unar_comm = FR_COMMAND_UNARCHIVER (comm);
	GList               *scan;

	unar_comm->n_line = 0;

	fr_process_use_standard_locale (comm->process, TRUE);
	fr_process_set_out_line_func (comm->process,
				      process_line__extract,
				      comm);

	fr_process_begin_command (comm->process, "unar");

	if (overwrite)
		fr_process_add_arg (comm->process, "-f");
	else
		fr_process_add_arg (comm->process, "-s");

	fr_process_add_arg (comm->process, "-D");

	if ((comm->password != NULL) && (comm->password[0] != '\0'))
		fr_process_add_arg_concat (comm->process, "-password=", comm->password, NULL);

	if (dest_dir != NULL)
		fr_process_add_arg_concat (comm->process, "-output-directory=", dest_dir, NULL);

	fr_process_add_arg (comm->process, comm->filename);

	for (scan = file_list; scan; scan = scan->next) {
		char *escaped;

		escaped = escape_str (scan->data, "[?");
		fr_process_add_arg (comm->process, escaped);
		g_free (escaped);
	}

	fr_process_end_command (comm->process);
}


static void
fr_command_unarchiver_handle_error (FrCommand   *comm,
				    FrProcError *error)
{
	GList *scan;

#if 0
	{
		for (scan = g_list_last (comm->process->err.raw); scan; scan = scan->prev)
			g_print ("%s\n", (char*)scan->data);
	}
#endif

	if (error->type == FR_PROC_ERROR_NONE)
		return;

	for (scan = g_list_last (comm->process->err.raw); scan; scan = scan->prev) {
		char *line = scan->data;

		if (strstr (line, "password") != NULL) {
			error->type = FR_PROC_ERROR_ASK_PASSWORD;
			break;
		}
	}
}


const char *unarchiver_mime_type[] = { "application/x-cbr",
				       "application/x-rar",
				       NULL };


static const char **
fr_command_unarchiver_get_mime_types (FrCommand *comm)
{
	return unarchiver_mime_type;
}


static FrCommandCap
fr_command_unarchiver_get_capabilities (FrCommand  *comm,
					const char *mime_type,
					gboolean    check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_DO_NOTHING;
	if (is_program_available ("lsar", check_command) && is_program_available ("unar", check_command))
		capabilities |= FR_COMMAND_CAN_READ;

	return capabilities;
}


static const char *
fr_command_unarchiver_get_packages (FrCommand  *comm,
				    const char *mime_type)
{
	return PACKAGES ("unarchiver");
}


static void
fr_command_unarchiver_class_init (FrCommandUnarchiverClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);
	FrCommandClass *afc;

	parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_unarchiver_finalize;

	afc->list             = fr_command_unarchiver_list;
	afc->extract          = fr_command_unarchiver_extract;
	afc->handle_error     = fr_command_unarchiver_handle_error;
	afc->get_mime_types   = fr_command_unarchiver_get_mime_types;
	afc->get_capabilities = fr_command_unarchiver_get_capabilities;
	afc->get_packages     = fr_command_unarchiver_get_packages;
}


static void
fr_command_unarchiver_init (FrCommand *comm)
{
	FrCommandUnarchiver *unar_comm;

	comm->propExtractCanAvoidOverwrite = TRUE;
	comm->propExtractCanSkipOlder      = FALSE;
	comm->propExtractCanJunkPaths      = FALSE;
	comm->propPassword                 = TRUE;
	comm->propTest                     = FALSE;
	comm->propListFromFile             = FALSE;

	unar_comm = FR_COMMAND_UNARCHIVER (comm);
	unar_comm->stream = NULL;
}


static void
fr_command_unarchiver_finalize (GObject *object)
{
	FrCommandUnarchiver *unar_comm;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FR_IS_COMMAND_UNARCHIVER (object));

	unar_comm = FR_COMMAND_UNARCHIVER (object);
	_g_object_unref (unar_comm->stream);

	/* Chain up */
	if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_unarchiver_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FrCommandUnarchiverClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_unarchiver_class_init,
			NULL,
			NULL,
			sizeof (FrCommandUnarchiver),
			0,
			(GInstanceInitFunc) fr_command_unarchiver_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FrCommandUnarchiver",
					       &type_info,
					       0);
	}

	return type;
}
