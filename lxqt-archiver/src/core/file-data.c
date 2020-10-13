/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001-2006 The Free Software Foundation, Inc.
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
#include "tr-wrapper.h"
#include <gio/gio.h>
#include "glib-utils.h"
#include "file-utils.h"
#include "file-data.h"


FileData *
file_data_new (void)
{
	FileData *fdata;

	fdata = g_new0 (FileData, 1);
	fdata->content_type = NULL;
	fdata->free_original_path = FALSE;
	fdata->dir_size = 0;

	return fdata;
}


void
file_data_free (FileData *fdata)
{
	if (fdata == NULL)
		return;
	if (fdata->free_original_path)
		g_free (fdata->original_path);
	g_free (fdata->full_path);
	g_free (fdata->name);
	g_free (fdata->path);
	g_free (fdata->link);
	g_free (fdata->list_name);
	g_free (fdata->sort_key);
	g_free (fdata);
}


FileData *
file_data_copy (FileData *src)
{
	FileData *fdata;

	fdata = g_new0 (FileData, 1);

	fdata->original_path = g_strdup (src->original_path);
	fdata->free_original_path = TRUE;

	fdata->full_path = g_strdup (src->full_path);
	fdata->link = g_strdup (src->link);
	fdata->size = src->size;
	fdata->modified = src->modified;
	fdata->name = g_strdup (src->name);
	fdata->path = g_strdup (src->path);
	fdata->content_type = src->content_type;
	fdata->encrypted = src->encrypted;
	fdata->dir = src->dir;
	fdata->dir_size = src->dir_size;

	fdata->list_dir = src->list_dir;
	fdata->list_name = g_strdup (src->list_name);
	fdata->sort_key = g_strdup (src->sort_key);

	return fdata;
}


GType
file_data_get_type (void)
{
	static GType type = 0;

	if (type == 0)
		type = g_boxed_type_register_static ("FRFileData", (GBoxedCopyFunc) file_data_copy, (GBoxedFreeFunc) file_data_free);

	return type;
}


void
file_data_update_content_type (FileData *fdata)
{
	if (fdata->dir)
		fdata->content_type = MIME_TYPE_DIRECTORY;
	else
		fdata->content_type = get_static_string (g_content_type_guess (fdata->full_path, NULL, 0, NULL));
}


gboolean
file_data_is_dir (FileData *fdata)
{
	return fdata->dir || fdata->list_dir;
}


void
file_data_set_list_name (FileData   *fdata,
			 const char *value)
{
	g_free (fdata->list_name);
	fdata->list_name = g_strdup (value);

	g_free (fdata->sort_key);
	if (fdata->list_name != NULL)
		fdata->sort_key = g_utf8_collate_key_for_filename (fdata->list_name, -1);
	else
		fdata->sort_key = NULL;
}


int
file_data_compare_by_path (gconstpointer a,
			   gconstpointer b)
{
	FileData *data_a = *((FileData **) a);
	FileData *data_b = *((FileData **) b);

	return strcmp (data_a->full_path, data_b->full_path);
}


int
find_path_in_file_data_array (GPtrArray  *array,
			      const char *path)
{
	int       path_l;
	int       left, right, p, cmp = -1;
	FileData *fd;

	if (path == NULL)
		return -1;

	path_l = strlen (path);
	left = 0;
	right = array->len;
	while (left < right) {
		p = left + ((right - left) / 2);
		fd = (FileData *) g_ptr_array_index (array, p);

		cmp = strcmp (path, fd->original_path);
		if (cmp != 0) {
			/* consider '/path/to/dir' and '/path/to/dir/' the same path */

			int original_path_l = strlen (fd->original_path);
			if ((path_l == original_path_l - 1) && (fd->original_path[original_path_l - 1] == '/')) {
				int cmp2 = strncmp (path, fd->original_path, original_path_l - 1);
				if (cmp2 == 0)
					cmp = cmp2;
			}
		}

		if (cmp == 0)
			return p;
		else if (cmp < 0)
			right = p;
		else
			left = p + 1;
	}

	return -1;
}
