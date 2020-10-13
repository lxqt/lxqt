/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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

#ifndef FR_ARCHIVE_H
#define FR_ARCHIVE_H

#include <glib.h>

G_BEGIN_DECLS

#include "fr-process.h"
#include "fr-command.h"

#define FR_TYPE_ARCHIVE            (fr_archive_get_type ())
#define FR_ARCHIVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_ARCHIVE, FrArchive))
#define FR_ARCHIVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_ARCHIVE, FrArchiveClass))
#define FR_IS_ARCHIVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_ARCHIVE))
#define FR_IS_ARCHIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_ARCHIVE))
#define FR_ARCHIVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_ARCHIVE, FrArchiveClass))

typedef struct _FrArchive         FrArchive;
typedef struct _FrArchiveClass    FrArchiveClass;
typedef struct _FrArchivePrivData FrArchivePrivData;

typedef gboolean (*FakeLoadFunc) (FrArchive *archive, gpointer data);

struct _FrArchive {
	GObject  __parent;

	GFile       *file;
	GFile       *local_copy;
	gboolean     is_remote;
	const char  *content_type;
	FrCommand   *command;
	FrProcess   *process;
	FrProcError  error;
	gboolean     can_create_compressed_file;
	gboolean     is_compressed_file;         /* Whether the file is not an
						  * archive that can contain
						  * many files but simply a
						  * compressed file, for
						  * example foo.txt.gz is a
						  * compressed file, foo.zip
						  * is not. */
	gboolean     read_only;                  /* Whether archive is
						  * read-only for whatever
						  * reason. */
	gboolean     have_permissions;           /* true if we have the
						  * permissions to write the
						  * file. */

	FrArchivePrivData *priv;
};

struct _FrArchiveClass {
	GObjectClass __parent_class;

	/* -- Signals -- */

	void (*start)            (FrArchive   *archive,
			          FrAction     action);
	void (*done)             (FrArchive   *archive,
			          FrAction     action,
			          FrProcError *error);
	void (*progress)         (FrArchive   *archive,
			          double       fraction);
	void (*message)          (FrArchive   *archive,
			          const char  *msg);
	void (*stoppable)        (FrArchive   *archive,
			          gboolean     value);
	void (*working_archive)  (FrCommand   *comm,
			          const char  *filename);
};

GType       fr_archive_get_type                  (void);
FrArchive * fr_archive_new                       (void);
void        fr_archive_set_fake_load_func        (FrArchive       *archive,
						  FakeLoadFunc     func,
						  gpointer         data);
gboolean    fr_archive_fake_load                 (FrArchive       *archive);
void        fr_archive_stoppable                 (FrArchive       *archive,
						  gboolean         stoppable);
void        fr_archive_stop	                 (FrArchive       *archive);
void        fr_archive_action_completed          (FrArchive       *archive,
						  FrAction         action,
						  FrProcErrorType  error_type,
						  const char      *error_details);

/**/

gboolean    fr_archive_create                    (FrArchive       *archive,
						  const char      *uri);
gboolean    fr_archive_load                      (FrArchive       *archive,
						  const char      *uri,
						  const char      *password);
gboolean    fr_archive_load_local                (FrArchive       *archive,
		       				  const char      *uri,
		       				  const char      *password);
void        fr_archive_reload                    (FrArchive       *archive,
						  const char      *password);

/**/

void        fr_archive_add                       (FrArchive       *archive,
						  GList           *file_list,
						  const char      *base_dir,
						  const char      *dest_dir,
						  gboolean         update,
						  gboolean         recursive,
						  const char      *password,
						  gboolean         encrypt_header,
						  FrCompression    compression,
						  guint            volume_size);
void        fr_archive_remove                    (FrArchive       *archive,
						  GList           *file_list,
						  FrCompression    compression);
void        fr_archive_extract                   (FrArchive       *archive,
						  GList           *file_list,
						  const char      *dest_uri,
						  const char      *base_dir,
						  gboolean         skip_older,
						  gboolean         overwrite,
						  gboolean         junk_path,
						  const char      *password);
void        fr_archive_extract_to_local          (FrArchive       *archive,
						  GList           *file_list,
						  const char      *dest_path,
						  const char      *base_dir,
						  gboolean         skip_older,
						  gboolean         overwrite,
						  gboolean         junk_path,
						  const char      *password);
gboolean    fr_archive_extract_here              (FrArchive       *archive,
						  gboolean         skip_older,
						  gboolean         overwrite,
						  gboolean         junk_path,
						  const char      *password);
const char *fr_archive_get_last_extraction_destination
						 (FrArchive       *archive);

/**/

void        fr_archive_add_files                 (FrArchive       *archive,
						  GList           *file_list,
						  const char      *base_dir,
						  const char      *dest_dir,
						  gboolean         update,
						  const char      *password,
						  gboolean         encrypt_header,
						  FrCompression    compression,
						  guint            volume_size);
void        fr_archive_add_with_wildcard         (FrArchive       *archive,
						  const char      *include_files,
						  const char      *exclude_files,
						  const char      *exclude_folders,
						  const char      *base_dir,
						  const char      *dest_dir,
						  gboolean         update,
						  gboolean         follow_links,
						  const char      *password,
						  gboolean         encrypt_header,
						  FrCompression    compression,
						  guint            volume_size);
void        fr_archive_add_directory             (FrArchive       *archive,
						  const char      *directory,
						  const char      *base_dir,
						  const char      *dest_dir,
						  gboolean         update,
						  const char      *password,
						  gboolean         encrypt_header,
						  FrCompression    compression,
						  guint            volume_size);
void        fr_archive_add_items                 (FrArchive       *archive,
						  GList           *item_list,
						  const char      *base_dir,
						  const char      *dest_dir,
						  gboolean         update,
						  const char      *password,
						  gboolean         encrypt_header,
						  FrCompression    compression,
						  guint            volume_size);
void        fr_archive_add_dropped_items         (FrArchive       *archive,
						  GList           *item_list,
						  const char      *base_dir,
						  const char      *dest_dir,
						  gboolean         update,
						  const char      *password,
						  gboolean         encrypt_header,
						  FrCompression    compression,
						  guint            volume_size);
void        fr_archive_test                      (FrArchive       *archive,
						  const char      *password);

/* utilities */

gboolean    uri_is_archive                       (const char      *uri);

G_END_DECLS

#endif /* FR_ARCHIVE_H */
