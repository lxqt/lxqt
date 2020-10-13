/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2004, 2008 Free Software Foundation, Inc.
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

#include <stdio.h>
#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>
#include "tr-wrapper.h"

#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-7z.h"
#include "rar-utils.h"

static void fr_command_7z_class_init  (FrCommand7zClass *class);
static void fr_command_7z_init        (FrCommand        *afile);
static void fr_command_7z_finalize    (GObject          *object);

static gboolean spd_support = FALSE;
static gboolean unexpected_end_of_archive = FALSE;
static gboolean password_required = FALSE;
static gboolean password_handled = FALSE;

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
	FrCommand    *comm = FR_COMMAND (data);
	FrCommand7z  *p7z_comm = FR_COMMAND_7Z (comm);
	char        **fields;
	FileData     *fdata;

	g_return_if_fail (line != NULL);

	if (! p7z_comm->list_started) {
		if (strncmp (line, "p7zip Version ", 14) == 0) {
			const char *ver_start;
			int         ver_len;
			char        version[256];

			ver_start = eat_spaces (line + 14);
			ver_len = strchr (ver_start, ' ') - ver_start;
			strncpy (version, ver_start, ver_len);
			version[ver_len] = 0;

			if ((strcmp (version, "4.55") < 0) && (ver_len > 1) && (version[1] == '.'))
				p7z_comm->old_style = TRUE;
			else
				p7z_comm->old_style = FALSE;
			if ((strcmp (version, "9.38") < 0) && (ver_len > 1) && (version[1] == '.'))
				spd_support = FALSE;
			else
				spd_support = TRUE;
		}
		else if (p7z_comm->old_style && (strncmp (line, "Listing archive: ", 17) == 0))
			p7z_comm->list_started = TRUE;
		else if (! p7z_comm->old_style && (strcmp (line, "----------") == 0))
			p7z_comm->list_started = TRUE;
		else if (strncmp (line, "Multivolume = ", 14) == 0) {
			fields = g_strsplit (line, " = ", 2);
			comm->multi_volume = (strcmp (fields[1], "+") == 0);
			g_strfreev (fields);
		}
		else if (strncmp (line, "Unexpected end of archive", 25) == 0)  { 
			unexpected_end_of_archive = TRUE;
		}
		return;
	}

	if (strcmp (line, "") == 0) {
		if (p7z_comm->fdata != NULL) {
			if (p7z_comm->fdata->original_path == NULL) {
				file_data_free (p7z_comm->fdata);
				p7z_comm->fdata = NULL;
			}
			else {
				fdata = p7z_comm->fdata;
				if (fdata->dir)
					fdata->name = dir_name_from_path (fdata->full_path);
				else
					fdata->name = g_strdup (file_name_from_path (fdata->full_path));
				fdata->path = remove_level_from_path (fdata->full_path);
				fr_command_add_file (comm, fdata);
				p7z_comm->fdata = NULL;
			}
		}
		return;
	}

	if (p7z_comm->fdata == NULL)
		p7z_comm->fdata = file_data_new ();

	fields = g_strsplit (line, " = ", 2);

	if (g_strv_length (fields) < 2) {
		g_strfreev (fields);
		return;
	}

	fdata = p7z_comm->fdata;

	if (strcmp (fields[0], "Path") == 0) {
		fdata->free_original_path = TRUE;
		fdata->original_path = g_strdup (fields[1]);
		fdata->full_path = g_strconcat ((fdata->original_path[0] != '/') ? "/" : "",
						fdata->original_path,
						(fdata->dir && (fdata->original_path[strlen (fdata->original_path) - 1] != '/')) ? "/" : "",
						NULL);
	}
	else if (strcmp (fields[0], "Folder") == 0) {
		fdata->dir = (strcmp (fields[1], "+") == 0);
	}
	else if (strcmp (fields[0], "Size") == 0) {
		fdata->size = g_ascii_strtoull (fields[1], NULL, 10);
	}
	else if (strcmp (fields[0], "Modified") == 0) {
		char **modified_fields;

		modified_fields = g_strsplit (fields[1], " ", 2);
		if (modified_fields[0] != NULL)
			fdata->modified = mktime_from_string (modified_fields[0], modified_fields[1]);
		g_strfreev (modified_fields);
	}
	else if (strcmp (fields[0], "Encrypted") == 0) {
		if (strcmp (fields[1], "+") == 0)
			fdata->encrypted = TRUE;
	}
	else if (strcmp (fields[0], "Method") == 0) {
		if (strstr (fields[1], "AES") != NULL)
			fdata->encrypted = TRUE;
	}
	else if (strcmp (fields[0], "Attributes") == 0) {
		if (fields[1][0] == 'D')
			fdata->dir = TRUE;
	}
	g_strfreev (fields);
}


