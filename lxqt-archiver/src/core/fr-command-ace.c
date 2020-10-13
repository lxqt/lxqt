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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>

#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-ace.h"

static void fr_command_ace_class_init  (FrCommandAceClass *class);
static void fr_command_ace_init        (FrCommand        *afile);
static void fr_command_ace_finalize    (GObject          *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;


/* -- list -- */


static time_t
mktime_from_string (char *date,
		    char *time)
{
	struct tm    tm = {0, };
	char       **fields;

	tm.tm_isdst = -1;

	/* date */

	fields = g_strsplit (date, ".", 3);
	if (fields[0] != NULL) {
		tm.tm_mday = atoi (fields[0]);
		if (fields[1] != NULL) {
			tm.tm_mon = atoi (fields[1]) - 1;
			if (fields[2] != NULL) {
				int y = atoi (fields[2]);
				if (y > 75)
					tm.tm_year = y;
				else
					tm.tm_year = 100 + y;
			}
		}
	}
	g_strfreev (fields);

	/* time */

	fields = g_strsplit (time, ":", 2);
	if (fields[0] != NULL) {
		tm.tm_hour = atoi (fields[0]);
		if (fields[1] != NULL)
			tm.tm_min = atoi (fields[1]);
	}
	tm.tm_sec = 0;
	g_strfreev (fields);

	return mktime (&tm);
}


static void
process_line (char     *line,
	      gpointer  data)
{
	FileData      *fdata;
	FrCommandAce  *ace_comm = FR_COMMAND_ACE (data);
	FrCommand     *comm = FR_COMMAND (data);
	char         **fields = NULL;
	const char    *field_name = NULL;

	g_return_if_fail (line != NULL);

	if (ace_comm->command_type == FR_ACE_COMMAND_UNKNOWN) {
		if (g_str_has_prefix (line, "UNACE")) {
			if (strstr (line, "public version") != NULL)
				ace_comm->command_type = FR_ACE_COMMAND_PUBLIC;
			else
				ace_comm->command_type = FR_ACE_COMMAND_NONFREE;
		}
		return;
	}

	if (! ace_comm->list_started) {
		if (ace_comm->command_type == FR_ACE_COMMAND_PUBLIC) {
			if (g_str_has_prefix (line, "Date"))
				ace_comm->list_started = TRUE;
		}
		else if (ace_comm->command_type == FR_ACE_COMMAND_NONFREE) {
			if (g_str_has_prefix (line, "  Date"))
				ace_comm->list_started = TRUE;
		}
		return;
	}

	fdata = file_data_new ();

	if (ace_comm->command_type == FR_ACE_COMMAND_PUBLIC)
		fields = g_strsplit (line, "|", 6);
	else if (ace_comm->command_type == FR_ACE_COMMAND_NONFREE)
		fields = split_line (line, 5);

	if ((fields == NULL) || (fields[0] == NULL) || (n_fields (fields) < 5))
		return;

	fdata->size = g_ascii_strtoull (fields[3], NULL, 10);
	fdata->modified = mktime_from_string (fields[0], fields[1]);

	if (ace_comm->command_type == FR_ACE_COMMAND_PUBLIC) {
		field_name = fields[5];
		field_name = field_name + 1;
	}
	else if (ace_comm->command_type == FR_ACE_COMMAND_NONFREE)
		field_name = get_last_field (line, 6);

        g_assert (field_name != NULL);
	if (field_name[0] != '/') {
		fdata->full_path = g_strconcat ("/", field_name, NULL);
		fdata->original_path = fdata->full_path + 1;
	}
	else {
		fdata->full_path = g_strdup (field_name);
		fdata->original_path = fdata->full_path;
	}

	g_strfreev (fields);

	fdata->name = g_strdup (file_name_from_path (fdata->full_path));
	fdata->path = remove_level_from_path (fdata->full_path);

	if (*fdata->name == 0)
		file_data_free (fdata);
	else
		fr_command_add_file (comm, fdata);
}


static void
list__begin (gpointer data)
{
	FrCommandAce *comm = data;

	comm->list_started = FALSE;
	comm->command_type = FR_ACE_COMMAND_UNKNOWN;
}


static void
fr_command_ace_list (FrCommand  *comm)
{
	fr_process_set_out_line_func (comm->process, process_line, comm);

	fr_process_begin_command (comm->process, "unace");
	fr_process_set_begin_func (comm->process, list__begin, comm);
	fr_process_add_arg (comm->process, "v");
	fr_process_add_arg (comm->process, "-y");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
	fr_process_start (comm->process);
}


static void
fr_command_ace_extract (FrCommand   *comm,
			const char  *from_file,
			GList       *file_list,
			const char  *dest_dir,
			gboolean     overwrite,
			gboolean     skip_older,
			gboolean     junk_paths)
{
	GList *scan;

	fr_process_begin_command (comm->process, "unace");

	if (dest_dir != NULL)
		fr_process_set_working_dir (comm->process, dest_dir);

	if (junk_paths)
		fr_process_add_arg (comm->process, "e");
	else
		fr_process_add_arg (comm->process, "x");
	fr_process_add_arg (comm->process, "-y");
	fr_process_add_arg (comm->process, comm->filename);

	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, scan->data);

	fr_process_end_command (comm->process);
}


