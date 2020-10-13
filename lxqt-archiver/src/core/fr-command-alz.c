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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <glib.h>

#include "file-data.h"
#include "file-utils.h"
#include "fr-command.h"
#include "fr-command-alz.h"
#include "glib-utils.h"

static void fr_command_alz_class_init  (FrCommandAlzClass *class);
static void fr_command_alz_init        (FrCommand         *afile);
static void fr_command_alz_finalize    (GObject           *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;


/* -- list -- */


static time_t
mktime_from_string (char *date_s,
		    char *time_s)
{
	struct tm   tm = {0, };
	char      **fields;

	/* date */

	fields = g_strsplit (date_s, "/", 3);
	if (fields[0] != NULL) {
		tm.tm_mon = atoi (fields[0]) - 1;
		if (fields[1] != NULL) {
			tm.tm_mday = atoi (fields[1]);
			if (fields[2] != NULL)
				tm.tm_year = 100 + atoi (fields[2]);
		}
	}
	g_strfreev (fields);

	/* time */

	fields = g_strsplit (time_s, ":", 3);
	if (fields[0] != NULL) {
		tm.tm_hour = atoi (fields[0]);
		if (fields[1] != NULL)
			tm.tm_min = atoi (fields[1]);
	}
	g_strfreev (fields);

	return mktime (&tm);
}


static void
process_line (char     *line,
	      gpointer  data)
{
	FrCommand     *comm = FR_COMMAND (data);
	FrCommandAlz  *alz_comm = FR_COMMAND_ALZ (comm);
	FileData      *fdata;
	char         **fields;
	char          *name_field;
	char           name_last;
	gsize	       name_len;

	g_return_if_fail (line != NULL);


	if (! alz_comm->list_started) {
		if (strncmp (line, "-----", 5 ) == 0 )
			alz_comm->list_started = TRUE;
		return;
	}

	if (strncmp (line, "-----", 5 ) == 0) {
		alz_comm->list_started = FALSE;
		return;

	}

	if (! alz_comm->list_started)
		return;

	fdata = file_data_new ();
	fields = split_line (line, 5);
	fdata->modified = mktime_from_string (fields[0], fields[1]);
	fdata->size = g_ascii_strtoull (fields[3], NULL, 10);

	name_field = g_strdup (get_last_field (line, 6));
	name_len = strlen (name_field);

	name_last = name_field[name_len - 1];
	fdata->dir = name_last == '\\';
	fdata->encrypted = name_last == '*';

	if (fdata->dir || fdata->encrypted)
		name_field[--name_len] = '\0';

	if (*name_field == '/') {
		fdata->full_path = g_strdup (name_field);
		fdata->original_path = fdata->full_path;
	}
	else {
		fdata->full_path = g_strconcat ("/", name_field, NULL);
		fdata->original_path = fdata->full_path + 1;
	}

	if (fdata->dir) {
		char *s;
		for (s = fdata->full_path; *s != '\0'; ++s)
			if (*s == '\\') *s = '/';
		for (s = fdata->original_path; *s != '\0'; ++s)
			if (*s == '\\') *s = '/';
		fdata->name = dir_name_from_path (fdata->full_path);
	}
	else {
		fdata->name = g_strdup (file_name_from_path (fdata->full_path));
	}

	fdata->path = remove_level_from_path (fdata->full_path);

	if (*fdata->name == 0)
		file_data_free (fdata);
	else
		fr_command_add_file (comm, fdata);

	g_free (name_field);
	g_strfreev (fields);
}


static void
add_codepage_arg (FrCommand *comm)
{
	const char  *env_list[] = { "LC_CTYPE", "LC_ALL", "LANG", NULL };
	const char **scan;
	const char  *arg = "-cp949";

	for (scan = env_list; *scan != NULL; ++scan) {
		char *env = getenv (*scan);

		if (! env)
			continue;

		if (strstr (env, "UTF-8") ||  strstr (env, "utf-8"))
			arg = "-utf8";
		else if (strstr (env, "euc") || strstr (env, "EUC"))
			arg = "-euc-kr";
		else
			continue;
		break;
	}

	fr_process_add_arg (comm->process, arg);
}


static void
add_password_arg (FrCommand  *comm,
		  const char *password,
		  gboolean    disable_query)
{
	if (password != NULL) {
		fr_process_add_arg (comm->process, "-pwd");
		fr_process_add_arg (comm->process, password);
	}
	else if (disable_query) {
		fr_process_add_arg (comm->process, "-pwd");
		fr_process_add_arg (comm->process, "");
	}
}


static void
list__begin (gpointer data)
{
	FrCommandAlz *comm = data;

	comm->list_started = FALSE;
	comm->invalid_password = FALSE;
}


static void
fr_command_alz_list (FrCommand  *comm)
{
	fr_process_set_out_line_func (FR_COMMAND (comm)->process, process_line, comm);

	fr_process_begin_command (comm->process, "unalz");
	fr_process_set_begin_func (comm->process, list__begin, comm);
	fr_process_add_arg (comm->process, "-l");
	add_codepage_arg(comm);
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
	fr_process_use_standard_locale (comm->process, TRUE);
	fr_process_start (comm->process);
}


/* -- extract -- */

static void
process_extract_line (char     *line,
		      gpointer  data)
{
	FrCommand     *comm = FR_COMMAND (data);
	FrCommandAlz  *alz_comm = FR_COMMAND_ALZ (comm);

	g_return_if_fail (line != NULL);

	/* - error check - */

	if (strncmp (line, "err code(28) (invalid password)", 31) == 0) {
		alz_comm->invalid_password = TRUE;
		fr_process_stop (comm->process);
		return;
	}

	if (alz_comm->extract_none && (strncmp (line, "unalziiiing :", 13) == 0)) {
		alz_comm->extract_none = FALSE;
	}
	else if ((strncmp (line, "done..", 6) == 0) && alz_comm->extract_none) {
		fr_process_stop (comm->process);
		return;
	}
}


static void
fr_command_alz_extract (FrCommand  *comm,
		        const char *from_file,
			GList      *file_list,
			const char *dest_dir,
			gboolean    overwrite,
			gboolean    skip_older,
			gboolean    junk_paths)
{
	GList *scan;

	FR_COMMAND_ALZ (comm)->extract_none = TRUE;

	fr_process_set_out_line_func (FR_COMMAND (comm)->process,
				      process_extract_line,
				      comm);

	fr_process_begin_command (comm->process, "unalz");
	if (dest_dir != NULL) {
		fr_process_add_arg (comm->process, "-d");
		fr_process_add_arg (comm->process, dest_dir);
	}
	add_codepage_arg (comm);
	add_password_arg (comm, comm->password, TRUE);
	fr_process_add_arg (comm->process, comm->filename);
	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, scan->data);
	fr_process_end_command (comm->process);
}


