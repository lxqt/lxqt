/*
 *      fm-path.c
 *
 *      Copyright 2009-2014 PCMan <pcman.tw@gmail.com>
 *      Copyright 2009 Juergen Hoetzel <juergen@archlinux.org>
 *      Copyright 2012-2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This file is a part of the Libfm library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:fm-path
 * @short_description: Path representation for libfm.
 * @title: FmPath
 *
 * @include: libfm/fm.h
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-path.h"
#include "fm-file-info.h"
#include "fm-file.h"
#include "fm-utils.h"

#include <string.h>
#include <limits.h>
#include <glib/gi18n-lib.h>

#define BASENAME_AS_DISP_NAME ((char *)-1)

struct _FmPath
{
    gint n_ref;
    FmPath* parent;
    char *disp_name;
    GSequenceIter *iter; /* iterator in parent, NULL for root path */
    GSequence *children; /* children to reuse paths */
    guchar flags; /* FmPathFlags flags : 8; */
    char name[1]; /* basename: in local encoding if native, uri-escaped otherwise */
};

struct _FmPathList
{
    FmList list;
};

static FmPath* root_path = NULL;

static const char* home_dir = NULL;
static int home_len = 0;
static FmPath* home_path = NULL;

static FmPath* desktop_path = NULL;
static const char* desktop_dir = NULL;
static int desktop_len = 0;

static FmPath* trash_root_path = NULL;
/*defined but not used
static FmPath* network_root = NULL;*/

static FmPath* apps_root_path = NULL;

static GSList* roots = NULL;

/* a lock for access to changeable data: roots list, and
   members of FmPath struct: disp_name, iter, children */
G_LOCK_DEFINE_STATIC(roots);

static FmPath* _fm_path_alloc(FmPath* parent, int name_len, int flags)
{
    FmPath* path;
    path = (FmPath*)g_malloc(sizeof(FmPath) + name_len);
    path->n_ref = 1;
    path->flags = flags;
    path->parent = parent ? fm_path_ref(parent) : NULL;
    path->disp_name = NULL;
    path->children = NULL;
    path->iter = NULL;
    return path;
}

static inline FmPath* _fm_path_new_internal(FmPath* parent, const char* name, int name_len, int flags)
{
    FmPath* path = _fm_path_alloc(parent, name_len, flags);
    memcpy(path->name, name, name_len);
    path->name[name_len] = '\0';
    if (parent)
    {
        G_LOCK(roots);
        if (parent->children == NULL)
            parent->children = g_sequence_new(NULL);
        path->iter = g_sequence_insert_sorted(parent->children, path,
                                              (GCompareDataFunc)fm_path_compare, NULL);
        G_UNLOCK(roots);
    }
    return path;
}

/**
 * _fm_path_new_uri_root
 * @uri: the uri in the form scheme://user@host/remaining/path
 * @len: length of the uri
 * @remaining: retrive the remaining path
 *
 * This function create a root FmpPath element for scheme://user@host/,
 * and return the remaining part in @remaining.
 */
static FmPath* _fm_path_new_uri_root(const char* uri, int len, const char** remaining)
{
    FmPath* path;
    GSList* l;
    char* buf;
    const char* uri_end = uri + len;
    const char* host;
    const char* host_end;
    const char *disp_name = NULL;
    int scheme_len, host_len;
    int flags;

    /* A generic URI: scheme://user@host/remaining/path */
    if(!g_ascii_isalpha(uri[0])) /* invalid */
        goto on_error;

    for(scheme_len = 1; scheme_len < len && uri[scheme_len] != ':';) /* find the : after scheme */
    {
        char ch = uri[scheme_len];
        if(g_ascii_isalnum(ch) || ch == '+' || ch == '-' || ch == '.') /* valid scheme characters */
            ++scheme_len;
        else /* this is not a valid URI */
            goto on_error;
    }

    if(scheme_len == len) /* there is no : in the URI, it's invalid */
        goto on_error;

    /* now there should be // following :, or no slashes at all, such as mailto:someone@somewhere.net */
    host = uri + scheme_len + 1;
    if(host + 2 < uri_end && host[0] == '/' && host[1] == '/')
        host += 2;

    flags = 0;
    host_len = 0;
    if(scheme_len == 4 && g_ascii_strncasecmp(uri, "file", 4) == 0 &&
       host[0] == '/') /* handle file:/ or file:///, file://host/ isn't our */
    {
        if(remaining)
            *remaining = host;
        return fm_path_ref(root_path);
    }
    /* special handling for some known schemes */
    else if(scheme_len == 5 && g_ascii_strncasecmp(uri, "trash", 5) == 0) /* trashed files are on local filesystems */
    {
        if(remaining)
            *remaining = host;
        return fm_path_ref(trash_root_path);
    }
    else if(scheme_len == 8 && g_ascii_strncasecmp(uri, "computer", 8) == 0)
    {
        flags |= FM_PATH_IS_VIRTUAL; /* FIXME: deprecated */
        host_end = host;
        disp_name = _("Devices");
    }
    else if(scheme_len == 7 && g_ascii_strncasecmp(uri, "network", 7) == 0)
    {
        flags |= FM_PATH_IS_VIRTUAL; /* FIXME: deprecated */
        host_end = host;
        disp_name = _("Network");
    }
    else if(scheme_len == 6 && g_ascii_strncasecmp(uri, "mailto", 6) == 0)
    {
        /* is any special handling needed? */
        if(remaining)
            *remaining = uri_end;
        return _fm_path_new_internal(NULL, uri, len, 0);
    }
    else /* it's a normal remote URI */
    {
        /* now we're at username@hostname, the authenticaion part */
        host_end = host;
        while(host_end < uri_end && *host_end != '/') /* find the end of host name */
            ++host_end;
        host_len = (host_end - host);
        if(scheme_len == 4 && g_ascii_strncasecmp(uri, "menu", 4) == 0)
        {
            if(host_len == 0) /* fallback to applications */
            {
                if(remaining)
                    *remaining = uri_end;
                return fm_path_ref(apps_root_path);
            }
            else if(host_len == 12 && strncmp(host, "applications", 12) == 0)
            {
                if(remaining)
                    *remaining = host_end;
                return fm_path_ref(apps_root_path);
            }
            flags |= (FM_PATH_IS_VIRTUAL|FM_PATH_IS_XDG_MENU); /* FIXME: deprecated */
        }
    }

    if(remaining)
        *remaining = host_end;

    /* it's reasonable to have double slashes :// for URIs other than mailto: */
    len = scheme_len + 3 + host_len + 1;
    G_LOCK(roots);
    for(l = roots; l; l = l->next)
    {
        path = l->data;
        if(strncmp(path->name, uri, scheme_len) == 0 &&
           (!host_len || !strncmp(&path->name[scheme_len + 3], host, host_len)) &&
           strcmp(&path->name[len-1], "/") == 0)
        {
            fm_path_ref(path);
            G_UNLOCK(roots);
            return path;
        }
    }
    path = _fm_path_alloc(NULL, len, flags);
    roots = g_slist_append(roots, path);
    G_UNLOCK(roots);
    buf = path->name;
    memcpy(buf, uri, scheme_len); /* the scheme */
    buf += scheme_len;
    memcpy(buf, "://", 3); /* :// */
    buf += 3;
    if(host_len > 0) /* host name */
    {
        memcpy(buf, host, host_len);
        buf += host_len;
    }
    buf[0] = '/'; /* the trailing / */
    buf[1] = '\0';
    if (disp_name)
        path->disp_name = g_strdup(disp_name); /* no lock required, it's new data */
    return path;

on_error: /* this is not a valid URI */
    if(remaining)
        *remaining = uri + len;
    return fm_path_ref(root_path);
}

