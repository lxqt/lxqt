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

#include <config.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>

#include <glib.h>
#include <gio/gio.h>
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-init.h"


#ifndef HAVE_MKDTEMP
#include "mkdtemp.h"
#endif

#define BUF_SIZE 4096
#define FILE_PREFIX    "file://"
#define FILE_PREFIX_L  7
#define SPECIAL_DIR(x) ((strcmp ((x), "..") == 0) || (strcmp ((x), ".") == 0))


gboolean
uri_exists (const char *uri)
{
	GFile     *file;
	gboolean   exists;

	if (uri == NULL)
		return FALSE;

	file = g_file_new_for_uri (uri);
	exists = g_file_query_exists (file, NULL);
	g_object_unref (file);

	return exists;
}


static gboolean
uri_is_filetype (const char *uri,
		 GFileType   file_type)
{
	gboolean   result = FALSE;
	GFile     *file;
	GFileInfo *info;
	GError    *error = NULL;

	file = g_file_new_for_uri (uri);

	if (! g_file_query_exists (file, NULL)) {
		g_object_unref (file);
		return FALSE;
	}

	info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_TYPE, 0, NULL, &error);
	if (error == NULL) {
		result = (g_file_info_get_file_type (info) == file_type);
	}
	else {
		g_warning ("Failed to get file type for uri %s: %s", uri, error->message);
		g_error_free (error);
	}

	g_object_unref (info);
	g_object_unref (file);

	return result;
}


gboolean
uri_is_file (const char *uri)
{
	return uri_is_filetype (uri, G_FILE_TYPE_REGULAR);
}


gboolean
uri_is_dir (const char *uri)
{
	return uri_is_filetype (uri, G_FILE_TYPE_DIRECTORY);
}


gboolean
path_is_dir (const char *path)
{
	char     *uri;
	gboolean  result;

	uri = g_filename_to_uri (path, NULL, NULL);
	result = uri_is_dir (uri);
	g_free (uri);

	return result;
}

gboolean
uri_is_local (const char  *uri)
{
	return strncmp (uri, "file://", 7) == 0;
}


gboolean
dir_is_empty (const char *uri)
{
	GFile           *file;
	GFileEnumerator *file_enum;
	GFileInfo       *info;
	GError          *error = NULL;
	int              n = 0;

	file = g_file_new_for_uri (uri);

	if (! g_file_query_exists (file, NULL)) {
		g_object_unref (file);
		return TRUE;
	}

	file_enum = g_file_enumerate_children (file, G_FILE_ATTRIBUTE_STANDARD_NAME, 0, NULL, &error);
	if (error != NULL) {
		g_warning ("Failed to enumerate children of %s: %s", uri, error->message);
		g_error_free (error);
		g_object_unref (file_enum);
		g_object_unref (file);
		return TRUE;
	}

	while ((n == 0) && ((info = g_file_enumerator_next_file (file_enum, NULL, &error)) != NULL)) {
		if (error != NULL) {
			g_warning ("Encountered error while enumerating children of %s (ignoring): %s", uri, error->message);
			g_error_free (error);
		}
		else if (! SPECIAL_DIR (g_file_info_get_name (info)))
			n++;
		g_object_unref (info);
	}

	g_object_unref (file);
	g_object_unref (file_enum);

	return (n == 0);
}


gboolean
dir_contains_one_object (const char *uri)
{
	GFile           *file;
	GFileEnumerator *file_enum;
	GFileInfo       *info;
	GError          *err = NULL;
	int              n = 0;

	file = g_file_new_for_uri (uri);

	if (! g_file_query_exists (file, NULL)) {
		g_object_unref (file);
		return FALSE;
	}

	file_enum = g_file_enumerate_children (file, G_FILE_ATTRIBUTE_STANDARD_NAME, 0, NULL, &err);
	if (err != NULL) {
		g_warning ("Failed to enumerate children of %s: %s", uri, err->message);
		g_error_free (err);
		g_object_unref (file_enum);
		g_object_unref (file);
		return FALSE;
	}

	while ((info = g_file_enumerator_next_file (file_enum, NULL, &err)) != NULL) {
		const char *name;

		if (err != NULL) {
			g_warning ("Encountered error while enumerating children of %s, ignoring: %s", uri, err->message);
			g_error_free (err);
			g_object_unref (info);
			continue;
		}

		name = g_file_info_get_name (info);
		if (strcmp (name, ".") == 0 || strcmp (name, "..") == 0) {
			g_object_unref (info);
 			continue;
		}

		g_object_unref (info);

		if (++n > 1)
			break;
	}

	g_object_unref (file);
	g_object_unref (file_enum);

	return (n == 1);
}


