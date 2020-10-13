/*
 * fm-vfs-search.c
 * 
 * Copyright 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * Copyright 2010 Shae Smittle <starfall87@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-file.h"

#include <glib/gi18n-lib.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define _GNU_SOURCE /* for FNM_CASEFOLD in fnmatch.h, a GNU extension */
#include <fnmatch.h>

#if __GNUC__ >= 4
#pragma GCC diagnostic ignored "-Wcomment" /* for comments below */
#endif

/* ---- Classes structures ---- */
typedef struct _FmSearchIntIter FmSearchIntIter;

struct _FmSearchIntIter
{
    FmSearchIntIter *parent; /* recursion path */
    GFile *folder_path; /* path to folder */
    GFileEnumerator *enu; /* children enumerator */
};

#define FM_TYPE_VFS_SEACRH_ENUMERATOR      (fm_vfs_search_enumerator_get_type())
#define FM_VFS_SEACRH_ENUMERATOR(o)        (G_TYPE_CHECK_INSTANCE_CAST((o),\
                            FM_TYPE_VFS_SEACRH_ENUMERATOR, FmVfsSearchEnumerator))

typedef struct _FmVfsSearchEnumerator         FmVfsSearchEnumerator;
typedef struct _FmVfsSearchEnumeratorClass    FmVfsSearchEnumeratorClass;

struct _FmVfsSearchEnumerator
{
    GFileEnumerator parent;

    FmSearchIntIter* iter;
    char* attributes;
    GFileQueryInfoFlags flags;
    GSList* target_folders; /* GFile */
    char** name_patterns;
    GRegex* name_regex;
    char* content_pattern;
    GRegex* content_regex;
    char** mime_types;
    guint64 min_mtime;
    guint64 max_mtime;
    guint64 min_size;
    guint64 max_size;
    gboolean name_case_insensitive : 1;
    gboolean content_case_insensitive : 1;
    gboolean recursive : 1;
    gboolean show_hidden : 1;
};

struct _FmVfsSearchEnumeratorClass
{
    GFileEnumeratorClass parent_class;
};


#define FM_TYPE_SEARCH_VFILE           (fm_vfs_search_file_get_type())
#define FM_SEARCH_VFILE(o)             (G_TYPE_CHECK_INSTANCE_CAST((o), \
                                        FM_TYPE_SEARCH_VFILE, FmSearchVFile))

typedef struct _FmSearchVFile           FmSearchVFile;
typedef struct _FmSearchVFileClass      FmSearchVFileClass;

static GType fm_vfs_search_file_get_type (void);

struct _FmSearchVFile
{
    GObject parent_object;

    char *path; /* full search path */
    GFile *current; /* last scanned directory */
};

struct _FmSearchVFileClass
{
  GObjectClass parent_class;
};


/* beforehand declarations */
static gboolean fm_search_job_match_file(FmVfsSearchEnumerator * priv,
                                         GFileInfo * info, GFile * parent,
                                         GCancellable *cancellable,
                                         GError **error);
static void fm_search_job_match_folder(FmVfsSearchEnumerator * priv,
                                       GFile * folder_path,
                                       GCancellable *cancellable,
                                       GError **error);
static void parse_search_uri(FmVfsSearchEnumerator* priv, const char* uri_str);


/* ---- Directory iterator ---- */
/* caller should g_object_ref(folder_path) if success */
static inline FmSearchIntIter *_search_iter_new(FmSearchIntIter *parent,
                                                const char *attributes,
                                                GFileQueryInfoFlags flags,
                                                GFile *folder_path,
                                                GCancellable *cancellable,
                                                GError **error)
{
    GFileEnumerator *enu;
    FmSearchIntIter *iter;

    enu = g_file_enumerate_children(folder_path, attributes, flags,
                                    cancellable, error);
    if(enu == NULL)
        return NULL;
    iter = g_slice_new(FmSearchIntIter);
    iter->parent = parent;
    iter->folder_path = folder_path;
    iter->enu = enu;
    return iter;
}

static inline void _search_iter_free(FmSearchIntIter *iter, GCancellable *cancellable)
{
    g_file_enumerator_close(iter->enu, cancellable, NULL);
    g_object_unref(iter->enu);
    g_object_unref(iter->folder_path);
    g_slice_free(FmSearchIntIter, iter);
}


/* ---- search enumerator class ---- */
static GType fm_vfs_search_enumerator_get_type   (void);

G_DEFINE_TYPE(FmVfsSearchEnumerator, fm_vfs_search_enumerator, G_TYPE_FILE_ENUMERATOR)

static void _fm_vfs_search_enumerator_dispose(GObject *object)
{
    FmVfsSearchEnumerator *priv = FM_VFS_SEACRH_ENUMERATOR(object);
    FmSearchIntIter *iter;

    while((iter = priv->iter))
    {
        priv->iter = iter->parent;
        _search_iter_free(iter, NULL);
    }

    if(priv->attributes)
    {
        g_free(priv->attributes);
        priv->attributes = NULL;
    }

    if(priv->target_folders)
    {
        g_slist_foreach(priv->target_folders, (GFunc)g_object_unref, NULL);
        g_slist_free(priv->target_folders);
        priv->target_folders = NULL;
    }

    if(priv->name_patterns)
    {
        g_strfreev(priv->name_patterns);
        priv->name_patterns = NULL;
    }

    if(priv->name_regex)
    {
        g_regex_unref(priv->name_regex);
        priv->name_regex = NULL;
    }

    if(priv->content_pattern)
    {
        g_free(priv->content_pattern);
        priv->content_pattern = NULL;
    }

    if(priv->content_regex)
    {
        g_regex_unref(priv->content_regex);
        priv->content_regex = NULL;
    }

    if(priv->mime_types)
    {
        g_strfreev(priv->mime_types);
        priv->mime_types = NULL;
    }

    G_OBJECT_CLASS(fm_vfs_search_enumerator_parent_class)->dispose(object);
}