/**
 * fm_path_new_child_len
 * @parent: (allow-none): a parent path
 * @basename: (allow-none): basename of a direct child of @parent directory
 * @name_len: length of @basename
 *
 * Creates new #FmPath for child of @parent directory which have name
 * @basename. The string length of @basename is @name_len. @basename is
 * in glib filename encoding (can be non-UTF-8) of target filesystem.
 * If @parent is %NULL then @basename assumed to be root of some file
 * system.
 *
 * Returns: (transfer full): a new #FmPath for the path. You have to call
 * fm_path_unref() when it's no longer needed.
 */
static
FmPath* _fm_path_new_child_len(FmPath* parent, const char* basename, int name_len,
                               gboolean dont_escape, gboolean is_query)
{
    FmPath* path;
    int flags;

    /* skip empty basename */
    if(G_UNLIKELY(!basename || name_len == 0))
        return parent ? fm_path_ref(parent) : NULL;

    if(G_LIKELY(parent)) /* remove slashes if needed. */
    {
        flags = parent->flags; /* inherit flags of parent */
        if (!is_query) while(basename[0] == '/')
        {
            ++basename;
            --name_len;
        }
        if (!is_query) while(name_len > 0 && basename[name_len-1] == '/')
            --name_len;

        /* special case for . and .. */
        if(basename[0] == '.')
        {
            if(name_len == 1) /* . */
                return fm_path_ref(parent);
            else if(name_len == 2 && basename[1] == '.') /* .. */
                return parent->parent ? fm_path_ref(parent->parent) : fm_path_ref(parent);
        }
    }
    else /* this basename is root of the fs (no parent), it can be "/" or something like "ftp://user@host/" */
    {
        /* remove duplicated leading slashes and treat them as one / */
        if(G_UNLIKELY(basename[0] == '/')) /* this is a posix path */
            return fm_path_ref(root_path);
        else /* This is something like: trash:///, computer:/// sftp://user@host/ */
            return _fm_path_new_uri_root(basename, name_len, NULL);
    }

    /* remove tailing slashes */
    if (!is_query) while(name_len > 0 && basename[name_len-1] == '/')
        --name_len;

    if(name_len == 0)
        return fm_path_ref(parent);

    /* create path data first */
    if(dont_escape)
    {
        path = _fm_path_alloc(parent, name_len, flags);
        memcpy(path->name, basename, name_len);
    }
    else
    {
        GString *str = g_string_new_len(basename, name_len);
        /* remote file names don't come escaped from gvfs; isn't that a bug of gvfs? */
        char *escaped = g_uri_escape_string(str->str, G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
        /* g_debug("got child %s", escaped); */
        name_len = strlen(escaped);
        path = _fm_path_alloc(parent, name_len, flags);
        memcpy(path->name, escaped, name_len);
        g_free(escaped);
        g_string_free(str, TRUE);
    }
    path->name[name_len] = '\0';

    G_LOCK(roots);
    if (parent->children)
    {
        GSequenceIter *iter;

        /* try to reuse existing path */
#if GLIB_CHECK_VERSION(2, 28, 0)
        iter = g_sequence_lookup(parent->children, path,
                                 (GCompareDataFunc)fm_path_compare, NULL);
#else
        iter = g_sequence_search(parent->children, path,
                                 (GCompareDataFunc)fm_path_compare, NULL);
        if (iter)
        {
            FmPath *pp = NULL;
            iter = g_sequence_iter_prev(iter);
            if (iter)
                pp = g_sequence_get(iter);
            if (fm_path_compare(pp, path) != 0)
                iter = NULL;
        }
#endif
        if (iter)
        {
                /* g_debug("found reusable path '%.*s'", name_len, basename); */
            FmPath *np = fm_path_ref(g_sequence_get(iter));
            G_UNLOCK(roots); /* we should not unref with lock up */
            fm_path_unref(path); /* drop this path and reuse found one */
            return np;
        }
        else
            path->iter = g_sequence_insert_sorted(parent->children, path,
                                                  (GCompareDataFunc)fm_path_compare,
                                                  NULL);
    }
    else
    {
        parent->children = g_sequence_new(NULL);
        path->iter = g_sequence_append(parent->children, path);
    }
    G_UNLOCK(roots);
    return path;
}

FmPath* fm_path_new_child_len(FmPath* parent, const char* basename, int name_len)
{
    return _fm_path_new_child_len(parent, basename, name_len,
                                  parent && fm_path_is_native(parent), FALSE);
}

/**
 * fm_path_new_child
 * @parent: (allow-none): a parent path
 * @basename: (allow-none): basename of a direct child of @parent directory
 *
 * Creates new #FmPath for child of @parent directory which have name
 * @basename. @basename is in glib filename encoding (can be non-UTF-8)
 * of target filesystem.
 *
 * Returns: (transfer full): a new #FmPath for the path. You have to call
 * fm_path_unref() when it's no longer needed.
 */
FmPath* fm_path_new_child(FmPath* parent, const char* basename)
{
    if(G_LIKELY(basename && *basename))
    {
        int baselen = strlen(basename);
        return _fm_path_new_child_len(parent, basename, baselen,
                                      parent && fm_path_is_native(parent), FALSE);
    }
    return G_LIKELY(parent) ? fm_path_ref(parent) : NULL;
}

/**
 * fm_path_new_for_gfile
 * @gf: a GFile object
 *
 * This function converts a GFile object to FmPath.
 *
 * Returns: a newly created FmPath for the path. You have to call
 * fm_path_unref() when it's no longer needed.
 */
FmPath* fm_path_new_for_gfile(GFile* gf)
{
    FmPath* path;
    char* str;
    if( g_file_is_native(gf) )
    {
        str = g_file_get_path(gf);
        path = fm_path_new_for_path(str);
    }
    else
    {
        str = g_file_get_uri(gf);
        path = fm_path_new_for_uri(str);
    }
    g_free(str);
    return path;
}

/**
 * fm_path_new_relative
 * @parent: (allow-none): a parent path
 * @rel: (allow-none): a path relative to @parent
 *
 * Creates new #FmPath which is relative to @parent directory by the
 * relative path string @rel. @rel is in glib filename encoding (can be
 * non-UTF-8) if @parent is native and should be escaped URI subpath
 * otherwise. For example, if @parent is "http://wiki.lxde.org/" and
 * @rel is "zh/\%E9\%A6\%96\%E9\%A0\%81", the resulting path will be
 * "http://wiki.lxde.org/zh/\%E9\%A6\%96\%E9\%A0\%81".
 *
 * If @parent is NULL, this works the same as fm_path_new_for_str(@rel)
 *
 * Returns: (transfer full): a new #FmPath for the path. You have to call
 * fm_path_unref() when it's no longer needed.
 */
FmPath* fm_path_new_relative(FmPath* parent, const char* rel)
{
    FmPath* path;
    if(G_UNLIKELY(!rel || !*rel)) /* relative path is empty */
        return parent ? fm_path_ref(parent) : fm_path_ref(root_path); /* return parent */

    /* g_debug("fm_path_new_relative: %s", rel); */
    if(G_LIKELY(parent))
    {
        char* sep;
        /* remove leading slashes */
        while(*rel == '/')
            ++rel;
        if(!*rel)
            path = fm_path_ref(parent);
        else
        {
            sep = strchr(rel, '/');
            if(sep)
            {
                FmPath *new_parent = _fm_path_new_child_len(parent, rel, sep - rel, TRUE, FALSE);
                path = fm_path_new_relative(new_parent, sep + 1);
                fm_path_unref(new_parent);
            }
            else
            {
                path = _fm_path_new_child_len(parent, rel, strlen(rel), TRUE, FALSE);
            }
        }
    }
    else /* this is actaully a full path */
        path = fm_path_new_for_str(rel);
    return path;
}

/**
 * fm_path_new_for_path
 * @path_name: (allow-none): a POSIX path.
 *
 * Returns: a newly created FmPath for the path. You have to call
 * fm_path_unref() when it's no longer needed.
 */
FmPath* fm_path_new_for_path(const char* path_name)
{
    FmPath* path;
    if(!path_name || !*path_name)
        return fm_path_ref(root_path);

    /* some special cases */
    if(G_LIKELY(path_name[0] == '/'))
    {
        if(G_UNLIKELY(path_name[1] == '\0')) /* pathname is / */
            path = fm_path_ref(root_path);
        else
            path = fm_path_new_relative(root_path, path_name + 1);
    }
    else /* pathname should be absolute path. otherwise its invalid */
        path = fm_path_ref(root_path); /* return root */
    return path;
}

/**
 * fm_path_new_for_uri
 * @uri: (allow-none): a URI with special characters escaped.
 *
 * Creates new #FmPath by given @uri. You have to call
 * fm_path_unref() when it's no longer needed.
 *
 * You can call fm_path_to_uri() to convert a FmPath to a escaped URI
 * string.
 *
 * This function never fail but it may return root path if URI is local
 * and contains illegal characters in path elements such as '/'.
 *
 * Returns: (transfer full): a new #FmPath for the @uri.
 */
FmPath* fm_path_new_for_uri(const char* uri)
{
    FmPath* path, *root;
    const char* rel_path;
    if(!uri || !*uri)
        return fm_path_ref(root_path);

    root = _fm_path_new_uri_root(uri, strlen(uri), &rel_path);
    if(*rel_path)
    {
        if(root == root_path)
        {
            /* handle file:/// URIs */
            char *filename = g_filename_from_uri(uri, NULL, NULL);
            /* NOTE: g_filename_from_uri() will fail on unescaping '%2F' */
            path = fm_path_new_relative(root, filename);
            g_free(filename);
        }
        else if (strchr(rel_path, '?') != NULL) /* it's query not regular URI */
            path = _fm_path_new_child_len(root, rel_path+1, strlen(rel_path)-1,
                                          TRUE, TRUE);
        else
            path = fm_path_new_relative(root, rel_path);
        fm_path_unref(root);
    }
    else
        path = root;
    return path;
}

/* looks for known display_name, returns referenced child */
static FmPath *_lookup_in_children(FmPath *path, const char *display_name)
{
    FmPath *subpath = NULL;

    G_LOCK(roots);
    if (path->children != NULL)
    {
        GSequenceIter *iter = g_sequence_get_begin_iter(path->children);
        const char *name;

        while (!g_sequence_iter_is_end(iter))
        {
            path = (FmPath*)g_sequence_get(iter);
            name = path->disp_name;
            if (name)
            {
                if (name == BASENAME_AS_DISP_NAME)
                    name = path->name;
                if (strcmp(display_name, name) == 0)
                {
                    subpath = fm_path_ref(path);
                    break;
                }
            }
            iter = g_sequence_iter_next(iter);
        }
    }
    G_UNLOCK(roots);
    return subpath;
}

/**
 * fm_path_new_for_display_name
 * @path_name: an UTF-8 encoded display path name
 *
 * The @path_name can either be a POSIX path in UTF-8 encoding, or an
 * unescaped URI (can contain non-ASCII characters and spaces). Returned
 * data should be freed with fm_path_unref() when it's no longer needed.
 * Display name may be fully unrelated to it's path name and built path
 * may be inexistant. Use FmJob to check and validate its existance.
 *
 * Returns: a newly created #FmPath.
 *
 * Since: 0.1.14
 */
FmPath* fm_path_new_for_display_name(const char* path_name)
{
    FmPath *path;
    FmPath *cpath;
    GFile *file = NULL, *child;
    char *path_copy, *c, *sep;
    char sep_char;

    if(!path_name || !*path_name || (path_name[0]=='/' && path_name[1] == '\0') )
        return fm_path_ref(root_path);

    if (path_name[0] != '/') /* it's an URI */
    {
        sep = strchr(path_name, ':');
        if (!sep) /* invalid path */
            /* FIXME: fail on this */
            return fm_path_ref(root_path);
        /* FIXME: should we test for "C:\path..." too? */
        if (sep[1] != '/') /* something like mailto:xx@yy */
            return fm_path_new_for_uri(path_name);
        if (sep[2] != '/') /* malformed URI */
            return fm_path_new_for_str(path_name); /* FIXME: handle this? */
        path_copy = g_strdup(path_name);
        c = path_copy + (sep + 3 - path_name); /* set after xx:// */
        sep = strchr(c, '/');
        if (sep)
        {
            if (strchr(sep, '?') != NULL)
            {
                /* it is not URI path but rather URI query, pass it "as is" */
                path = fm_path_new_for_uri(path_copy);
                goto _finish;
            }
            /* terminate root URI after third slash */
            sep++;
        }
        /* use c for escaped copy of root path temporarily
           we use another set of allowed chars, see bug #3599638 */
        c = g_uri_escape_string(path_copy,
                                G_URI_RESERVED_CHARS_GENERIC_DELIMITERS
                                G_URI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS,
                                FALSE);
        path = _fm_path_new_uri_root(c, strlen(c), NULL);
        /* g_debug("** display URI root: %s",path->name); */
        g_free(c);
        c = sep;
        sep_char = '/'; /* separator for URI is always slash */
    }
    else
    {
        path_copy = g_strdup(path_name);
        path = fm_path_ref(root_path);
        c = path_copy + 1;
        sep_char = G_DIR_SEPARATOR;
    }
    for ( ; c; c = sep)
    {
        sep = strchr(c, sep_char);
        if (sep)
            *sep++ = '\0'; /* separate one part of path */
        /* g_debug("** next subpath: %s",c); */
        if (*c == '\0') /* duplicate '/' */
            continue;
        if (strcmp(c, ".") == 0) /* skip "." */
            continue;
        /* check if we have it already cached */
        cpath = _lookup_in_children(path, c);
        if (cpath)
        {
            /* g_debug("** found child: %s => %s",c,cpath->name); */
        }
        else if (strcmp(c, "..") == 0) /* go back one dir */
        {
            if (path->parent == NULL) /* attempt to go below root */
                break;
            cpath = fm_path_ref(path->parent);
        }
        else
        {
            file = fm_path_to_gfile(path);
            child = g_file_get_child_for_display_name(file, c, NULL);
            g_object_unref(file);
            if (child) /* successfull conversion, use name we got */
            {
                /* char *cc = g_file_get_uri(child);
                g_debug("** resolved display name: %s => %s",c,cc);
                g_free(cc); */
                c = g_file_get_basename(child);
            }
            /* otherwise use basename as fallback */
            cpath = fm_path_new_child(path, c);
            /* g_debug("** adding child: %s",cpath->name); */
            if (child)
            {
                g_free(c);
                g_object_unref(child);
            }
        }
        fm_path_unref(path);
        path = cpath;
    }
_finish:
    g_free(path_copy);
    return path;
}

/**
 * fm_path_new_for_str
 * @path_str: (allow-none): a string representing the file path in its native
 * encoding (can be non-UTF-8). It can either be a native path or an
 * unescaped URI (can contain non-ASCII characters and spaces).
 * The function will try to figure out what to do.
 *
 * You can call fm_path_to_str() to convert a FmPath back to its string
 * presentation.
 *
 * Returns: a newly created FmPath for the path. You have to call
 * fm_path_unref() when it's no longer needed.
 */
FmPath* fm_path_new_for_str(const char* path_str)
{
    char *escaped;
    FmPath *path;

    if(!path_str || !*path_str)
        return fm_path_ref(root_path);
    if(path_str[0] == '/')
        return fm_path_new_for_path(path_str);
    /* UTF-8 should be allowed, I think. */
    escaped = g_uri_escape_string(path_str,
                                  G_URI_RESERVED_CHARS_GENERIC_DELIMITERS
                                  G_URI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS,
                                  TRUE);
    path = fm_path_new_for_uri(escaped);
    g_free(escaped);
    return path;
}

/**
 * fm_path_new_for_commandline_arg
 * @arg: a file path passed in command line argv to the program. The @arg
 * can be a POSIX path in glib filename encoding (can be non-UTTF-8) and
 * can be a URI with non-ASCII characters escaped, like
 * http://wiki.lxde.org/zh/\%E9\%A6\%96\%E9\%A0\%81.
 *
 * Returns: a newly created FmPath for the path. You have to call
 * fm_path_unref() when it's no longer needed.
 */
FmPath* fm_path_new_for_commandline_arg(const char* arg)
{
    if(!arg || !*arg || (arg[0]=='/' && arg[1] == '\0'))
        return fm_path_ref(root_path);
    if(arg[0] == '/')
        return fm_path_new_for_path(arg);
    return fm_path_new_for_uri(arg);
}

/**
 * fm_path_ref
 * @path: an existing #FmPath
 *
 * Increases reference count on @path.
 *
 * Returns: @path.
 */
FmPath* fm_path_ref(FmPath* path)
{
    g_return_val_if_fail(path != NULL, NULL);
    g_atomic_int_inc(&path->n_ref);
    return path;
}

/**
 * fm_path_unref
 * @path: an existing #FmPath
 *
 * Decreases reference count on @path. When reference count becomes 0
 * the @path will be destroyed.
 */
void fm_path_unref(FmPath* path)
{
    g_return_if_fail(path != NULL);
    /* g_debug("fm_path_unref: %s, n_ref = %d", fm_path_to_str(path), path->n_ref); */
    if(g_atomic_int_dec_and_test(&path->n_ref))
    {
        G_LOCK(roots);
        if(G_LIKELY(path->parent))
        {
            if (G_LIKELY(path->iter))
                g_sequence_remove(path->iter);
                /* otherwise it's fresh abandoned one, see above */
            G_UNLOCK(roots); /* we should not unref with lock up */
            fm_path_unref(path->parent);
        }
        else
        {
            roots = g_slist_remove(roots, path);
            G_UNLOCK(roots);
        }
        /* no further lock required, it is removed from lists */
        if (path->disp_name != BASENAME_AS_DISP_NAME)
            g_free(path->disp_name);
        if (G_UNLIKELY(path->children))
        {
            g_assert(g_sequence_get_length(path->children) == 0);
            g_sequence_free(path->children);
        }
        g_free(path);
    }
}

/**
 * fm_path_get_parent
 * @path: a path
 *
 * Retrieves path of directory containing @path.
 * Returned data are owned by @path and should be not freed by caller.
 *
 * Returns: (transfer none): path of parent directory or %NULL if @path is root path.
 *
 * Since: 0.1.0
 */
FmPath* fm_path_get_parent(FmPath* path)
{
    return path->parent;
}

/**
 * fm_path_get_scheme_path
 * @path: a path
 *
 * Retrieves a root path for scheme used by @path. If @path is a local
 * path then returned data are the same as returned with fm_path_get_root().
 * Returned data are owned by @path and should be not freed by caller.
 *
 * Returns: (transfer none): path of root of @path scheme.
 *
 * Since: 1.2.0
 */
FmPath* fm_path_get_scheme_path(FmPath* path)
{
    g_return_val_if_fail(path != NULL, NULL);
    while(path->parent != NULL)
        path = path->parent;
    return path;
}

/**
 * fm_path_get_basename
 * @path: a path
 *
 * Retrieves basename of @path.
 * Returned data are owned by @path and should be not freed by caller.
 *
 * Returns: basename of path.
 *
 * Since: 0.1.0
 */
const char* fm_path_get_basename(FmPath* path)
{
    return path->name;
}

/**
 * fm_path_get_flags
 * @path: a path
 *
 * Retrieves attributes of @path.
 *
 * Returns: attributes of path.
 *
 * Since: 0.1.0
 */
FmPathFlags fm_path_get_flags(FmPath* path)
{
    return path->flags;
}

/**
 * fm_path_has_prefix
 * @path: a sub path
 * @prefix: a prefix
 *
 * Check if @prefix is a prefix of @path.
 * For example: /usr/share is the prefix of /usr/share/docs/libfm
 * but /etc is not.
 *
 * Returns: TRUE if @prefix is the prefix of @path.
 */
gboolean fm_path_has_prefix(FmPath* path, FmPath* prefix)
{
    for(; path; path = path->parent)
    {
        if(fm_path_equal(path, prefix))
            return TRUE;
    }
    return FALSE;
}

/* recursive internal implem. of fm_path_to_str returns end of current
   build string */
static gchar* fm_path_to_str_int(FmPath* path, gchar** ret, gint str_len)
{
    gint name_len = strlen(path->name);
    gchar* pbuf;

    if (!path->parent)
    {
        *ret = g_new0(gchar, str_len + name_len + 1 );
        pbuf = *ret;
    }
    else
    {
        pbuf = fm_path_to_str_int( path->parent, ret, str_len + name_len + 1 );
        if (path->parent->parent) /* if parent dir is not root_path */
            *pbuf++ = G_DIR_SEPARATOR;
    }
    memcpy( pbuf, path->name, name_len );
    return pbuf + name_len;
}

/**
 * fm_path_to_str
 * @path: a path
 *
 * Creates string representation of @path. It can be either file path in
 * local encoding or URI with non-ASCII characters escaped (RFC 3986).
 * Returned data should be freed with g_free() after usage.
 *
 * Returns: (transfer full): path string.
 *
 * Since: 0.1.0
 */
char* fm_path_to_str(FmPath* path)
{
    gchar *ret;
    fm_path_to_str_int( path, &ret, 0 );
    return ret;
}

/**
 * fm_path_to_uri
 * @path: a path
 *
 * Creates URI representation of @path.
 * Returned data should be freed with g_free() after usage.
 *
 * Returns: path URI.
 *
 * Since: 0.1.0
 */
char* fm_path_to_uri(FmPath* path)
{
    char* uri = NULL;
    char* str = fm_path_to_str(path);
    if( G_LIKELY(str) )
    {
        if(str[0] == '/') /* absolute path */
            uri = g_filename_to_uri(str, NULL, NULL);
        else /* it's already an URI */
            return str;
        g_free(str);
    }
    return uri;
}

/**
 * fm_path_display_name
 * @path: a path
 * @human_readable: ignored since 1.2.0
 *
 * Creates string representation of @path as displayable UTF-8 string.
 * The conversion is the most probably unreversible so returned value
 * should be used only for displaying purposes. The displayable path
 * may be reliable only if there was already #FmFileInfo retrieved for
 * the @path, otherwise some estimation will be returned instead, be
 * careful with this API.
 * The main purpose for this API is to create displayable path for the
 * path entry. Applications are encouraged to use g_file_get_parse_name()
 * instead for any other purposes.
 * Returned data should be freed with g_free() after usage.
 *
 * See also: fm_path_display_basename(), fm_path_to_str().
 *
 * Returns: (transfer full): path string.
 *
 * Since: 0.1.0
 */
/* FIXME: maybe we can support different encoding for different mount points? */
static char *_fm_path_display_name(FmPath* path, gboolean *is_query)
{
    char* disp;

    if(G_LIKELY(path->parent))
    {
        char* disp_parent;
        char* disp_base;
        if (G_UNLIKELY(*is_query)) /* already detected query */
            goto _is_query;
        else if (fm_path_is_native(path))
        {
            disp_parent = _fm_path_display_name(path->parent, is_query);
            disp_base = fm_path_display_basename(path);
        }
        else if (strchr(path->name, '?') == NULL)
        {
            /* URI query not detected yet, check parent beforehand */
            disp_parent = _fm_path_display_name(path->parent, is_query);
            if (G_UNLIKELY(*is_query)) /* parent found that it is a query URI */
                goto _is_query2;
            else
                disp_base = fm_path_display_basename(path);
        }
        else
        {
            /* it is not URI path but rather URI query, pass it "as is" */
            *is_query = TRUE;
_is_query:
            disp_parent = _fm_path_display_name(path->parent, is_query);
_is_query2:
            disp = g_strjoin(NULL, disp_parent, path->name, NULL);
            g_free(disp_parent);
            return disp;
        }
        if (fm_path_is_native(path))
            disp = g_build_filename( disp_parent, disp_base, NULL);
        else
            disp = g_build_path("/", disp_parent, disp_base, NULL);
        g_free(disp_parent);
        g_free(disp_base);
    }
    else
        disp = g_uri_unescape_string(path->name, NULL);
    return disp;
}

char* fm_path_display_name(FmPath* path, gboolean human_readable)
{
    gboolean is_query = FALSE;
    return _fm_path_display_name(path, &is_query);
}

/**
 * fm_path_display_basename
 * @path: a path
 *
 * Creates displayable basename of @path. If @path is a scheme root path
 * (i.e. "/", "sftp://", etc.) then returns basename of @path. For other
 * paths displayable basename may be reliable only if there was already
 * #FmFileInfo retrieved for the @path, otherwise some estimation will be
 * done instead, be careful with this API.
 * Returned data should be freed with g_free() after usage.
 *
 * See also: fm_file_info_get_disp_name().
 *
 * Returns: (transfer full): displayable basename of path.
 *
 * Since: 0.1.0
 */
/* FIXME: maybe we can support different encoding for different mount points? */
char* fm_path_display_basename(FmPath* path)
{
    if(G_UNLIKELY(!path->parent)) /* root_path element */
        return g_strdup(path->name);
    G_LOCK(roots);
    if (G_LIKELY(path->disp_name == BASENAME_AS_DISP_NAME))
    {
        G_UNLOCK(roots);
        return g_strdup(path->name);
    }
    if (path->disp_name)
    {
        char *name = g_strdup(path->disp_name);
        G_UNLOCK(roots);
        return name;
    }
    G_UNLOCK(roots);
    if(!fm_path_is_native(path))
        return g_uri_unescape_string(path->name, NULL);
    return g_filename_display_name(path->name);
}

void _fm_path_set_display_name(FmPath *path, const char *disp_name)
{
    if (!disp_name || !*disp_name)
    {
        char *_name = NULL;
        if(!fm_path_is_native(path))
            _name = g_uri_unescape_string(path->name, NULL);
        else
            _name = g_filename_display_name(path->name);
        _fm_path_set_display_name(path, _name);
        g_free(_name);
        return;
    }
    G_LOCK(roots);
    if (path->disp_name != BASENAME_AS_DISP_NAME)
    {
        /* check if it is set already */
        if (g_strcmp0(disp_name, path->disp_name) == 0)
        {
            G_UNLOCK(roots);
            return;
        }
        g_free(path->disp_name);
    }
    /* g_debug("set display name of %s to %s", path->name, disp_name); */
    /* By default we use the real file base name for display.
     * if the base name is not in UTF-8 encoding, we
     * need to convert it to UTF-8 for display and save its
     * UTF-8 version in fi->disp_name */
    if (g_strcmp0(disp_name, path->name) == 0)
        path->disp_name = BASENAME_AS_DISP_NAME;
    else
        path->disp_name = g_strdup(disp_name);
    G_UNLOCK(roots);
}

/* use this to avoid change from another thread */
static char *_display_name_static_keeper = NULL;

/* this API is not thread capable! */
const char *_fm_path_get_display_name(FmPath *path)
{
    G_LOCK(roots);
    if (path->disp_name == BASENAME_AS_DISP_NAME)
    {
        G_UNLOCK(roots);
        return path->name;
    }
    g_free(_display_name_static_keeper);
    /* use this to keep disp_name after returning from lock because another
       thread may change it at that time, although _display_name_static_keeper
       isn't protected by lock so should be protected by general glib lock */
    _display_name_static_keeper = g_strdup(path->disp_name);
    G_UNLOCK(roots);
    return _display_name_static_keeper;
}

/**
 * fm_path_to_gfile
 * @path: a path
 *
 * Creates #GFile representation of @path.
 * Returned data should be freed with g_object_unref() after usage.
 *
 * Returns: (transfer full): a #GFile object.
 *
 * Since: 0.1.0
 */
GFile* fm_path_to_gfile(FmPath* path)
{
    GFile* gf;
    char* str;
    str = fm_path_to_str(path);
    if(fm_path_is_native(path))
        gf = g_file_new_for_path(str);
    else
        gf = fm_file_new_for_uri(str);
    g_free(str);
    return gf;
}

/**
 * fm_path_get_root
 *
 * Retrieves #FmPath for root directory.
 * Returned data are owned by @path and should be not freed by caller.
 *
 * Returns: a path.
 *
 * Since: 0.1.0
 */
FmPath* fm_path_get_root()
{
    return root_path;
}

/**
 * fm_path_get_home
 *
 * Retrieves #FmPath for home directory.
 * Returned data are owned by @path and should be not freed by caller.
 *
 * Returns: a path.
 *
 * Since: 0.1.0
 */
FmPath* fm_path_get_home()
{
    return home_path;
}

/**
 * fm_path_get_desktop
 *
 * Retrieves #FmPath for desktop directory.
 * Returned data are owned by @path and should be not freed by caller.
 *
 * Returns: a path.
 *
 * Since: 0.1.0
 */
FmPath* fm_path_get_desktop()
{
    return desktop_path;
}

/**
 * fm_path_get_trash
 *
 * Retrieves #FmPath for Trash can.
 * Returned data are owned by @path and should be not freed by caller.
 *
 * Returns: a path.
 *
 * Since: 0.1.0
 */
FmPath* fm_path_get_trash()
{
    return trash_root_path;
}

/**
 * fm_path_get_apps_menu
 *
 * Retrieves #FmPath for menu:// virtual directory.
 * Returned data are owned by @path and should be not freed by caller.
 *
 * Returns: a path.
 *
 * Since: 0.1.0
 */
FmPath* fm_path_get_apps_menu()
{
    return apps_root_path;
}

void _fm_path_init()
{
    const char* sep, *name;
    FmPath* tmp, *parent;

    /* path object of root_path dir */
    root_path = _fm_path_new_internal(NULL, "/", 1, FM_PATH_IS_LOCAL|FM_PATH_IS_NATIVE);
    home_dir = fm_get_home_dir();
    home_len = strlen(home_dir);
    while(home_dir[home_len - 1] == '/')
        --home_len;

    /* build path object for home dir */
    name = home_dir + 1; /* skip leading / */
    parent = fm_path_ref(root_path);
    while((sep = strchr(name, '/')))
    {
        int len = (sep - name);
        if(len > 0)
        {
            tmp = _fm_path_new_internal(parent, name, len, FM_PATH_IS_LOCAL|FM_PATH_IS_NATIVE);
            fm_path_unref(parent);
            parent = tmp;
        }
        name = sep + 1;
    }
    home_path = _fm_path_new_internal(parent, name, strlen(name), FM_PATH_IS_LOCAL|FM_PATH_IS_NATIVE);
    fm_path_unref(parent);

    desktop_dir = g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP);
    desktop_len = strlen(desktop_dir);
    while(desktop_dir[desktop_len - 1] == '/')
        --desktop_len;

    /* build path object for desktop_path dir */
    if(G_UNLIKELY(strncmp(desktop_dir, home_dir, home_len)))
    {
        /* can it be that desktop_dir is outside of home_dir ? */
        name = &desktop_dir[1];
        parent = fm_path_ref(root_path);
    }
    else
    {
        name = desktop_dir + home_len + 1; /* skip home_path dir part / */
        while (*name == '/') name++; /* skip extra / if any */
        if (*name == '\0')
            name = "Desktop"; /* fallback: never use home_path as desktop_path */
        parent = fm_path_ref(home_path); /* preserve ref */
    }
    while((sep = strchr(name, '/')))
    {
        int len = (sep - name);
        if(len > 0)
        {
            tmp = _fm_path_new_internal(parent, name, len, FM_PATH_IS_LOCAL|FM_PATH_IS_NATIVE);
            fm_path_unref(parent); /* parent is reffed by new child */
            parent = tmp;
        }
        name = sep + 1;
    }
    desktop_path = _fm_path_new_internal(parent, name, strlen(name), FM_PATH_IS_LOCAL|FM_PATH_IS_NATIVE);
    fm_path_unref(parent); /* we used it already */

    /* build path object for trash can */
    trash_root_path = _fm_path_new_internal(NULL, "trash:///", 9, FM_PATH_IS_TRASH|FM_PATH_IS_VIRTUAL|FM_PATH_IS_LOCAL);
    _fm_path_set_display_name(trash_root_path, _("Trash Can"));
    apps_root_path = _fm_path_new_internal(NULL, "menu://applications/", 20, FM_PATH_IS_VIRTUAL|FM_PATH_IS_XDG_MENU);
}