char *
get_dir_content_if_unique (const char  *uri)
{
	GFile           *file;
	GFileEnumerator *file_enum;
	GFileInfo       *info;
	GError          *err = NULL;
	char            *content_uri = NULL;

	file = g_file_new_for_uri (uri);

	if (! g_file_query_exists (file, NULL)) {
		g_object_unref (file);
		return NULL;
	}

	file_enum = g_file_enumerate_children (file, G_FILE_ATTRIBUTE_STANDARD_NAME, 0, NULL, &err);
	if (err != NULL) {
		g_warning ("Failed to enumerate children of %s: %s", uri, err->message);
		g_error_free (err);
		return NULL;
	}

	while ((info = g_file_enumerator_next_file (file_enum, NULL, &err)) != NULL) {
		const char *name;

		if (err != NULL) {
			g_warning ("Failed to get info while enumerating children: %s", err->message);
			g_clear_error (&err);
			g_object_unref (info);
			continue;
		}

		name = g_file_info_get_name (info);
		if ((strcmp (name, ".") == 0) || (strcmp (name, "..") == 0)) {
			g_object_unref (info);
			continue;
		}

		if (content_uri != NULL) {
			g_free (content_uri);
			g_object_unref (info);
			content_uri = NULL;
			break;
		}

		content_uri = build_uri (uri, name, NULL);
		g_object_unref (info);
	}

	if (err != NULL) {
		g_warning ("Failed to get info after enumerating children: %s", err->message);
		g_clear_error (&err);
	}

	g_object_unref (file_enum);
	g_object_unref (file);

	return content_uri;
}


/* Check whether the dirname is contained in filename */
gboolean
path_in_path (const char *dirname,
	      const char *filename)
{
	int dirname_l, filename_l, separator_position;

	if ((dirname == NULL) || (filename == NULL))
		return FALSE;

	dirname_l = strlen (dirname);
	filename_l = strlen (filename);

	if ((dirname_l == filename_l + 1)
	     && (dirname[dirname_l - 1] == '/'))
		return FALSE;

	if ((filename_l == dirname_l + 1)
	     && (filename[filename_l - 1] == '/'))
		return FALSE;

	if (dirname[dirname_l - 1] == '/')
		separator_position = dirname_l - 1;
	else
		separator_position = dirname_l;

	return ((filename_l > dirname_l)
		&& (strncmp (dirname, filename, dirname_l) == 0)
		&& (filename[separator_position] == '/'));
}


goffset
get_file_size (const char *uri)
{
	goffset    size = 0;
	GFile     *file;
	GFileInfo *info;
	GError    *err = NULL;

	if ((uri == NULL) || (*uri == '\0'))
		return 0;

	file = g_file_new_for_uri (uri);
	info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE, 0, NULL, &err);
	if (err == NULL) {
		size = g_file_info_get_size (info);
	}
	else {
		g_warning ("Failed to get file size for %s: %s", uri, err->message);
		g_error_free (err);
	}

	g_object_unref (info);
	g_object_unref (file);

	return size;
}


goffset
get_file_size_for_path (const char *path)
{
	char    *uri;
	goffset  result;

	uri = g_filename_to_uri (path, NULL, NULL);
	result = get_file_size (uri);
	g_free (uri);

	return result;
}


static time_t
get_file_time_type (const char *uri,
		    const char *type)
{
	time_t     result = 0;
	GFile     *file;
	GFileInfo *info;
	GError    *err = NULL;

	if ((uri == NULL) || (*uri == '\0'))
 		return 0;

	file = g_file_new_for_uri (uri);
	info = g_file_query_info (file, type, 0, NULL, &err);
	if (err == NULL) {
		result = (time_t) g_file_info_get_attribute_uint64 (info, type);
	}
	else {
		g_warning ("Failed to get %s: %s", type, err->message);
		g_error_free (err);
		result = 0;
	}

	g_object_unref (info);
	g_object_unref (file);

	return result;
}


