/*
 *      fm-folder-config.h
 *
 *      This file is a part of the LibFM project.
 *
 *      Copyright 2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifndef __FM_FOLDER_CONFIG_H__
#define __FM_FOLDER_CONFIG_H__ 1

#include <gio/gio.h>
#include "fm-path.h"

G_BEGIN_DECLS

typedef struct _FmFolderConfig FmFolderConfig;

FmFolderConfig *fm_folder_config_open(FmPath *path);
gboolean fm_folder_config_close(FmFolderConfig *fc, GError **error);

gboolean fm_folder_config_is_empty(FmFolderConfig *fc);
gboolean fm_folder_config_get_integer(FmFolderConfig *fc, const char *key,
                                      gint *val);
gboolean fm_folder_config_get_uint64(FmFolderConfig *fc, const char *key,
                                     guint64 *val);
gboolean fm_folder_config_get_double(FmFolderConfig *fc, const char *key,
                                     gdouble *val);
gboolean fm_folder_config_get_boolean(FmFolderConfig *fc, const char *key,
                                      gboolean *val);
char *fm_folder_config_get_string(FmFolderConfig *fc, const char *key);
char **fm_folder_config_get_string_list(FmFolderConfig *fc,
                                        const char *key, gsize *length);

void fm_folder_config_set_integer(FmFolderConfig *fc, const char *key, gint val);
void fm_folder_config_set_uint64(FmFolderConfig *fc, const char *key, guint64 val);
void fm_folder_config_set_double(FmFolderConfig *fc, const char *key, gdouble val);
void fm_folder_config_set_boolean(FmFolderConfig *fc, const char *key,
                                  gboolean val);
void fm_folder_config_set_string(FmFolderConfig *fc, const char *key,
                                 const char *string);
void fm_folder_config_set_string_list(FmFolderConfig *fc, const char *key,
                                      const gchar * const list[], gsize length);
void fm_folder_config_remove_key(FmFolderConfig *fc, const char *key);
void fm_folder_config_purge(FmFolderConfig *fc);

void fm_folder_config_save_cache(void);

void _fm_folder_config_init(void);
void _fm_folder_config_finalize(void);

G_END_DECLS

#endif