static GFileInfo *_fm_vfs_search_enumerator_next_file(GFileEnumerator *enumerator,
                                                      GCancellable *cancellable,
                                                      GError **error)
{
    FmVfsSearchEnumerator *enu = FM_VFS_SEACRH_ENUMERATOR(enumerator);
    FmSearchIntIter *iter;
    GFileInfo * file_info;
    GError *err = NULL;
    FmSearchVFile *container;

    /* g_debug("_fm_vfs_search_enumerator_next_file"); */
    while(!g_cancellable_set_error_if_cancelled(cancellable, error))
    {
        iter = enu->iter;
        if(iter == NULL) /* ended with folder */
        {
            if(enu->target_folders == NULL)
                break;
            iter = _search_iter_new(NULL, enu->attributes, enu->flags,
                                    enu->target_folders->data,
                                    cancellable, error);
            if(iter == NULL)
                break;
            /* data is moved into iter now so free link itself */
            enu->target_folders = g_slist_delete_link(enu->target_folders,
                                                      enu->target_folders);
            container = FM_SEARCH_VFILE(g_file_enumerator_get_container(enumerator));
            if(container->current)
                g_object_unref(container->current);
            container->current = g_object_ref(iter->folder_path);
            enu->iter = iter;
        }

        file_info = g_file_enumerator_next_file(iter->enu, cancellable, &err);
        if(file_info && g_file_info_get_name(file_info))
        {
            /* check if directory itself matches criteria */
            if(fm_search_job_match_file(enu, file_info, iter->folder_path,
                                        cancellable, &err))
            {
                g_debug("found matched: %s", g_file_info_get_name(file_info));
                return file_info;
            }

            /* recurse upon each directory */
            if(err == NULL && enu->recursive &&
               /* SF bug #969: very possibly we get multiple instances of the
                  same file if we follow symlink to a directory
                  FIXME: make it optional? */
               !g_file_info_get_is_symlink(file_info) &&
               g_file_info_get_file_type(file_info) == G_FILE_TYPE_DIRECTORY)
            {
                if(enu->show_hidden || !g_file_info_get_is_hidden(file_info))
                {
                    const char * name = g_file_info_get_name(file_info);
                    GFile * file = g_file_get_child(iter->folder_path, name);
                    /* go into directory and iterate it now */
                    fm_search_job_match_folder(enu, file, cancellable, &err);
                    g_object_unref(file);
                }
            }

            g_object_unref(file_info);
            if(err == NULL)
                continue;
        }

        if(err != NULL) /* file_info == NULL */
        {
            if(err->domain == G_IO_ERROR && err->code == G_IO_ERROR_PERMISSION_DENIED)
            {
                g_error_free(err); /* ignore this error */
                err = NULL;
                continue;
            }
            g_propagate_error(error, err);
            break;
        }
        /* else end of file list - go up */
        enu->iter = iter->parent;
        container = FM_SEARCH_VFILE(g_file_enumerator_get_container(enumerator));
        if(container->current)
            g_object_unref(container->current);
        if(enu->iter)
            container->current = g_object_ref(enu->iter->folder_path);
        else
            container->current = NULL;
        _search_iter_free(iter, cancellable);
    }
    return NULL;
}

static gboolean _fm_vfs_search_enumerator_close(GFileEnumerator *enumerator,
                                              GCancellable *cancellable,
                                              GError **error)
{
    FmVfsSearchEnumerator *enu = FM_VFS_SEACRH_ENUMERATOR(enumerator);
    FmSearchIntIter *iter;

    while((iter = enu->iter))
    {
        enu->iter = iter->parent;
        _search_iter_free(iter, cancellable);
    }
    return TRUE;
}

static void fm_vfs_search_enumerator_class_init(FmVfsSearchEnumeratorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GFileEnumeratorClass *enumerator_class = G_FILE_ENUMERATOR_CLASS(klass);

  gobject_class->dispose = _fm_vfs_search_enumerator_dispose;

  enumerator_class->next_file = _fm_vfs_search_enumerator_next_file;
  enumerator_class->close_fn = _fm_vfs_search_enumerator_close;
  
}

static void fm_vfs_search_enumerator_init(FmVfsSearchEnumerator *enumerator)
{
    /* nothing */
}

static GFileEnumerator *_fm_vfs_search_enumerator_new(GFile *file,
                                                      const char *path_str,
                                                      const char *attributes,
                                                      GFileQueryInfoFlags flags,
                                                      GError **error)
{
    FmVfsSearchEnumerator *enumerator;

    enumerator = g_object_new(FM_TYPE_VFS_SEACRH_ENUMERATOR, "container", file, NULL);

    enumerator->attributes = g_strdup(attributes);
    enumerator->flags = flags;
    parse_search_uri(enumerator, path_str);
    /* FIXME: don't ignore flags */

    return G_FILE_ENUMERATOR(enumerator);
}


