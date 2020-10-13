/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2003, 2007, 2008 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>

#include <glib.h>
#include "tr-wrapper.h"
#include <gio/gio.h>
#include "glib-utils.h"
#include "file-utils.h"
#include "gio-utils.h"
#include "file-data.h"
#include "fr-archive.h"
#include "fr-command.h"
#include "fr-error.h"
#include "fr-marshal.h"
#include "fr-proc-error.h"
#include "fr-process.h"
#include "fr-init.h"

#if ENABLE_MAGIC
#include <magic.h>
#endif

#ifndef NCARGS
#define NCARGS _POSIX_ARG_MAX
#endif


/* -- DroppedItemsData -- */


typedef struct {
	FrArchive     *archive;
	GList         *item_list;
	char          *base_dir;
	char          *dest_dir;
	gboolean       update;
	char          *password;
	gboolean       encrypt_header;
	FrCompression  compression;
	guint          volume_size;
} DroppedItemsData;


static DroppedItemsData *
dropped_items_data_new (FrArchive     *archive,
			GList         *item_list,
			const char    *base_dir,
			const char    *dest_dir,
			gboolean       update,
			const char    *password,
			gboolean       encrypt_header,
			FrCompression  compression,
			guint          volume_size)
{
	DroppedItemsData *data;

	data = g_new0 (DroppedItemsData, 1);
	data->archive = archive;
	data->item_list = path_list_dup (item_list);
	if (base_dir != NULL)
		data->base_dir = g_strdup (base_dir);
	if (dest_dir != NULL)
		data->dest_dir = g_strdup (dest_dir);
	data->update = update;
	if (password != NULL)
		data->password = g_strdup (password);
	data->encrypt_header = encrypt_header;
	data->compression = compression;
	data->volume_size = volume_size;

	return data;
}


static void
dropped_items_data_free (DroppedItemsData *data)
{
	if (data == NULL)
		return;
	path_list_free (data->item_list);
	g_free (data->base_dir);
	g_free (data->dest_dir);
	g_free (data->password);
	g_free (data);
}


struct _FrArchivePrivData {
	FakeLoadFunc         fake_load_func;                /* If returns TRUE, archives are not read when
							     * fr_archive_load is invoked, used
							     * in batch mode. */
	gpointer             fake_load_data;
	GCancellable        *cancellable;
	char                *temp_dir;
	gboolean             continue_adding_dropped_items;
	DroppedItemsData    *dropped_items_data;

	char                *temp_extraction_dir;
	char                *extraction_destination;
	gboolean             remote_extraction;
	gboolean             extract_here;
};


typedef struct {
	FrArchive      *archive;
	char           *uri;
	FrAction        action;
	GList          *file_list;
	char           *base_uri;
	char           *dest_dir;
	gboolean        update;
	char           *tmp_dir;
	guint           source_id;
	char           *password;
	gboolean        encrypt_header;
	FrCompression   compression;
	guint           volume_size;
} XferData;


static void
xfer_data_free (XferData *data)
{
	if (data == NULL)
		return;

	g_free (data->uri);
	g_free (data->password);
	path_list_free (data->file_list);
	g_free (data->base_uri);
	g_free (data->dest_dir);
	g_free (data->tmp_dir);
	g_free (data);
}


#define MAX_CHUNK_LEN (NCARGS * 2 / 3) /* Max command line length */
#define UNKNOWN_TYPE "application/octet-stream"
#define SAME_FS (FALSE)
#define NO_BACKUP_FILES (TRUE)
#define NO_DOT_FILES (FALSE)
#define IGNORE_CASE (FALSE)
#define LIST_LENGTH_TO_USE_FILE 10 /* FIXME: find a good value */


enum {
	START,
	DONE,
	PROGRESS,
	MESSAGE,
	STOPPABLE,
	WORKING_ARCHIVE,
	LAST_SIGNAL
};

static GObjectClass *parent_class;
static guint fr_archive_signals[LAST_SIGNAL] = { 0 };

static void fr_archive_class_init (FrArchiveClass *class);
static void fr_archive_init       (FrArchive *archive);
static void fr_archive_finalize   (GObject *object);


GType
fr_archive_get_type (void)
{
	static GType type = 0;

	if (! type) {
		static const GTypeInfo type_info = {
			sizeof (FrArchiveClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_archive_class_init,
			NULL,
			NULL,
			sizeof (FrArchive),
			0,
			(GInstanceInitFunc) fr_archive_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "FrArchive",
					       &type_info,
					       0);
	}

	return type;
}


static void
fr_archive_class_init (FrArchiveClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	gobject_class->finalize = fr_archive_finalize;

	class->start = NULL;
	class->done = NULL;
	class->progress = NULL;
	class->message = NULL;
	class->working_archive = NULL;

	/* signals */

	fr_archive_signals[START] =
		g_signal_new ("start",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrArchiveClass, start),
			      NULL, NULL,
			      fr_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1, G_TYPE_INT);
	fr_archive_signals[DONE] =
		g_signal_new ("done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrArchiveClass, done),
			      NULL, NULL,
			      fr_marshal_VOID__INT_BOXED,
			      G_TYPE_NONE, 2,
			      G_TYPE_INT,
			      FR_TYPE_PROC_ERROR);
	fr_archive_signals[PROGRESS] =
		g_signal_new ("progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrArchiveClass, progress),
			      NULL, NULL,
			      fr_marshal_VOID__DOUBLE,
			      G_TYPE_NONE, 1,
			      G_TYPE_DOUBLE);
	fr_archive_signals[MESSAGE] =
		g_signal_new ("message",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrArchiveClass, message),
			      NULL, NULL,
			      fr_marshal_VOID__STRING,
			      G_TYPE_NONE, 1,
			      G_TYPE_STRING);
	fr_archive_signals[STOPPABLE] =
		g_signal_new ("stoppable",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrArchiveClass, stoppable),
			      NULL, NULL,
			      fr_marshal_VOID__BOOL,
			      G_TYPE_NONE,
			      1, G_TYPE_BOOLEAN);
	fr_archive_signals[WORKING_ARCHIVE] =
		g_signal_new ("working_archive",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrArchiveClass, working_archive),
			      NULL, NULL,
			      fr_marshal_VOID__STRING,
			      G_TYPE_NONE, 1,
			      G_TYPE_STRING);
}


void
fr_archive_stoppable (FrArchive *archive,
		      gboolean   stoppable)
{
	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[STOPPABLE],
		       0,
		       stoppable);
}


void
fr_archive_stop (FrArchive *archive)
{
	if (archive->process != NULL) {
		fr_process_stop (archive->process);
		return;
	}

	if (! g_cancellable_is_cancelled (archive->priv->cancellable))
		g_cancellable_cancel (archive->priv->cancellable);
}


void
fr_archive_action_completed (FrArchive       *archive,
			     FrAction         action,
			     FrProcErrorType  error_type,
			     const char      *error_details)
{
	archive->error.type = error_type;
	archive->error.status = 0;
	g_clear_error (&archive->error.gerror);
	if (error_details != NULL)
		archive->error.gerror = g_error_new_literal (fr_error_quark (),
							     0,
							     error_details);
	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[DONE],
		       0,
		       action,
		       &archive->error);
}


static gboolean
archive_sticky_only_cb (FrProcess *process,
			FrArchive *archive)
{
	fr_archive_stoppable (archive, FALSE);
	return TRUE;
}


static void
fr_archive_init (FrArchive *archive)
{
	archive->file = NULL;
	archive->local_copy = NULL;
	archive->is_remote = FALSE;
	archive->command = NULL;
	archive->is_compressed_file = FALSE;
	archive->can_create_compressed_file = FALSE;

	archive->priv = g_new0 (FrArchivePrivData, 1);
	archive->priv->fake_load_func = NULL;
	archive->priv->fake_load_data = NULL;

	archive->priv->extraction_destination = NULL;
	archive->priv->temp_extraction_dir = NULL;
	archive->priv->cancellable = g_cancellable_new ();

	archive->process = fr_process_new ();
	g_signal_connect (G_OBJECT (archive->process),
			  "sticky_only",
			  G_CALLBACK (archive_sticky_only_cb),
			  archive);
}


FrArchive *
fr_archive_new (void)
{
	return FR_ARCHIVE (g_object_new (FR_TYPE_ARCHIVE, NULL));
}


static GFile *
get_local_copy_for_file (GFile *remote_file)
{
	char  *temp_dir;
	GFile *local_copy = NULL;

	temp_dir = get_temp_work_dir (NULL);
	if (temp_dir != NULL) {
		char  *archive_name;
		char  *local_path;

		archive_name = g_file_get_basename (remote_file);
		local_path = g_build_filename (temp_dir, archive_name, NULL);
		local_copy = g_file_new_for_path (local_path);

		g_free (local_path);
		g_free (archive_name);
	}
	g_free (temp_dir);

	return local_copy;
}


static void
fr_archive_set_uri (FrArchive  *archive,
		    const char *uri)
{
	if ((archive->local_copy != NULL) && archive->is_remote) {
		GFile  *temp_folder;
		GError *err = NULL;

		g_file_delete (archive->local_copy, NULL, &err);
		if (err != NULL) {
			g_warning ("Failed to delete the local copy: %s", err->message);
			g_clear_error (&err);
		}

		temp_folder = g_file_get_parent (archive->local_copy);
		g_file_delete (temp_folder, NULL, &err);
		if (err != NULL) {
			g_warning ("Failed to delete temp folder: %s", err->message);
			g_clear_error (&err);
		}

		g_object_unref (temp_folder);
	}

	if (archive->file != NULL) {
		g_object_unref (archive->file);
		archive->file = NULL;
	}
	if (archive->local_copy != NULL) {
		g_object_unref (archive->local_copy);
		archive->local_copy = NULL;
	}
	archive->content_type = NULL;

	if (uri == NULL)
		return;

	archive->file = g_file_new_for_uri (uri);
	archive->is_remote = ! g_file_has_uri_scheme (archive->file, "file");
	if (archive->is_remote)
		archive->local_copy = get_local_copy_for_file (archive->file);
	else
		archive->local_copy = g_file_dup (archive->file);
}


static void
fr_archive_remove_temp_work_dir (FrArchive *archive)
{
	if (archive->priv->temp_dir == NULL)
		return;
	remove_local_directory (archive->priv->temp_dir);
	g_free (archive->priv->temp_dir);
	archive->priv->temp_dir = NULL;
}


static void
fr_archive_finalize (GObject *object)
{
	FrArchive *archive;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FR_IS_ARCHIVE (object));

	archive = FR_ARCHIVE (object);

	fr_archive_set_uri (archive, NULL);
	fr_archive_remove_temp_work_dir (archive);
	if (archive->command != NULL)
		g_object_unref (archive->command);
	g_object_unref (archive->process);
	if (archive->priv->dropped_items_data != NULL) {
		dropped_items_data_free (archive->priv->dropped_items_data);
		archive->priv->dropped_items_data = NULL;
	}
	g_free (archive->priv->temp_extraction_dir);
	g_free (archive->priv->extraction_destination);
	g_free (archive->priv);

	/* Chain up */

	if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


static const char *
get_mime_type_from_content (GFile *file)
{
	GFileInfo  *info;
	GError     *err = NULL;
 	const char *content_type = NULL;

	info = g_file_query_info (file,
				  G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				  0, NULL, &err);
	if (info == NULL) {
		g_warning ("could not get content type: %s", err->message);
		g_clear_error (&err);
	}
	else {
		content_type = get_static_string (g_file_info_get_content_type (info));
		g_object_unref (info);
	}

	return content_type;
}


