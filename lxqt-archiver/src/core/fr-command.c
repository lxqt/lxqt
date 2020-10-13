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

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <glib.h>
#include "file-data.h"
#include "file-utils.h"
#include "fr-command.h"
#include "fr-enum-types.h"
#include "fr-marshal.h"
#include "fr-proc-error.h"
#include "fr-process.h"
#include "glib-utils.h"

#define INITIAL_SIZE 256


/* Signals */
enum {
	START,
	DONE,
	PROGRESS,
	MESSAGE,
	WORKING_ARCHIVE,
	LAST_SIGNAL
};

/* Properties */
enum {
        PROP_0,
        PROP_FILE,
        PROP_MIME_TYPE,
        PROP_PROCESS,
        PROP_PASSWORD,
        PROP_ENCRYPT_HEADER,
        PROP_COMPRESSION,
        PROP_VOLUME_SIZE
};

static GObjectClass *parent_class = NULL;
static guint fr_command_signals[LAST_SIGNAL] = { 0 };

static void fr_command_class_init  (FrCommandClass *class);
static void fr_command_init        (FrCommand *afile);
static void fr_command_finalize    (GObject *object);

char *action_names[] = { "NONE",
			 "CREATING_NEW_ARCHIVE",
			 "LOADING_ARCHIVE",
			 "LISTING_CONTENT",
			 "DELETING_FILES",
			 "TESTING_ARCHIVE",
			 "GETTING_FILE_LIST",
			 "COPYING_FILES_FROM_REMOTE",
			 "ADDING_FILES",
			 "EXTRACTING_FILES",
			 "COPYING_FILES_TO_REMOTE",
			 "CREATING_ARCHIVE",
			 "SAVING_REMOTE_ARCHIVE" };

GType
fr_command_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FrCommandClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_class_init,
			NULL,
			NULL,
			sizeof (FrCommand),
			0,
			(GInstanceInitFunc) fr_command_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "FRCommand",
					       &type_info,
					       0);
	}

	return type;
}


static void
base_fr_command_list (FrCommand  *comm)
{
}


static void
base_fr_command_add (FrCommand     *comm,
		     const char    *from_file,
		     GList         *file_list,
		     const char    *base_dir,
		     gboolean       update,
		     gboolean       recursive)
{
}


static void
base_fr_command_delete (FrCommand  *comm,
		        const char *from_file,
			GList       *file_list)
{
}


static void
base_fr_command_extract (FrCommand  *comm,
		         const char *from_file,
			 GList      *file_list,
			 const char *dest_dir,
			 gboolean    overwrite,
			 gboolean    skip_older,
			 gboolean    junk_paths)
{
}


static void
base_fr_command_test (FrCommand *comm)
{
}


static void
base_fr_command_uncompress (FrCommand *comm)
{
}


static void
base_fr_command_recompress (FrCommand *comm)
{
}


static void
base_fr_command_handle_error (FrCommand   *comm,
			      FrProcError *error)
{
}


const char **void_mime_types = { NULL };


static const char **
base_fr_command_get_mime_types (FrCommand *comm)
{
	return void_mime_types;
}


static FrCommandCap
base_fr_command_get_capabilities (FrCommand  *comm,
			          const char *mime_type,
			          gboolean    check_command)
{
	return FR_COMMAND_CAN_DO_NOTHING;
}


static void
base_fr_command_set_mime_type (FrCommand  *comm,
			       const char *mime_type)
{
	comm->mime_type = get_static_string (mime_type);
	fr_command_update_capabilities (comm);
}


static const char *
base_fr_command_get_packages (FrCommand  *comm,
			      const char *mime_type)
{
	return NULL;
}


static void
fr_command_start (FrProcess *process,
		  gpointer   data)
{
	FrCommand *comm = FR_COMMAND (data);

	g_signal_emit (G_OBJECT (comm),
		       fr_command_signals[START],
		       0,
		       comm->action);
}