/* ---- The search engine ---- */

/*
 * name: parse_date_str
 * @str: a string in YYYY-MM-DD format
 * Return: a time_t value
 */
static time_t parse_date_str(const char* str)
{
    int len = strlen(str);
    if(G_LIKELY(len >= 8))
    {
        struct tm timeinfo = {0};
        if(sscanf(str, "%04d-%02d-%02d", &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday) == 3)
        {
            timeinfo.tm_year -= 1900; /* should be years since 1900 */
            --timeinfo.tm_mon; /* month should be 0-11 */
            return mktime(&timeinfo);
        }
    }
    return 0;
}

/*
 * parse_search_uri
 * @job
 * @uri: a search uri
 * 
 * Format of a search URI is similar to that of an http URI:
 * 
 * search://<folder1>,<folder2>,<folder...>?<parameter1=value1>&<parameter2=value2>&...
 * The optional parameter key/value pairs are:
 * show_hidden=<0 or 1>: whether to search for hidden files
 * recursive=<0 or 1>: whether to search sub folders recursively
 * name=<patterns>: patterns of filenames, separated by comma
 * name_regex=<regular expression>: regular expression
 * name_case_sensitive=<0 or 1>
 * content=<content pattern>: search for files containing the pattern
 * content_regex=<regular expression>: regular expression
 * content_case_sensitive=<0 or 1>
 * mime_types=<mime-types>: mime-types to search for, can use /* (ex: image/*), separated by ';'
 * min_size=<bytes>
 * max_size=<bytes>
 * min_mtime=YYYY-MM-DD
 * max_mtime=YYYY-MM-DD
 * 
 * An example to search all *.desktop files in /usr/share and /usr/local/share
 * can be written like this:
 * 
 * search:///usr/share,/usr/local/share?recursive=1&show_hidden=0&name=*.desktop&name_ci=0
 * 
 * If the folder paths and parameters contain invalid characters for a
 * URI, they should be escaped.
 * 
 */
static void parse_search_uri(FmVfsSearchEnumerator* priv, const char* uri_str)
{
    const char scheme[] = "search://"; /* NOTE: sizeof(scheme) includes '\0' */
    if(g_ascii_strncasecmp(uri_str, scheme, sizeof(scheme)-1) == 0)
    {
        const char* p = uri_str + sizeof(scheme)-1; /* skip scheme part */
        char* params = strchr(p, '?');
        char* name_regex = NULL;
        char* content_regex = NULL;

        /* add folder paths */
        while (p)
        {
            char* sep = strchr(p, ','); /* use , to separate multiple paths */
            char *path;

            if (sep && (params == NULL || sep < params))
                path = g_uri_unescape_segment(p, sep, NULL);
            else if (params != NULL)
            {
                path = g_uri_unescape_segment(p, params, NULL);
                sep = NULL;
            }
            else
                path = g_uri_unescape_string(p, NULL);
            /* g_debug("target folder path: %s", path); */
            /* add the path to target folders */
            priv->target_folders = g_slist_prepend(priv->target_folders,
                                                   fm_file_new_for_commandline_arg(path));
            g_free(path);

            p = sep;
            if (p) /* it's on ':' now */
                p++;
        }

        /* priv->target_folders = g_slist_reverse(priv->target_folders); */

        /* decode parameters */
        if(params)
        {
            params++; /* skip '?' */
            while(*params)
            {
                /* parameters are in name=value pairs */
                char *name;
                char* value = strchr(params, '=');
                char* sep = strchr(params, '&');

                if (value && (sep == NULL || value < sep))
                {
                    name = g_strndup(params, value - params);
                    if (sep)
                        value = g_uri_unescape_segment(value+1, sep, NULL);
                    else
                        value = g_uri_unescape_string(value+1, NULL);
                }
                else if (sep)
                {
                    name = g_strndup(params, sep - params);
                    value = NULL;
                }
                else /* value == NULL && sep == NULL */
                    name = g_strdup(params);

                /* g_printf("parameter name/value: %s = %s\n", name, value); */

                if(strcmp(name, "show_hidden") == 0)
                    priv->show_hidden = (value[0] == '1') ? TRUE : FALSE;
                else if(strcmp(name, "recursive") == 0)
                    priv->recursive = (value[0] == '1') ? TRUE : FALSE;
                else if(strcmp(name, "name") == 0)
                    priv->name_patterns = g_strsplit(value, ",", 0);
                else if(strcmp(name, "name_regex") == 0)
                {
                    g_free(name_regex);
                    name_regex = value;
                    value = NULL;
                }
                else if(strcmp(name, "name_ci") == 0)
                    priv->name_case_insensitive = (value[0] == '1') ? TRUE : FALSE;
                else if(strcmp(name, "content") == 0)
                {
                    g_free(priv->content_pattern);
                    priv->content_pattern = value;
                    value = NULL;
                }
                else if(strcmp(name, "content_regex") == 0)
                {
                    g_free(content_regex);
                    content_regex = value;
                    value = NULL;
                }
                else if(strcmp(name, "content_ci") == 0)
                    priv->content_case_insensitive = (value[0] == '1') ? TRUE : FALSE;
                else if(strcmp(name, "mime_types") == 0)
                {
                    priv->mime_types = g_strsplit(value, ";", -1);

                    /* For mime_type patterns such as image/* and audio/*,
                     * we move the trailing '*' to begining of the string
                     * as a measure of optimization. Later we can detect if it's a
                     * pattern or a full type name by checking the first char. */
                    if(priv->mime_types)
                    {
                        char** pmime_type;
                        for(pmime_type = priv->mime_types; *pmime_type; ++pmime_type)
                        {
                            char* mime_type = *pmime_type;
                            int len = strlen(mime_type);
                            /* if the mime_type is end with "/*" */
                            if(len > 2 && /*mime_type[len - 2] == '/' &&*/ mime_type[len - 1] == '*')
                            {
                                /* move the trailing * to first char */
                                memmove(mime_type + 1, mime_type, len - 1);
                                mime_type[0] = '*';
                            }
                        }

                        if (!g_strstr_len(priv->attributes, -1, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE))
                        {
                            gchar * attributes = g_strconcat(priv->attributes, ",", G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, NULL);
                            g_free(priv->attributes);
                            priv->attributes = attributes;
                        }

                    }
                }
                else if(strcmp(name, "min_size") == 0)
                    priv->min_size = atoll(value);
                else if(strcmp(name, "max_size") == 0)
                    priv->max_size = atoll(value);
                else if(strcmp(name, "min_mtime") == 0)
                    priv->min_mtime = (guint64)parse_date_str(value);
                else if(strcmp(name, "max_mtime") == 0)
                    priv->max_mtime = (guint64)parse_date_str(value);

                g_free(name);
                g_free(value);

                /* continue with the next param=value pair */
                if(sep)
                    params = sep + 1;
                else
                    break;
            }

            if(name_regex)
            {
                /* we set G_REGEX_RAW because GLib might cause a crash
                   if a search is done in a non-utf8 string */
                GRegexCompileFlags flags = G_REGEX_RAW;
                if(priv->name_case_insensitive)
                    flags |= G_REGEX_CASELESS;
                priv->name_regex = g_regex_new(name_regex, flags, 0, NULL);
                g_free(name_regex);
            }

            if(content_regex)
            {
                GRegexCompileFlags flags = G_REGEX_RAW; /* like above */
                if(priv->content_case_insensitive)
                    flags |= G_REGEX_CASELESS;
                priv->content_regex = g_regex_new(content_regex, flags, 0, NULL);
                g_free(content_regex);
            }

            if(priv->content_case_insensitive) /* case insensitive */
            {
                if(priv->content_pattern) /* make sure the pattern is lower case */
                {
                    char* down = g_utf8_strdown(priv->content_pattern, -1);
                    g_free(priv->content_pattern);
                    priv->content_pattern = down;
                }
            }
        }
    }
}