static void
fr_command_ace_test (FrCommand   *comm)
{
        fr_process_begin_command (comm->process, "unace");
        fr_process_add_arg (comm->process, "t");
	fr_process_add_arg (comm->process, "-y");
	fr_process_add_arg (comm->process, comm->filename);
        fr_process_end_command (comm->process);
}


static void
fr_command_ace_handle_error (FrCommand   *comm,
			     FrProcError *error)
{
	/* FIXME */
}


const char *ace_mime_type[] = { "application/x-ace", NULL };


static const char **
fr_command_ace_get_mime_types (FrCommand *comm)
{
	return ace_mime_type;
}


static FrCommandCap
fr_command_ace_get_capabilities (FrCommand  *comm,
			         const char *mime_type,
				 gboolean    check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES;
	if (is_program_available ("unace", check_command))
		capabilities |= FR_COMMAND_CAN_READ;

	return capabilities;
}


static const char *
fr_command_ace_get_packages (FrCommand  *comm,
			     const char *mime_type)
{
	return PACKAGES ("unace");
}


static void
fr_command_ace_class_init (FrCommandAceClass *class)
{
        GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
        FrCommandClass *afc;

        parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_ace_finalize;

        afc->list             = fr_command_ace_list;
	afc->extract          = fr_command_ace_extract;
	afc->test             = fr_command_ace_test;
	afc->handle_error     = fr_command_ace_handle_error;
	afc->get_mime_types   = fr_command_ace_get_mime_types;
	afc->get_capabilities = fr_command_ace_get_capabilities;
	afc->get_packages     = fr_command_ace_get_packages;
}


static void
fr_command_ace_init (FrCommand *comm)
{
	comm->propAddCanUpdate             = TRUE;
	comm->propAddCanReplace            = TRUE;
	comm->propExtractCanAvoidOverwrite = FALSE;
	comm->propExtractCanSkipOlder      = FALSE;
	comm->propExtractCanJunkPaths      = TRUE;
	comm->propPassword                 = FALSE;
	comm->propTest                     = TRUE;
}


static void
fr_command_ace_finalize (GObject *object)
{
        g_return_if_fail (object != NULL);
        g_return_if_fail (FR_IS_COMMAND_ACE (object));

	/* Chain up */
        if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_ace_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (FrCommandAceClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_ace_class_init,
			NULL,
			NULL,
			sizeof (FrCommandAce),
			0,
			(GInstanceInitFunc) fr_command_ace_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandAce",
					       &type_info,
					       0);
        }

        return type;
}
