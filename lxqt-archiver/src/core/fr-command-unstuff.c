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
#include "fr-command-unstuff.h"

static void fr_command_unstuff_class_init  (FrCommandUnstuffClass *class);
static void fr_command_unstuff_init        (FrCommand         *afile);
static void fr_command_unstuff_finalize    (GObject           *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;

/* recursive rmdir to remove the left-overs from unstuff */
static void
recursive_rmdir (const char *path)
{
	GDir *dir;
	const char *dirname;

	dir = g_dir_open (path, 0, NULL);
	if (dir == NULL)
		return;

	dirname = g_dir_read_name (dir);
	while (dirname != NULL)
	{
		char *full_path;

		if (strcmp (dirname, ".") == 0 || strcmp (dirname, "..") == 0)
			continue;

		full_path = g_build_filename (path, dirname, NULL);
		recursive_rmdir (full_path);
		g_free (full_path);

		dirname = g_dir_read_name (dir);
	}

	rmdir (path);

	g_dir_close (dir);
}


/* unstuff doesn't like file paths starting with /, that's so shite */
static char *
unstuff_is_shit_with_filenames (const char *orig)
{
	int i, num_slashes;
	char *current_dir, *filename;

	g_return_val_if_fail (orig != NULL, NULL);

	current_dir = g_get_current_dir ();
	i = num_slashes = 0;
	while (current_dir[i] != '\0') {
		if (current_dir[i]  == '/')
			num_slashes++;
		i++;
	}
	g_free (current_dir);

	/* 3 characters for each ../ plus filename length plus \0 */
	filename = g_malloc (3 * i + strlen (orig) + 1);
	i = 0;
	for ( ; num_slashes > 0 ; num_slashes--) {
		memcpy (filename + i, "../", 3);
		i+=3;
	}
	memcpy (filename + i, orig, strlen (orig) + 1);

	return filename;
}


static void
process_line (char     *line,
	      gpointer  data)
{
	FrCommand        *comm = FR_COMMAND (data);
	FrCommandUnstuff *unstuff_comm = FR_COMMAND_UNSTUFF (comm);
	const char       *str_start;
	char             *filename, *real_filename;
	int               i;
	FileData         *fdata;

	g_return_if_fail (line != NULL);

	if (strstr (line, "progressEvent - ")) {
		const char *ssize;
		guint size;

		ssize = strstr (line, "progressEvent - ")
			+ strlen ("progressEvent - ");
		if (ssize[0] == '\0')
			size = 0;
		else
			size = g_ascii_strtoull (ssize, NULL, 10);

		if (unstuff_comm->fdata != NULL)
			unstuff_comm->fdata->size = size;

		return;
	}

	if (strstr (line, "fileEvent") == NULL)
		return;
	if (strstr (line, unstuff_comm->target_dir + 1) == NULL)
		return;

	/* Look for the filename, ends with a comma */
	str_start = strstr (line, unstuff_comm->target_dir + 1);
	str_start = str_start + strlen (unstuff_comm->target_dir) - 1;
	if (str_start[0] != '/')
		str_start--;
	if (str_start[0] == '.')
		str_start--;
	i = 0;
	while (str_start[i] != '\0' && str_start[i] != ',') {
		i++;
	}
	/* This is not supposed to happen */
	g_return_if_fail (str_start[i] != '\0');
	filename = g_strndup (str_start, i);

	/* Same thing with the real filename */
	str_start = strstr (line, unstuff_comm->target_dir);
	i = 0;
	while (str_start[i] != '\0' && str_start[i] != ',') {
		i++;
	}
	real_filename = g_strndup (str_start, i);

	fdata = file_data_new ();
	fdata->full_path = filename;
	fdata->original_path = filename;
	fdata->link = NULL;
	fdata->name = g_strdup (file_name_from_path (fdata->full_path));
	fdata->path = remove_level_from_path (fdata->full_path);

	fdata->size = 0;
	fdata->modified = time (NULL);

	unstuff_comm->fdata = fdata;
	fr_command_add_file (comm, fdata);

	unlink (real_filename);
	g_free (real_filename);
}


static void
list__begin (gpointer data)
{
	FrCommandUnstuff *comm = data;

	comm->fdata = NULL;
}


static void
fr_command_unstuff_list (FrCommand *comm)
{
	char *arg, *path;
	char *filename;
	char *path_dots;

	fr_process_set_out_line_func (comm->process, process_line, comm);

	fr_process_begin_command (comm->process, "unstuff");
	fr_process_set_begin_func (comm->process, list__begin, comm);
	fr_process_add_arg (comm->process, "--trace");

	/* Actually unpack everything in a temporary directory */
	path = get_temp_work_dir (NULL);
	path_dots = unstuff_is_shit_with_filenames (path);
	g_free (path);

	arg = g_strdup_printf ("-d=%s", path_dots);
	FR_COMMAND_UNSTUFF (comm)->target_dir = path_dots;
	fr_process_add_arg (comm->process, arg);
	g_free (arg);

	filename = unstuff_is_shit_with_filenames (comm->filename);
	fr_process_add_arg (comm->process, filename);
	g_free (filename);
	fr_process_end_command (comm->process);
	fr_process_start (comm->process);
}


static void
fr_command_unstuff_extract (FrCommand  *comm,
			    const char  *from_file,
			    GList      *file_list,
			    const char *dest_dir,
			    gboolean    overwrite,
			    gboolean    skip_older,
			    gboolean    junk_paths)
{
#if 0
	GList *scan;
#endif
	char  *filename;

	fr_process_begin_command (comm->process, "unstuff");

	if (dest_dir != NULL) {
		char *dest_dir_dots;
		char *arg;

		dest_dir_dots = unstuff_is_shit_with_filenames (dest_dir);
		arg = g_strdup_printf ("-d=%s", dest_dir_dots);
		fr_process_add_arg (comm->process, arg);
		FR_COMMAND_UNSTUFF (comm)->target_dir = NULL;
		g_free (arg);
		g_free (dest_dir_dots);
	}

	fr_process_add_arg (comm->process, "--trace");

	/* unstuff doesn't like file paths starting with /, that's so shite */
	filename = unstuff_is_shit_with_filenames (comm->filename);
	fr_process_add_arg (comm->process, filename);
	g_free (filename);

	/* FIXME it is not possible to unpack only some files */
#if 0
	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, scan->data);
#endif

	fr_process_end_command (comm->process);
}