static void fm_search_job_match_folder(FmVfsSearchEnumerator * priv,
                                       GFile * folder_path,
                                       GCancellable *cancellable,
                                       GError **error)
{
    FmSearchIntIter *iter;
    FmSearchVFile *container;

    /* FIXME: make error if NULL */
    iter = _search_iter_new(priv->iter, priv->attributes, priv->flags, folder_path,
                            cancellable, error);
    if(iter == NULL) /* error */
        return;
    g_object_ref(folder_path); /* it's copied into iter */
    priv->iter = iter;
    container = FM_SEARCH_VFILE(g_file_enumerator_get_container(G_FILE_ENUMERATOR(priv)));
    if(container->current)
        g_object_unref(container->current);
    container->current = g_object_ref(folder_path);
}

static gboolean fm_search_job_match_filename(FmVfsSearchEnumerator* priv, GFileInfo* info)
{
    gboolean ret;

    /* g_debug("fm_search_job_match_filename: %s", g_file_info_get_name(info)); */
    if(priv->name_regex)
    {
        const char* name = g_file_info_get_name(info);
        ret = g_regex_match(priv->name_regex, name, 0, NULL);
    }
    else if(priv->name_patterns)
    {
        ret = FALSE;
        const char* name = g_file_info_get_name(info);
        char** ppattern;
        for(ppattern = priv->name_patterns; *ppattern; ++ppattern)
        {
            const char* pattern = *ppattern;
            /* FIXME: FNM_CASEFOLD is a GNU extension */
            int flags = FNM_PERIOD;
            if(priv->name_case_insensitive)
                flags |= FNM_CASEFOLD;
            if(fnmatch(pattern, name, flags) == 0)
                ret = TRUE;
        }
    }
    else
        ret = TRUE;
    return ret;
}

