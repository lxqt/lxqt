/*
 *      fm-path-entry.h
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2009 Jürgen Hötzel <juergen@archlinux.org>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */


#ifndef __FM_PATH_ENTRY_H__
#define __FM_PATH_ENTRY_H__

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "fm-path.h"

G_BEGIN_DECLS

#define FM_TYPE_PATH_ENTRY (fm_path_entry_get_type())
#define FM_PATH_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), FM_TYPE_PATH_ENTRY, FmPathEntry))
#define FM_PATH_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), FM_TYPE_PATH_ENTRY, FmPathEntryClass))
#define FM_IS_TYPE_PATH_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FM_TYPE_PATH_ENTRY))
#define FM_IS_TYPE_PATH_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FM_TYPE_PATH_ENTRY), FmPathEntry)

typedef struct _FmPathEntry FmPathEntry;
typedef struct _FmPathEntryClass FmPathEntryClass;

GType fm_path_entry_get_type(void);
FmPathEntry* fm_path_entry_new();
void fm_path_entry_set_path(FmPathEntry *entry, FmPath* path);

/* The function does not increase ref count. The caller is responsible for calling
 * fm_path_ref if it wants to keep the path. */
FmPath* fm_path_entry_get_path(FmPathEntry *entry);

G_END_DECLS

#endif /* __FM_PATH_ENTRY_H__ */
