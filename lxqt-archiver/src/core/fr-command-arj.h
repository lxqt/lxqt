/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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

#ifndef FR_COMMAND_ARJ_H
#define FR_COMMAND_ARJ_H

#include <glib.h>
#include "fr-command.h"
#include "fr-process.h"

#define FR_TYPE_COMMAND_ARJ            (fr_command_arj_get_type ())
#define FR_COMMAND_ARJ(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_COMMAND_ARJ, FrCommandArj))
#define FR_COMMAND_ARJ_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_COMMAND_ARJ, FrCommandArjClass))
#define FR_IS_COMMAND_ARJ(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_COMMAND_ARJ))
#define FR_IS_COMMAND_ARJ_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_COMMAND_ARJ))
#define FR_COMMAND_ARJ_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_COMMAND_ARJ, FrCommandArjClass))

typedef struct _FrCommandArj       FrCommandArj;
typedef struct _FrCommandArjClass  FrCommandArjClass;

struct _FrCommandArj
{
	FrCommand  __parent;

	gboolean  list_started;
	int       line_no;
	FileData *fdata;
	GRegex   *filename_line_regex;
};

struct _FrCommandArjClass
{
	FrCommandClass __parent_class;
};

GType fr_command_arj_get_type (void);

#endif /* FR_COMMAND_ARJ_H */