static gboolean fm_search_job_match_content_line_based(FmVfsSearchEnumerator* priv,
                                                       GFileInfo* info,
                                                       GInputStream* stream,
                                                       GCancellable* cancellable,
                                                       GError** error)
{
    gboolean ret = FALSE;
    /* create a buffered data input stream for line-based I/O */
    GDataInputStream *input_stream = g_data_input_stream_new(stream);
    do
    {
        gsize line_len;
        char* line = g_data_input_stream_read_line(input_stream, &line_len, cancellable, error);
        if(line == NULL) /* error or EOF */
            break;
        if(priv->content_regex)
        {
            /* match using regexp */
            ret = g_regex_match(priv->content_regex, line, 0, NULL);
        }
        else if(priv->content_pattern && priv->content_case_insensitive)
        {
            /* case insensitive search is line-based because we need to
             * do utf8 validation + case conversion and it's easier to
             * do with lines than with raw streams. */
            if(g_utf8_validate(line, -1, NULL))
            {
                /* this whole line contains valid UTF-8 */
                char* down = g_utf8_strdown(line, -1);
                g_free(line);
                line = down;
            }
            else /* non-UTF8, treat as ASCII */
            {
                char* p;
                for(p = line; *p; ++p)
                    *p = g_ascii_tolower(*p);
            }

            if(strstr(line, priv->content_pattern))
                ret = TRUE;
        }
        g_free(line);
    }while(ret == FALSE);
    g_object_unref(input_stream);
    return ret;
}

static gboolean fm_search_job_match_content_exact(FmVfsSearchEnumerator* priv,
                                                  GFileInfo* info,
                                                  GInputStream* stream,
                                                  GCancellable* cancellable,
                                                  GError** error)
{
    gboolean ret = FALSE;
    char *buf, *pbuf;
    gssize size;

    /* Ensure that the allocated buffer is longer than the string being
     * searched for. Otherwise it's not possible for the buffer to 
     * contain a string fully matching the pattern. */
    int pattern_len = strlen(priv->content_pattern);
    int buf_size = pattern_len > 4095 ? pattern_len : 4095;
    int bytes_to_read;

    buf = g_new(char, buf_size + 1); /* +1 for terminating null char. */
    bytes_to_read = buf_size;
    pbuf = buf;
    for(;;)
    {
        char* found;
        size = g_input_stream_read(stream, pbuf, bytes_to_read, cancellable, error);
        if(size <=0) /* EOF or error */
            break;
        pbuf[size] = '\0'; /* make the string null terminated */

        found = strstr(buf, priv->content_pattern);
        if(found) /* the string is found in the buffer */
        {
            ret = TRUE;
            break;
        }
        else if(size == bytes_to_read) /* if size < bytes_to_read, we're at EOF and there are no further data. */
        {
            /* Preserve the last <pattern_len-1> bytes and move them to 
             * the beginning of the buffer.
             * Append further data after this chunk of data at next read. */
            int preserve_len = pattern_len - 1;
            char* buf_end = buf + buf_size;
            memmove(buf, buf_end - preserve_len, preserve_len);
            pbuf = buf + preserve_len;
            bytes_to_read = buf_size - preserve_len;
        }
    }
    g_free(buf);
    return ret;
}

static gboolean fm_search_job_match_content(FmVfsSearchEnumerator* priv,
                                            GFileInfo* info, GFile* parent,
                                            GCancellable* cancellable,
                                            GError** error)
{
    gboolean ret;
    if(priv->content_pattern || priv->content_regex)
    {
        ret = FALSE;
        if(g_file_info_get_file_type(info) == G_FILE_TYPE_REGULAR && g_file_info_get_size(info) > 0)
        {
            GFile* file = g_file_get_child(parent, g_file_info_get_name(info));
            /* NOTE: I disabled mmap-based search since this could cause
             * unexpected crashes sometimes if the mapped files are
             * removed or changed during the search. */
            GFileInputStream * stream = g_file_read(file, cancellable, error);
            g_object_unref(file);

            if(stream)
            {
                if(priv->content_pattern && !priv->content_case_insensitive)
                {
                    /* stream based search optimized for case sensitive
                     * exact match. */
                    ret = fm_search_job_match_content_exact(priv, info,
                                                        G_INPUT_STREAM(stream),
                                                        cancellable, error);
                }
                else
                {
                    /* grep-like regexp search and case insensitive search
                     * are line-based. */
                    ret = fm_search_job_match_content_line_based(priv, info,
                                                        G_INPUT_STREAM(stream),
                                                        cancellable, error);
                }

                g_input_stream_close(G_INPUT_STREAM(stream), cancellable, NULL);
                g_object_unref(stream);
            }
        }
    }
    else
        ret = TRUE;
    return ret;
}

static gboolean fm_search_job_match_file_type(FmVfsSearchEnumerator* priv, GFileInfo* info)
{
    gboolean ret;
    if(priv->mime_types)
    {
        const char* file_type = g_file_info_get_content_type(info);
        char** pmime_type;
        ret = FALSE;
        for(pmime_type = priv->mime_types; *pmime_type; ++pmime_type)
        {
            const char* mime_type = *pmime_type;
            /* For mime_type patterns such as image/* and audio/*,
             * we move the trailing '*' to begining of the string
             * as a measure of optimization. We can know it's a
             * pattern not a full type name by checking the first char. */
            if(mime_type[0] == '*')
            {
                if(g_str_has_prefix(file_type, mime_type + 1))
                {
                    ret = TRUE;
                    break;
                }
            }
            else if(g_content_type_is_a(file_type, mime_type))
            {
                ret = TRUE;
                break;
            }
        }
    }
    else
        ret = TRUE;
    return ret;
}