static void
fr_command_unstuff_handle_error (FrCommand   *comm,
				 FrProcError *error)
{
	if ((error->type != FR_PROC_ERROR_NONE)
	    && (error->status <= 1))
	{
		error->type = FR_PROC_ERROR_NONE;
	}
}


const char *unstuff_mime_type[] = { "application/x-stuffit", NULL };


static const char **
fr_command_unstuff_get_mime_types (FrCommand *comm)
{
	return unstuff_mime_type;
}


static FrCommandCap
fr_command_unstuff_get_capabilities (FrCommand  *comm,
			             const char *mime_type,
				     gboolean    check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES;
	if (is_program_available ("unstuff", check_command))
		capabilities |= FR_COMMAND_CAN_READ;

	return capabilities;
}


static const char *
fr_command_unstaff_get_packages (FrCommand  *comm,
			         const char *mime_type)
{
	return PACKAGES ("unstaff");
}


static void
fr_command_unstuff_class_init (FrCommandUnstuffClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);
	FrCommandClass *afc;

	parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_unstuff_finalize;

	afc->list             = fr_command_unstuff_list;
	afc->add              = NULL;
	afc->delete_           = NULL;
	afc->extract          = fr_command_unstuff_extract;
	afc->handle_error     = fr_command_unstuff_handle_error;
	afc->get_mime_types   = fr_command_unstuff_get_mime_types;
	afc->get_capabilities = fr_command_unstuff_get_capabilities;
	afc->get_packages     = fr_command_unstaff_get_packages;
}


static void
fr_command_unstuff_init (FrCommand *comm)
{
	comm->propAddCanUpdate             = FALSE;
	comm->propAddCanReplace            = FALSE;
	comm->propExtractCanAvoidOverwrite = FALSE;
	comm->propExtractCanSkipOlder      = FALSE;
	comm->propExtractCanJunkPaths      = FALSE;
	comm->propPassword                 = TRUE;
	comm->propTest                     = FALSE;
}


static void
fr_command_unstuff_finalize (GObject *object)
{
	FrCommandUnstuff *unstuff_comm = FR_COMMAND_UNSTUFF (object);
	g_return_if_fail (object != NULL);
	g_return_if_fail (FR_IS_COMMAND_UNSTUFF (object));

	if (unstuff_comm->target_dir != NULL) {
		recursive_rmdir (unstuff_comm->target_dir);
		g_free (unstuff_comm->target_dir);
	}

	/* Chain up */
	if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_unstuff_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FrCommandUnstuffClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_unstuff_class_init,
			NULL,
			NULL,
			sizeof (FrCommandUnstuff),
			0,
			(GInstanceInitFunc) fr_command_unstuff_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandUnstuff",
					       &type_info,
					       0);
	}

	return type;
}