static const char *
get_mime_type_from_magic_numbers (GFile *file)
{
#if ENABLE_MAGIC
	static magic_t magic = NULL;

	if (! magic) {
		magic = magic_open (MAGIC_MIME_TYPE);
		if (magic)
			magic_load (magic, NULL);
		else
			g_warning ("unable to open magic database");
	}

	if (magic) {
		const char * mime_type = NULL;
		char * filepath = g_file_get_path (file);

		if (filepath) {
			mime_type = magic_file (magic, filepath);
			g_free (filepath);
		}

		if (mime_type)
			return mime_type;

		g_warning ("unable to detect filetype from magic: %s",
			   magic_error (magic));
	}
#else
	static const struct magic {
		const unsigned int off;
		const unsigned int len;
		const char * const id;
		const char * const mime_type;
	} magic_ids [] = {
		/* magic ids taken from magic/Magdir/archive from the file-4.21 tarball */
		{ 0,  6, "7z\274\257\047\034",                   "application/x-7z-compressed" },
		{ 7,  7, "**ACE**",                              "application/x-ace"           },
		{ 0,  2, "\x60\xea",                             "application/x-arj"           },
		{ 0,  3, "BZh",                                  "application/x-bzip2"         },
		{ 0,  2, "\037\213",                             "application/gzip"            },
		{ 0,  4, "LZIP",                                 "application/x-lzip"          },
		{ 0,  9, "\x89\x4c\x5a\x4f\x00\x0d\x0a\x1a\x0a", "application/x-lzop",         },
		{ 0,  4, "Rar!",                                 "application/x-rar"           },
		{ 0,  4, "RZIP",                                 "application/x-rzip"          },
		{ 0,  6, "\3757zXZ\000",                         "application/x-xz"            },
		{ 0,  4, "\x28\xB5\x2F\xFD",                     "application/zstd"            },
		{ 20, 4, "\xdc\xa7\xc4\xfd",                     "application/x-zoo",          },
		{ 0,  4, "PK\003\004",                           "application/zip"             },
		{ 0,  8, "PK00PK\003\004",                       "application/zip"             },
		{ 0,  4, "LRZI",                                 "application/x-lrzip"         },
	};

	char   buffer[32];
	size_t i;

	if (! g_load_file_in_buffer (file, buffer, sizeof (buffer), NULL))
		return NULL;

	for (i = 0; i < G_N_ELEMENTS (magic_ids); i++) {
		const struct magic * const magic = &magic_ids[i];

		if (sizeof (buffer) < (magic->off + magic->len))
			g_warning ("buffer underrun for mime type '%s' magic",
				   magic->mime_type);
		else if (! memcmp (buffer + magic->off, magic->id, magic->len))
			return magic->mime_type;
	}
#endif

	return NULL;
}


static const char *
get_mime_type_from_filename (GFile *file)
{
	const char *mime_type = NULL;
	char       *filename;

	if (file == NULL)
		return NULL;

	filename = g_file_get_path (file);
	mime_type = get_mime_type_from_extension (get_file_extension (filename));
	g_free (filename);

	return mime_type;
}


static gboolean
create_command_from_type (FrArchive     *archive,
			  const char    *mime_type,
		          GType          command_type,
		          FrCommandCaps  requested_capabilities)
{
	if (command_type == 0)
		return FALSE;

	archive->command = FR_COMMAND (g_object_new (command_type,
					             "process", archive->process,
					             "mime-type", mime_type,
					             NULL));

	if (! fr_command_is_capable_of (archive->command, requested_capabilities)) {
		g_object_unref (archive->command);
		archive->command = NULL;
		archive->is_compressed_file = FALSE;
	}
	else
		archive->is_compressed_file = ! fr_command_is_capable_of (archive->command, FR_COMMAND_CAN_ARCHIVE_MANY_FILES);

	return (archive->command != NULL);
}


static gboolean
create_command_to_load_archive (FrArchive  *archive,
			        const char *mime_type)
{
	FrCommandCaps requested_capabilities = FR_COMMAND_CAN_DO_NOTHING;
	GType         command_type;

	if (mime_type == NULL)
		return FALSE;

	/* try with the WRITE capability even when loading, this way we give
	 * priority to the commands that can read and write over commands
	 * that can only read a specific file format. */

	requested_capabilities |= FR_COMMAND_CAN_READ_WRITE;
	command_type = get_command_type_from_mime_type (mime_type, requested_capabilities);

	/* if no command was found, remove the write capability and try again */

	if (command_type == 0) {
		requested_capabilities ^= FR_COMMAND_CAN_WRITE;
		command_type = get_command_type_from_mime_type (mime_type, requested_capabilities);
	}

	return create_command_from_type (archive,
					 mime_type,
					 command_type,
					 requested_capabilities);
}


static gboolean
create_command_to_create_archive (FrArchive  *archive,
			          const char *mime_type)
{
	FrCommandCaps requested_capabilities = FR_COMMAND_CAN_DO_NOTHING;
	GType         command_type;

	if (mime_type == NULL)
		return FALSE;

	requested_capabilities |= FR_COMMAND_CAN_WRITE;
	command_type = get_command_type_from_mime_type (mime_type, requested_capabilities);

    return create_command_from_type (archive,
					 mime_type,
					 command_type,
					 requested_capabilities);
}


static void
action_started (FrCommand *command,
		FrAction   action,
		FrArchive *archive)
{
#ifdef DEBUG
	debug (DEBUG_INFO, "%s [START] (FR::Archive)\n", action_names[action]);
#endif

	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[START],
		       0,
		       action);
}


/* -- copy_to_remote_location -- */


static void
fr_archive_copy_done (FrArchive *archive,
		      FrAction   action,
		      GError    *error)
{
	FrProcErrorType  error_type = FR_PROC_ERROR_NONE;
	const char      *error_details = NULL;

	if (error != NULL) {
		error_type = (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) ? FR_PROC_ERROR_STOPPED : FR_PROC_ERROR_GENERIC);
		error_details = error->message;
	}
	fr_archive_action_completed (archive, action, error_type, error_details);
}


static void
copy_to_remote_location_done (GError   *error,
			      gpointer  user_data)
{
	XferData *xfer_data = user_data;

	fr_archive_copy_done (xfer_data->archive, xfer_data->action, error);
	xfer_data_free (xfer_data);
}


static void
copy_to_remote_location_progress (goffset   current_file,
                                  goffset   total_files,
                                  GFile    *source,
                                  GFile    *destination,
                                  goffset   current_num_bytes,
                                  goffset   total_num_bytes,
                                  gpointer  user_data)
{
	XferData *xfer_data = user_data;

	g_signal_emit (G_OBJECT (xfer_data->archive),
		       fr_archive_signals[PROGRESS],
		       0,
		       (double) current_num_bytes / total_num_bytes);
}


static void
copy_to_remote_location (FrArchive  *archive,
			 FrAction    action)
{
	XferData *xfer_data;

	xfer_data = g_new0 (XferData, 1);
	xfer_data->archive = archive;
	xfer_data->action = action;

	g_copy_file_async (archive->local_copy,
			   archive->file,
			   G_FILE_COPY_OVERWRITE,
			   G_PRIORITY_DEFAULT,
			   archive->priv->cancellable,
			   copy_to_remote_location_progress,
			   xfer_data,
			   copy_to_remote_location_done,
			   xfer_data);
}


/* -- copy_extracted_files_to_destination -- */


static void
move_here (FrArchive *archive)
{
	char   *content_uri;
	char   *parent;
	char   *parent_parent;
	char   *new_content_uri;
	GFile  *source, *destination, *parent_file;
	GError *error = NULL;

	content_uri = get_dir_content_if_unique (archive->priv->extraction_destination);
	if (content_uri == NULL)
		return;

	parent = remove_level_from_path (content_uri);

	if (uricmp (parent, archive->priv->extraction_destination) == 0) {
		char *new_uri;

		new_uri = get_alternative_uri_for_uri (archive->priv->extraction_destination);

		source = g_file_new_for_uri (archive->priv->extraction_destination);
		destination = g_file_new_for_uri (new_uri);
		if (! g_file_move (source, destination, 0, NULL, NULL, NULL, &error)) {
			g_warning ("could not rename %s to %s: %s", archive->priv->extraction_destination, new_uri, error->message);
			g_clear_error (&error);
		}
		g_object_unref (source);
		g_object_unref (destination);

		g_free (archive->priv->extraction_destination);
		archive->priv->extraction_destination = new_uri;

		g_free (parent);

		content_uri = get_dir_content_if_unique (archive->priv->extraction_destination);
		if (content_uri == NULL)
			return;

		parent = remove_level_from_path (content_uri);
	}

	parent_parent = remove_level_from_path (parent);
	new_content_uri = get_alternative_uri (parent_parent, file_name_from_path (content_uri));

	source = g_file_new_for_uri (content_uri);
	destination = g_file_new_for_uri (new_content_uri);
	if (! g_file_move (source, destination, 0, NULL, NULL, NULL, &error)) {
		g_warning ("could not rename %s to %s: %s", content_uri, new_content_uri, error->message);
		g_clear_error (&error);
	}

	parent_file = g_file_new_for_uri (parent);
	if (! g_file_delete (parent_file, NULL, &error)) {
		g_warning ("could not remove directory %s: %s", parent, error->message);
		g_clear_error (&error);
	}
	g_object_unref (parent_file);

	g_free (archive->priv->extraction_destination);
	archive->priv->extraction_destination = new_content_uri;

	g_free (parent_parent);
	g_free (parent);
	g_free (content_uri);
}


static void
copy_extracted_files_done (GError   *error,
			   gpointer  user_data)
{
	FrArchive *archive = user_data;

	remove_local_directory (archive->priv->temp_extraction_dir);
	g_free (archive->priv->temp_extraction_dir);
	archive->priv->temp_extraction_dir = NULL;

	fr_archive_action_completed (archive,
				     FR_ACTION_COPYING_FILES_TO_REMOTE,
				     FR_PROC_ERROR_NONE,
				     NULL);

	if ((error == NULL) && (archive->priv->extract_here))
		move_here (archive);

	fr_archive_copy_done (archive, FR_ACTION_EXTRACTING_FILES, error);
}


static void
copy_extracted_files_progress (goffset   current_file,
                               goffset   total_files,
                               GFile    *source,
                               GFile    *destination,
                               goffset   current_num_bytes,
                               goffset   total_num_bytes,
                               gpointer  user_data)
{
	FrArchive *archive = user_data;

	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[PROGRESS],
		       0,
		       (double) current_file / (total_files + 1));
}


static void
copy_extracted_files_to_destination (FrArchive *archive)
{
	g_directory_copy_async (archive->priv->temp_extraction_dir,
				archive->priv->extraction_destination,
				G_FILE_COPY_OVERWRITE,
				G_PRIORITY_DEFAULT,
				archive->priv->cancellable,
				copy_extracted_files_progress,
				archive,
				copy_extracted_files_done,
				archive);
}


static void add_dropped_items (DroppedItemsData *data);


static void
fr_archive_change_name (FrArchive  *archive,
		        const char *filename)
{
	const char *name;
	GFile      *parent;

	name = file_name_from_path (filename);

	parent = g_file_get_parent (archive->file);
	g_object_unref (archive->file);
	archive->file = g_file_get_child (parent, name);
	g_object_unref (parent);

	parent = g_file_get_parent (archive->local_copy);
	g_object_unref (archive->local_copy);
	archive->local_copy = g_file_get_child (parent, name);
	g_object_unref (parent);
}


