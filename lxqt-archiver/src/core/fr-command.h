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

#ifndef FR_COMMAND_H
#define FR_COMMAND_H

#include <glib.h>
#include <gio/gio.h>

#include "file-data.h"
#include "fr-process.h"

#define PACKAGES(x) (x)

#define FR_TYPE_COMMAND            (fr_command_get_type ())
#define FR_COMMAND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_COMMAND, FrCommand))
#define FR_COMMAND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_COMMAND, FrCommandClass))
#define FR_IS_COMMAND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_COMMAND))
#define FR_IS_COMMAND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_COMMAND))
#define FR_COMMAND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_COMMAND, FrCommandClass))

typedef struct _FrCommand         FrCommand;
typedef struct _FrCommandClass    FrCommandClass;

typedef enum {
	FR_ACTION_NONE,
	FR_ACTION_CREATING_NEW_ARCHIVE,
	FR_ACTION_LOADING_ARCHIVE,            /* loading the archive from a remote location */
	FR_ACTION_LISTING_CONTENT,            /* listing the content of the archive */
	FR_ACTION_DELETING_FILES,             /* deleting files from the archive */
	FR_ACTION_TESTING_ARCHIVE,            /* testing the archive integrity */
	FR_ACTION_GETTING_FILE_LIST,          /* getting the file list (when fr_archive_add_with_wildcard or
						 fr_archive_add_directory are used, we need to scan a directory
						 and collect the files to add to the archive, this
						 may require some time to complete, so the operation
						 is asynchronous) */
	FR_ACTION_COPYING_FILES_FROM_REMOTE,  /* copying files to be added to the archive from a remote location */
	FR_ACTION_ADDING_FILES,               /* adding files to an archive */
	FR_ACTION_EXTRACTING_FILES,           /* extracting files */
	FR_ACTION_COPYING_FILES_TO_REMOTE,    /* copying extracted files to a remote location */
	FR_ACTION_CREATING_ARCHIVE,           /* creating a local archive */
	FR_ACTION_SAVING_REMOTE_ARCHIVE       /* copying the archive to a remote location */
} FrAction;

#ifdef DEBUG
extern char *action_names[];
#endif

struct _FrCommand
{
	GObject  __parent;

	/*<public, read only>*/

	GPtrArray     *files;           /* Array of FileData* */
	int            n_regular_files;
	FrProcess     *process;         /* the process object used to execute
				         * commands. */
	char          *filename;        /* archive file path. */
	char          *e_filename;      /* escaped archive filename. */
	const char    *mime_type;
	gboolean       multi_volume;

	/*<protected>*/

	/* options */

	char          *password;
	gboolean       encrypt_header : 1;
	FrCompression  compression;
	guint          volume_size;
	gboolean       creating_archive;

	/* features. */

	guint          propAddCanUpdate : 1;
	guint          propAddCanReplace : 1;
	guint          propAddCanStoreFolders : 1;
	guint          propExtractCanAvoidOverwrite : 1;
	guint          propExtractCanSkipOlder : 1;
	guint          propExtractCanJunkPaths : 1;
	guint          propPassword : 1;
	guint          propTest : 1;
	guint          propCanExtractAll : 1;
	guint          propCanDeleteNonEmptyFolders : 1;
	guint          propCanExtractNonEmptyFolders : 1;
	guint          propListFromFile : 1;

	/*<private>*/

	FrCommandCaps  capabilities;
	FrAction       action;        /* current action. */
	gboolean       fake_load;     /* if TRUE does nothing when the list
				       * operation is invoked. */

	/* progress data */

	int            n_file;
	int            n_files;
};

struct _FrCommandClass
{
	GObjectClass __parent_class;

	/*<virtual functions>*/

	void          (*list)             (FrCommand     *comm);
	void          (*add)              (FrCommand     *comm,
					   const char    *from_file,
				           GList         *file_list,
				           const char    *base_dir,
				           gboolean       update,
				           gboolean       recursive);
	void          (*delete_)           (FrCommand     *comm,
			                   const char    *from_file,
				           GList         *file_list);
	void          (*extract)          (FrCommand     *comm,
			                   const char    *from_file,
				           GList         *file_list,
				           const char    *dest_dir,
				           gboolean       overwrite,
				           gboolean       skip_older,
				           gboolean       junk_paths);
	void          (*test)             (FrCommand     *comm);
	void          (*uncompress)       (FrCommand     *comm);
	void          (*recompress)       (FrCommand     *comm);
	void          (*handle_error)     (FrCommand     *comm,
				           FrProcError   *error);
	const char ** (*get_mime_types)   (FrCommand     *comm);
	FrCommandCap  (*get_capabilities) (FrCommand     *comm,
					   const char    *mime_type,
					   gboolean       check_command);
	void          (*set_mime_type)    (FrCommand     *comm,
				           const char    *mime_type);
	const char *  (*get_packages)     (FrCommand     *comm,
					   const char    *mime_type);

	/*<signals>*/

	void          (*start)            (FrCommand   *comm,
			 	           FrAction     action);
	void          (*done)             (FrCommand   *comm,
				           FrAction     action,
				           FrProcError *error);
	void          (*progress)         (FrCommand   *comm,
				           double       fraction);
	void          (*message)          (FrCommand   *comm,
				           const char  *msg);
	void          (*working_archive)  (FrCommand   *comm,
					   const char  *filename);
};

GType          fr_command_get_type            (void);
void           fr_command_set_file            (FrCommand     *comm,
					       GFile         *file);
void           fr_command_set_multi_volume    (FrCommand     *comm,
					       GFile         *file);
void           fr_command_list                (FrCommand     *comm);
void           fr_command_add                 (FrCommand     *comm,
					       const char    *from_file,
					       GList         *file_list,
					       const char    *base_dir,
					       gboolean       update,
					       gboolean       recursive);
void           fr_command_delete              (FrCommand     *comm,
					       const char    *from_file,
					       GList         *file_list);
void           fr_command_extract             (FrCommand     *comm,
					       const char    *from_file,
					       GList         *file_list,
					       const char    *dest_dir,
					       gboolean       overwrite,
					       gboolean       skip_older,
					       gboolean       junk_paths);
void           fr_command_test                (FrCommand     *comm);
void           fr_command_uncompress          (FrCommand     *comm);
void           fr_command_recompress          (FrCommand     *comm);
gboolean       fr_command_is_capable_of       (FrCommand     *comm,
					       FrCommandCaps  capabilities);
const char **  fr_command_get_mime_types      (FrCommand     *comm);
void           fr_command_update_capabilities (FrCommand     *comm);
FrCommandCap   fr_command_get_capabilities    (FrCommand     *comm,
					       const char    *mime_type,
					       gboolean       check_command);
void           fr_command_set_mime_type       (FrCommand     *comm,
					       const char    *mime_type);
gboolean       fr_command_is_capable_of       (FrCommand     *comm,
					       FrCommandCaps  capabilities);
const char *   fr_command_get_packages        (FrCommand     *comm,
					       const char    *mime_type);

/* protected functions */

void           fr_command_progress            (FrCommand     *comm,
					       double         fraction);
void           fr_command_message             (FrCommand     *comm,
					       const char    *msg);
void           fr_command_working_archive     (FrCommand     *comm,
		                               const char    *archive_name);
void           fr_command_set_n_files         (FrCommand     *comm,
					       int            n_files);
void           fr_command_add_file            (FrCommand     *comm,
					       FileData      *fdata);

/* private functions */

void           fr_command_handle_error        (FrCommand     *comm,
					       FrProcError   *error);

#endif /* FR_COMMAND_H */
