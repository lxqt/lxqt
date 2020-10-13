/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2004 The Free Software Foundation, Inc.
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
#include "fr-command-iso.h"

static void fr_command_iso_class_init  (FrCommandIsoClass *class);
static void fr_command_iso_init        (FrCommand         *afile);
static void fr_command_iso_finalize    (GObject           *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;


static time_t
mktime_from_string (char *month,
		    char *mday,
		    char *year)
{
	static char  *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
				   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	struct tm     tm = {0, };

	tm.tm_isdst = -1;

	if (month != NULL) {
		int i;
		for (i = 0; i < 12; i++)
			if (strcmp (months[i], month) == 0) {
				tm.tm_mon = i;
				break;
			}
	}
	tm.tm_mday = atoi (mday);
	tm.tm_year = atoi (year) - 1900;

	return mktime (&tm);
}


static void
list__process_line (char     *line,
		    gpointer  data)
{
	FileData      *fdata;
	FrCommand     *comm = FR_COMMAND (data);
	FrCommandIso  *comm_iso = FR_COMMAND_ISO (comm);
	char         **fields;
	const char    *name_field;

	g_return_if_fail (line != NULL);

	if (line[0] == 'd') /* Ignore directories. */
		return;

	if (line[0] == 'D') {
		g_free (comm_iso->cur_path);
		comm_iso->cur_path = g_strdup (get_last_field (line, 4));

	} else if (line[0] == '-') { /* Is file */
		const char *last_field, *first_bracket;

		fdata = file_data_new ();

		fields = split_line (line, 8);
		fdata->size = g_ascii_strtoull (fields[4], NULL, 10);
		fdata->modified = mktime_from_string (fields[5], fields[6], fields[7]);
		g_strfreev (fields);

		/* Full path */

		last_field = get_last_field (line, 9);
		first_bracket = strchr (last_field, ']');
		if (first_bracket == NULL) {
			file_data_free (fdata);
			return;
		}

		name_field = eat_spaces (first_bracket + 1);
		if ((name_field == NULL)
		    || (strcmp (name_field, ".") == 0)
		    || (strcmp (name_field, "..") == 0)) {
			file_data_free (fdata);
			return;
		}

		if (comm_iso->cur_path[0] != '/')
			fdata->full_path = g_strstrip (g_strconcat ("/", comm_iso->cur_path, name_field, NULL));
		else
			fdata->full_path = g_strstrip (g_strconcat (comm_iso->cur_path, name_field, NULL));
		fdata->original_path = fdata->full_path;
		fdata->name = g_strdup (file_name_from_path (fdata->full_path));
		fdata->path = remove_level_from_path (fdata->full_path);

		fr_command_add_file (comm, fdata);
	}
}


static void
list__begin (gpointer data)
{
	FrCommandIso *comm = data;

	g_free (comm->cur_path);
	comm->cur_path = NULL;
}


static void
fr_command_iso_list (FrCommand *comm)
{
	fr_process_set_out_line_func (comm->process, list__process_line, comm);

	fr_process_begin_command (comm->process, "sh");
	fr_process_set_begin_func (comm->process, list__begin, comm);
	fr_process_add_arg (comm->process, SHDIR "isoinfo.sh");
	fr_process_add_arg (comm->process, "-i");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_add_arg (comm->process, "-l");
	fr_process_end_command (comm->process);

	fr_process_start (comm->process);
}


static void
fr_command_iso_extract (FrCommand  *comm,
			const char *from_file,
			GList      *file_list,
			const char *dest_dir,
			gboolean    overwrite,
			gboolean    skip_older,
			gboolean    junk_paths)
{
	GList *scan;

	for (scan = file_list; scan; scan = scan->next) {
		char       *path = scan->data;
		const char *filename;
		char       *file_dir;
		char       *temp_dest_dir = NULL;

		filename = file_name_from_path (path);
		file_dir = remove_level_from_path (path);
		if ((file_dir != NULL) && (strcmp (file_dir, "/") != 0))
			temp_dest_dir = g_build_filename (dest_dir, file_dir, NULL);
		 else
			temp_dest_dir = g_strdup (dest_dir);
		g_free (file_dir);

		if (temp_dest_dir == NULL)
			continue;

		make_directory_tree_from_path (temp_dest_dir, 0700, NULL);

		fr_process_begin_command (comm->process, "sh");
		fr_process_set_working_dir (comm->process, temp_dest_dir);
		fr_process_add_arg (comm->process, SHDIR "isoinfo.sh");
		fr_process_add_arg (comm->process, "-i");
		fr_process_add_arg (comm->process, comm->filename);
		fr_process_add_arg (comm->process, "-x");
		fr_process_add_arg (comm->process, path);
		fr_process_add_arg (comm->process, filename);
		fr_process_end_command (comm->process);

		g_free (temp_dest_dir);
	}
}


const char *iso_mime_type[] = { "application/x-cd-image", NULL };


static const char **
fr_command_iso_get_mime_types (FrCommand *comm)
{
	return iso_mime_type;
}


static FrCommandCap
fr_command_iso_get_capabilities (FrCommand  *comm,
			         const char *mime_type,
				 gboolean    check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES;
	if (is_program_available ("isoinfo", check_command))
		capabilities |= FR_COMMAND_CAN_READ;

	return capabilities;
}


static const char *
fr_command_iso_get_packages (FrCommand  *comm,
			     const char *mime_type)
{
	return PACKAGES ("genisoimage");
}


static void
fr_command_iso_class_init (FrCommandIsoClass *class)
{
	GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
	FrCommandClass *afc;

	parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_iso_finalize;

	afc->list             = fr_command_iso_list;
	afc->extract          = fr_command_iso_extract;
	afc->get_mime_types   = fr_command_iso_get_mime_types;
	afc->get_capabilities = fr_command_iso_get_capabilities;
	afc->get_packages     = fr_command_iso_get_packages;
}


static void
fr_command_iso_init (FrCommand *comm)
{
	FrCommandIso *comm_iso = FR_COMMAND_ISO (comm);

	comm_iso->cur_path = NULL;
	comm_iso->joliet = TRUE;

	comm->propAddCanUpdate             = FALSE;
	comm->propAddCanReplace            = FALSE;
	comm->propExtractCanAvoidOverwrite = FALSE;
	comm->propExtractCanSkipOlder      = FALSE;
	comm->propExtractCanJunkPaths      = FALSE;
	comm->propPassword                 = FALSE;
	comm->propTest                     = FALSE;
	comm->propCanExtractAll            = FALSE;
}


static void
fr_command_iso_finalize (GObject *object)
{
	FrCommandIso *comm_iso;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FR_IS_COMMAND_ISO (object));

	comm_iso = FR_COMMAND_ISO (object);

	g_free (comm_iso->cur_path);
	comm_iso->cur_path = NULL;

	/* Chain up */
	if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_iso_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FrCommandIsoClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_iso_class_init,
			NULL,
			NULL,
			sizeof (FrCommandIso),
			0,
			(GInstanceInitFunc) fr_command_iso_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandIso",
					       &type_info,
					       0);
	}

	return type;
}