static void
action_performed (FrCommand   *command,
		  FrAction     action,
		  FrProcError *error,
		  FrArchive   *archive)
{
#ifdef DEBUG
	debug (DEBUG_INFO, "%s [DONE] (FR::Archive)\n", action_names[action]);
#endif

	switch (action) {
	case FR_ACTION_DELETING_FILES:
		if (error->type == FR_PROC_ERROR_NONE) {
			if (! g_file_has_uri_scheme (archive->file, "file")) {
				copy_to_remote_location (archive, action);
				return;
			}
		}
		break;

	case FR_ACTION_ADDING_FILES:
		if (error->type == FR_PROC_ERROR_NONE) {
			fr_archive_remove_temp_work_dir (archive);
			if (archive->priv->continue_adding_dropped_items) {
				add_dropped_items (archive->priv->dropped_items_data);
				return;
			}
			if (archive->priv->dropped_items_data != NULL) {
				dropped_items_data_free (archive->priv->dropped_items_data);
				archive->priv->dropped_items_data = NULL;
			}
			/* the name of the volumes are different from the
			 * original name */
			if (archive->command->multi_volume)
				fr_archive_change_name (archive, archive->command->filename);
			if (! g_file_has_uri_scheme (archive->file, "file")) {
				copy_to_remote_location (archive, action);
				return;
			}
		}
		break;

	case FR_ACTION_EXTRACTING_FILES:
		if (error->type == FR_PROC_ERROR_NONE) {
			if (archive->priv->remote_extraction) {
				copy_extracted_files_to_destination (archive);
				return;
			}
			else if (archive->priv->extract_here)
				move_here (archive);
		}
		else {
			/* if an error occurred during extraction remove the
			 * temp extraction dir, if used. */
			g_print ("action_performed: ERROR!\n");

			if ((archive->priv->remote_extraction) && (archive->priv->temp_extraction_dir != NULL)) {
				remove_local_directory (archive->priv->temp_extraction_dir);
				g_free (archive->priv->temp_extraction_dir);
				archive->priv->temp_extraction_dir = NULL;
			}

			if (archive->priv->extract_here)
				remove_directory (archive->priv->extraction_destination);
		}
		break;

	case FR_ACTION_LISTING_CONTENT:
		/* the name of the volumes are different from the
		 * original name */
		if (archive->command->multi_volume)
			fr_archive_change_name (archive, archive->command->filename);
		fr_command_update_capabilities (archive->command);
		if (! fr_command_is_capable_of (archive->command, FR_COMMAND_CAN_WRITE))
			archive->read_only = TRUE;
		break;

	default:
		/* nothing */
		break;
	}

	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[DONE],
		       0,
		       action,
		       error);
}


static gboolean
archive_progress_cb (FrCommand *command,
		     double     fraction,
		     FrArchive *archive)
{
	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[PROGRESS],
		       0,
		       fraction);
	return TRUE;
}


static gboolean
archive_message_cb  (FrCommand  *command,
		     const char *msg,
		     FrArchive  *archive)
{
	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[MESSAGE],
		       0,
		       msg);
	return TRUE;
}


static gboolean
archive_working_archive_cb  (FrCommand  *command,
			     const char *archive_filename,
			     FrArchive  *archive)
{
	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[WORKING_ARCHIVE],
		       0,
		       archive_filename);
	return TRUE;
}


static void
fr_archive_connect_to_command (FrArchive *archive)
{
	g_signal_connect (G_OBJECT (archive->command),
			  "start",
			  G_CALLBACK (action_started),
			  archive);
	g_signal_connect (G_OBJECT (archive->command),
			  "done",
			  G_CALLBACK (action_performed),
			  archive);
	g_signal_connect (G_OBJECT (archive->command),
			  "progress",
			  G_CALLBACK (archive_progress_cb),
			  archive);
	g_signal_connect (G_OBJECT (archive->command),
			  "message",
			  G_CALLBACK (archive_message_cb),
			  archive);
	g_signal_connect (G_OBJECT (archive->command),
			  "working_archive",
			  G_CALLBACK (archive_working_archive_cb),
			  archive);
}


gboolean
fr_archive_create (FrArchive  *archive,
		   const char *uri)
{
	FrCommand  *tmp_command;
	const char *mime_type;

	if (uri == NULL)
		return FALSE;

	fr_archive_set_uri (archive, uri);

	tmp_command = archive->command;

	mime_type = get_mime_type_from_filename (archive->local_copy);
	if (! create_command_to_create_archive (archive, mime_type)) {
		archive->command = tmp_command;
		return FALSE;
	}

	if (tmp_command != NULL) {
		g_signal_handlers_disconnect_by_data (tmp_command, archive);
		g_object_unref (G_OBJECT (tmp_command));
	}

	fr_archive_connect_to_command (archive);
	archive->read_only = FALSE;

	return TRUE;
}


void
fr_archive_set_fake_load_func (FrArchive    *archive,
			       FakeLoadFunc  func,
			       gpointer      data)
{
	archive->priv->fake_load_func = func;
	archive->priv->fake_load_data = data;
}


gboolean
fr_archive_fake_load (FrArchive *archive)
{
	if (archive->priv->fake_load_func != NULL)
		return (*archive->priv->fake_load_func) (archive, archive->priv->fake_load_data);
	else
		return FALSE;
}


/* -- fr_archive_load -- */


static void
load_local_archive (FrArchive  *archive,
		    const char *password)
{
	FrCommand  *old_command;
	const char *mime_type;

	if (! g_file_query_exists (archive->file, archive->priv->cancellable)) {
		fr_archive_action_completed (archive,
					     FR_ACTION_LOADING_ARCHIVE,
					     FR_PROC_ERROR_GENERIC,
					     _("File not found."));
		return;
	}

	archive->have_permissions = check_file_permissions (archive->file, W_OK);
	archive->read_only = ! archive->have_permissions;

	old_command = archive->command;

	mime_type = get_mime_type_from_filename (archive->local_copy);
	if (! create_command_to_load_archive (archive, mime_type)) {
		mime_type = get_mime_type_from_content (archive->local_copy);
		if (! create_command_to_load_archive (archive, mime_type)) {
			mime_type = get_mime_type_from_magic_numbers (archive->local_copy);
			if (! create_command_to_load_archive (archive, mime_type)) {
				archive->command = old_command;
				archive->content_type = mime_type;
				fr_archive_action_completed (archive,
							     FR_ACTION_LOADING_ARCHIVE,
							     FR_PROC_ERROR_UNSUPPORTED_FORMAT,
							     _("Archive type not supported."));
				return;
			}
		}
	}

	if (old_command != NULL) {
		g_signal_handlers_disconnect_by_data (old_command, archive);
		g_object_unref (old_command);
	}

	fr_archive_connect_to_command (archive);
	archive->content_type = mime_type;
	if (! fr_command_is_capable_of (archive->command, FR_COMMAND_CAN_WRITE))
		archive->read_only = TRUE;
	fr_archive_stoppable (archive, TRUE);
	archive->command->fake_load = fr_archive_fake_load (archive);

	fr_archive_action_completed (archive,
				     FR_ACTION_LOADING_ARCHIVE,
				     FR_PROC_ERROR_NONE,
				     NULL);

	/**/

	fr_process_clear (archive->process);
	g_object_set (archive->command,
		      "file", archive->local_copy,
		      "password", password,
		      NULL);
	fr_command_list (archive->command);
}


static void
copy_remote_file_done (GError   *error,
		       gpointer  user_data)
{
	XferData *xfer_data = user_data;

	if (error != NULL)
		fr_archive_copy_done (xfer_data->archive, FR_ACTION_LOADING_ARCHIVE, error);
	else
		load_local_archive (xfer_data->archive, xfer_data->password);
	xfer_data_free (xfer_data);
}


static void
copy_remote_file_progress (goffset   current_file,
                           goffset   total_files,
                           GFile    *source,
                           GFile    *destination,
                           goffset   current_num_bytes,
                           goffset   total_num_bytes,
                           gpointer  user_data)
{
	XferData *xfer_data = user_data;

	g_signal_emit (G_OBJECT (xfer_data->archive),
		       fr_archive_signals[PROGRESS],
		       0,
		       (double) current_num_bytes / total_num_bytes);
}


static gboolean
copy_remote_file_done_cb (gpointer user_data)
{
	XferData *xfer_data = user_data;

	g_source_remove (xfer_data->source_id);
	copy_remote_file_done (NULL, xfer_data);
	return FALSE;
}


static void
copy_remote_file (FrArchive  *archive,
		  const char *password)
{
	XferData *xfer_data;

	if (! g_file_query_exists (archive->file, archive->priv->cancellable)) {
		GError *error;

        error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_NOT_FOUND, _("Archive not found"));
		fr_archive_copy_done (archive, FR_ACTION_LOADING_ARCHIVE, error);
		g_error_free (error);

		return;
	}

	xfer_data = g_new0 (XferData, 1);
	xfer_data->archive = archive;
	xfer_data->uri = g_file_get_uri (archive->file);
	if (password != NULL)
		xfer_data->password = g_strdup (password);

	if (! archive->is_remote) {
		xfer_data->source_id = g_idle_add (copy_remote_file_done_cb, xfer_data);
		return;
	}

	g_copy_file_async (archive->file,
			   archive->local_copy,
			   G_FILE_COPY_OVERWRITE,
			   G_PRIORITY_DEFAULT,
			   archive->priv->cancellable,
			   copy_remote_file_progress,
			   xfer_data,
			   copy_remote_file_done,
			   xfer_data);
}


gboolean
fr_archive_load (FrArchive  *archive,
		 const char *uri,
		 const char *password)
{
	g_return_val_if_fail (archive != NULL, FALSE);

	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[START],
		       0,
		       FR_ACTION_LOADING_ARCHIVE);

	fr_archive_set_uri (archive, uri);
	copy_remote_file (archive, password);

	return TRUE;
}


gboolean
fr_archive_load_local (FrArchive  *archive,
		       const char *uri,
		       const char *password)
{
	g_return_val_if_fail (archive != NULL, FALSE);

	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[START],
		       0,
		       FR_ACTION_LOADING_ARCHIVE);

	fr_archive_set_uri (archive, uri);
	copy_remote_file (archive, password);

	return TRUE;
}


void
fr_archive_reload (FrArchive  *archive,
		   const char *password)
{
	char *uri;

	g_return_if_fail (archive != NULL);
	g_return_if_fail (archive->file != NULL);

	fr_archive_stoppable (archive, TRUE);
	archive->command->fake_load = fr_archive_fake_load (archive);

	uri = g_file_get_uri (archive->file);
	fr_archive_load (archive, uri, password);
	g_free (uri);
}


/* -- add -- */


