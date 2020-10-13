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
#include "fr-command.h"
#include "fr-command-ar.h"

static void fr_command_ar_class_init  (FrCommandArClass *class);
static void fr_command_ar_init        (FrCommand        *afile);
static void fr_command_ar_finalize    (GObject          *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;


/* -- list -- */

static time_t
mktime_from_string (char *time_s,
		    char *day_s,
		    char *month_s,
		    char *year_s)
{
	static char  *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
				   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	struct tm    tm = {0, };
	char       **fields;

	tm.tm_isdst = -1;

	/* date */

	if (month_s != NULL) {
		int i;
		for (i = 0; i < 12; i++)
			if (strcmp (months[i], month_s) == 0) {
				tm.tm_mon = i;
				break;
			}
	}
	tm.tm_mday = atoi (day_s);
	tm.tm_year = atoi (year_s) - 1900;

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
ar_get_last_field (const char *line,
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
	char        *field_month, *field_day, *field_time, *field_year;
	char        *field_size, *field_name;

	g_return_if_fail (line != NULL);

	fdata = file_data_new ();

	date_idx = file_list__get_index_from_pattern (line, "%c%c%c %a%n %n%n:%n%n %n%n%n%n");

	field_size = file_list__get_prev_field (line, date_idx, 1);
	fdata->size = g_ascii_strtoull (field_size, NULL, 10);
	g_free (field_size);

	field_month = file_list__get_next_field (line, date_idx, 1);
	field_day = file_list__get_next_field (line, date_idx, 2);
	field_time = file_list__get_next_field (line, date_idx, 3);
	field_year = file_list__get_next_field (line, date_idx, 4);
	fdata->modified = mktime_from_string (field_time, field_day, field_month, field_year);
	g_free (field_day);
	g_free (field_month);
	g_free (field_year);
	g_free (field_time);

	/* Full path */

	field_name = ar_get_last_field (line, date_idx, 5);

	fields = g_strsplit (field_name, " -> ", 2);

	if (fields[0] == NULL) {
		g_strfreev (fields);
		g_free (field_name);
		file_data_free (fdata);
		return;
	}

	if (fields[1] == NULL) {
		g_strfreev (fields);
		fields = g_strsplit (field_name, " link to ", 2);
	}

	if (*(fields[0]) == '/') {
		fdata->full_path = g_strdup (fields[0]);
		fdata->original_path = fdata->full_path;
	} else {
		fdata->full_path = g_strconcat ("/", fields[0], NULL);
		fdata->original_path = fdata->full_path + 1;
	}

	if (fields[1] != NULL)
		fdata->link = g_strdup (fields[1]);
	g_strfreev (fields);
	g_free (field_name);

	fdata->name = g_strdup (file_name_from_path (fdata->full_path));
	fdata->path = remove_level_from_path (fdata->full_path);

	if (*fdata->name == 0)
		file_data_free (fdata);
	else
		fr_command_add_file (comm, fdata);
}


static void
fr_command_ar_list (FrCommand *comm)
{
	fr_process_set_out_line_func (comm->process, process_line, comm);

	fr_process_begin_command (comm->process, "ar");
	fr_process_add_arg (comm->process, "tv");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
	fr_process_start (comm->process);
}


static void
fr_command_ar_add (FrCommand     *comm,
		   const char    *from_file,
		   GList         *file_list,
		   const char    *base_dir,
		   gboolean       update,
		   gboolean       recursive)
{
	GList *scan;

	fr_process_begin_command (comm->process, "ar");

	if (update)
		fr_process_add_arg (comm->process, "ru");
	else
		fr_process_add_arg (comm->process, "r");

	if (base_dir != NULL)
		fr_process_set_working_dir (comm->process, base_dir);

	fr_process_add_arg (comm->process, comm->filename);

	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, scan->data);

	fr_process_end_command (comm->process);
}


static void
fr_command_ar_delete (FrCommand  *comm,
		      const char *from_file,
		      GList      *file_list)
{
	GList *scan;

	fr_process_begin_command (comm->process, "ar");
	fr_process_add_arg (comm->process, "d");
	fr_process_add_arg (comm->process, comm->filename);
	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, scan->data);
	fr_process_end_command (comm->process);
}


static void
fr_command_ar_extract (FrCommand  *comm,
		       const char *from_file,
		       GList      *file_list,
		       const char *dest_dir,
		       gboolean    overwrite,
		       gboolean    skip_older,
		       gboolean    junk_paths)
{
	GList *scan;

	fr_process_begin_command (comm->process, "ar");

	if (dest_dir != NULL)
		fr_process_set_working_dir (comm->process, dest_dir);

	fr_process_add_arg (comm->process, "x");
	fr_process_add_arg (comm->process, comm->filename);
	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, scan->data);
	fr_process_end_command (comm->process);
}


static void
fr_command_ar_handle_error (FrCommand   *comm,
			    FrProcError *error)
{
	/* FIXME */
}


const char *ar_mime_type[] = { "application/x-ar", 
			       "application/x-deb",
			       NULL };


static const char **
fr_command_ar_get_mime_types (FrCommand *comm)
{
	return ar_mime_type;
}


static FrCommandCap
fr_command_ar_get_capabilities (FrCommand  *comm,
			        const char *mime_type,
				gboolean    check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES;
	if (is_program_available ("ar", check_command)) {
		if (is_mime_type (mime_type, "application/x-deb"))
			capabilities |= FR_COMMAND_CAN_READ;
		else if (is_mime_type (mime_type, "application/x-ar"))
			capabilities |= FR_COMMAND_CAN_READ_WRITE;
	}

	return capabilities;
}


static const char *
fr_command_ar_get_packages (FrCommand  *comm,
			    const char *mime_type)
{
	return PACKAGES ("binutils");
}


static void
fr_command_ar_class_init (FrCommandArClass *class)
{
        GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
        FrCommandClass *afc;

        parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_ar_finalize;

        afc->list             = fr_command_ar_list;
	afc->add              = fr_command_ar_add;
	afc->delete_           = fr_command_ar_delete;
	afc->extract          = fr_command_ar_extract;
	afc->handle_error     = fr_command_ar_handle_error;
	afc->get_mime_types   = fr_command_ar_get_mime_types;
	afc->get_capabilities = fr_command_ar_get_capabilities;
	afc->get_packages     = fr_command_ar_get_packages;
}


static void
fr_command_ar_init (FrCommand *comm)
{
	comm->propAddCanUpdate             = TRUE;
	comm->propAddCanReplace            = TRUE;
	comm->propAddCanStoreFolders       = FALSE;
	comm->propExtractCanAvoidOverwrite = FALSE;
	comm->propExtractCanSkipOlder      = FALSE;
	comm->propExtractCanJunkPaths      = FALSE;
	comm->propPassword                 = FALSE;
	comm->propTest                     = FALSE;
}


static void
fr_command_ar_finalize (GObject *object)
{
        g_return_if_fail (object != NULL);
        g_return_if_fail (FR_IS_COMMAND_AR (object));

	/* Chain up */
        if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_ar_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (FrCommandArClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_ar_class_init,
			NULL,
			NULL,
			sizeof (FrCommandAr),
			0,
			(GInstanceInitFunc) fr_command_ar_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandAr",
					       &type_info,
					       0);
        }

        return type;
}
