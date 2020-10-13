/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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

#ifndef _GLIB_UTILS_H
#define _GLIB_UTILS_H

#include <time.h>

#define g_signal_handlers_disconnect_by_data(instance, data) \
    g_signal_handlers_disconnect_matched ((instance), G_SIGNAL_MATCH_DATA, \
					  0, 0, NULL, NULL, (data))

/* gobject utils */

gpointer _g_object_ref (gpointer object);
void _g_object_unref (gpointer object);

/* string utils */

gboolean            strchrs                      (const char *str,
						  const char *chars);
char *              str_substitute               (const char *str,
						  const char *from_str,
						  const char *to_str);
int                 strcmp_null_tolerant         (const char *s1, const char *s2);
char*               escape_str_common            (const char *str,
						  const char *meta_chars,
						  const char  prefix,
						  const char  postfix);
char*               escape_str                   (const char  *str,
						  const char  *meta_chars);
gchar *             shell_escape                 (const gchar *filename);
gboolean            match_regexps                (GRegex     **regexps,
						  const char  *string,
						  GRegexMatchFlags match_options);
char **             search_util_get_patterns     (const char  *pattern_string);
GRegex **           search_util_get_regexps      (const char  *pattern_string,
			                          GRegexCompileFlags  compile_options);
void                free_regexps                 (GRegex     **regexps);
/*char *              _g_strdup_with_max_size      (const char  *s,
						  int          max_size);*/
const char *        eat_spaces                   (const char  *line);
const char *        eat_void_chars               (const char  *line);
char **             split_line                   (const char  *line,
						  int          n_fields);
const char *        get_last_field               (const char  *line,
						  int          last_field);
int                 n_fields                     (char       **str_array);
char *              get_time_string              (time_t       time);
/*GPtrArray *         g_ptr_array_copy             (GPtrArray   *array);*/
/*void                g_ptr_array_reverse          (GPtrArray   *array);
int                 g_ptr_array_binary_search    (GPtrArray   *array,
						  gpointer     value,
						  GCompareFunc func);*/
const char *        get_static_string            (const char  *s);
/*char*               g_uri_display_basename       (const char  *uri);
char **            _g_strv_prepend               (char **str_array,
                                                  const char *str);
gboolean           _g_strv_remove                (char **str_array,
                                                  const char *str);*/

/* path filename */

/*const char *        _g_path_get_file_name          (const char          *path);
const char *        _g_path_get_base_name          (const char          *path,
						    const char          *base_dir,
						    gboolean             junk_paths);*/

/**/

#ifndef __GNUC__
#define __FUNCTION__ ""
#endif

#define DEBUG_INFO __FILE__, __LINE__, __FUNCTION__

void     debug                     (const char *file,
				    int         line,
				    const char *function,
				    const char *format, ...);

#endif /* _GLIB_UTILS_H */