static void
fr_command_7z_begin_command (FrCommand *comm)
{
	if (is_program_in_path ("7z"))
		fr_process_begin_command (comm->process, "7z");
	else if (is_program_in_path ("7za"))
		fr_process_begin_command (comm->process, "7za");
	else if (is_program_in_path ("7zr"))
		fr_process_begin_command (comm->process, "7zr");
}


static void
add_password_arg (FrCommand     *comm,
		  const char    *password,
		  gboolean       always_specify)
{
	if (always_specify || ((password != NULL) && (*password != 0))) {
		char *arg;

		arg = g_strconcat ("-p", password, NULL);
		fr_process_add_arg (comm->process, arg);
		g_free (arg);
		password_handled = TRUE;
	}
}


static void
list__begin (gpointer data)
{
	FrCommand7z *p7z_comm = data;

	if (p7z_comm->fdata != NULL) {
		file_data_free (p7z_comm->fdata);
		p7z_comm->fdata = NULL;
	}
	p7z_comm->list_started = FALSE;
}


static void
fr_command_7z_list (FrCommand  *comm)
{
	rar_check_multi_volume (comm);

	fr_process_set_out_line_func (comm->process, list__process_line, comm);

	fr_command_7z_begin_command (comm);
	fr_process_set_begin_func (comm->process, list__begin, comm);
	fr_process_add_arg (comm->process, "l");
	fr_process_add_arg (comm->process, "-slt");
	fr_process_add_arg (comm->process, "-bd");
	fr_process_add_arg (comm->process, "-y");
	add_password_arg (comm, comm->password, FALSE);
	fr_process_add_arg (comm->process, "--");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);

	fr_process_start (comm->process);
}


static void
parse_progress_line (FrCommand  *comm,
		     const char *prefix,
		     const char *message_prefix,
		     const char *line)
{
	int prefix_len;

	prefix_len = strlen (prefix);
	if (strncmp (line, prefix, prefix_len) == 0)
		fr_command_progress (comm, (double) ++comm->n_file / (comm->n_files + 1));
}


static void
process_line__add (char     *line,
		   gpointer  data)
{
	FrCommand *comm = FR_COMMAND (data);

	if (strstr (line, "Enter password") != NULL)
		password_required = TRUE;

	if ((comm->volume_size > 0) && (strncmp (line, "Creating archive", 16) == 0)) {
		char  *volume_filename;
		GFile *volume_file;

		volume_filename = g_strconcat (comm->filename, ".001", NULL);
		volume_file = g_file_new_for_path (volume_filename);
		fr_command_set_multi_volume (comm, volume_file);

		g_object_unref (volume_file);
		g_free (volume_filename);
	}

	if (comm->n_files != 0)
		parse_progress_line (comm, "Compressing  ", _("Adding file: "), line);
}