void _fm_path_finalize(void)
{
    fm_path_unref(root_path);
    fm_path_unref(home_path);
    fm_path_unref(desktop_path);
    fm_path_unref(trash_root_path);
    fm_path_unref(apps_root_path);
    root_path = home_path = desktop_path = trash_root_path = apps_root_path = NULL;
}

/* For used in hash tables */

/**
 * fm_path_hash
 * @path: a path key
 *
 * Converts a path to a hash value.
 *
 * Returns: a hash value corresponding to the key.
 *
 * Since: 0.1.0
 */
/* FIXME: is this good enough? */
guint fm_path_hash(FmPath* path)
{
    guint hash = g_str_hash(path->name);
    if(path->parent)
    {
        /* this is learned from g_str_hash() of glib. */
        hash = (hash << 5) - hash + '/';
        /* this is learned from g_icon_hash() of gio. */
        hash ^= fm_path_hash(path->parent);
    }
    return hash;
}

/**
 * fm_path_equal
 * @p1: first path
 * @p2: second path
 *
 * Compares two paths and returns %TRUE if they are equal.
 *
 * Note that this function is primarily meant as a hash table comparison
 * function.
 *
 * See also: fm_path_compare().
 *
 * Returns: %TRUE if paths are equal.
 *
 * Since: 0.1.0
 */