static char *
create_tmp_base_dir (const char *base_dir,
		     const char *dest_path)
{
	char *dest_dir;
	char *temp_dir;
	char *tmp;
	char *parent_dir;
	char *dir;

	if ((dest_path == NULL)
	    || (*dest_path == '\0')
	    || (strcmp (dest_path, "/") == 0))
	{
		return g_strdup (base_dir);
	}

	dest_dir = g_strdup (dest_path);
	if (dest_dir[strlen (dest_dir) - 1] == G_DIR_SEPARATOR)
		dest_dir[strlen (dest_dir) - 1] = 0;

	debug (DEBUG_INFO, "base_dir: %s\n", base_dir);
	debug (DEBUG_INFO, "dest_dir: %s\n", dest_dir);

	temp_dir = get_temp_work_dir (NULL);
	tmp = remove_level_from_path (dest_dir);
	parent_dir =  g_build_filename (temp_dir, tmp, NULL);
	g_free (tmp);

	debug (DEBUG_INFO, "mkdir %s\n", parent_dir);
	make_directory_tree_from_path (parent_dir, 0700, NULL);
	g_free (parent_dir);

	dir = g_build_filename (temp_dir, "/", dest_dir, NULL);
	debug (DEBUG_INFO, "symlink %s --> %s\n", dir, base_dir);
	if (! symlink (base_dir, dir))
		g_warning("Could not create the symbolic link '%s', pointing to '%s'", dir, base_dir);

	g_free (dir);
	g_free (dest_dir);

	return temp_dir;
}


static FileData *
find_file_in_archive (FrArchive *archive,
		      char      *path)
{
	int i;

	g_return_val_if_fail (path != NULL, NULL);

	i = find_path_in_file_data_array (archive->command->files, path);
	if (i >= 0)
		return (FileData *) g_ptr_array_index (archive->command->files, i);
	else
		return NULL;
}


static void delete_from_archive (FrArchive *archive, GList *file_list);


static GList *
newer_files_only (FrArchive  *archive,
		  GList      *file_list,
		  const char *base_dir)
{
	GList *newer_files = NULL;
	GList *scan;

	for (scan = file_list; scan; scan = scan->next) {
		char     *filename = scan->data;
		char     *fullpath;
		char     *uri;
		FileData *fdata;

		fdata = find_file_in_archive (archive, filename);

		if (fdata == NULL) {
			newer_files = g_list_prepend (newer_files, g_strdup (scan->data));
			continue;
		}

		fullpath = g_strconcat (base_dir, "/", filename, NULL);
		uri = g_filename_to_uri (fullpath, NULL, NULL);

		if (fdata->modified >= get_file_mtime (uri)) {
			g_free (fullpath);
			g_free (uri);
			continue;
		}
		g_free (fullpath);
		g_free (uri);

		newer_files = g_list_prepend (newer_files, g_strdup (scan->data));
	}

	return newer_files;
}

static gboolean
save_list_to_temp_file (GList   *file_list,
		        char   **list_dir,
		        char   **list_filename,
		        GError **error)
{
	gboolean           error_occurred = FALSE;
	GFile             *list_file;
	GFileOutputStream *ostream;

	if (error != NULL)
		*error = NULL;
	*list_dir = get_temp_work_dir (NULL);
	*list_filename = g_build_filename (*list_dir, "file-list", NULL);
	list_file = g_file_new_for_path (*list_filename);
	ostream = g_file_create (list_file, G_FILE_CREATE_PRIVATE, NULL, error);

	if (ostream != NULL) {
		GList *scan;

		for (scan = file_list; scan != NULL; scan = scan->next) {
			char *filename = scan->data;

			filename = str_substitute (filename, "\n", "\\n");
			if ((g_output_stream_write (G_OUTPUT_STREAM (ostream), filename, strlen (filename), NULL, error) < 0)
			    || (g_output_stream_write (G_OUTPUT_STREAM (ostream), "\n", 1, NULL, error) < 0))
			{
				error_occurred = TRUE;
			}

			g_free (filename);

			if (error_occurred)
				break;
		}
		if (! error_occurred && ! g_output_stream_close (G_OUTPUT_STREAM (ostream), NULL, error))
			error_occurred = TRUE;
		g_object_unref (ostream);
	}
	else
		error_occurred = TRUE;

	if (error_occurred) {
		remove_local_directory (*list_dir);
		g_free (*list_dir);
		g_free (*list_filename);
		*list_dir = NULL;
		*list_filename = NULL;
	}

	g_object_unref (list_file);

	return ! error_occurred;
}


static GList *
split_in_chunks (GList *file_list)
{
	GList *chunks = NULL;
	GList *new_file_list;
	GList *scan;

	new_file_list = g_list_copy (file_list);
	for (scan = new_file_list; scan != NULL; /* void */) {
		GList *prev = scan->prev;
		GList *chunk;
		int    l;

		chunk = scan;
		l = 0;
		while ((scan != NULL) && (l < MAX_CHUNK_LEN)) {
			if (l == 0)
				l = strlen (scan->data);
			prev = scan;
			scan = scan->next;
			if (scan != NULL)
				l += strlen (scan->data);
		}
		if (prev != NULL) {
			if (prev->next != NULL)
				prev->next->prev = NULL;
			prev->next = NULL;
		}
		chunks = g_list_append (chunks, chunk);
	}

	return chunks;
}


void
fr_archive_add (FrArchive     *archive,
		GList         *file_list,
		const char    *base_dir,
		const char    *dest_dir,
		gboolean       update,
		gboolean       recursive,
		const char    *password,
		gboolean       encrypt_header,
		FrCompression  compression,
		guint          volume_size)
{
	GList    *new_file_list = NULL;
	gboolean  base_dir_created = FALSE;
	GList    *scan;
	char     *tmp_base_dir = NULL;
	char     *tmp_archive_dir = NULL;
	char     *archive_filename = NULL;
	char     *tmp_archive_filename = NULL;
	gboolean  error_occurred = FALSE;

	if (file_list == NULL)
		return;

	if (archive->read_only)
		return;

	g_object_set (archive->command,
		      "password", password,
		      "encrypt_header", encrypt_header,
		      "compression", compression,
		      "volume_size", volume_size,
		      NULL);

	fr_archive_stoppable (archive, TRUE);

	/* dest_dir is the destination folder inside the archive */
g_print("src_dir: %s, dst: %s\n", base_dir, dest_dir);
	if ((dest_dir != NULL) && (*dest_dir != '\0') && (strcmp (dest_dir, "/") != 0)) {
		const char *rel_dest_dir = dest_dir;

		tmp_base_dir = create_tmp_base_dir (base_dir, dest_dir);
		base_dir_created = TRUE;

		if (dest_dir[0] == G_DIR_SEPARATOR)
			rel_dest_dir = dest_dir + 1;

		new_file_list = NULL;
		for (scan = file_list; scan != NULL; scan = scan->next) {
			char *filename = scan->data;
			new_file_list = g_list_prepend (new_file_list, g_build_filename (rel_dest_dir, filename, NULL));
            g_print(" dst_fn: %s\n", (char*)new_file_list->data);
        }
	}
	else {
		tmp_base_dir = g_strdup (base_dir);
		new_file_list = path_list_dup(file_list);
	}

	/* if the command cannot update,  get the list of files that are
	 * newer than the ones in the archive. */

	if (update && ! archive->command->propAddCanUpdate) {
		GList *tmp_file_list;

		tmp_file_list = new_file_list;
		new_file_list = newer_files_only (archive, tmp_file_list, tmp_base_dir);
		path_list_free (tmp_file_list);
	}

	if (new_file_list == NULL) {
		debug (DEBUG_INFO, "nothing to update.\n");

		if (base_dir_created)
			remove_local_directory (tmp_base_dir);
		g_free (tmp_base_dir);

		archive->process->error.type = FR_PROC_ERROR_NONE;
		g_signal_emit_by_name (G_OBJECT (archive->process),
				       "done",
				       &archive->process->error);
		return;
	}

	archive->command->creating_archive = ! g_file_query_exists (archive->local_copy, archive->priv->cancellable);

	/* create the new archive in a temporary sub-directory, this allows
	 * to cancel the operation without losing the original archive and
	 * removing possible temporary files created by the command. */

	{
		GFile *local_copy_parent;
		char  *archive_dir;
		GFile *tmp_file;

		/* create the new archive in a sub-folder of the original
		 * archive this way the 'mv' command is fast. */

		local_copy_parent = g_file_get_parent (archive->local_copy);
		archive_dir = g_file_get_path (local_copy_parent);
		tmp_archive_dir = get_temp_work_dir (archive_dir);
		archive_filename = g_file_get_path (archive->local_copy);
		tmp_archive_filename = g_build_filename (tmp_archive_dir, file_name_from_path (archive_filename), NULL);
		tmp_file = g_file_new_for_path (tmp_archive_filename);
		g_object_set (archive->command, "file", tmp_file, NULL);

		if (! archive->command->creating_archive) {
			/* copy the original archive to the new position */

			fr_process_begin_command (archive->process, "cp");
			fr_process_add_arg (archive->process, "-f");
			fr_process_add_arg (archive->process, archive_filename);
			fr_process_add_arg (archive->process, tmp_archive_filename);
			fr_process_end_command (archive->process);
		}

		g_object_unref (tmp_file);
		g_free (archive_dir);
		g_object_unref (local_copy_parent);
	}

	fr_command_uncompress (archive->command);

	/* when files are already present in a tar archive and are added
	 * again, they are not replaced, so we have to delete them first. */

	/* if we are adding (== ! update) and 'add' cannot replace or
	 * if we are updating and 'add' cannot update,
	 * delete the files first. */

	if ((! update && ! archive->command->propAddCanReplace)
	    || (update && ! archive->command->propAddCanUpdate))
	{
		GList *del_list = NULL;

		for (scan = new_file_list; scan != NULL; scan = scan->next) {
			char *filename = scan->data;
			if (find_file_in_archive (archive, filename))
				del_list = g_list_prepend (del_list, filename);
		}

		/* delete */

		if (del_list != NULL) {
			delete_from_archive (archive, del_list);
			fr_process_set_ignore_error (archive->process, TRUE);
			g_list_free (del_list);
		}
	}

	/* add now. */

	fr_command_set_n_files (archive->command, g_list_length (new_file_list));

	if (archive->command->propListFromFile
	    && (archive->command->n_files > LIST_LENGTH_TO_USE_FILE))
	{
		char   *list_dir;
		char   *list_filename;
		GError *error = NULL;

		if (! save_list_to_temp_file (new_file_list, &list_dir, &list_filename, &error)) {
			archive->process->error.type = FR_PROC_ERROR_GENERIC;
			archive->process->error.status = 0;
			archive->process->error.gerror = g_error_copy (error);
			g_signal_emit_by_name (G_OBJECT (archive->process),
					       "done",
					       &archive->process->error);
			g_clear_error (&error);
			error_occurred = TRUE;
		}
		else {
			fr_command_add (archive->command,
					list_filename,
					new_file_list,
					tmp_base_dir,
					update,
					recursive);

			/* remove the temp dir */

			fr_process_begin_command (archive->process, "rm");
			fr_process_set_working_dir (archive->process, g_get_tmp_dir());
			fr_process_set_sticky (archive->process, TRUE);
			fr_process_add_arg (archive->process, "-rf");
			fr_process_add_arg (archive->process, list_dir);
			fr_process_end_command (archive->process);
		}

		g_free (list_filename);
		g_free (list_dir);
	}
	else {
		GList *chunks = NULL;

		/* specify the file list on the command line, splitting
		 * in more commands to avoid to overflow the command line
		 * length limit. */

		chunks = split_in_chunks (new_file_list);
		for (scan = chunks; scan != NULL; scan = scan->next) {
			GList *chunk = scan->data;

			fr_command_add (archive->command,
					NULL,
					chunk,
					tmp_base_dir,
					update,
					recursive);
			g_list_free (chunk);
		}

		g_list_free (chunks);
	}

	path_list_free (new_file_list);

    g_print("final: %s -> %s, %d\n", tmp_archive_filename, archive_filename, error_occurred);
    if (! error_occurred) {
		fr_command_recompress (archive->command);

		/* move the new archive to the original position */

		fr_process_begin_command (archive->process, "mv");
		fr_process_add_arg (archive->process, "-f");
		fr_process_add_arg (archive->process, tmp_archive_filename);
		fr_process_add_arg (archive->process, archive_filename);
		fr_process_end_command (archive->process);

		/* remove the temp sub-directory */

		fr_process_begin_command (archive->process, "rm");
		fr_process_set_working_dir (archive->process, g_get_tmp_dir());
		fr_process_set_sticky (archive->process, TRUE);
		fr_process_add_arg (archive->process, "-rf");
		fr_process_add_arg (archive->process, tmp_archive_dir);
		fr_process_end_command (archive->process);

		/* remove the base dir */

		if (base_dir_created) {
			fr_process_begin_command (archive->process, "rm");
			fr_process_set_working_dir (archive->process, g_get_tmp_dir());
			fr_process_set_sticky (archive->process, TRUE);
			fr_process_add_arg (archive->process, "-rf");
			fr_process_add_arg (archive->process, tmp_base_dir);
			fr_process_end_command (archive->process);
		}
	}

	g_free (tmp_archive_filename);
	g_free (archive_filename);
	g_free (tmp_archive_dir);
	g_free (tmp_base_dir);
}


