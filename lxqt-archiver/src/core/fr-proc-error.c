/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "fr-proc-error.h"


static gpointer
fr_proc_error_copy (gpointer boxed)
{
	FrProcError *old_error = boxed;
	FrProcError *new_error;

	new_error = g_new (FrProcError, 1);
	new_error->type = old_error->type;
	new_error->status = old_error->status;
	new_error->gerror = (old_error->gerror != NULL) ? g_error_copy (old_error->gerror) : NULL;

	return new_error;
}


static void
fr_proc_error_free (gpointer boxed)
{
	FrProcError *error = boxed;

	if (error->gerror != NULL)
		g_error_free (error->gerror);
	g_free (error);
}


G_DEFINE_BOXED_TYPE (FrProcError,
		     fr_proc_error,
		     fr_proc_error_copy,
		     fr_proc_error_free)
