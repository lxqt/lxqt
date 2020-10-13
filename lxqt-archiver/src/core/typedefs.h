/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <glib.h>
#include <glib-object.h>

#define MEGABYTE (1024 * 1024)

#define ADD_FOLDER_OPTIONS_DIR  "engrampa/options"

typedef enum { /*< skip >*/
	FR_WINDOW_SORT_BY_NAME = 0,
	FR_WINDOW_SORT_BY_SIZE = 1,
	FR_WINDOW_SORT_BY_TYPE = 2,
	FR_WINDOW_SORT_BY_TIME = 3,
	FR_WINDOW_SORT_BY_PATH = 4
} FrWindowSortMethod;

typedef enum { /*< skip >*/
	FR_WINDOW_LIST_MODE_FLAT,
	FR_WINDOW_LIST_MODE_AS_DIR
} FrWindowListMode;

typedef enum {
	FR_COMPRESSION_VERY_FAST,
	FR_COMPRESSION_FAST,
	FR_COMPRESSION_NORMAL,
	FR_COMPRESSION_MAXIMUM
} FrCompression;

typedef enum {
	FR_OVERWRITE_YES,
	FR_OVERWRITE_NO,
	FR_OVERWRITE_ASK
} FrOverwrite;

typedef enum { /*< skip >*/
	FR_PROC_ERROR_NONE,
	FR_PROC_ERROR_GENERIC,
	FR_PROC_ERROR_COMMAND_ERROR,
	FR_PROC_ERROR_COMMAND_NOT_FOUND,
	FR_PROC_ERROR_EXITED_ABNORMALLY,
	FR_PROC_ERROR_SPAWN,
	FR_PROC_ERROR_STOPPED,
	FR_PROC_ERROR_ASK_PASSWORD,
	FR_PROC_ERROR_MISSING_VOLUME,
	FR_PROC_ERROR_IO_CHANNEL,
	FR_PROC_ERROR_BAD_CHARSET,
	FR_PROC_ERROR_UNSUPPORTED_FORMAT
} FrProcErrorType;

typedef struct {
	FrProcErrorType  type;
	int              status;
	GError          *gerror;
} FrProcError;

typedef enum { /*< skip >*/
	FR_COMMAND_CAN_DO_NOTHING = 0,
	FR_COMMAND_CAN_READ = 1 << 0,
	FR_COMMAND_CAN_WRITE = 1 << 1,
	FR_COMMAND_CAN_ARCHIVE_MANY_FILES = 1 << 2,
	FR_COMMAND_CAN_ENCRYPT = 1 << 3,
	FR_COMMAND_CAN_ENCRYPT_HEADER = 1 << 4,
	FR_COMMAND_CAN_CREATE_VOLUMES = 1 << 5
} FrCommandCap;

#define FR_COMMAND_CAN_READ_WRITE (FR_COMMAND_CAN_READ | FR_COMMAND_CAN_WRITE)

typedef guint8 FrCommandCaps;

typedef struct {
	const char    *mime_type;
	FrCommandCaps  current_capabilities;
	FrCommandCaps  potential_capabilities;
} FrMimeTypeCap;

typedef struct {
	const char *mime_type;
	const char *packages;
} FrMimeTypePackages;

typedef struct {
	int        ref;
	GType      type;
	GPtrArray *caps;  /* array of FrMimeTypeCap */
	GPtrArray *packages;  /* array of FrMimeTypePackages */
} FrRegisteredCommand;

typedef struct {
	const char    *mime_type;
	char          *default_ext;
	char          *name;
	FrCommandCaps  capabilities;
} FrMimeTypeDescription;

typedef struct {
	char       *ext;
	const char *mime_type;
} FrExtensionType;

#endif /* TYPEDEFS_H */