static void
fr_command_7z_add (FrCommand     *comm,
		   const char    *from_file,
		   GList         *file_list,
		   const char    *base_dir,
		   gboolean       update,
		   gboolean       recursive)
{
	GList *scan;

	fr_process_use_standard_locale (comm->process, TRUE);
	fr_process_set_out_line_func (comm->process,
				      process_line__add,
				      comm);

	fr_command_7z_begin_command (comm);

	if (update)
		fr_process_add_arg (comm->process, "u");
	else
		fr_process_add_arg (comm->process, "a");

	if (base_dir != NULL) {
		fr_process_set_working_dir (comm->process, base_dir);
		fr_process_add_arg_concat (comm->process, "-w", base_dir, NULL);
	}

	if (is_mime_type (comm->mime_type, "application/zip")
	    || is_mime_type (comm->mime_type, "application/x-cbz"))
	{
		fr_process_add_arg (comm->process, "-tzip");
		fr_process_add_arg (comm->process, "-mem=AES128");
	}

	if (spd_support) fr_process_add_arg (comm->process, "-spd");
	fr_process_add_arg (comm->process, "-bd");
	fr_process_add_arg (comm->process, "-y");
	fr_process_add_arg (comm->process, "-l");
	add_password_arg (comm, comm->password, FALSE);
	if ((comm->password != NULL)
	    && (*comm->password != 0)
	    && comm->encrypt_header
	    && fr_command_is_capable_of (comm, FR_COMMAND_CAN_ENCRYPT_HEADER))
	{
		fr_process_add_arg (comm->process, "-mhe=on");
	}

	/* fr_process_add_arg (comm->process, "-ms=off"); FIXME: solid mode off? */

	switch (comm->compression) {
	case FR_COMPRESSION_VERY_FAST:
		fr_process_add_arg (comm->process, "-mx=1");
		break;
	case FR_COMPRESSION_FAST:
		fr_process_add_arg (comm->process, "-mx=5");
		break;
	case FR_COMPRESSION_NORMAL:
		fr_process_add_arg (comm->process, "-mx=7");
		break;
	case FR_COMPRESSION_MAXIMUM:
		fr_process_add_arg (comm->process, "-mx=9");
		if (! is_mime_type (comm->mime_type, "application/zip")
		    && ! is_mime_type (comm->mime_type, "application/x-cbz"))
		{
			fr_process_add_arg (comm->process, "-m0=lzma2");;
		}
		break;
	}

	if (is_mime_type (comm->mime_type, "application/x-ms-dos-executable"))
		fr_process_add_arg (comm->process, "-sfx");

	if (comm->volume_size > 0)
		fr_process_add_arg_printf (comm->process, "-v%ub", comm->volume_size);

	if (from_file != NULL)
		fr_process_add_arg_concat (comm->process, "-i@", from_file, NULL);

	if (from_file == NULL)
		for (scan = file_list; scan; scan = scan->next)
			/* Files prefixed with '@' need to be handled specially */
			if (g_str_has_prefix (scan->data, "@"))
				fr_process_add_arg_concat (comm->process, "-i!", scan->data, NULL);

	fr_process_add_arg (comm->process, "--");
	fr_process_add_arg (comm->process, comm->filename);

	if (from_file == NULL)
		for (scan = file_list; scan; scan = scan->next)
			/* Skip files prefixed with '@', already added */
			if (!g_str_has_prefix (scan->data, "@"))
				fr_process_add_arg (comm->process, scan->data);

	fr_process_end_command (comm->process);
}


static void
fr_command_7z_delete (FrCommand  *comm,
		      const char *from_file,
		      GList      *file_list)
{
	GList *scan;

	fr_command_7z_begin_command (comm);
	fr_process_add_arg (comm->process, "d");
	if (spd_support) fr_process_add_arg (comm->process, "-spd");
	fr_process_add_arg (comm->process, "-bd");
	fr_process_add_arg (comm->process, "-y");
	if (is_mime_type (comm->mime_type, "application/x-ms-dos-executable"))
		fr_process_add_arg (comm->process, "-sfx");

	if (from_file != NULL)
		fr_process_add_arg_concat (comm->process, "-i@", from_file, NULL);

	if (from_file == NULL)
		for (scan = file_list; scan; scan = scan->next)
			/* Files prefixed with '@' need to be handled specially */
			if (g_str_has_prefix (scan->data, "@"))
				fr_process_add_arg_concat (comm->process, "-i!", scan->data, NULL);

	add_password_arg (comm, FR_COMMAND (comm)->password, FALSE);

	fr_process_add_arg (comm->process, "--");
	fr_process_add_arg (comm->process, comm->filename);

	if (from_file == NULL)
		for (scan = file_list; scan; scan = scan->next)
			/* Skip files prefixed with '@', already added */
			if (!g_str_has_prefix (scan->data, "@"))
				fr_process_add_arg (comm->process, scan->data);

	fr_process_end_command (comm->process);
}


static void
process_line__extract (char     *line,
		       gpointer  data)
{
	FrCommand *comm = FR_COMMAND (data);

	if (comm->n_files != 0)
		parse_progress_line (comm, "Extracting  ", _("Extracting file: "), line);
}