time_t
get_file_mtime (const char *uri)
{
	return get_file_time_type (uri, G_FILE_ATTRIBUTE_TIME_MODIFIED);
}


time_t
get_file_mtime_for_path (const char *path)
{
	char   *uri;
	time_t  result;

	uri = g_filename_to_uri (path, NULL, NULL);
	result = get_file_mtime (uri);
	g_free (uri);

	return result;
}


time_t
get_file_ctime (const char *uri)
{
	return get_file_time_type (uri, G_FILE_ATTRIBUTE_TIME_CREATED);
}


gboolean
file_is_hidden (const gchar *name)
{
	if (name[0] != '.') return FALSE;
	if (name[1] == '\0') return FALSE;
	if ((name[1] == '.') && (name[2] == '\0')) return FALSE;

	return TRUE;
}


/* like g_path_get_basename but does not warn about NULL and does not
 * alloc a new string. */
const gchar* file_name_from_path(const gchar *file_name)
{
	register char   *base;
	register gssize  last_char;

	if (file_name == NULL)
		return NULL;

	if (file_name[0] == '\0')
		return "";

	last_char = strlen (file_name) - 1;

	if (file_name [last_char] == G_DIR_SEPARATOR)
		return "";

	base = g_utf8_strrchr (file_name, -1, G_DIR_SEPARATOR);
	if (! base)
		return file_name;

	return base + 1;
}


char *
dir_name_from_path (const gchar *path)
{
	register gssize base;
	register gssize last_char;

	if (path == NULL)
		return NULL;

	if (path[0] == '\0')
		return g_strdup ("");

	last_char = strlen (path) - 1;
	if (path[last_char] == G_DIR_SEPARATOR)
		last_char--;

	base = last_char;
	while ((base >= 0) && (path[base] != G_DIR_SEPARATOR))
		base--;

	return g_strndup (path + base + 1, last_char - base);
}


gchar *
remove_level_from_path (const gchar *path)
{
	int         p;
	const char *ptr = path;
	char       *new_path;

	if (path == NULL)
		return NULL;

	p = strlen (path) - 1;
	if (p < 0)
		return NULL;

	while ((p > 0) && (ptr[p] != '/'))
		p--;
	if ((p == 0) && (ptr[p] == '/'))
		p++;
	new_path = g_strndup (path, (guint)p);

	return new_path;
}


char *
remove_ending_separator (const char *path)
{
	gint len, copy_len;

	if (path == NULL)
		return NULL;

	copy_len = len = strlen (path);
	if ((len > 1) && (path[len - 1] == '/'))
		copy_len--;

	return g_strndup (path, copy_len);
}


char *
build_uri (const char *base, ...)
{
	va_list     args;
	const char *child;
	GString    *uri;

	uri = g_string_new (base);

	va_start (args, base);
        while ((child = va_arg (args, const char *)) != NULL) {
        	if (! g_str_has_suffix (uri->str, "/") && ! g_str_has_prefix (child, "/"))
        		g_string_append (uri, "/");
        	g_string_append (uri, child);
        }
	va_end (args);

	return g_string_free (uri, FALSE);
}


gchar *
remove_extension_from_path (const gchar *path)
{
	int         len;
	int         p;
	const char *ptr = path;
	char       *new_path;

	if (! path)
		return NULL;

	len = strlen (path);
	if (len == 1)
		return g_strdup (path);

	p = len - 1;
	while ((p > 0) && (ptr[p] != '.'))
		p--;
	if (p == 0)
		p = len;
	new_path = g_strndup (path, (guint) p);

	return new_path;
}


gboolean
make_directory_tree (GFile    *dir,
		     mode_t    mode,
		     GError  **error)
{
	gboolean  success = TRUE;
	GFile    *parent;

	if ((dir == NULL) || g_file_query_exists (dir, NULL))
		return TRUE;

	parent = g_file_get_parent (dir);
	if (parent != NULL) {
		success = make_directory_tree (parent, mode, error);
		g_object_unref (parent);
		if (! success)
			return FALSE;
	}

	success = g_file_make_directory (dir, NULL, error);
	if ((error != NULL) && (*error != NULL) && g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
		g_clear_error (error);
		success = TRUE;
	}

	if (success)
		g_file_set_attribute_uint32 (dir,
					     G_FILE_ATTRIBUTE_UNIX_MODE,
					     mode,
					     0,
					     NULL,
					     NULL);

	return success;
}