/* this is not equal to fm_path_compare()==0
 * fm_path_equal is optimized for equality
 * while fm_path_compare() needs to determine
 * orders of two items, which is slower.
 * When you only care about equality, not orders,
 * use this. */
gboolean fm_path_equal(FmPath* p1, FmPath* p2)
{
    return (p1 == p2);
}

/*
 * fm_path_compare
 * @p1: path 1
 * @p2: path 2
 *
 * Compare two paths to determine their orders.
 *
 * Note that this function is primarily meant for sorting and therefore
 * is slow. If you need only know if paths are equal then use
 * fm_path_equal() instead.
 *
 * See also: fm_path_equal().
 *
 * Returns: -1 if @p1 is less than @p2, 0 if they're equal, and +1 if
 * @p1 is greater than @p2.
 *
 * Since: 1.0.2
 */
int fm_path_compare(FmPath* p1, FmPath* p2)
{
    int result;
    if(p1 == p2)
        return 0;
    if(!p1) /* if p2 is also NULL then p1==p2 and that is handled above */
        return -1;
    if(!p2) /* case of p1==NULL handled above */
        return 1;
    result = fm_path_compare(p1->parent, p2->parent);
    if(result == 0) /* if parent paths are equal, compare children */
        result = strcmp(p1->name, p2->name);
    return result;
}