static void
fr_command_7z_extract (FrCommand  *comm,
		       const char *from_file,
		       GList      *file_list,
		       const char *dest_dir,
		       gboolean    overwrite,
		       gboolean    skip_older,
		       gboolean    junk_paths)
{
	GList *scan;

	fr_process_use_standard_locale (comm->process, TRUE);
	fr_process_set_out_line_func (comm->process,
				      process_line__extract,
				      comm);
	fr_command_7z_begin_command (comm);

	if (junk_paths)
		fr_process_add_arg (comm->process, "e");
	else
		fr_process_add_arg (comm->process, "x");

	if (spd_support) fr_process_add_arg (comm->process, "-spd");
	fr_process_add_arg (comm->process, "-bd");
	fr_process_add_arg (comm->process, "-y");
	add_password_arg (comm, comm->password, FALSE);

	if (dest_dir != NULL)
		fr_process_add_arg_concat (comm->process, "-o", dest_dir, NULL);

	if (from_file != NULL)
		fr_process_add_arg_concat (comm->process, "-i@", from_file, NULL);

	if (from_file == NULL)
		for (scan = file_list; scan; scan = scan->next)
			/* Files prefixed with '@' need to be handled specially */
			if (g_str_has_prefix (scan->data, "@"))
				fr_process_add_arg_concat (comm->process, "-i!", scan->data, NULL);

	fr_process_add_arg (comm->process, "--");
	fr_process_add_arg (comm->process, comm->filename);

	if (from_file == NULL)
		for (scan = file_list; scan; scan = scan->next)
			/* Skip files prefixed with '@', already added */
			if (!g_str_has_prefix (scan->data, "@"))
				fr_process_add_arg (comm->process, scan->data);

	if (unexpected_end_of_archive) fr_process_set_ignore_error (comm->process, TRUE);

	fr_process_end_command (comm->process);
}


static void
fr_command_7z_test (FrCommand   *comm)
{
	fr_command_7z_begin_command (comm);
	fr_process_add_arg (comm->process, "t");
	fr_process_add_arg (comm->process, "-bd");
	fr_process_add_arg (comm->process, "-y");
	add_password_arg (comm, comm->password, FALSE);
	fr_process_add_arg (comm->process, "--");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
}


static void
fr_command_7z_handle_error (FrCommand   *comm,
			    FrProcError *error)
{
	if (error->type == FR_PROC_ERROR_NONE) {
		FileData *first;
		char     *basename;
		char     *testname;

		/* This is a way to fix bug #582712. */

		if (comm->files->len != 1)
			return;

		if (! g_str_has_suffix (comm->filename, ".001"))
			return;

		first = g_ptr_array_index (comm->files, 0);
		basename = g_path_get_basename (comm->filename);
		testname = g_strconcat (first->original_path, ".001", NULL);

		if (strcmp (basename, testname) == 0)
			error->type = FR_PROC_ERROR_ASK_PASSWORD;

		g_free (testname);
		g_free (basename);

		return;
	}

	if ((error->status <= 1) || (unexpected_end_of_archive)) {
		error->type = FR_PROC_ERROR_NONE;
	}
	else {
		if (password_required && (!password_handled))
		{
			error->type = FR_PROC_ERROR_ASK_PASSWORD;
			return;
		}

		GList *scan;

		for (scan = g_list_last (comm->process->out.raw); scan; scan = scan->prev) {
			char *line = scan->data;

			if ((strstr (line, "Wrong password?") != NULL)
			    || (strstr (line, "Enter password") != NULL))
			{
				error->type = FR_PROC_ERROR_ASK_PASSWORD;
				break;
			}
		}
	}
}


const char *sevenz_mime_types[] = { "application/x-7z-compressed",
				    "application/x-arj",
				    "application/vnd.ms-cab-compressed",
				    "application/x-cd-image",
				    /*"application/x-cbr",*/
				    "application/x-cbz",
				    "application/x-ms-dos-executable",
				    "application/x-ms-wim",
				    "application/x-rar",
				    "application/zip",
				    NULL };


static const char **
fr_command_7z_get_mime_types (FrCommand *comm)
{
	return sevenz_mime_types;
}