static void
fr_command_alz_handle_error (FrCommand   *comm,
			     FrProcError *error)
{
	if ((error->type == FR_PROC_ERROR_STOPPED)) {
		if  (FR_COMMAND_ALZ (comm)->extract_none ||
		     FR_COMMAND_ALZ (comm)->invalid_password ) {
			error->type = FR_PROC_ERROR_ASK_PASSWORD;
		}
	}
}


const char *alz_mime_type[] = { "application/x-alz", NULL };


static const char **
fr_command_alz_get_mime_types (FrCommand *comm)
{
	return alz_mime_type;
}


static FrCommandCap
fr_command_alz_get_capabilities (FrCommand  *comm,
			         const char *mime_type,
				 gboolean    check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES;
	if (is_program_available ("unalz", check_command))
		capabilities |= FR_COMMAND_CAN_READ;

	return capabilities;
}


static const char *
fr_command_alz_get_packages (FrCommand  *comm,
			     const char *mime_type)
{
	return PACKAGES ("unalz");
}


static void
fr_command_alz_class_init (FrCommandAlzClass *class)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (class);
        FrCommandClass *afc;

        parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_alz_finalize;

        afc->list             = fr_command_alz_list;
	afc->add              = NULL;
	afc->delete_           = NULL;
	afc->extract          = fr_command_alz_extract;
	afc->handle_error     = fr_command_alz_handle_error;
	afc->get_mime_types   = fr_command_alz_get_mime_types;
	afc->get_capabilities = fr_command_alz_get_capabilities;
	afc->get_packages     = fr_command_alz_get_packages;
}


static void
fr_command_alz_init (FrCommand *comm)
{
	comm->propAddCanUpdate             = TRUE;
	comm->propAddCanReplace            = TRUE;
	comm->propExtractCanAvoidOverwrite = FALSE;
	comm->propExtractCanSkipOlder      = FALSE;
	comm->propExtractCanJunkPaths      = FALSE;
	comm->propPassword                 = TRUE;
	comm->propTest                     = FALSE;
}


static void
fr_command_alz_finalize (GObject *object)
{
        g_return_if_fail (object != NULL);
        g_return_if_fail (FR_IS_COMMAND_ALZ (object));

	/* Chain up */
        if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_alz_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (FrCommandAlzClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_alz_class_init,
			NULL,
			NULL,
			sizeof (FrCommandAlz),
			0,
			(GInstanceInitFunc) fr_command_alz_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FrCommandAlz",
					       &type_info,
					       0);
        }

        return type;
}