/**
 * fm_path_equal_str
 * @path: a path
 * @str: a string
 * @n: length of @string
 *
 * Compares path string representation with @string.
 *
 * Returns: %TRUE if path and string are equal.
 *
 * Since: 0.1.0
 */
/* Check if this path contains absolute pathname str*/
gboolean fm_path_equal_str(FmPath *path, const gchar *str, int n)
{
    const gchar *last_part;

    if(G_UNLIKELY(!path))
        return FALSE;

    /* default compare str len */
    if (n == -1)
        n = strlen( str );

    /* end of recursion */
    if ((path->parent == NULL) && g_str_equal ( path->name, "/" ) && n == 0 )
        return TRUE;

    /* must also contain leading slash */
    if ((size_t)n < (strlen(path->name) + 1))
        return FALSE;

    /* check for current part mismatch */
    last_part  = str + n - strlen(path->name) - 1;
    if ( strncmp( last_part + 1, path->name, strlen(path->name)) != 0 )
        return FALSE;
    if ( *last_part != G_DIR_SEPARATOR )
        return FALSE;

    /* tail-end recursion */
    return fm_path_equal_str( path->parent, str, n - strlen(path->name) - 1 );
}

/**
 * fm_path_depth
 * @path: a path
 *
 * Calculates how many elements are in this path.
 *
 * Returns: %TRUE if paths are equal.
 *
 * Since: 1.0.0
 */
