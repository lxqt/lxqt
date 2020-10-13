/*
 *      fm-file.c
 *
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
 * SECTION:fm-file
 * @short_description: Extensions for GFile interface.
 * @title: FmFile
 *
 * @include: libfm/fm.h
 *
 * The #FmFile represents interface to build extensions to GFile which
 * will handle schemas that are absent in Glib/GVFS - such as "search:".
 *
 * To use it the GFile implementation should also implement FmFile vtable
 * calls. The implementation should be added to list of known schemes via
 * call to fm_file_add_vfs() then calls such as fm_file_new_for_uri() can
 * use it.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <glib-compat.h>

#include "fm-file.h"

#include <string.h>

static GHashTable *schemes = NULL;

#define FM_FILE_MODULE_MIN_VERSION 1
#define FM_FILE_MODULE_MAX_VERSION 1

G_LOCK_DEFINE_STATIC(vfs);

G_DEFINE_INTERFACE(FmFile, fm_file, G_TYPE_FILE)

static gboolean fm_file_wants_incremental_false(GFile *unused)
{
    return FALSE;
}

static void fm_file_default_init(FmFileInterface *iface)
{
    iface->wants_incremental = fm_file_wants_incremental_false;
}


static inline FmFileInitTable *fm_find_scheme(const char *name)
{
    FmFileInitTable *t;
    CHECK_MODULES();
    G_LOCK(vfs);
    t = (FmFileInitTable*)g_hash_table_lookup(schemes, name);
    G_UNLOCK(vfs);
    return t;
}

/**
 * fm_file_add_vfs
 * @name: scheme to act upon
 * @init: table of functions
 *
 * Adds VFS to list of extensions that will be applied on next call to
 * fm_file_new_for_uri() or fm_file_new_for_commandline_arg(). The @name
 * is a schema which will be handled by those calls.
 *
 * Since: 1.0.2
 */
void fm_file_add_vfs(const char *name, FmFileInitTable *init)
{
    G_LOCK(vfs);
    if(g_hash_table_lookup(schemes, name) == NULL)
        g_hash_table_insert(schemes, g_strdup(name), init);
    G_UNLOCK(vfs);
}

/**
 * fm_file_wants_incremental
 * @file: file to inspect
 *
 * Checks if contents of directory @file cannot be retrieved at once so
 * scanning it may be done in incremental manner for the best results.
 *
 * Returns: %TRUE if retrieve of contents of @file will be incremental.
 *
 * Since: 1.0.2
 */
gboolean fm_file_wants_incremental(GFile* file)
{
    FmFileInterface *iface;

    g_return_val_if_fail(file != NULL, FALSE);
    if(!FM_IS_FILE(file))
        return FALSE;
    iface = FM_FILE_GET_IFACE(file);
    return iface->wants_incremental ? iface->wants_incremental(file) : FALSE;
}

/**
 * fm_file_new_for_uri
 * @uri: a UTF8 string containing a URI
 *
 * Constructs a #GFile for a given URI. This operation never fails,
 * but the returned object might not support any I/O operation if @uri
 * is malformed or if the uri type is not supported.
 *
 * Returns: a new #GFile.
 *
 * Since: 1.0.2
 */
GFile *fm_file_new_for_uri(const char *uri)
{
    char *scheme;
    FmFileInitTable *iface;
    GFile *file = NULL;

    scheme = g_uri_parse_scheme(uri);
    if(scheme)
    {
        iface = fm_find_scheme(scheme);
        if(iface && iface->new_for_uri)
            file = iface->new_for_uri(uri);
        g_free(scheme);
        if(file)
            return file;
    }
    return g_file_new_for_uri(uri);
}

/**
 * fm_file_new_for_commandline_arg
 * @arg: a command line string
 *
 * Creates a #GFile with the given argument from the command line.
 * The value of @arg can be either a URI, an absolute path or
 * a relative path resolved relative to the current working directory.
 * This operation never fails, but the returned object might not support
 * any I/O operation if @arg points to a malformed path.
 *
 * Returns: a new #GFile.
 *
 * Since: 1.0.2
 */
GFile *fm_file_new_for_commandline_arg(const char *arg)
{
    char *scheme;
    FmFileInitTable *iface;
    GFile *file = NULL;

    scheme = g_uri_parse_scheme(arg);
    if(scheme)
    {
        iface = fm_find_scheme(scheme);
        if(iface && iface->new_for_uri)
            file = iface->new_for_uri(arg);
        g_free(scheme);
        if(file)
            return file;
    }
    return g_file_new_for_commandline_arg(arg);
}

/* TODO: implement fm_file_parse_name() too */

/* modules stuff */
FM_MODULE_DEFINE_TYPE(vfs, FmFileInitTable, 1)
static gboolean fm_module_callback_vfs(const char *name, gpointer init, int ver)
{
    /* we don't check version and don't support any other than 1
       otherwise we should allocate own copy of FmFileInitTable */
    fm_file_add_vfs(name, init);
    return TRUE;
}

void _fm_file_init(void)
{
    schemes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    fm_module_register_vfs();
}

void _fm_file_finalize(void)
{
    fm_module_unregister_type("vfs");
    g_hash_table_destroy(schemes);
    schemes = NULL;
}
