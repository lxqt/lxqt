/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2004 Free Software Foundation, Inc.
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

#ifndef FR_COMMAND_7Z_H
#define FR_COMMAND_7Z_H

#include <glib.h>
#include "fr-command.h"
#include "fr-process.h"

#define FR_TYPE_COMMAND_7Z            (fr_command_7z_get_type ())
#define FR_COMMAND_7Z(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_COMMAND_7Z, FrCommand7z))
#define FR_COMMAND_7Z_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_COMMAND_7Z, FrCommand7zClass))
#define FR_IS_COMMAND_7Z(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_COMMAND_7Z))
#define FR_IS_COMMAND_7Z_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_COMMAND_7Z))
#define FR_COMMAND_7Z_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_COMMAND_7Z, FrCommand7zClass))

typedef struct _FrCommand7z       FrCommand7z;
typedef struct _FrCommand7zClass  FrCommand7zClass;

struct _FrCommand7z
{
	FrCommand __parent;
	gboolean   list_started;
	gboolean   old_style;
	FileData  *fdata;
};

struct _FrCommand7zClass
{
	FrCommandClass __parent_class;
};

GType fr_command_7z_get_type (void);

#endif /* FR_COMMAND_7Z_H */