int fm_path_depth(FmPath* path)
{
    int depth = 1;
    while(path->parent)
    {
        ++depth;
        path = path->parent;
    }
    return depth;
}


/* path list */

static FmListFuncs funcs =
{
    .item_ref = (gpointer (*)(gpointer))&fm_path_ref,
    .item_unref = (void (*)(gpointer))&fm_path_unref
};

FmPathList* fm_path_list_new()
{
    return (FmPathList*)fm_list_new(&funcs);
}

/**
 * fm_path_list_new_from_uris
 * @uris: NULL-terminated list of URIs
 *
 * Creates a #FmPathList from @uris.
 *
 * Returns: (transfer full): new #FmPathList.
 *
 * Since: 0.1.0
 */
FmPathList* fm_path_list_new_from_uris(char* const* uris)
{
    char* const* uri;
    FmPathList* pl = fm_path_list_new();
    for(uri = uris; *uri; ++uri)
    {
        const char* puri = *uri;
        if(puri[0] != '\0') /* ensure that it's not an empty string */
        {
            FmPath* path;
            if(puri[0] == '/')
                path = fm_path_new_for_path(puri);
            else
            {
                path = fm_path_new_for_uri(puri);
                if (path == root_path)
                {
                    /* it's not a valid path or URI */
                    fm_path_unref(path);
                    continue;
                }
            }
            fm_list_push_tail_noref((FmList*)pl, path);
        }
    }
    return pl;
}