static void
fr_archive_add_local_files (FrArchive     *archive,
			    GList         *file_list,
			    const char    *base_dir,
			    const char    *dest_dir,
			    gboolean       update,
			    const char    *password,
			    gboolean       encrypt_header,
			    FrCompression  compression,
			    guint          volume_size)
{
	fr_process_clear (archive->process);
	fr_archive_add (archive,
			file_list,
			base_dir,
			dest_dir,
			update,
			FALSE,
			password,
			encrypt_header,
			compression,
			volume_size);
	fr_process_start (archive->process);
}


static void
copy_remote_files_done (GError   *error,
			gpointer  user_data)
{
	XferData *xfer_data = user_data;

	fr_archive_copy_done (xfer_data->archive, FR_ACTION_COPYING_FILES_FROM_REMOTE, error);

	if (error == NULL)
		fr_archive_add_local_files (xfer_data->archive,
					    xfer_data->file_list,
					    xfer_data->tmp_dir,
					    xfer_data->dest_dir,
					    FALSE,
					    xfer_data->password,
					    xfer_data->encrypt_header,
					    xfer_data->compression,
					    xfer_data->volume_size);
	xfer_data_free (xfer_data);
}


static void
copy_remote_files_progress (goffset   current_file,
                            goffset   total_files,
                            GFile    *source,
                            GFile    *destination,
                            goffset   current_num_bytes,
                            goffset   total_num_bytes,
                            gpointer  user_data)
{
	XferData *xfer_data = user_data;

	g_signal_emit (G_OBJECT (xfer_data->archive),
		       fr_archive_signals[PROGRESS],
		       0,
		       (double) current_file / (total_files + 1));
}


static void
copy_remote_files (FrArchive     *archive,
		   GList         *file_list,
		   const char    *base_uri,
		   const char    *dest_dir,
		   gboolean       update,
		   const char    *password,
		   gboolean       encrypt_header,
		   FrCompression  compression,
		   guint          volume_size,
		   const char    *tmp_dir)
{
	GList      *sources = NULL, *destinations = NULL;
	GHashTable *created_folders;
	GList      *scan;
	XferData   *xfer_data;

	created_folders = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, NULL);
	for (scan = file_list; scan; scan = scan->next) {
		char  *partial_filename = scan->data;
		char  *local_uri;
		char  *local_folder_uri;
		char  *remote_uri;

		local_uri = g_strconcat ("file://", tmp_dir, "/", partial_filename, NULL);
		local_folder_uri = remove_level_from_path (local_uri);
		if (g_hash_table_lookup (created_folders, local_folder_uri) == NULL) {
			GError *error = NULL;
			if (! ensure_dir_exists (local_folder_uri, 0755, &error)) {
				g_free (local_folder_uri);
				g_free (local_uri);
				gio_file_list_free (sources);
				gio_file_list_free (destinations);
				g_hash_table_destroy (created_folders);

				fr_archive_action_completed (archive,
							     FR_ACTION_COPYING_FILES_FROM_REMOTE,
							     FR_PROC_ERROR_GENERIC,
							     error->message);
				g_clear_error (&error);
				return;
			}

			g_hash_table_insert (created_folders, local_folder_uri, GINT_TO_POINTER (1));
		}
		else
			g_free (local_folder_uri);

		remote_uri = g_strconcat (base_uri, "/", partial_filename, NULL);
		sources = g_list_append (sources, g_file_new_for_uri (remote_uri));
		g_free (remote_uri);

		destinations = g_list_append (destinations, g_file_new_for_uri (local_uri));
		g_free (local_uri);
	}
	g_hash_table_destroy (created_folders);

	xfer_data = g_new0 (XferData, 1);
	xfer_data->archive = archive;
	xfer_data->file_list = path_list_dup (file_list);
	xfer_data->base_uri = g_strdup (base_uri);
	xfer_data->dest_dir = g_strdup (dest_dir);
	xfer_data->update = update;
	xfer_data->dest_dir = g_strdup (dest_dir);
	xfer_data->password = g_strdup (password);
	xfer_data->encrypt_header = encrypt_header;
	xfer_data->compression = compression;
	xfer_data->volume_size = volume_size;
	xfer_data->tmp_dir = g_strdup (tmp_dir);

	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[START],
		       0,
		       FR_ACTION_COPYING_FILES_FROM_REMOTE);

	g_copy_files_async (sources,
			    destinations,
			    G_FILE_COPY_OVERWRITE,
			    G_PRIORITY_DEFAULT,
			    archive->priv->cancellable,
			    copy_remote_files_progress,
			    xfer_data,
			    copy_remote_files_done,
			    xfer_data);

	gio_file_list_free (sources);
	gio_file_list_free (destinations);
}


static char *
fr_archive_get_temp_work_dir (FrArchive *archive)
{
	fr_archive_remove_temp_work_dir (archive);
	archive->priv->temp_dir = get_temp_work_dir (NULL);
	return archive->priv->temp_dir;
}


void
fr_archive_add_files (FrArchive     *archive,
		      GList         *file_list,
		      const char    *base_dir,
		      const char    *dest_dir,
		      gboolean       update,
		      const char    *password,
		      gboolean       encrypt_header,
		      FrCompression  compression,
		      guint          volume_size)
{
	if (uri_is_local (base_dir)) {
		char *local_dir = g_filename_from_uri (base_dir, NULL, NULL);
		fr_archive_add_local_files (archive,
					    file_list,
					    local_dir,
					    dest_dir,
					    update,
					    password,
					    encrypt_header,
					    compression,
					    volume_size);
		g_free (local_dir);
	}
	else
		copy_remote_files (archive,
				   file_list,
				   base_dir,
				   dest_dir,
				   update,
				   password,
				   encrypt_header,
				   compression,
				   volume_size,
				   fr_archive_get_temp_work_dir (archive));
}


/* -- add with wildcard -- */


typedef struct {
	FrArchive     *archive;
	char          *source_dir;
	char          *dest_dir;
	gboolean       update;
	char          *password;
	gboolean       encrypt_header;
	FrCompression  compression;
	guint          volume_size;
} AddWithWildcardData;


static void
add_with_wildcard_data_free (AddWithWildcardData *aww_data)
{
	g_free (aww_data->source_dir);
	g_free (aww_data->dest_dir);
	g_free (aww_data->password);
	g_free (aww_data);
}


static void
add_with_wildcard__step2 (GList    *file_list,
			  GList    *dirs_list,
			  GError   *error,
			  gpointer  data)
{
	AddWithWildcardData *aww_data = data;
	FrArchive           *archive = aww_data->archive;

	if (error != NULL) {
		fr_archive_action_completed (archive,
					     FR_ACTION_GETTING_FILE_LIST,
					     (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) ? FR_PROC_ERROR_STOPPED : FR_PROC_ERROR_GENERIC),
					     error->message);
		return;
	}

	fr_archive_action_completed (archive,
				     FR_ACTION_GETTING_FILE_LIST,
				     FR_PROC_ERROR_NONE,
				     NULL);

	if (file_list != NULL)
		fr_archive_add_files (aww_data->archive,
				      file_list,
				      aww_data->source_dir,
				      aww_data->dest_dir,
				      aww_data->update,
				      aww_data->password,
				      aww_data->encrypt_header,
				      aww_data->compression,
				      aww_data->volume_size);

	path_list_free (file_list);
	path_list_free (dirs_list);
	add_with_wildcard_data_free (aww_data);
}


void
fr_archive_add_with_wildcard (FrArchive     *archive,
			      const char    *include_files,
			      const char    *exclude_files,
			      const char    *exclude_folders,
			      const char    *source_dir,
			      const char    *dest_dir,
			      gboolean       update,
			      gboolean       follow_links,
			      const char    *password,
			      gboolean       encrypt_header,
			      FrCompression  compression,
			      guint          volume_size)
{
	AddWithWildcardData *aww_data;

	g_return_if_fail (! archive->read_only);

	aww_data = g_new0 (AddWithWildcardData, 1);
	aww_data->archive = archive;
	aww_data->source_dir = g_strdup (source_dir);
	aww_data->dest_dir = g_strdup (dest_dir);
	aww_data->update = update;
	aww_data->password = g_strdup (password);
	aww_data->encrypt_header = encrypt_header;
	aww_data->compression = compression;
	aww_data->volume_size = volume_size;

	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[START],
		       0,
		       FR_ACTION_GETTING_FILE_LIST);

	g_directory_list_async (source_dir,
				source_dir,
				TRUE,
				follow_links,
				NO_BACKUP_FILES,
				NO_DOT_FILES,
				include_files,
				exclude_files,
				exclude_folders,
				IGNORE_CASE,
				archive->priv->cancellable,
				add_with_wildcard__step2,
				aww_data);
}


/* -- fr_archive_add_directory -- */


typedef struct {
	FrArchive     *archive;
	char          *base_dir;
	char          *dest_dir;
	gboolean       update;
	char          *password;
	gboolean       encrypt_header;
	FrCompression  compression;
	guint          volume_size;
} AddDirectoryData;


static void
add_directory_data_free (AddDirectoryData *ad_data)
{
	g_free (ad_data->base_dir);
	g_free (ad_data->dest_dir);
	g_free (ad_data->password);
	g_free (ad_data);
}


static void
add_directory__step2 (GList    *file_list,
		      GList    *dir_list,
		      GError   *error,
		      gpointer  data)
{
	AddDirectoryData *ad_data = data;
	FrArchive        *archive = ad_data->archive;

	if (error != NULL) {
		fr_archive_action_completed (archive,
					     FR_ACTION_GETTING_FILE_LIST,
					     (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) ? FR_PROC_ERROR_STOPPED : FR_PROC_ERROR_GENERIC),
					     error->message);
		return;
	}

	fr_archive_action_completed (archive,
				     FR_ACTION_GETTING_FILE_LIST,
				     FR_PROC_ERROR_NONE,
				     NULL);

	if (archive->command->propAddCanStoreFolders)
		file_list = g_list_concat (file_list, dir_list);
	else
		path_list_free (dir_list);

	if (file_list != NULL) {
		fr_archive_add_files (ad_data->archive,
				      file_list,
				      ad_data->base_dir,
				      ad_data->dest_dir,
				      ad_data->update,
				      ad_data->password,
				      ad_data->encrypt_header,
				      ad_data->compression,
				      ad_data->volume_size);
		path_list_free (file_list);
	}

	add_directory_data_free (ad_data);
}


