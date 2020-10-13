/*
 *      fm-mime-type.h
 *
 *      Copyright 2009 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef _FM_MIME_TYPE_H_
#define _FM_MIME_TYPE_H_

#include <glib.h>
#include <gio/gio.h>
#include <glib/gstdio.h>

#include "fm-icon.h"
#include "fm-thumbnailer.h"

G_BEGIN_DECLS

typedef struct _FmMimeType FmMimeType;

void _fm_mime_type_init();

void _fm_mime_type_finalize();

/* file name used in this API should be encoded in UTF-8 */
FmMimeType* fm_mime_type_from_file_name(const char* ufile_name);

FmMimeType* fm_mime_type_from_native_file(const char* file_path,  /* Should be on-disk encoding */
                                          const char* base_name,  /* Should be in UTF-8 */
                                          struct stat* pstat);   /* Can be NULL */

FmMimeType* fm_mime_type_from_name(const char* type);

FmMimeType* _fm_mime_type_get_inode_directory();
FmMimeType* _fm_mime_type_get_inode_x_shortcut();
FmMimeType* _fm_mime_type_get_inode_mount_point();
FmMimeType* _fm_mime_type_get_application_x_desktop();

FmMimeType* fm_mime_type_ref(FmMimeType* mime_type);
void fm_mime_type_unref(gpointer mime_type_);

FmIcon* fm_mime_type_get_icon(FmMimeType* mime_type);

/* Get mime-type string */
const char* fm_mime_type_get_type(FmMimeType* mime_type);

/* Get human-readable description of mime-type */
const char* fm_mime_type_get_desc(FmMimeType* mime_type);

/* Get installed external thumbnailers for the mime-type.
 * Returns a list of FmThumbnailer. */
#ifndef FM_DISABLE_DEPRECATED
const GList* fm_mime_type_get_thumbnailers(FmMimeType* mime_type);
#endif
GList* fm_mime_type_get_thumbnailers_list(FmMimeType* mime_type);

void fm_mime_type_add_thumbnailer(FmMimeType* mime_type, gpointer thumbnailer);

void fm_mime_type_remove_thumbnailer(FmMimeType* mime_type, gpointer thumbnailer);

G_END_DECLS

#endif