gboolean
ensure_dir_exists (const char  *uri,
		   mode_t       mode,
		   GError     **error)
{
	GFile  *dir;
	GError *priv_error = NULL;

	if (uri == NULL)
		return FALSE;

	if (error == NULL)
		error = &priv_error;

	dir = g_file_new_for_uri (uri);
	if (! make_directory_tree (dir, mode, error)) {
		g_warning ("could create directory %s: %s", uri, (*error)->message);
		if (priv_error != NULL)
			g_clear_error (&priv_error);
		return FALSE;
	}

	return TRUE;
}


gboolean
make_directory_tree_from_path (const char  *path,
		   	       mode_t       mode,
		   	       GError     **error)
{
	char     *uri;
	gboolean  result;

	uri = g_filename_to_uri (path, NULL, NULL);
	result = ensure_dir_exists (uri, mode, error);
	g_free (uri);

	return result;
}


const char *
get_file_extension (const char *filename)
{
	const char *ptr = filename;
	int         len;
	int         p;
	const char *ext;

	if (filename == NULL)
		return NULL;

	len = strlen (filename);
	if (len <= 1)
		return NULL;

	p = len - 1;
	while ((p >= 0) && (ptr[p] != '.'))
		p--;
	if (p < 0)
		return NULL;

	ext = filename + p;
	if (ext - 4 > filename) {
		const char *test = ext - 4;
		if (strncmp (test, ".tar", 4) == 0)
			ext = ext - 4;
	}
	return ext;
}


gboolean
file_extension_is (const char *filename,
		   const char *ext)
{
	int filename_l, ext_l;

	filename_l = strlen (filename);
	ext_l = strlen (ext);

	if (filename_l < ext_l)
		return FALSE;
	return strcasecmp (filename + filename_l - ext_l, ext) == 0;
}


gboolean
is_mime_type (const char *mime_type,
	      const char *pattern)
{
	return (strcasecmp (mime_type, pattern) == 0);
}


const char*
get_file_mime_type (const char *uri,
                    gboolean    fast_file_type)
{
	GFile      *file;
	GFileInfo  *info;
	GError     *err = NULL;
 	const char *result = NULL;

 	file = g_file_new_for_uri (uri);
	info = g_file_query_info (file,
				  fast_file_type ?
				  G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE :
				  G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				  0, NULL, &err);
	if (info == NULL) {
		g_warning ("could not get content type for %s: %s", uri, err->message);
		g_clear_error (&err);
	}
	else {
		result = get_static_string (g_file_info_get_content_type (info));
		g_object_unref (info);
	}

	g_object_unref (file);

	return result;
}


const char*
get_file_mime_type_for_path (const char  *filename,
                    	     gboolean     fast_file_type)
{
	char       *uri;
	const char *mime_type;

	uri = g_filename_to_uri (filename, NULL, NULL);
	mime_type = get_file_mime_type (uri, fast_file_type);
	g_free (uri);

	return mime_type;
}


void
path_list_free (GList *path_list)
{
	if (path_list == NULL)
		return;
	g_list_foreach (path_list, (GFunc) g_free, NULL);
	g_list_free (path_list);
}


GList *
path_list_dup (GList *path_list)
{
	GList *new_list = NULL;
	GList *scan;

	for (scan = path_list; scan; scan = scan->next)
		new_list = g_list_prepend (new_list, g_strdup (scan->data));

	return g_list_reverse (new_list);
}


guint64
get_dest_free_space (const char *path)
{
	guint64    freespace = 0;
	GFile     *file;
	GFileInfo *info;
	GError    *err = NULL;

	file = g_file_new_for_path (path);
	info = g_file_query_filesystem_info (file, G_FILE_ATTRIBUTE_FILESYSTEM_FREE, NULL, &err);
	if (info != NULL) {
		freespace = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
		g_object_unref (info);
	}
	else {
		g_warning ("Could not get filesystem free space on volume that contains %s: %s", path, err->message);
		g_error_free (err);
	}
	g_object_unref (file);

	return freespace;
}