static gboolean fm_search_job_match_size(FmVfsSearchEnumerator* priv, GFileInfo* info)
{
    guint64 size = g_file_info_get_size(info);
    gboolean ret = TRUE;
    if(priv->min_size > 0 && size < priv->min_size)
        ret = FALSE;
    else if(priv->max_size > 0 && size > priv->max_size)
        ret = FALSE;
    else if ((priv->min_size > 0 || priv->max_size > 0) && g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY)
        ret = FALSE;
    return ret;
}

static gboolean fm_search_job_match_mtime(FmVfsSearchEnumerator* priv, GFileInfo* info)
{
    gboolean ret = TRUE;
    if(priv->min_mtime || priv->max_mtime)
    {
        guint64 mtime = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
        /* g_print("file mtime: %llu, min_mtime=%llu, max_mtime=%llu\n", mtime, priv->min_mtime, priv->max_mtime); */
        if(priv->min_mtime > 0 && mtime < priv->min_mtime)
            ret = FALSE; /* earlier than min_mtime */
        else if(priv->max_mtime > 0 && mtime > priv->max_mtime)
            ret = FALSE; /* later than max_mtime */
    }
    return ret;
}

static gboolean fm_search_job_match_file(FmVfsSearchEnumerator * priv,
                                         GFileInfo * info, GFile * parent,
                                         GCancellable *cancellable,
                                         GError **error)
{
    //g_print("matching file %s\n", g_file_info_get_name(info));

    if(!priv->show_hidden && g_file_info_get_is_hidden(info))
        return FALSE;

    if(!fm_search_job_match_filename(priv, info))
        return FALSE;

    if(!fm_search_job_match_file_type(priv, info))
        return FALSE;

    if(!fm_search_job_match_size(priv, info))
        return FALSE;

    if(!fm_search_job_match_mtime(priv, info))
        return FALSE;

    if(!fm_search_job_match_content(priv, info, parent, cancellable, error))
        return FALSE;

    return TRUE;
}


/* end of rule functions */

/* ---- FmSearchVFile class ---- */
static void fm_search_g_file_init(GFileIface *iface);
static void fm_search_fm_file_init(FmFileInterface *iface);

G_DEFINE_TYPE_WITH_CODE(FmSearchVFile, fm_vfs_search_file, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(G_TYPE_FILE, fm_search_g_file_init)
                        G_IMPLEMENT_INTERFACE(FM_TYPE_FILE, fm_search_fm_file_init))

static void fm_vfs_search_file_finalize(GObject *object)
{
    FmSearchVFile *item = FM_SEARCH_VFILE(object);

    g_free(item->path);
    if(item->current)
        g_object_unref(item->current);

    G_OBJECT_CLASS(fm_vfs_search_file_parent_class)->finalize(object);
}

static void fm_vfs_search_file_class_init(FmSearchVFileClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = fm_vfs_search_file_finalize;
}

static void fm_vfs_search_file_init(FmSearchVFile *item)
{
    /* nothing */
}

static FmSearchVFile *_fm_search_vfile_new(void)
{
    return (FmSearchVFile*)g_object_new(FM_TYPE_SEARCH_VFILE, NULL);
}


/* ---- GFile implementation ---- */
#define ERROR_UNSUPPORTED(err) g_set_error_literal(err, G_IO_ERROR, \
                        G_IO_ERROR_NOT_SUPPORTED, _("Operation not supported"))

static GFile *_fm_vfs_search_dup(GFile *file)
{
    FmSearchVFile *item, *new_item;

    item = FM_SEARCH_VFILE(file);
    new_item = _fm_search_vfile_new();
    if(item->path)
        new_item->path = g_strdup(item->path);
    return (GFile*)new_item;
}

static guint _fm_vfs_search_hash(GFile *file)
{
    return g_str_hash(FM_SEARCH_VFILE(file)->path);
}

static gboolean _fm_vfs_search_equal(GFile *file1, GFile *file2)
{
    char *path1 = FM_SEARCH_VFILE(file1)->path;
    char *path2 = FM_SEARCH_VFILE(file2)->path;

    return g_str_equal(path1, path2);
}

static gboolean _fm_vfs_search_is_native(GFile *file)
{
    return FALSE;
}

static gboolean _fm_vfs_search_has_uri_scheme(GFile *file, const char *uri_scheme)
{
    return g_ascii_strcasecmp(uri_scheme, "search") == 0;
}

static char *_fm_vfs_search_get_uri_scheme(GFile *file)
{
    return g_strdup("search");
}

static char *_fm_vfs_search_get_basename(GFile *file)
{
    return g_strdup("/");
}

static char *_fm_vfs_search_get_path(GFile *file)
{
    return NULL;
}

static char *_fm_vfs_search_get_uri(GFile *file)
{
    FmSearchVFile *item = FM_SEARCH_VFILE(file);

    if(item->current)
        return g_file_get_uri(item->current);
    return g_strdup(item->path);
}

static char *_fm_vfs_search_get_parse_name(GFile *file)
{
    /* FIXME: need name to be converted to UTF-8? */
    return g_strdup(_("Search"));
}

static GFile *_fm_vfs_search_get_parent(GFile *file)
{
    return NULL;
}

static gboolean _fm_vfs_search_prefix_matches(GFile *prefix, GFile *file)
{
    return FALSE;
}

