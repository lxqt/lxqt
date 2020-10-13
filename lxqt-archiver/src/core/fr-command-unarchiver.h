/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2012 The Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FR_COMMAND_UNARCHIVER_H
#define FR_COMMAND_UNARCHIVER_H

#include <glib.h>
#include "file-data.h"
#include "fr-command.h"
#include "fr-process.h"

#define FR_TYPE_COMMAND_UNARCHIVER            (fr_command_unarchiver_get_type ())
#define FR_COMMAND_UNARCHIVER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_COMMAND_UNARCHIVER, FrCommandUnarchiver))
#define FR_COMMAND_UNARCHIVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_COMMAND_UNARCHIVER, FrCommandUnarchiverClass))
#define FR_IS_COMMAND_UNARCHIVER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_COMMAND_UNARCHIVER))
#define FR_IS_COMMAND_UNARCHIVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_COMMAND_UNARCHIVER))
#define FR_COMMAND_UNARCHIVER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_COMMAND_UNARCHIVER, FrCommandUnarchiverClass))

typedef struct _FrCommandUnarchiver       FrCommandUnarchiver;
typedef struct _FrCommandUnarchiverClass  FrCommandUnarchiverClass;

struct _FrCommandUnarchiver
{
	FrCommand  __parent;

	GInputStream *stream;
	int           n_line;
};

struct _FrCommandUnarchiverClass
{
	FrCommandClass __parent_class;
};

GType fr_command_unarchiver_get_type (void);

#endif /* FR_COMMAND_UNARCHIVER_H */