/**
 * fm_path_list_new_from_uri_list
 * @uri_list: list of URIs separated by newline characters
 *
 * Creates a #FmPathList from @uri_list.
 *
 * Returns: (transfer full): new #FmPathList.
 *
 * Since: 0.1.0
 */
FmPathList* fm_path_list_new_from_uri_list(const char* uri_list)
{
    /* The line separator used by text/urilist is CRLF, not LF. However, some
      existing implementations seems to use '\n'. Let's handle both. */
    char** uris = g_strsplit_set(uri_list, "\r\n", -1);
    FmPathList* pl = fm_path_list_new_from_uris(uris);
    g_strfreev(uris);
    return pl;
}

/**
 * fm_path_list_to_uri_list
 * @pl: a path list
 *
 * Creates newline-separated list from @pl. Returned data should be freed
 * with g_free() after usage.
 *
 * Returns: (transfer full): string representation of @pl.
 *
 * Since: 0.1.0
 */
char* fm_path_list_to_uri_list(FmPathList* pl)
{
    GString* buf = g_string_sized_new(4096);
    fm_path_list_write_uri_list(pl, buf);
    return g_string_free(buf, FALSE);
}

/*
char** fm_path_list_to_uris(FmPathList* pl)
{
    if( G_LIKELY(!fm_list_is_empty(pl)) )
    {
        GList* l = fm_list_peek_head_link(pl);
        char** uris = g_new0(char*, fm_list_get_length(pl) + 1);
        for(i=0; l; ++i, l=l->next)
        {
            FmFileInfo* fi = (FmFileInfo*)l->data;
            FmPath* path = fi->path;
            char* uri = fm_path_to_uri(path);
            uris[i] = uri;
        }
    }
    return NULL;
}
*/