void
fr_archive_add_directory (FrArchive     *archive,
			  const char    *directory,
			  const char    *base_dir,
			  const char    *dest_dir,
			  gboolean       update,
			  const char    *password,
			  gboolean       encrypt_header,
			  FrCompression  compression,
			  guint          volume_size)

{
	AddDirectoryData *ad_data;

	g_return_if_fail (! archive->read_only);

	ad_data = g_new0 (AddDirectoryData, 1);
	ad_data->archive = archive;
	ad_data->base_dir = g_strdup (base_dir);
	ad_data->dest_dir = g_strdup (dest_dir);
	ad_data->update = update;
	ad_data->password = g_strdup (password);
	ad_data->encrypt_header = encrypt_header;
	ad_data->compression = compression;
	ad_data->volume_size = volume_size;

	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[START],
		       0,
		       FR_ACTION_GETTING_FILE_LIST);

	g_directory_list_all_async (directory,
				    base_dir,
				    TRUE,
				    archive->priv->cancellable,
				    add_directory__step2,
				    ad_data);
}


void
fr_archive_add_items (FrArchive     *archive,
		      GList         *item_list,
		      const char    *base_dir,
		      const char    *dest_dir,
		      gboolean       update,
		      const char    *password,
		      gboolean       encrypt_header,
		      FrCompression  compression,
		      guint          volume_size)

{
	AddDirectoryData *ad_data;

	g_return_if_fail (! archive->read_only);

	ad_data = g_new0 (AddDirectoryData, 1);
	ad_data->archive = archive;
	ad_data->base_dir = g_strdup (base_dir);
	ad_data->dest_dir = g_strdup (dest_dir);
	ad_data->update = update;
	ad_data->password = g_strdup (password);
	ad_data->encrypt_header = encrypt_header;
	ad_data->compression = compression;
	ad_data->volume_size = volume_size;

	g_signal_emit (G_OBJECT (archive),
		       fr_archive_signals[START],
		       0,
		       FR_ACTION_GETTING_FILE_LIST);

	g_list_items_async (item_list,
			    base_dir,
			    archive->priv->cancellable,
			    add_directory__step2,
			    ad_data);
}


/* -- fr_archive_add_dropped_items -- */


static gboolean
all_files_in_same_dir (GList *list)
{
	gboolean  same_dir = TRUE;
	char     *first_basedir;
	GList    *scan;

	if (list == NULL)
		return FALSE;

	first_basedir = remove_level_from_path (list->data);
	if (first_basedir == NULL)
		return TRUE;

	for (scan = list->next; scan; scan = scan->next) {
		char *path = scan->data;
		char *basedir;

		basedir = remove_level_from_path (path);
		if (basedir == NULL) {
			same_dir = FALSE;
			break;
		}

		if (strcmp (first_basedir, basedir) != 0) {
			same_dir = FALSE;
			g_free (basedir);
			break;
		}
		g_free (basedir);
	}
	g_free (first_basedir);

	return same_dir;
}


static void
add_dropped_items (DroppedItemsData *data)
{
	FrArchive *archive = data->archive;
	GList     *list = data->item_list;
	GList     *scan;

	if (list == NULL) {
		dropped_items_data_free (archive->priv->dropped_items_data);
		archive->priv->dropped_items_data = NULL;
		fr_archive_action_completed (archive,
					     FR_ACTION_ADDING_FILES,
					     FR_PROC_ERROR_NONE,
					     NULL);
		return;
	}

	/* if all files/dirs are in the same directory call fr_archive_add_items... */

	if (all_files_in_same_dir (list)) {
		char *first_base_dir;

		first_base_dir = remove_level_from_path (list->data);
		fr_archive_add_items (data->archive,
				      list,
				      first_base_dir,
				      data->dest_dir,
				      data->update,
				      data->password,
				      data->encrypt_header,
				      data->compression,
				      data->volume_size);
		g_free (first_base_dir);

		dropped_items_data_free (archive->priv->dropped_items_data);
		archive->priv->dropped_items_data = NULL;

		return;
	}

	/* ...else add a directory at a time. */

	for (scan = list; scan; scan = scan->next) {
		char *path = scan->data;
		char *base_dir;

		if (! uri_is_dir (path))
			continue;

		data->item_list = g_list_remove_link (list, scan);
		if (data->item_list != NULL)
			archive->priv->continue_adding_dropped_items = TRUE;
		base_dir = remove_level_from_path (path);

		fr_archive_add_directory (archive,
					  file_name_from_path (path),
					  base_dir,
					  data->dest_dir,
					  data->update,
					  data->password,
					  data->encrypt_header,
					  data->compression,
					  data->volume_size);

		g_free (base_dir);
		g_free (path);

		return;
	}

	/* if all files are in the same directory call fr_archive_add_files. */

	if (all_files_in_same_dir (list)) {
		char  *first_basedir;
		GList *only_names_list = NULL;

		first_basedir = remove_level_from_path (list->data);

		for (scan = list; scan; scan = scan->next) {
			char *name;

			name = g_uri_unescape_string (file_name_from_path (scan->data), NULL);
			only_names_list = g_list_prepend (only_names_list, name);
		}

		fr_archive_add_files (archive,
				      only_names_list,
				      first_basedir,
				      data->dest_dir,
				      data->update,
				      data->password,
				      data->encrypt_header,
				      data->compression,
				      data->volume_size);

		path_list_free (only_names_list);
		g_free (first_basedir);

		return;
	}

	/* ...else call fr_command_add for each file.  This is needed to add
	 * files without path info. FIXME: doesn't work with remote files. */

	fr_archive_stoppable (archive, FALSE);
	archive->command->creating_archive = ! g_file_query_exists (archive->local_copy, archive->priv->cancellable);
	g_object_set (archive->command,
		      "file", archive->local_copy,
		      "password", data->password,
		      "encrypt_header", data->encrypt_header,
		      "compression", data->compression,
		      "volume_size", data->volume_size,
		      NULL);
	fr_process_clear (archive->process);
	fr_command_uncompress (archive->command);
	for (scan = list; scan; scan = scan->next) {
		char  *fullpath = scan->data;
		char  *basedir;
		GList *singleton;

		basedir = remove_level_from_path (fullpath);
		singleton = g_list_prepend (NULL, (char*)file_name_from_path (fullpath));
		fr_command_add (archive->command,
				NULL,
				singleton,
				basedir,
				data->update,
				FALSE);
		g_list_free (singleton);
		g_free (basedir);
	}
	fr_command_recompress (archive->command);
	fr_process_start (archive->process);

	path_list_free (data->item_list);
	data->item_list = NULL;
}


void
fr_archive_add_dropped_items (FrArchive     *archive,
			      GList         *item_list,
			      const char    *base_dir,
			      const char    *dest_dir,
			      gboolean       update,
			      const char    *password,
			      gboolean       encrypt_header,
			      FrCompression  compression,
			      guint          volume_size)
{
	GList *scan;
	char  *archive_uri;

	if (archive->read_only) {
		fr_archive_action_completed (archive,
					     FR_ACTION_ADDING_FILES,
					     FR_PROC_ERROR_GENERIC,
					     ! archive->have_permissions ? _("You don't have the right permissions.") : _("This archive type cannot be modified"));
		return;
	}

	/* FIXME: make this check for all the add actions */
	archive_uri = g_file_get_uri (archive->file);
	for (scan = item_list; scan; scan = scan->next) {
		if (uricmp (scan->data, archive_uri) == 0) {
			g_free (archive_uri);
			fr_archive_action_completed (archive,
						     FR_ACTION_ADDING_FILES,
						     FR_PROC_ERROR_GENERIC,
						     _("You can't add an archive to itself."));
			return;
		}
	}
	g_free (archive_uri);

	if (archive->priv->dropped_items_data != NULL)
		dropped_items_data_free (archive->priv->dropped_items_data);
	archive->priv->dropped_items_data = dropped_items_data_new (
						archive,
				       		item_list,
				       		base_dir,
				       		dest_dir,
				       		update,
				       		password,
				       		encrypt_header,
				       		compression,
				       		volume_size);
	add_dropped_items (archive->priv->dropped_items_data);
}


/* -- remove -- */


static gboolean
file_is_in_subfolder_of (const char *filename,
		         GList      *folder_list)
{
	GList *scan;

	if (filename == NULL)
		return FALSE;

	for (scan = folder_list; scan; scan = scan->next) {
		char *folder_in_list = (char*) scan->data;

		if (path_in_path (folder_in_list, filename))
			return TRUE;
	}

	return FALSE;
}

static gboolean
archive_type_has_issues_deleting_non_empty_folders (FrArchive *archive)
{
       return ! archive->command->propCanDeleteNonEmptyFolders;
}


static void
delete_from_archive (FrArchive *archive,
		     GList     *file_list)
{
	gboolean  file_list_created = FALSE;
	GList    *tmp_file_list = NULL;
	gboolean  tmp_file_list_created = FALSE;
	GList    *scan;

	/* file_list == NULL means delete all the files in the archive. */

	if (file_list == NULL) {
		int i;

		for (i = 0; i < archive->command->files->len; i++) {
			FileData *fdata = g_ptr_array_index (archive->command->files, i);
			file_list = g_list_prepend (file_list, fdata->original_path);
		}

		file_list_created = TRUE;
	}

	if (archive_type_has_issues_deleting_non_empty_folders (archive)) {
		GList *folders_to_remove;

		/* remove from the list the files contained in folders to be
		 * removed. */

		folders_to_remove = NULL;
		for (scan = file_list; scan != NULL; scan = scan->next) {
			char *path = scan->data;

			if (path[strlen (path) - 1] == '/')
				folders_to_remove = g_list_prepend (folders_to_remove, path);
		}

		if (folders_to_remove != NULL) {
			tmp_file_list = NULL;
			for (scan = file_list; scan != NULL; scan = scan->next) {
				char *path = scan->data;

				if (! file_is_in_subfolder_of (path, folders_to_remove))
					tmp_file_list = g_list_prepend (tmp_file_list, path);
			}
			tmp_file_list_created = TRUE;
			g_list_free (folders_to_remove);
		}
	}

	if (! tmp_file_list_created)
		tmp_file_list = g_list_copy (file_list);

	if (file_list_created)
		g_list_free (file_list);

	fr_command_set_n_files (archive->command, g_list_length (tmp_file_list));

	if (archive->command->propListFromFile
	    && (archive->command->n_files > LIST_LENGTH_TO_USE_FILE))
	{
		char *list_dir;
		char *list_filename;

		if (save_list_to_temp_file (tmp_file_list, &list_dir, &list_filename, NULL)) {
			fr_command_delete (archive->command,
					   list_filename,
					   tmp_file_list);

			/* remove the temp dir */

			fr_process_begin_command (archive->process, "rm");
			fr_process_set_working_dir (archive->process, g_get_tmp_dir());
			fr_process_set_sticky (archive->process, TRUE);
			fr_process_add_arg (archive->process, "-rf");
			fr_process_add_arg (archive->process, list_dir);
			fr_process_end_command (archive->process);
		}

		g_free (list_filename);
		g_free (list_dir);
	}
	else {
		for (scan = tmp_file_list; scan != NULL; ) {
			GList *prev = scan->prev;
			GList *chunk_list;
			int    l;

			chunk_list = scan;
			l = 0;
			while ((scan != NULL) && (l < MAX_CHUNK_LEN)) {
				if (l == 0)
					l = strlen (scan->data);
				prev = scan;
				scan = scan->next;
				if (scan != NULL)
					l += strlen (scan->data);
			}

			prev->next = NULL;
			fr_command_delete (archive->command, NULL, chunk_list);
			prev->next = scan;
		}
	}

	g_list_free (tmp_file_list);
}