static gboolean
delete_directory_recursive (GFile   *dir,
			    GError **error)
{
	char            *uri;
	GFileEnumerator *file_enum;
	GFileInfo       *info;
	gboolean         error_occurred = FALSE;

	if (error != NULL)
		*error = NULL;

	file_enum = g_file_enumerate_children (dir,
					       G_FILE_ATTRIBUTE_STANDARD_NAME ","
					       G_FILE_ATTRIBUTE_STANDARD_TYPE,
					       0, NULL, error);

	uri = g_file_get_uri (dir);
	while (! error_occurred && (info = g_file_enumerator_next_file (file_enum, NULL, error)) != NULL) {
		char  *child_uri;
		GFile *child;

		child_uri = build_uri (uri, g_file_info_get_name (info), NULL);
		child = g_file_new_for_uri (child_uri);

		switch (g_file_info_get_file_type (info)) {
		case G_FILE_TYPE_DIRECTORY:
			if (! delete_directory_recursive (child, error))
				error_occurred = TRUE;
			break;
		default:
			if (! g_file_delete (child, NULL, error))
				error_occurred = TRUE;
			break;
		}

		g_object_unref (child);
		g_free (child_uri);
		g_object_unref (info);
	}
	g_free (uri);

	if (! error_occurred && ! g_file_delete (dir, NULL, error))
 		error_occurred = TRUE;

	g_object_unref (file_enum);

	return ! error_occurred;
}


gboolean
remove_directory (const char *uri)
{
	GFile     *dir;
	gboolean   result;
	GError    *error = NULL;

	dir = g_file_new_for_uri (uri);
	result = delete_directory_recursive (dir, &error);
	if (! result) {
		g_warning ("Cannot delete %s: %s", uri, error->message);
		g_clear_error (&error);
	}
	g_object_unref (dir);

	return result;
}


gboolean
remove_local_directory (const char *path)
{
	char     *uri;
	gboolean  result;

	if (path == NULL)
		return TRUE;

	uri = g_filename_to_uri (path, NULL, NULL);
	result = remove_directory (uri);
	g_free (uri);

	return result;
}


static const char *try_folder[] = { "cache", "~", "tmp", NULL };


static char *
ith_temp_folder_to_try (int n)
{
	const char *folder;

	folder = try_folder[n];
	if (strcmp (folder, "cache") == 0)
		folder = g_get_user_cache_dir ();
	else if (strcmp (folder, "~") == 0)
		folder = g_get_home_dir ();
	else if (strcmp (folder, "tmp") == 0)
		folder = g_get_tmp_dir ();

	return g_strdup (folder);
}


char *
get_temp_work_dir (const char *parent_folder)
{
	guint64  max_size = 0;
	char    *best_folder = NULL;
	int      i;
	char    *template;
	char    *result = NULL;

	if (parent_folder == NULL) {
		/* find the folder with more free space. */

		for (i = 0; try_folder[i] != NULL; i++) {
			char    *folder;
			guint64  size;

			folder = ith_temp_folder_to_try (i);
			size = get_dest_free_space (folder);
			if (max_size < size) {
				max_size = size;
				g_free (best_folder);
				best_folder = folder;
			}
			else
				g_free (folder);
		}
	}
	else
		best_folder = g_strdup (parent_folder);

	if (best_folder == NULL)
		return NULL;

	template = g_strconcat (best_folder, "/.fr-XXXXXX", NULL);
	result = mkdtemp (template);

	if ((result == NULL) || (*result == '\0')) {
		g_free (template);
		result = NULL;
	}
	
	g_free(best_folder);
	
	return result;
}


gboolean
is_temp_work_dir (const char *dir)
{
	int i;
	char *folder = NULL;

	if (strncmp (dir, "file://", 7) == 0)
		dir = dir + 7;
	else if (dir[0] != '/')
		return FALSE;

	for (i = 0; try_folder[i] != NULL; i++) {
		folder = ith_temp_folder_to_try (i);
		if (strncmp (dir, folder, strlen (folder)) == 0)
			if (strncmp (dir + strlen (folder), "/.fr-", 5) == 0) {
				g_free (folder);
				return TRUE;
			}
		g_free (folder);
	}

	return FALSE;
}