/**
 * fm_path_list_new_from_file_info_list
 * @fis: a file info list
 *
 * Creates a #FmPathList from @fis.
 *
 * Returns: (transfer full): new #FmPathList.
 *
 * Since: 0.1.0
 */
FmPathList* fm_path_list_new_from_file_info_list(FmFileInfoList* fis)
{
    FmPathList* list = fm_path_list_new();
    GList* l;
    for(l=fm_list_peek_head_link((FmList*)fis);l;l=l->next)
    {
        FmFileInfo* fi = (FmFileInfo*)l->data;
        fm_path_list_push_tail(list, fm_file_info_get_path(fi));
    }
    return list;
}

/**
 * fm_path_list_new_from_file_info_glist
 * @fis: (element-type FmFileInfo): list of file infos
 *
 * Creates a #FmPathList from @fis.
 *
 * Returns: (transfer full): new #FmPathList.
 *
 * Since: 0.1.0
 */
FmPathList* fm_path_list_new_from_file_info_glist(GList* fis)
{
    FmPathList* list = fm_path_list_new();
    GList* l;
    for(l=fis;l;l=l->next)
    {
        FmFileInfo* fi = (FmFileInfo*)l->data;
        fm_path_list_push_tail(list, fm_file_info_get_path(fi));
    }
    return list;
}

/**
 * fm_path_list_new_from_file_info_gslist
 * @fis: (element-type FmFileInfo): list of file infos
 *
 * Creates a #FmPathList from @fis.
 *
 * Returns: (transfer full): new #FmPathList.
 *
 * Since: 0.1.0
 */
FmPathList* fm_path_list_new_from_file_info_gslist(GSList* fis)
{
    FmPathList* list = fm_path_list_new();
    GSList* l;
    for(l=fis;l;l=l->next)
    {
        FmFileInfo* fi = (FmFileInfo*)l->data;
        fm_path_list_push_tail(list, fm_file_info_get_path(fi));
    }
    return list;
}

/**
 * fm_path_list_write_uri_list
 * @pl: a path list
 * @buf: (out): a storage for resulting list
 *
 * Creates newline-separated list of URIs from @pl.
 *
 * Since: 0.1.0
 */
void fm_path_list_write_uri_list(FmPathList* pl, GString* buf)
{
    GList* l;
    for(l = fm_path_list_peek_head_link(pl); l; l=l->next)
    {
        FmPath* path = (FmPath*)l->data;
        char* uri = fm_path_to_uri(path);
        g_string_append(buf, uri);
        g_free(uri);
        if(l->next)
            g_string_append_c(buf, '\n');
    }
}