void
fr_archive_remove (FrArchive     *archive,
		   GList         *file_list,
		   FrCompression  compression)
{
	char *tmp_archive_dir = NULL;
	char *archive_filename = NULL;
	char *tmp_archive_filename = NULL;

	g_return_if_fail (archive != NULL);

	if (archive->read_only)
		return;

	fr_archive_stoppable (archive, TRUE);
	archive->command->creating_archive = FALSE;
	g_object_set (archive->command, "compression", compression, NULL);

	/* create the new archive in a temporary sub-directory, this allows
	 * to cancel the operation without losing the original archive and
	 * removing possible temporary files created by the command. */

	{
		GFile *local_copy_parent;
		char  *archive_dir;
		GFile *tmp_file;

		/* create the new archive in a sub-folder of the original
		 * archive this way the 'mv' command is fast. */

		local_copy_parent = g_file_get_parent (archive->local_copy);
		archive_dir = g_file_get_path (local_copy_parent);
		tmp_archive_dir = get_temp_work_dir (archive_dir);
		archive_filename = g_file_get_path (archive->local_copy);
		tmp_archive_filename = g_build_filename (tmp_archive_dir, file_name_from_path (archive_filename), NULL);
		tmp_file = g_file_new_for_path (tmp_archive_filename);
		g_object_set (archive->command, "file", tmp_file, NULL);

		if (! archive->command->creating_archive) {
			/* copy the original archive to the new position */

			fr_process_begin_command (archive->process, "cp");
			fr_process_add_arg (archive->process, "-f");
			fr_process_add_arg (archive->process, archive_filename);
			fr_process_add_arg (archive->process, tmp_archive_filename);
			fr_process_end_command (archive->process);
		}

		g_object_unref (tmp_file);
		g_free (archive_dir);
		g_object_unref (local_copy_parent);
	}

	/* uncompress, delete and recompress */

	fr_command_uncompress (archive->command);
	delete_from_archive (archive, file_list);
	fr_command_recompress (archive->command);

	/* move the new archive to the original position */

	fr_process_begin_command (archive->process, "mv");
	fr_process_add_arg (archive->process, "-f");
	fr_process_add_arg (archive->process, tmp_archive_filename);
	fr_process_add_arg (archive->process, archive_filename);
	fr_process_end_command (archive->process);

	/* remove the temp sub-directory */

	fr_process_begin_command (archive->process, "rm");
	fr_process_set_working_dir (archive->process, g_get_tmp_dir());
	fr_process_set_sticky (archive->process, TRUE);
	fr_process_add_arg (archive->process, "-rf");
	fr_process_add_arg (archive->process, tmp_archive_dir);
	fr_process_end_command (archive->process);

	g_free (tmp_archive_filename);
	g_free (archive_filename);
	g_free (tmp_archive_dir);
}


/* -- extract -- */


static void
move_files_to_dir (FrArchive  *archive,
		   GList      *file_list,
		   const char *source_dir,
		   const char *dest_dir,
		   gboolean    overwrite)
{
	GList *list;
	GList *scan;

	/* we prefer mv instead of cp for performance reasons,
	 * but if the destination folder already exists mv
	 * doesn't work correctly. (bug #590027) */

	list = g_list_copy (file_list);
	for (scan = list; scan; /* void */) {
		GList *next = scan->next;
		char  *filename = scan->data;
		char  *basename;
		char  *destname;

		basename = g_path_get_basename (filename);
		destname = g_build_filename (dest_dir, basename, NULL);
		if (g_file_test (destname, G_FILE_TEST_IS_DIR)) {
			fr_process_begin_command (archive->process, "cp");
			fr_process_add_arg (archive->process, "-R");
			if (overwrite)
				fr_process_add_arg (archive->process, "-f");
			else
				fr_process_add_arg (archive->process, "-n");
			if (filename[0] == '/')
				fr_process_add_arg_concat (archive->process, source_dir, filename, NULL);
			else
				fr_process_add_arg_concat (archive->process, source_dir, "/", filename, NULL);
			fr_process_add_arg (archive->process, dest_dir);
			fr_process_end_command (archive->process);

			list = g_list_remove_link (list, scan);
			g_list_free (scan);
		}

		g_free (destname);
		g_free (basename);

		scan = next;
	}

	if (list == NULL)
		return;

	/* 'list' now contains the files that can be moved without problems */

	fr_process_begin_command (archive->process, "mv");
	if (overwrite)
		fr_process_add_arg (archive->process, "-f");
	else
		fr_process_add_arg (archive->process, "-n");
	for (scan = list; scan; scan = scan->next) {
		char *filename = scan->data;

		if (filename[0] == '/')
			fr_process_add_arg_concat (archive->process, source_dir, filename, NULL);
		else
			fr_process_add_arg_concat (archive->process, source_dir, "/", filename, NULL);
	}
	fr_process_add_arg (archive->process, dest_dir);
	fr_process_end_command (archive->process);

	g_list_free (list);
}


static void
move_files_in_chunks (FrArchive  *archive,
		      GList      *file_list,
		      const char *temp_dir,
		      const char *dest_dir,
		      gboolean    overwrite)
{
	GList *scan;
	int    temp_dir_l;

	temp_dir_l = strlen (temp_dir);

	for (scan = file_list; scan != NULL; ) {
		GList *prev = scan->prev;
		GList *chunk_list;
		int    l;

		chunk_list = scan;
		l = 0;
		while ((scan != NULL) && (l < MAX_CHUNK_LEN)) {
			if (l == 0)
				l = temp_dir_l + 1 + strlen (scan->data);
			prev = scan;
			scan = scan->next;
			if (scan != NULL)
				l += temp_dir_l + 1 + strlen (scan->data);
		}

		prev->next = NULL;
		move_files_to_dir (archive, chunk_list, temp_dir, dest_dir, overwrite);
		prev->next = scan;
	}
}


static void
extract_from_archive (FrArchive  *archive,
		      GList      *file_list,
		      const char *dest_dir,
		      gboolean    overwrite,
		      gboolean    skip_older,
		      gboolean    junk_paths,
		      const char *password)
{
	FrCommand *command = archive->command;
	GList     *scan;

	g_object_set (command, "password", password, NULL);

	if (file_list == NULL) {
		fr_command_extract (command,
				    NULL,
				    file_list,
				    dest_dir,
				    overwrite,
				    skip_older,
				    junk_paths);
		return;
	}

	if (command->propListFromFile
	    && (g_list_length (file_list) > LIST_LENGTH_TO_USE_FILE))
	{
		char *list_dir;
		char *list_filename;

		if (save_list_to_temp_file (file_list, &list_dir, &list_filename, NULL)) {
			fr_command_extract (command,
					    list_filename,
					    file_list,
					    dest_dir,
					    overwrite,
					    skip_older,
					    junk_paths);

			/* remove the temp dir */

			fr_process_begin_command (archive->process, "rm");
			fr_process_set_working_dir (archive->process, g_get_tmp_dir());
			fr_process_set_sticky (archive->process, TRUE);
			fr_process_add_arg (archive->process, "-rf");
			fr_process_add_arg (archive->process, list_dir);
			fr_process_end_command (archive->process);
		}

		g_free (list_filename);
		g_free (list_dir);
	}
	else {
		for (scan = file_list; scan != NULL; ) {
			GList *prev = scan->prev;
			GList *chunk_list;
			int    l;

			chunk_list = scan;
			l = 0;
			while ((scan != NULL) && (l < MAX_CHUNK_LEN)) {
				if (l == 0)
					l = strlen (scan->data);
				prev = scan;
				scan = scan->next;
				if (scan != NULL)
					l += strlen (scan->data);
			}

			prev->next = NULL;
			fr_command_extract (command,
					    NULL,
					    chunk_list,
					    dest_dir,
					    overwrite,
					    skip_older,
					    junk_paths);
			prev->next = scan;
		}
	}
}


static char*
compute_base_path (const char *base_dir,
		   const char *path,
		   gboolean    junk_paths,
		   gboolean    can_junk_paths)
{
	int         base_dir_len = strlen (base_dir);
	int         path_len = strlen (path);
	const char *base_path;
	char       *name_end;
	char       *new_path;

	if (junk_paths) {
		if (can_junk_paths)
			new_path = g_strdup (file_name_from_path (path));
		else
			new_path = g_strdup (path);

		/*debug (DEBUG_INFO, "%s, %s --> %s\n", base_dir, path, new_path);*/

		return new_path;
	}

	if (path_len < base_dir_len)
		return NULL;

	base_path = path + base_dir_len;
	if (path[0] != '/')
		base_path -= 1;
	name_end = strchr (base_path, '/');

	if (name_end == NULL)
		new_path = g_strdup (path);
	else {
		int name_len = name_end - path;
		new_path = g_strndup (path, name_len);
	}

	/*debug (DEBUG_INFO, "%s, %s --> %s\n", base_dir, path, new_path);*/

	return new_path;
}


static GList*
compute_list_base_path (const char *base_dir,
			GList      *filtered,
			gboolean    junk_paths,
			gboolean    can_junk_paths)
{
	GList *scan;
	GList *list = NULL, *list_unique = NULL;
	GList *last_inserted;

	if (filtered == NULL)
		return NULL;

	for (scan = filtered; scan; scan = scan->next) {
		const char *path = scan->data;
		char       *new_path;

		new_path = compute_base_path (base_dir, path, junk_paths, can_junk_paths);
		if (new_path != NULL)
			list = g_list_prepend (list, new_path);
	}

	/* The above operation can create duplicates, we remove them here. */
	list = g_list_sort (list, (GCompareFunc)strcmp);

	last_inserted = NULL;
	for (scan = list; scan; scan = scan->next) {
		const char *path = scan->data;

		if (last_inserted != NULL) {
			const char *last_path = (const char*)last_inserted->data;
			if (strcmp (last_path, path) == 0) {
				g_free (scan->data);
				continue;
			}
		}

		last_inserted = scan;
		list_unique = g_list_prepend (list_unique, scan->data);
	}

	g_list_free (list);

	return list_unique;
}


static gboolean
archive_type_has_issues_extracting_non_empty_folders (FrArchive *archive)
{
	/*if ((archive->command->files == NULL) || (archive->command->files->len == 0))
		return FALSE;  FIXME: test with extract_here */
	return ! archive->command->propCanExtractNonEmptyFolders;
}


static gboolean
file_list_contains_files_in_this_dir (GList      *file_list,
				      const char *dirname)
{
	GList *scan;

	for (scan = file_list; scan; scan = scan->next) {
		char *filename = scan->data;

		if (path_in_path (dirname, filename))
			return TRUE;
	}

	return FALSE;
}


static GList*
remove_files_contained_in_this_dir (GList *file_list,
				    GList *dir_pointer)
{
	char  *dirname = dir_pointer->data;
	int    dirname_l = strlen (dirname);
	GList *scan;

	for (scan = dir_pointer->next; scan; /* empty */) {
		char *filename = scan->data;

		if (strncmp (dirname, filename, dirname_l) != 0)
			break;

		if (path_in_path (dirname, filename)) {
			GList *next = scan->next;

			file_list = g_list_remove_link (file_list, scan);
			g_list_free (scan);

			scan = next;
		}
		else
			scan = scan->next;
	}

	return file_list;
}