gboolean
is_temp_dir (const char *dir)
{
	if (strncmp (dir, "file://", 7) == 0)
		dir = dir + 7;
	if (strcmp (g_get_tmp_dir (), dir) == 0)
		return TRUE;
	if (path_in_path (g_get_tmp_dir (), dir))
		return TRUE;
	else
		return is_temp_work_dir (dir);
}


/* file list utils */


gboolean
file_list__match_pattern (const char *line,
			  const char *pattern)
{
	const char *l = line, *p = pattern;

	for (; (*p != 0) && (*l != 0); p++, l++) {
		if (*p != '%') {
			if (*p != *l)
				return FALSE;
		}
		else {
			p++;
			switch (*p) {
			case 'a':
				break;
			case 'n':
				if (!isdigit (*l))
					return FALSE;
				break;
			case 'c':
				if (!isalpha (*l))
					return FALSE;
				break;
			default:
				return FALSE;
			}
		}
	}

	return (*p == 0);
}


int
file_list__get_index_from_pattern (const char *line,
				   const char *pattern)
{
	int         line_l, pattern_l;
	const char *l;

	line_l = strlen (line);
	pattern_l = strlen (pattern);

	if ((pattern_l == 0) || (line_l == 0))
		return -1;

	for (l = line; *l != 0; l++)
		if (file_list__match_pattern (l, pattern))
			return (l - line);

	return -1;
}


char*
file_list__get_next_field (const char *line,
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

	return g_strndup (f_start, f_end - f_start);
}


char*
file_list__get_prev_field (const char *line,
			   int         start_from,
			   int         field_n)
{
	const char *f_start, *f_end;

	f_start = line + start_from - 1;
	while ((*f_start == ' ') && (*f_start != *line))
		f_start--;
	f_end = f_start;

	while ((field_n > 0) && (*f_start != *line)) {
		if (*f_start == ' ') {
			field_n--;
			if (field_n != 0) {
				while ((*f_start == ' ') && (*f_start != *line))
					f_start--;
				f_end = f_start;
			}
		} else
			f_start--;
	}

	return g_strndup (f_start + 1, f_end - f_start);
}


gboolean
check_permissions (const char *uri,
		   int         mode)
{
	GFile    *file;
	gboolean  result;

	file = g_file_new_for_uri (uri);
	result = check_file_permissions (file, mode);

	g_object_unref (file);

	return result;
}


gboolean
check_file_permissions (GFile *file,
		        int    mode)
{
	gboolean   result = TRUE;
	GFileInfo *info;
	GError    *err = NULL;
	gboolean   default_permission_when_unknown = TRUE;

	info = g_file_query_info (file, "access::*", 0, NULL, &err);
	if (err != NULL) {
		g_clear_error (&err);
		result = FALSE;
	}
	else {
		if ((mode & R_OK) == R_OK) {
			if (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ))
				result = (result && g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ));
			else
				result = (result && default_permission_when_unknown);
		}

		if ((mode & W_OK) == W_OK) {
			if (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE))
				result = (result && g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE));
			else
				result = (result && default_permission_when_unknown);
		}

		if ((mode & X_OK) == X_OK) {
			if (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE))
				result = (result && g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE));
			else
				result = (result && default_permission_when_unknown);
		}

		g_object_unref (info);
	}

	return result;
}


gboolean
is_program_in_path (const char *filename)
{
	char *str;
	char *value;
	int   result = FALSE;

	value = g_hash_table_lookup (ProgramsCache, filename);
	if (value != NULL) {
		result = (strcmp (value, "1") == 0);
		return result;
	}

	str = g_find_program_in_path (filename);
	if (str != NULL) {
		g_free (str);
		result = TRUE;
	}

	g_hash_table_insert (ProgramsCache,
			     g_strdup (filename),
			     result ? "1" : "0");

	return result;
}


gboolean
is_program_available (const char *filename,
		      gboolean    check)
{
	return ! check || is_program_in_path (filename);
}


const char *
get_home_uri (void)
{
	static char *home_uri = NULL;
	if (home_uri == NULL)
		home_uri = g_filename_to_uri (g_get_home_dir (), NULL, NULL);
	return home_uri;
}


char *
get_home_relative_uri (const char *partial_uri)
{
	return g_strconcat (get_home_uri (),
			    "/",
			    partial_uri,
			    NULL);
}