static void
fr_command_done (FrProcess   *process,
		 FrProcError *error,
		 gpointer     data)
{
	FrCommand *comm = FR_COMMAND (data);

	comm->process->restart = FALSE;
	if (error->type != FR_PROC_ERROR_STOPPED)
		fr_command_handle_error (comm, error);

	if (comm->process->restart) {
		fr_process_start (comm->process);
		return;
	}

	if (comm->action == FR_ACTION_LISTING_CONTENT) {
		/* order the list by name to speed up search */
		g_ptr_array_sort (comm->files, file_data_compare_by_path);
	}

	g_signal_emit (G_OBJECT (comm),
		       fr_command_signals[DONE],
		       0,
		       comm->action,
		       error);
}


static void
fr_command_set_process (FrCommand *comm,
		        FrProcess *process)
{
	if (comm->process != NULL) {
		g_signal_handlers_disconnect_matched (G_OBJECT (comm->process),
					      G_SIGNAL_MATCH_DATA,
					      0,
					      0, NULL,
					      0,
					      comm);
		g_object_unref (G_OBJECT (comm->process));
		comm->process = NULL;
	}

	if (process == NULL)
		return;

	g_object_ref (G_OBJECT (process));
	comm->process = process;
	g_signal_connect (G_OBJECT (comm->process),
			  "start",
			  G_CALLBACK (fr_command_start),
			  comm);
	g_signal_connect (G_OBJECT (comm->process),
			  "done",
			  G_CALLBACK (fr_command_done),
			  comm);
}


static void
fr_command_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	FrCommand *comm;

	comm = FR_COMMAND (object);

	switch (prop_id) {
	case PROP_PROCESS:
		fr_command_set_process (comm, g_value_get_object (value));
		break;
	case PROP_FILE:
		fr_command_set_file (comm, g_value_get_object (value));
		break;
	case PROP_MIME_TYPE:
		fr_command_set_mime_type (comm, g_value_get_string (value));
		break;
	case PROP_PASSWORD:
		g_free (comm->password);
		comm->password = g_strdup (g_value_get_string (value));
		break;
	case PROP_ENCRYPT_HEADER:
		comm->encrypt_header = g_value_get_boolean (value);
		break;
	case PROP_COMPRESSION:
		comm->compression = g_value_get_enum (value);
		break;
	case PROP_VOLUME_SIZE:
		comm->volume_size = g_value_get_uint (value);
		break;
	default:
		break;
	}
}


