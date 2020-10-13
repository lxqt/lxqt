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

#ifndef FILE_DATA_H
#define FILE_DATA_H

#include <glib.h>
#include <glib-object.h>
#include <time.h>

G_BEGIN_DECLS

typedef struct {
	char       *original_path;    /* path read from command line. */
	char       *full_path;        /* "/" + original_path. */
	char       *link;
	goffset     size;
	time_t      modified;

	char       *name;             /* The file name. */
	char       *path;             /* The directory. */
	gboolean    encrypted;        /* Whether the file is encrypted. */
	gboolean    dir;              /* Whether this is a directory listed in the archive */
	goffset     dir_size;
	const char *content_type;

	/* Additional data. */

	gboolean    list_dir;         /* Whether this entry is used to show
				       * a directory. */
	char       *list_name;        /* The string visualized in the list
				       * view. */
	char       *sort_key;

	/* Private data */

	gboolean    free_original_path;
} FileData;

#define FR_TYPE_FILE_DATA (file_data_get_type ())

GType           file_data_get_type            (void);
FileData *      file_data_new                 (void);
FileData *      file_data_copy                (FileData      *src);
void            file_data_free                (FileData      *fdata);
void            file_data_update_content_type (FileData      *fdata);
gboolean        file_data_is_dir              (FileData      *fdata);
void            file_data_set_list_name       (FileData      *fdata,
					       const char    *value);
int  file_data_compare_by_path                (gconstpointer  a,
				               gconstpointer  b);
int  find_path_in_file_data_array             (GPtrArray     *array,
				               const char    *path);

G_END_DECLS

#endif /* FILE_DATA_H */