void
fr_archive_extract_to_local (FrArchive  *archive,
			     GList      *file_list,
			     const char *destination,
			     const char *base_dir,
			     gboolean    skip_older,
			     gboolean    overwrite,
			     gboolean    junk_paths,
			     const char *password)
{
	GList    *filtered;
	GList    *scan;
	gboolean  extract_all;
	gboolean  use_base_dir;
	gboolean  all_options_supported;
	gboolean  move_to_dest_dir;
	gboolean  file_list_created = FALSE;
g_print("dest: %s\n", destination);
	g_return_if_fail (archive != NULL);

	fr_archive_stoppable (archive, TRUE);
	g_object_set (archive->command, "file", archive->local_copy, NULL);

	/* if a command supports all the requested options use
	 * fr_command_extract directly. */

	use_base_dir = ! ((base_dir == NULL)
			  || (strcmp (base_dir, "") == 0)
			  || (strcmp (base_dir, "/") == 0));

	all_options_supported = (! use_base_dir
				 && ! (! overwrite && ! archive->command->propExtractCanAvoidOverwrite)
				 && ! (skip_older && ! archive->command->propExtractCanSkipOlder)
				 && ! (junk_paths && ! archive->command->propExtractCanJunkPaths));

	extract_all = (file_list == NULL);
	if (extract_all && (! all_options_supported || ! archive->command->propCanExtractAll)) {
		int i;

		file_list = NULL;
		for (i = 0; i < archive->command->files->len; i++) {
			FileData *fdata = g_ptr_array_index (archive->command->files, i);
			file_list = g_list_prepend (file_list, g_strdup (fdata->original_path));
		}
		file_list_created = TRUE;
	}

	if (extract_all && (file_list == NULL))
		fr_command_set_n_files (archive->command, archive->command->n_regular_files);
	else
		fr_command_set_n_files (archive->command, g_list_length (file_list));

	if (all_options_supported) {
		gboolean created_filtered_list = FALSE;

		if (! extract_all && archive_type_has_issues_extracting_non_empty_folders (archive)) {
			created_filtered_list = TRUE;
			filtered = g_list_copy (file_list);
			filtered = g_list_sort (filtered, (GCompareFunc) strcmp);
			for (scan = filtered; scan; scan = scan->next)
				filtered = remove_files_contained_in_this_dir (filtered, scan);
		}
		else
			filtered = file_list;

		if (! (created_filtered_list && (filtered == NULL)))
			extract_from_archive (archive,
					      filtered,
					      destination,
					      overwrite,
					      skip_older,
					      junk_paths,
					      password);

		if (created_filtered_list && (filtered != NULL))
			g_list_free (filtered);

		if (file_list_created)
			path_list_free (file_list);

		return;
	}

	/* .. else we have to implement the unsupported options. */

	move_to_dest_dir = (use_base_dir
			    || ((junk_paths
				 && ! archive->command->propExtractCanJunkPaths)));

	if (extract_all && ! file_list_created) {
		int i;

		file_list = NULL;
		for (i = 0; i < archive->command->files->len; i++) {
			FileData *fdata = g_ptr_array_index (archive->command->files, i);
			file_list = g_list_prepend (file_list, g_strdup (fdata->original_path));
		}

		file_list_created = TRUE;
	}

	filtered = NULL;
	for (scan = file_list; scan; scan = scan->next) {
		FileData   *fdata;
		char       *archive_list_filename = scan->data;
		char        dest_filename[4096];
		const char *filename;

        fdata = find_file_in_archive (archive, archive_list_filename);
g_print("  fn: %s, p:%p\n", archive_list_filename, fdata);
		if (fdata == NULL)
			continue;

		if (archive_type_has_issues_extracting_non_empty_folders (archive)
		    && fdata->dir
		    && file_list_contains_files_in_this_dir (file_list, archive_list_filename))
			continue;

		/* get the destination file path. */

		if (! junk_paths)
			filename = archive_list_filename;
		else
			filename = file_name_from_path (archive_list_filename);

		if ((destination[strlen (destination) - 1] == '/')
		    || (filename[0] == '/'))
			sprintf (dest_filename, "%s%s", destination, filename);
		else
			sprintf (dest_filename, "%s/%s", destination, filename);

		/*debug (DEBUG_INFO, "-> %s\n", dest_filename);*/
g_print("dest fn: %s\n", dest_filename);
		/**/

		if (! archive->command->propExtractCanSkipOlder
		    && skip_older
		    && g_file_test (dest_filename, G_FILE_TEST_EXISTS)
		    && (fdata->modified < get_file_mtime_for_path (dest_filename)))
			continue;

		if (! archive->command->propExtractCanAvoidOverwrite
		    && ! overwrite
		    && g_file_test (dest_filename, G_FILE_TEST_EXISTS))
			continue;

		filtered = g_list_prepend (filtered, fdata->original_path);
	}

	if (filtered == NULL) {
		/* all files got filtered, do nothing. */
		debug (DEBUG_INFO, "All files got filtered, nothing to do.\n");

		if (extract_all)
			path_list_free (file_list);

		fr_archive_action_completed (archive,
					     FR_ACTION_EXTRACTING_FILES,
					     FR_PROC_ERROR_NONE,
					     NULL);
		return;
	}

	if (move_to_dest_dir) {
		char *temp_dir;

		temp_dir = get_temp_work_dir (destination);
		extract_from_archive (archive,
				      filtered,
				      temp_dir,
				      overwrite,
				      skip_older,
				      junk_paths,
				      password);

		if (use_base_dir) {
			GList *tmp_list = compute_list_base_path (base_dir, filtered, junk_paths, archive->command->propExtractCanJunkPaths);
			g_list_free (filtered);
			filtered = tmp_list;
		}

		move_files_in_chunks (archive,
				      filtered,
				      temp_dir,
				      destination,
				      overwrite);

		/* remove the temp dir. */

		fr_process_begin_command (archive->process, "rm");
		fr_process_add_arg (archive->process, "-rf");
		fr_process_add_arg (archive->process, temp_dir);
		fr_process_end_command (archive->process);

		g_free (temp_dir);
	}
	else
		extract_from_archive (archive,
				      filtered,
				      destination,
				      overwrite,
				      skip_older,
				      junk_paths,
				      password);

	if (filtered != NULL)
		g_list_free (filtered);
	if (file_list_created)
		path_list_free (file_list);
}


void
fr_archive_extract (FrArchive  *archive,
		    GList      *file_list,
		    const char *destination,
		    const char *base_dir,
		    gboolean    skip_older,
		    gboolean    overwrite,
		    gboolean    junk_paths,
		    const char *password)
{
	g_free (archive->priv->extraction_destination);
	archive->priv->extraction_destination = g_strdup (destination);

	g_free (archive->priv->temp_extraction_dir);
	archive->priv->temp_extraction_dir = NULL;

	archive->priv->remote_extraction = ! uri_is_local (destination);
	if (archive->priv->remote_extraction) {
		archive->priv->temp_extraction_dir = get_temp_work_dir (NULL);
		fr_archive_extract_to_local (archive,
					     file_list,
					     archive->priv->temp_extraction_dir,
					     base_dir,
					     skip_older,
					     overwrite,
					     junk_paths,
					     password);
	}
	else {
		char *local_destination;

		local_destination = g_filename_from_uri (destination, NULL, NULL);
		fr_archive_extract_to_local (archive,
					     file_list,
					     local_destination,
					     base_dir,
					     skip_older,
					     overwrite,
					     junk_paths,
					     password);
		g_free (local_destination);
	}
}


static char *
get_desired_destination_for_archive (GFile *file)
{
	GFile      *directory;
	char       *directory_uri;
	char       *name;
	const char *ext;
	char       *new_name;
	char       *new_name_escaped;
	char       *desired_destination = NULL;

	directory = g_file_get_parent (file);
	directory_uri = g_file_get_uri (directory);

	name = g_file_get_basename (file);
	ext = get_archive_filename_extension (name);
	if (ext == NULL)
		/* if no extension is present add a suffix to the name... */
		new_name = g_strconcat (name, "_FILES", NULL);
	else
		/* ...else use the name without the extension */
		new_name = g_strndup (name, strlen (name) - strlen (ext));
	new_name_escaped = g_uri_escape_string (new_name, "", FALSE);

	desired_destination = g_strconcat (directory_uri, "/", new_name_escaped, NULL);

	g_free (new_name_escaped);
	g_free (new_name);
	g_free (name);
	g_free (directory_uri);
	g_object_unref (directory);

	return desired_destination;
}


static char *
get_extract_here_destination (GFile   *file,
			      GError **error)
{
	char  *desired_destination;
	char  *destination = NULL;
	int    n = 1;
	GFile *directory;

	desired_destination = get_desired_destination_for_archive (file);
	do {
		*error = NULL;

		g_free (destination);
		if (n == 1)
			destination = g_strdup (desired_destination);
		else
			destination = g_strdup_printf ("%s%%20(%d)", desired_destination, n);

		directory = g_file_new_for_uri (destination);
		g_file_make_directory (directory, NULL, error);
		g_object_unref (directory);

		n++;
	} while (g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_EXISTS));

	g_free (desired_destination);

	if (*error != NULL) {
		g_warning ("could not create destination folder: %s\n", (*error)->message);
		g_free (destination);
		destination = NULL;
	}

	return destination;
}


gboolean
fr_archive_extract_here (FrArchive  *archive,
			 gboolean    skip_older,
			 gboolean    overwrite,
			 gboolean    junk_path,
			 const char *password)
{
	char   *destination;
	GError *error = NULL;

	destination = get_extract_here_destination (archive->file, &error);
	if (error != NULL) {
		fr_archive_action_completed (archive,
					     FR_ACTION_EXTRACTING_FILES,
					     FR_PROC_ERROR_GENERIC,
					     error->message);
		g_clear_error (&error);
		return FALSE;
	}

	archive->priv->extract_here = TRUE;
	fr_archive_extract (archive,
			    NULL,
			    destination,
			    NULL,
			    skip_older,
			    overwrite,
			    junk_path,
			    password);

	g_free (destination);

	return TRUE;
}


const char *
fr_archive_get_last_extraction_destination (FrArchive *archive)
{
	return archive->priv->extraction_destination;
}


void
fr_archive_test (FrArchive  *archive,
		 const char *password)
{
	fr_archive_stoppable (archive, TRUE);

	g_object_set (archive->command,
		      "file", archive->local_copy,
		      "password", password,
		      NULL);
	fr_process_clear (archive->process);
	fr_command_set_n_files (archive->command, 0);
	fr_command_test (archive->command);
	fr_process_start (archive->process);
}


gboolean
uri_is_archive (const char *uri)
{
	GFile      *file;
	const char *mime_type;
	gboolean    is_archive = FALSE;

	file = g_file_new_for_uri (uri);
	mime_type = get_mime_type_from_magic_numbers (file);
	if (mime_type == NULL)
		mime_type = get_mime_type_from_content (file);
	if (mime_type == NULL)
		mime_type = get_mime_type_from_filename (file);

	if (mime_type != NULL) {
		int i;

		for (i = 0; mime_type_desc[i].mime_type != NULL; i++) {
			if (strcmp (mime_type_desc[i].mime_type, mime_type) == 0) {
				is_archive = TRUE;
				break;
			}
		}
	}
	g_object_unref (file);

	return is_archive;
}