static void
fr_command_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	FrCommand *comm;

	comm = FR_COMMAND (object);

	switch (prop_id) {
	case PROP_PROCESS:
		g_value_set_object (value, comm->process);
		break;
	case PROP_FILE:
		g_value_take_object (value, g_file_new_for_path (comm->filename));
		break;
	case PROP_MIME_TYPE:
		g_value_set_static_string (value, comm->mime_type);
		break;
	case PROP_PASSWORD:
		g_value_set_string (value, comm->password);
		break;
	case PROP_ENCRYPT_HEADER:
		g_value_set_boolean (value, comm->encrypt_header);
		break;
	case PROP_COMPRESSION:
		g_value_set_enum (value, comm->compression);
		break;
	case PROP_VOLUME_SIZE:
		g_value_set_uint (value, comm->volume_size);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
fr_command_class_init (FrCommandClass *class)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (class);

	gobject_class = G_OBJECT_CLASS (class);

	/* virtual functions */

	gobject_class->finalize = fr_command_finalize;
	gobject_class->set_property = fr_command_set_property;
	gobject_class->get_property = fr_command_get_property;

	class->list             = base_fr_command_list;
	class->add              = base_fr_command_add;
	class->delete_           = base_fr_command_delete;
	class->extract          = base_fr_command_extract;
	class->test             = base_fr_command_test;
	class->uncompress       = base_fr_command_uncompress;
	class->recompress       = base_fr_command_recompress;
	class->handle_error     = base_fr_command_handle_error;
	class->get_mime_types   = base_fr_command_get_mime_types;
	class->get_capabilities = base_fr_command_get_capabilities;
	class->set_mime_type    = base_fr_command_set_mime_type;
	class->get_packages     = base_fr_command_get_packages;
	class->start            = NULL;
	class->done             = NULL;
	class->progress         = NULL;
	class->message          = NULL;

	/* signals */

	fr_command_signals[START] =
		g_signal_new ("start",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrCommandClass, start),
			      NULL, NULL,
			      fr_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1, G_TYPE_INT);
	fr_command_signals[DONE] =
		g_signal_new ("done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrCommandClass, done),
			      NULL, NULL,
			      fr_marshal_VOID__INT_BOXED,
			      G_TYPE_NONE, 2,
			      G_TYPE_INT,
			      FR_TYPE_PROC_ERROR);
	fr_command_signals[PROGRESS] =
		g_signal_new ("progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrCommandClass, progress),
			      NULL, NULL,
			      fr_marshal_VOID__DOUBLE,
			      G_TYPE_NONE, 1,
			      G_TYPE_DOUBLE);
	fr_command_signals[MESSAGE] =
		g_signal_new ("message",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrCommandClass, message),
			      NULL, NULL,
			      fr_marshal_VOID__STRING,
			      G_TYPE_NONE, 1,
			      G_TYPE_STRING);
	fr_command_signals[WORKING_ARCHIVE] =
			g_signal_new ("working_archive",
				      G_TYPE_FROM_CLASS (class),
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET (FrCommandClass, working_archive),
				      NULL, NULL,
				      fr_marshal_VOID__STRING,
				      G_TYPE_NONE, 1,
				      G_TYPE_STRING);

	/* properties */

	g_object_class_install_property (gobject_class,
					 PROP_PROCESS,
					 g_param_spec_object ("process",
							      "Process",
							      "The process object used by the command",
							      FR_TYPE_PROCESS,
							      G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_FILE,
					 g_param_spec_object ("file",
							      "File",
							      "The archive local file",
							      G_TYPE_FILE,
							      G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_MIME_TYPE,
					 g_param_spec_string ("mime-type",
							      "Mime type",
							      "The file mime-type",
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_PASSWORD,
					 g_param_spec_string ("password",
							      "Password",
							      "The archive password",
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_ENCRYPT_HEADER,
					 g_param_spec_boolean ("encrypt-header",
							       "Encrypt header",
							       "Whether to encrypt the archive header when creating the archive",
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_COMPRESSION,
					 g_param_spec_enum ("compression",
							    "Compression type",
							    "The compression type to use when creating the archive",
							    FR_TYPE_COMPRESSION,
							    FR_COMPRESSION_NORMAL,
							    G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_VOLUME_SIZE,
					 g_param_spec_uint ("volume-size",
							    "Volume size",
							    "The size of each volume or 0 to not use volumes",
							    0L,
							    G_MAXUINT,
							    0,
							    G_PARAM_READWRITE));
}


static void
fr_command_init (FrCommand *comm)
{
	comm->files = g_ptr_array_new_full (INITIAL_SIZE, (GDestroyNotify) file_data_free);

	comm->password = NULL;
	comm->encrypt_header = FALSE;
	comm->compression = FR_COMPRESSION_NORMAL;
	comm->volume_size = 0;
	comm->filename = NULL;
	comm->e_filename = NULL;
	comm->fake_load = FALSE;

	comm->propAddCanUpdate = FALSE;
	comm->propAddCanReplace = FALSE;
	comm->propAddCanStoreFolders = FALSE;
	comm->propExtractCanAvoidOverwrite = FALSE;
	comm->propExtractCanSkipOlder = FALSE;
	comm->propExtractCanJunkPaths = FALSE;
	comm->propPassword = FALSE;
	comm->propTest = FALSE;
	comm->propCanExtractAll = TRUE;
	comm->propCanDeleteNonEmptyFolders = TRUE;
	comm->propCanExtractNonEmptyFolders = TRUE;
	comm->propListFromFile = FALSE;
}


static void
fr_command_finalize (GObject *object)
{
	FrCommand* comm;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FR_IS_COMMAND (object));

	comm = FR_COMMAND (object);

	g_free (comm->filename);
	g_free (comm->e_filename);
	g_free (comm->password);
	if (comm->files != NULL)
		g_ptr_array_free (comm->files, TRUE);
	fr_command_set_process (comm, NULL);

	/* Chain up */
	if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
fr_command_set_filename (FrCommand  *comm,
			 const char *filename)
{
	g_return_if_fail (FR_IS_COMMAND (comm));

	if (comm->filename != NULL) {
		g_free (comm->filename);
		comm->filename = NULL;
	}

	if (comm->e_filename != NULL) {
		g_free (comm->e_filename);
		comm->e_filename = NULL;
	}

	if (filename != NULL) {
		if (! g_path_is_absolute (filename)) {
			char *current_dir;

			current_dir = g_get_current_dir ();
			comm->filename = g_strconcat (current_dir,
						      "/",
						      filename,
						      NULL);
			g_free (current_dir);
		}
		else
			comm->filename = g_strdup (filename);

		comm->e_filename = g_shell_quote (comm->filename);

		debug (DEBUG_INFO, "filename : %s\n", comm->filename);
		debug (DEBUG_INFO, "e_filename : %s\n", comm->e_filename);
	}

	fr_command_working_archive (comm, comm->filename);
}


void
fr_command_set_file (FrCommand *comm,
		     GFile     *file)
{
	char *filename;

	filename = g_file_get_path (file);
	fr_command_set_filename (comm, filename);

	g_free (filename);
}


void
fr_command_set_multi_volume (FrCommand *comm,
			     GFile     *file)
{
	comm->multi_volume = TRUE;
	fr_command_set_file (comm, file);
}


void
fr_command_list (FrCommand *comm)
{
	g_return_if_fail (FR_IS_COMMAND (comm));

	fr_command_progress (comm, -1.0);

	if (comm->files != NULL) {
		g_ptr_array_free (comm->files, TRUE);
		comm->files = g_ptr_array_new_full (INITIAL_SIZE, (GDestroyNotify) file_data_free);
	}

	comm->action = FR_ACTION_LISTING_CONTENT;
	fr_process_set_out_line_func (comm->process, NULL, NULL);
	fr_process_set_err_line_func (comm->process, NULL, NULL);
	fr_process_use_standard_locale (comm->process, TRUE);
	comm->multi_volume = FALSE;

	if (! comm->fake_load)
		FR_COMMAND_GET_CLASS (G_OBJECT (comm))->list (comm);
	else
		g_signal_emit (G_OBJECT (comm),
			       fr_command_signals[DONE],
			       0,
			       comm->action,
			       &comm->process->error);
}


void
fr_command_add (FrCommand     *comm,
		const char    *from_file,
		GList         *file_list,
		const char    *base_dir,
		gboolean       update,
		gboolean       recursive)
{
	fr_command_progress (comm, -1.0);

	comm->action = FR_ACTION_ADDING_FILES;
	fr_process_set_out_line_func (FR_COMMAND (comm)->process, NULL, NULL);
	fr_process_set_err_line_func (FR_COMMAND (comm)->process, NULL, NULL);

	FR_COMMAND_GET_CLASS (G_OBJECT (comm))->add (comm,
						     from_file,
						     file_list,
						     base_dir,
						     update,
						     recursive);
}


void
fr_command_delete (FrCommand   *comm,
		   const char  *from_file,
		   GList       *file_list)
{
	fr_command_progress (comm, -1.0);

	comm->action = FR_ACTION_DELETING_FILES;
	fr_process_set_out_line_func (FR_COMMAND (comm)->process, NULL, NULL);
	fr_process_set_err_line_func (FR_COMMAND (comm)->process, NULL, NULL);

	FR_COMMAND_GET_CLASS (G_OBJECT (comm))->delete_ (comm, from_file, file_list);
}


void
fr_command_extract (FrCommand  *comm,
		    const char *from_file,
		    GList      *file_list,
		    const char *dest_dir,
		    gboolean    overwrite,
		    gboolean    skip_older,
		    gboolean    junk_paths)
{
	fr_command_progress (comm, -1.0);

	comm->action = FR_ACTION_EXTRACTING_FILES;
	fr_process_set_out_line_func (FR_COMMAND (comm)->process, NULL, NULL);
	fr_process_set_err_line_func (FR_COMMAND (comm)->process, NULL, NULL);

	FR_COMMAND_GET_CLASS (G_OBJECT (comm))->extract (comm,
							 from_file,
							 file_list,
							 dest_dir,
							 overwrite,
							 skip_older,
							 junk_paths);
}


void
fr_command_test (FrCommand *comm)
{
	fr_command_progress (comm, -1.0);

	comm->action = FR_ACTION_TESTING_ARCHIVE;
	fr_process_set_out_line_func (FR_COMMAND (comm)->process, NULL, NULL);
	fr_process_set_err_line_func (FR_COMMAND (comm)->process, NULL, NULL);

	FR_COMMAND_GET_CLASS (G_OBJECT (comm))->test (comm);
}


void
fr_command_uncompress (FrCommand *comm)
{
	fr_command_progress (comm, -1.0);
	FR_COMMAND_GET_CLASS (G_OBJECT (comm))->uncompress (comm);
}


void
fr_command_recompress (FrCommand *comm)
{
	fr_command_progress (comm, -1.0);
	FR_COMMAND_GET_CLASS (G_OBJECT (comm))->recompress (comm);
}


const char **
fr_command_get_mime_types (FrCommand *comm)
{
	return FR_COMMAND_GET_CLASS (G_OBJECT (comm))->get_mime_types (comm);
}


void
fr_command_update_capabilities (FrCommand *comm)
{
	comm->capabilities = fr_command_get_capabilities (comm, comm->mime_type, TRUE);
}


FrCommandCap
fr_command_get_capabilities (FrCommand  *comm,
			     const char *mime_type,
			     gboolean    check_command)
{
	return FR_COMMAND_GET_CLASS (G_OBJECT (comm))->get_capabilities (comm, mime_type, check_command);
}


gboolean
fr_command_is_capable_of (FrCommand     *comm,
			  FrCommandCaps  requested_capabilities)
{
	return (((comm->capabilities ^ requested_capabilities) & requested_capabilities) == 0);
}


const char *
fr_command_get_packages (FrCommand  *comm,
			 const char *mime_type)
{
	return FR_COMMAND_GET_CLASS (G_OBJECT (comm))->get_packages (comm, mime_type);
}


/* fraction == -1 means : I don't known how much time the current operation
 *                        will take, the dialog will display this info pulsing
 *                        the progress bar.
 * fraction in [0.0, 1.0] means the amount of work, in percentage,
 *                        accomplished.
 */
void
fr_command_progress (FrCommand *comm,
		     double     fraction)
{
	g_signal_emit (G_OBJECT (comm),
		       fr_command_signals[PROGRESS],
		       0,
		       fraction);
}


void
fr_command_message (FrCommand  *comm,
		    const char *msg)
{
	g_signal_emit (G_OBJECT (comm),
		       fr_command_signals[MESSAGE],
		       0,
		       msg);
}


void
fr_command_working_archive (FrCommand  *comm,
		            const char *archive_name)
{
	g_signal_emit (G_OBJECT (comm),
		       fr_command_signals[WORKING_ARCHIVE],
		       0,
		       archive_name);
}


void
fr_command_set_n_files (FrCommand *comm,
			int        n_files)
{
	comm->n_files = n_files;
	comm->n_file = 0;
}


void
fr_command_add_file (FrCommand *comm,
		     FileData  *fdata)
{
	file_data_update_content_type (fdata);
	g_ptr_array_add (comm->files, fdata);
	if (! fdata->dir)
		comm->n_regular_files++;
}


void
fr_command_set_mime_type (FrCommand  *comm,
			  const char *mime_type)
{
	FR_COMMAND_GET_CLASS (G_OBJECT (comm))->set_mime_type (comm, mime_type);
}


void
fr_command_handle_error (FrCommand   *comm,
			 FrProcError *error)
{
	FR_COMMAND_GET_CLASS (G_OBJECT (comm))->handle_error (comm, error);
}