static FrCommandCap
fr_command_7z_get_capabilities (FrCommand  *comm,
				const char *mime_type,
				gboolean    check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES;
	if (! is_program_available ("7za", check_command) && ! is_program_available ("7zr", check_command) && ! is_program_available ("7z", check_command))
		return capabilities;

	if (is_mime_type (mime_type, "application/x-7z-compressed")) {
		capabilities |= FR_COMMAND_CAN_READ_WRITE | FR_COMMAND_CAN_CREATE_VOLUMES;
		if (is_program_available ("7z", check_command))
			capabilities |= FR_COMMAND_CAN_ENCRYPT | FR_COMMAND_CAN_ENCRYPT_HEADER;
	}
	else if (is_mime_type (mime_type, "application/x-7z-compressed-tar")) {
		capabilities |= FR_COMMAND_CAN_READ_WRITE;
		if (is_program_available ("7z", check_command))
			capabilities |= FR_COMMAND_CAN_ENCRYPT | FR_COMMAND_CAN_ENCRYPT_HEADER;
	}
	else if (is_program_available ("7z", check_command)) {
		if (is_mime_type (mime_type, "application/x-rar")
		    || is_mime_type (mime_type, "application/x-cbr"))
		{
			if (! check_command || g_file_test ("/usr/lib/p7zip/Codecs/Rar29.so", G_FILE_TEST_EXISTS) || g_file_test ("/usr/lib/p7zip/Codecs/Rar.so", G_FILE_TEST_EXISTS)
			    || g_file_test ("/usr/libexec/p7zip/Codecs/Rar29.so", G_FILE_TEST_EXISTS) || g_file_test ("/usr/libexec/p7zip/Codecs/Rar.so", G_FILE_TEST_EXISTS))
				capabilities |= FR_COMMAND_CAN_READ;
		}
		else
			capabilities |= FR_COMMAND_CAN_READ;

		if (is_mime_type (mime_type, "application/x-cbz")
		    || is_mime_type (mime_type, "application/x-ms-dos-executable")
		    || is_mime_type (mime_type, "application/zip"))
		{
			capabilities |= FR_COMMAND_CAN_WRITE | FR_COMMAND_CAN_ENCRYPT;
		}
	}
	else if (is_program_available ("7za", check_command)) {
		if (is_mime_type (mime_type, "application/vnd.ms-cab-compressed")
		    || is_mime_type (mime_type, "application/zip"))
		{
			capabilities |= FR_COMMAND_CAN_READ;
		}

		if (is_mime_type (mime_type, "application/zip"))
			capabilities |= FR_COMMAND_CAN_WRITE;
	}

	/* multi-volumes are read-only */
	if ((comm->files->len > 0) && comm->multi_volume && (capabilities & FR_COMMAND_CAN_WRITE))
		capabilities ^= FR_COMMAND_CAN_WRITE;

	return capabilities;
}


static const char *
fr_command_7z_get_packages (FrCommand  *comm,
			    const char *mime_type)
{
	if (is_mime_type (mime_type, "application/x-rar"))
		return PACKAGES ("p7zip,p7zip-rar");
	else if (is_mime_type (mime_type, "application/zip") || is_mime_type (mime_type, "application/vnd.ms-cab-compressed"))
		return PACKAGES ("p7zip,p7zip-full");
	else
		return PACKAGES ("p7zip");
}


static void
fr_command_7z_class_init (FrCommand7zClass *class)
{
	GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
	FrCommandClass *afc;

	parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_7z_finalize;

	afc->list             = fr_command_7z_list;
	afc->add              = fr_command_7z_add;
    afc->delete_           = fr_command_7z_delete;
	afc->extract          = fr_command_7z_extract;
	afc->test             = fr_command_7z_test;
	afc->handle_error     = fr_command_7z_handle_error;
	afc->get_mime_types   = fr_command_7z_get_mime_types;
	afc->get_capabilities = fr_command_7z_get_capabilities;
	afc->get_packages     = fr_command_7z_get_packages;
}


static void
fr_command_7z_init (FrCommand *comm)
{
	comm->propAddCanUpdate             = TRUE;
	comm->propAddCanReplace            = TRUE;
	comm->propAddCanStoreFolders       = TRUE;
	comm->propExtractCanAvoidOverwrite = FALSE;
	comm->propExtractCanSkipOlder      = FALSE;
	comm->propExtractCanJunkPaths      = TRUE;
	comm->propPassword                 = TRUE;
	comm->propTest                     = TRUE;
	comm->propListFromFile             = TRUE;
}


static void
fr_command_7z_finalize (GObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (FR_IS_COMMAND_7Z (object));

	/* Chain up */
	if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_7z_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FrCommand7zClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_7z_class_init,
			NULL,
			NULL,
			sizeof (FrCommand7z),
			0,
			(GInstanceInitFunc) fr_command_7z_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommand7z",
					       &type_info,
					       0);
	}

	return type;
}