static char *_fm_vfs_search_get_relative_path(GFile *parent, GFile *descendant)
{
    return NULL;
}

static GFile *_fm_vfs_search_resolve_relative_path(GFile *file, const char *relative_path)
{
    return NULL;
}

static GFile *_fm_vfs_search_get_child_for_display_name(GFile *file,
                                                        const char *display_name,
                                                        GError **error)
{
    FmSearchVFile *new_item;

    g_return_val_if_fail(file != NULL, NULL);

    if (display_name == NULL || *display_name == '\0')
        return g_object_ref(file);
    /* just append "/"display_name to file->path */
    new_item = _fm_search_vfile_new();
    new_item->path = g_strdup_printf("%s/%s", FM_SEARCH_VFILE(file)->path, display_name);
    return (GFile*)new_item;
}

static GFileEnumerator *_fm_vfs_search_enumerate_children(GFile *file,
                                                          const char *attributes,
                                                          GFileQueryInfoFlags flags,
                                                          GCancellable *cancellable,
                                                          GError **error)
{
    const char *path = FM_SEARCH_VFILE(file)->path;

    return _fm_vfs_search_enumerator_new(file, path, attributes, flags, error);
}

static GFileInfo *_fm_vfs_search_query_info(GFile *file,
                                            const char *attributes,
                                            GFileQueryInfoFlags flags,
                                            GCancellable *cancellable,
                                            GError **error)
{
    GFileInfo *fileinfo = g_file_info_new();
    GIcon* icon;

    /* g_debug("_fm_vfs_search_query_info on %s", FM_SEARCH_VFILE(file)->path); */
    /* FIXME: use matcher to set only requested data */
    g_file_info_set_name(fileinfo, FM_SEARCH_VFILE(file)->path);
    g_file_info_set_display_name(fileinfo, _("Search Results"));
    icon = g_themed_icon_new("search");
    g_file_info_set_icon(fileinfo, icon);
    g_object_unref(icon);
    g_file_info_set_file_type(fileinfo, G_FILE_TYPE_DIRECTORY);
    return fileinfo;
}

