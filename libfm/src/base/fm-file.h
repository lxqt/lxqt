/*
 *      fm-file.h
 *
 *      Copyright 2012 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifndef _FM_FILE_H_
#define _FM_FILE_H_ 1

#include "fm-module.h"

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define FM_TYPE_FILE                    (fm_file_get_type())
#define FM_FILE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST((obj),\
                                                FM_TYPE_FILE, FmFile))
#define FM_IS_FILE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
                                                FM_TYPE_FILE))
#define FM_FILE_GET_IFACE(obj)          (G_TYPE_INSTANCE_GET_INTERFACE((obj),\
                                                FM_TYPE_FILE, FmFileInterface))

typedef struct _FmFile                  FmFile; /* Dummy typedef */
typedef struct _FmFileInterface         FmFileInterface;

/**
 * FmFileInterface:
 * @wants_incremental: VTable func, see fm_file_wants_incremental()
 */
struct _FmFileInterface
{
    /*< private >*/
    GTypeInterface g_iface;

    /*< public >*/
    gboolean (*wants_incremental)(GFile* file);

    /*< private >*/
    gpointer _reserved1;
    gpointer _reserved2;
};

typedef struct _FmFileInitTable         FmFileInitTable;

/**
 * FmFileInitTable:
 * @new_for_uri: function to create new #GFile object from URI
 *
 * Functions to initialize FmFile instance.
 *
 * This structure is used for "vfs" module initialization. The key for
 * module of this type is scheme name to support.
 */
struct _FmFileInitTable
{
    /*< public >*/
    GFile * (*new_for_uri)(const char *uri);
    /*< private >*/
    gpointer _reserved1;
    gpointer _reserved2;
};

GType           fm_file_get_type(void);

void            fm_file_add_vfs(const char *name, FmFileInitTable *init);

/* VTable calls */
gboolean        fm_file_wants_incremental(GFile* file);

/* Gfile functions replacements */
GFile          *fm_file_new_for_uri(const char *uri);
GFile          *fm_file_new_for_commandline_arg(const char *arg);

void _fm_file_init(void);
void _fm_file_finalize(void);

/* for modules */
#define FM_MODULE_vfs_VERSION 1
extern FmFileInitTable fm_module_init_vfs;

G_END_DECLS
#endif /* _FM_FILE_H_ */