GFile *
get_home_relative_file (const char *partial_uri)
{
	GFile *file;
	char  *uri;

	uri = g_strconcat (get_home_uri (), "/", partial_uri, NULL);
	file = g_file_new_for_uri (uri);
	g_free (uri);

	return file;
}


GFile *
get_user_config_subdirectory (const char *child_name,
			      gboolean    create_child)
{
	char   *full_path;
	GFile  *file;
	GError *error = NULL;

	full_path = g_strconcat (g_get_user_config_dir (), "/", child_name, NULL);
	file = g_file_new_for_path (full_path);
	g_free (full_path);

	if  (create_child && ! make_directory_tree (file, 0700, &error)) {
		g_warning ("%s", error->message);
		g_error_free (error);
		g_object_unref (file);
		file = NULL;
	}

	return file;
}


const char *
remove_host_from_uri (const char *uri)
{
        const char *idx, *sep;

        if (uri == NULL)
                return NULL;

        idx = strstr (uri, "://");
        if (idx == NULL)
                return uri;
        idx += 3;
        if (*idx == '\0')
                return "/";
        sep = strstr (idx, "/");
        if (sep == NULL)
                return idx;
        return sep;
}


char *
get_uri_host (const char *uri)
{
	const char *idx;

	idx = strstr (uri, "://");
	if (idx == NULL)
		return NULL;
	idx = strstr (idx + 3, "/");
	if (idx == NULL)
		return NULL;
	return g_strndup (uri, (idx - uri));
}


char *
get_uri_root (const char *uri)
{
	char *host;
	char *root;

	host = get_uri_host (uri);
	if (host == NULL)
		return NULL;
	root = g_strconcat (host, "/", NULL);
	g_free (host);

	return root;
}


int
uricmp (const char *uri1,
	const char *uri2)
{
	return strcmp_null_tolerant (uri1, uri2);
}


char *
get_alternative_uri (const char *folder,
	     const char *name)
{
	char *new_uri = NULL;
	int   n = 1;

	do {
		g_free (new_uri);
		if (n == 1)
			new_uri = g_strconcat (folder, "/", name, NULL);
		else
			new_uri = g_strdup_printf ("%s/%s%%20(%d)", folder, name, n);
		n++;
	} while (uri_exists (new_uri));

	return new_uri;
}


char *
get_alternative_uri_for_uri (const char *uri)
{
	char *base_uri;
	char *new_uri;

	base_uri = remove_level_from_path (uri);
	new_uri = get_alternative_uri (base_uri, file_name_from_path (uri));
	g_free (base_uri);

	return new_uri;
}


GList *
gio_file_list_dup (GList *l)
{
	GList *r = NULL, *scan;
	for (scan = l; scan; scan = scan->next)
		r = g_list_prepend (r, g_file_dup ((GFile*)scan->data));
	return g_list_reverse (r);
}


void
gio_file_list_free (GList *l)
{
	GList *scan;
	for (scan = l; scan; scan = scan->next)
		g_object_unref (scan->data);
	g_list_free (l);
}


GList *
gio_file_list_new_from_uri_list (GList *uris)
{
	GList *r = NULL, *scan;
	for (scan = uris; scan; scan = scan->next)
		r = g_list_prepend (r, g_file_new_for_uri ((char*)scan->data));
	return g_list_reverse (r);
}


void
g_key_file_save (GKeyFile *key_file,
	         GFile    *file)
{
	char   *file_data;
	gsize   size;
	GError *error = NULL;

	file_data = g_key_file_to_data (key_file, &size, &error);
	if (error != NULL) {
		g_warning ("Could not save options: %s\n", error->message);
		g_clear_error (&error);
	}
	else {
		GFileOutputStream *stream;

		stream = g_file_replace (file, NULL, FALSE, 0, NULL, &error);
		if (stream == NULL) {
			g_warning ("Could not save options: %s\n", error->message);
			g_clear_error (&error);
		}
		else if (! g_output_stream_write_all (G_OUTPUT_STREAM (stream), file_data, size, NULL, NULL, &error)) {
			g_warning ("Could not save options: %s\n", error->message);
			g_clear_error (&error);
		}
		else if (! g_output_stream_close (G_OUTPUT_STREAM (stream), NULL, &error)) {
			g_warning ("Could not save options: %s\n", error->message);
			g_clear_error (&error);
		}

		g_object_unref (stream);
	}

	g_free (file_data);
}
