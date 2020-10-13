/*
 * fm-search-uri.c
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

#include "fm-search.h"
#include <string.h>

struct _FmSearch
{
    gboolean recursive;
    gboolean show_hidden;
    char* name_patterns;
    gboolean name_ci;
    gboolean name_regex;
    char* content_pattern;
    gboolean content_ci;
    gboolean content_regex;
    GList* mime_types;
    GList* search_path_list;
    guint64 max_size;
    guint64 min_size;
    char* max_mtime;
    char* min_mtime;
};

FmSearch* fm_search_new (void)
{
    FmSearch* search = (FmSearch*)g_slice_new0(FmSearch);
    return search;
}

void fm_search_free(FmSearch* search)
{
    g_list_free_full(search->mime_types, (GDestroyNotify)g_free);
    g_list_free_full(search->search_path_list, (GDestroyNotify)g_free);
    g_free(search->name_patterns);
    g_free(search->content_pattern);
    g_free(search->max_mtime);
    g_free(search->min_mtime);
    g_slice_free(FmSearch, search);
}

gboolean fm_search_get_recursive(FmSearch* search)
{
    return search->recursive;
}

void fm_search_set_recursive(FmSearch* search, gboolean recursive)
{
    search->recursive = recursive;
}

gboolean fm_search_get_show_hidden(FmSearch* search)
{
    return search->show_hidden;
}

void fm_search_set_show_hidden(FmSearch* search, gboolean show_hidden)
{
    search->show_hidden = show_hidden;
}

const char* fm_search_get_name_patterns(FmSearch* search)
{
    return search->name_patterns;
}

void fm_search_set_name_patterns(FmSearch* search, const char* name_patterns)
{
    g_free(search->name_patterns);
    search->name_patterns = g_strdup(name_patterns);
}

gboolean fm_search_get_name_ci(FmSearch* search)
{
    return search->name_ci;
}

void fm_search_set_name_ci(FmSearch* search, gboolean name_ci)
{
    search->name_ci = name_ci;
}

gboolean fm_search_get_name_regex(FmSearch* search)
{
    return search->name_regex;
}

void fm_search_set_name_regex(FmSearch* search, gboolean name_regex)
{
    search->name_regex = name_regex;
}

const char* fm_search_get_content_pattern(FmSearch* search)
{
    return search->content_pattern;
}

void fm_search_set_content_pattern(FmSearch* search, const char* content_pattern)
{
    g_free(search->content_pattern);
    search->content_pattern = g_strdup(content_pattern);
}

gboolean fm_search_get_content_ci(FmSearch* search)
{
    return search->content_ci;
}

void fm_search_set_content_ci(FmSearch* search, gboolean content_ci)
{
    search->content_ci = content_ci;
}

gboolean fm_search_get_content_regex(FmSearch* search)
{
    return search->content_regex;
}

void fm_search_set_content_regex(FmSearch* search, gboolean content_regex)
{
    search->content_regex = content_regex;
}

void fm_search_add_dir(FmSearch* search, const char* dir)
{
    GList* l = g_list_find_custom(search->search_path_list, dir, (GCompareFunc)strcmp);
    if(!l)
        search->search_path_list = g_list_prepend(search->search_path_list, g_strdup(dir));
}

void fm_search_remove_dir(FmSearch* search, const char* dir)
{
    GList* l = g_list_find_custom(search->search_path_list, dir, (GCompareFunc)strcmp);
    if(G_LIKELY(l))
    {
        g_free(l->data);
        search->search_path_list = g_list_delete_link(search->search_path_list, l);
    }
}

GList* fm_search_get_dirs(FmSearch* search)
{
    return search->search_path_list;
}

void fm_search_add_mime_type(FmSearch* search, const char* mime_type)
{
    GList* l = g_list_find_custom(search->mime_types, mime_type, (GCompareFunc)strcmp);
    if(!l)
        search->mime_types = g_list_prepend(search->mime_types, g_strdup(mime_type));
}

void fm_search_remove_mime_type(FmSearch* search, const char* mime_type)
{
    GList* l = g_list_find_custom(search->mime_types, mime_type, (GCompareFunc)strcmp);
    if(G_LIKELY(l))
    {
        g_free(l->data);
        search->mime_types = g_list_delete_link(search->mime_types, l);
    }
}

GList* fm_search_get_mime_types(FmSearch* search)
{
    return search->mime_types;
}

guint64 fm_search_get_max_size(FmSearch* search)
{
    return search->max_size;
}

void fm_search_set_max_size(FmSearch* search, guint64 size)
{
    search->max_size = size;
}

guint64 fm_search_get_min_size(FmSearch* search)
{
    return search->min_size;
}

void fm_search_set_min_size(FmSearch* search, guint64 size)
{
    search->min_size = size;
}

/* format of mtime: YYYY-MM-DD */
const char* fm_search_get_max_mtime(FmSearch* search)
{
    return search->max_mtime;
}

