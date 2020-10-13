/*
 * fm-search-uri.h
 * 
 * Copyright 2015 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * Copyright 2012-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* FmSearch implements a tool used to generate a search:// URI used by libfm to search for files.
 * This API might become part of libfm in the future.
 */

#ifndef _FM_SEARCH_H_
#define _FM_SEARCH_H_

#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _FmSearch         FmSearch;

FmSearch* fm_search_new(void);
void fm_search_free(FmSearch* search);

GFile* fm_search_to_gfile(FmSearch* search);

gboolean fm_search_get_recursive(FmSearch* search);
void fm_search_set_recursive(FmSearch* search, gboolean recursive);

gboolean fm_search_get_show_hidden(FmSearch* search);
void fm_search_set_show_hidden(FmSearch* search, gboolean show_hidden);

const char* fm_search_get_name_patterns(FmSearch* search);
void fm_search_set_name_patterns(FmSearch* search, const char* name_patterns);

gboolean fm_search_get_name_ci(FmSearch* search);
void fm_search_set_name_ci(FmSearch* search, gboolean name_ci);

gboolean fm_search_get_name_regex(FmSearch* search);
void fm_search_set_name_regex(FmSearch* search, gboolean name_regex);

const char* fm_search_get_content_pattern(FmSearch* search);
void fm_search_set_content_pattern(FmSearch* search, const char* content_pattern);

gboolean fm_search_get_content_ci(FmSearch* search);
void fm_search_set_content_ci(FmSearch* search, gboolean content_ci);

gboolean fm_search_get_content_regex(FmSearch* search);
void fm_search_set_content_regex(FmSearch* search, gboolean content_regex);

void fm_search_add_dir(FmSearch* search, const char* dir);
void fm_search_remove_dir(FmSearch* search, const char* dir);
GList* fm_search_get_dirs(FmSearch* search);

void fm_search_add_mime_type(FmSearch* search, const char* mime_type);
void fm_search_remove_mime_type(FmSearch* search, const char* mime_type);
GList* fm_search_get_mime_types(FmSearch* search);

guint64 fm_search_get_max_size(FmSearch* search);
void fm_search_set_max_size(FmSearch* search, guint64 size);

guint64 fm_search_get_min_size(FmSearch* search);
void fm_search_set_min_size(FmSearch* search, guint64 size);

/* format of mtime: YYYY-MM-DD */
const char* fm_search_get_max_mtime(FmSearch* search);
void fm_search_set_max_mtime(FmSearch* search, const char* mtime);

/* format of mtime: YYYY-MM-DD */
const char* fm_search_get_min_mtime(FmSearch* search);
void fm_search_set_min_mtime(FmSearch* search, const char* mtime);

G_END_DECLS

#endif /* _FM_SEARCH_H_ */