static GFileInfo *_fm_vfs_search_query_filesystem_info(GFile *file,
                                                       const char *attributes,
                                                       GCancellable *cancellable,
                                                       GError **error)
{
    /* FIXME: set some info on it */
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static GMount *_fm_vfs_search_find_enclosing_mount(GFile *file,
                                                   GCancellable *cancellable,
                                                   GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static GFile *_fm_vfs_search_set_display_name(GFile *file,
                                              const char *display_name,
                                              GCancellable *cancellable,
                                              GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static GFileAttributeInfoList *_fm_vfs_search_query_settable_attributes(GFile *file,
                                                                        GCancellable *cancellable,
                                                                        GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static GFileAttributeInfoList *_fm_vfs_search_query_writable_namespaces(GFile *file,
                                                                        GCancellable *cancellable,
                                                                        GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static gboolean _fm_vfs_search_set_attribute(GFile *file,
                                             const char *attribute,
                                             GFileAttributeType type,
                                             gpointer value_p,
                                             GFileQueryInfoFlags flags,
                                             GCancellable *cancellable,
                                             GError **error)
{
    ERROR_UNSUPPORTED(error);
    return FALSE;
}

static gboolean _fm_vfs_search_set_attributes_from_info(GFile *file,
                                                        GFileInfo *info,
                                                        GFileQueryInfoFlags flags,
                                                        GCancellable *cancellable,
                                                        GError **error)
{
    ERROR_UNSUPPORTED(error);
    return FALSE;
}

static GFileInputStream *_fm_vfs_search_read_fn(GFile *file,
                                                GCancellable *cancellable,
                                                GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static GFileOutputStream *_fm_vfs_search_append_to(GFile *file,
                                                   GFileCreateFlags flags,
                                                   GCancellable *cancellable,
                                                   GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static GFileOutputStream *_fm_vfs_search_create(GFile *file,
                                                GFileCreateFlags flags,
                                                GCancellable *cancellable,
                                                GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static GFileOutputStream *_fm_vfs_search_replace(GFile *file,
                                                 const char *etag,
                                                 gboolean make_backup,
                                                 GFileCreateFlags flags,
                                                 GCancellable *cancellable,
                                                 GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static gboolean _fm_vfs_search_delete_file(GFile *file,
                                           GCancellable *cancellable,
                                           GError **error)
{
    ERROR_UNSUPPORTED(error);
    return FALSE;
}

static gboolean _fm_vfs_search_trash(GFile *file,
                                     GCancellable *cancellable,
                                     GError **error)
{
    ERROR_UNSUPPORTED(error);
    return FALSE;
}

static gboolean _fm_vfs_search_make_directory(GFile *file,
                                              GCancellable *cancellable,
                                              GError **error)
{
    ERROR_UNSUPPORTED(error);
    return FALSE;
}

static gboolean _fm_vfs_search_make_symbolic_link(GFile *file,
                                                  const char *symlink_value,
                                                  GCancellable *cancellable,
                                                  GError **error)
{
    ERROR_UNSUPPORTED(error);
    return FALSE;
}

static gboolean _fm_vfs_search_copy(GFile *source,
                                    GFile *destination,
                                    GFileCopyFlags flags,
                                    GCancellable *cancellable,
                                    GFileProgressCallback progress_callback,
                                    gpointer progress_callback_data,
                                    GError **error)
{
    ERROR_UNSUPPORTED(error);
    return FALSE;
}

static gboolean _fm_vfs_search_move(GFile *source,
                                    GFile *destination,
                                    GFileCopyFlags flags,
                                    GCancellable *cancellable,
                                    GFileProgressCallback progress_callback,
                                    gpointer progress_callback_data,
                                    GError **error)
{
    ERROR_UNSUPPORTED(error);
    return FALSE;
}

static GFileMonitor *_fm_vfs_search_monitor_dir(GFile *file,
                                                GFileMonitorFlags flags,
                                                GCancellable *cancellable,
                                                GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static GFileMonitor *_fm_vfs_search_monitor_file(GFile *file,
                                                 GFileMonitorFlags flags,
                                                 GCancellable *cancellable,
                                                 GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

#if GLIB_CHECK_VERSION(2, 22, 0)
static GFileIOStream *_fm_vfs_search_open_readwrite(GFile *file,
                                                    GCancellable *cancellable,
                                                    GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static GFileIOStream *_fm_vfs_search_create_readwrite(GFile *file,
                                                      GFileCreateFlags flags,
                                                      GCancellable *cancellable,
                                                      GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static GFileIOStream *_fm_vfs_search_replace_readwrite(GFile *file,
                                                       const char *etag,
                                                       gboolean make_backup,
                                                       GFileCreateFlags flags,
                                                       GCancellable *cancellable,
                                                       GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}
#endif /* Glib >= 2.22 */

static void fm_search_g_file_init(GFileIface *iface)
{
    iface->dup = _fm_vfs_search_dup;
    iface->hash = _fm_vfs_search_hash;
    iface->equal = _fm_vfs_search_equal;
    iface->is_native = _fm_vfs_search_is_native;
    iface->has_uri_scheme = _fm_vfs_search_has_uri_scheme;
    iface->get_uri_scheme = _fm_vfs_search_get_uri_scheme;
    iface->get_basename = _fm_vfs_search_get_basename;
    iface->get_path = _fm_vfs_search_get_path;
    iface->get_uri = _fm_vfs_search_get_uri;
    iface->get_parse_name = _fm_vfs_search_get_parse_name;
    iface->get_parent = _fm_vfs_search_get_parent;
    iface->prefix_matches = _fm_vfs_search_prefix_matches;
    iface->get_relative_path = _fm_vfs_search_get_relative_path;
    iface->resolve_relative_path = _fm_vfs_search_resolve_relative_path;
    iface->get_child_for_display_name = _fm_vfs_search_get_child_for_display_name;
    iface->enumerate_children = _fm_vfs_search_enumerate_children;
    iface->query_info = _fm_vfs_search_query_info;
    iface->query_filesystem_info = _fm_vfs_search_query_filesystem_info;
    iface->find_enclosing_mount = _fm_vfs_search_find_enclosing_mount;
    iface->set_display_name = _fm_vfs_search_set_display_name;
    iface->query_settable_attributes = _fm_vfs_search_query_settable_attributes;
    iface->query_writable_namespaces = _fm_vfs_search_query_writable_namespaces;
    iface->set_attribute = _fm_vfs_search_set_attribute;
    iface->set_attributes_from_info = _fm_vfs_search_set_attributes_from_info;
    iface->read_fn = _fm_vfs_search_read_fn;
    iface->append_to = _fm_vfs_search_append_to;
    iface->create = _fm_vfs_search_create;
    iface->replace = _fm_vfs_search_replace;
    iface->delete_file = _fm_vfs_search_delete_file;
    iface->trash = _fm_vfs_search_trash;
    iface->make_directory = _fm_vfs_search_make_directory;
    iface->make_symbolic_link = _fm_vfs_search_make_symbolic_link;
    iface->copy = _fm_vfs_search_copy;
    iface->move = _fm_vfs_search_move;
    iface->monitor_dir = _fm_vfs_search_monitor_dir;
    iface->monitor_file = _fm_vfs_search_monitor_file;
#if GLIB_CHECK_VERSION(2, 22, 0)
    iface->open_readwrite = _fm_vfs_search_open_readwrite;
    iface->create_readwrite = _fm_vfs_search_create_readwrite;
    iface->replace_readwrite = _fm_vfs_search_replace_readwrite;
    iface->supports_thread_contexts = TRUE;
#endif /* Glib >= 2.22 */
}


/* ---- FmFile implementation ---- */
static gboolean _fm_vfs_search_wants_incremental(GFile* file)
{
    return TRUE;
}

static void fm_search_fm_file_init(FmFileInterface *iface)
{
    iface->wants_incremental = _fm_vfs_search_wants_incremental;
}


/* ---- interface for loading ---- */
static GFile *_fm_vfs_search_new_for_uri(const char *uri)
{
    FmSearchVFile *item;

    g_return_val_if_fail(uri != NULL, NULL);
    item = _fm_search_vfile_new();
    item->path = g_strdup(uri);
    return (GFile*)item;
}

FM_DEFINE_MODULE(vfs, search)

FmFileInitTable fm_module_init_vfs =
{
    .new_for_uri = &_fm_vfs_search_new_for_uri
};