void fm_search_set_max_mtime(FmSearch* search, const char* mtime)
{
    g_free(search->max_mtime);
    search->max_mtime = g_strdup(mtime);
}

/* format of mtime: YYYY-MM-DD */
const char* fm_search_get_min_mtime(FmSearch* search)
{
    return search->min_mtime;
}

void fm_search_set_min_mtime(FmSearch* search, const char* mtime)
{
    g_free(search->min_mtime);
    search->min_mtime = g_strdup(mtime);
}

/* really build the path */
GFile* fm_search_to_gfile(FmSearch* search)
{
    GFile* search_path = NULL;
    GString* search_str = g_string_sized_new(1024);
    /* build the search:// URI to perform the search */
    g_string_append(search_str, "search://");

    if(search->search_path_list) /* we need to have at least one dir path */
    {
        char *escaped;
        /* add paths */
        GList* l;
        for(l = search->search_path_list; ; )
        {
            char *path_str = (char*)l->data;
            /* escape possible '?' and ',' */
            escaped = g_uri_escape_string(path_str, "!$&'()*+:;=/@", TRUE);
            g_string_append(search_str, escaped);
            g_free(escaped);

            l = l->next;
            if(!l) /* no more items */
                break;
            g_string_append_c(search_str, ','); /* separator for paths */
        }

        g_string_append_c(search_str, '?');
        g_string_append_printf(search_str, "recursive=%c", search->recursive ? '1' : '0');
        g_string_append_printf(search_str, "&show_hidden=%c", search->show_hidden ? '1' : '0');
        if(search->name_patterns && *search->name_patterns)
        {
            /* escape ampersands in pattern */
            escaped = g_uri_escape_string(search->name_patterns, ":/?#[]@!$'()*+,;", TRUE);
            if(search->name_regex)
                g_string_append_printf(search_str, "&name_regex=%s", escaped);
            else
                g_string_append_printf(search_str, "&name=%s", escaped);
            if(search->name_ci)
                g_string_append_printf(search_str, "&name_ci=%c", search->name_ci ? '1' : '0');
            g_free(escaped);
        }

        if(search->content_pattern && *search->content_pattern)
        {
            /* escape ampersands in pattern */
            escaped = g_uri_escape_string(search->content_pattern, ":/?#[]@!$'()*+,;^<>{}", TRUE);
            if(search->content_regex)
                g_string_append_printf(search_str, "&content_regex=%s", escaped);
            else
                g_string_append_printf(search_str, "&content=%s", escaped);
            g_free(escaped);
            if(search->content_ci)
                g_string_append_printf(search_str, "&content_ci=%c", search->content_ci ? '1' : '0');
        }

        /* search for the files of specific mime-types */
        if(search->mime_types)
        {
            GList* l;
            g_string_append(search_str, "&mime_types=");
            for(l = search->mime_types; l; l=l->next)
            {
                const char* mime_type = (const char*)l->data;
                g_string_append(search_str, mime_type);
                if(l->next)
                    g_string_append_c(search_str, ';');
            }
        }

        if(search->min_size)
            g_string_append_printf(search_str, "&min_size=%llu", (unsigned long long)search->min_size);

        if(search->max_size)
            g_string_append_printf(search_str, "&max_size=%llu", (unsigned long long)search->max_size);

        if(search->min_mtime)
            g_string_append_printf(search_str, "&min_mtime=%s", search->min_mtime);

        if(search->max_mtime)
            g_string_append_printf(search_str, "&max_mtime=%s", search->max_mtime);

        search_path = g_file_new_for_uri(search_str->str);
        g_string_free(search_str, TRUE);
    }
    return search_path;
}
