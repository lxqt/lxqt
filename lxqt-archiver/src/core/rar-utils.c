/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2011 The Free Software Foundation, Inc.
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
#include <string.h>
#include "file-utils.h"
#include "fr-command.h"
#include "gio-utils.h"
#include "rar-utils.h"


typedef enum {
	FIRST_VOLUME_IS_000,
	FIRST_VOLUME_IS_001,
	FIRST_VOLUME_IS_RAR
} FirstVolumeExtension;


static char *
get_first_volume_name (const char           *name,
		       const char           *pattern,
		       FirstVolumeExtension  extension_type)
{
	char   *volume_name = NULL;
	GRegex *re;

	re = g_regex_new (pattern, G_REGEX_CASELESS, 0, NULL);
	if (g_regex_match (re, name, 0, NULL)) {
		char **parts;
		int    l, i;

		parts = g_regex_split (re, name, 0);
		l = strlen (parts[2]);
		switch (extension_type) {
		case FIRST_VOLUME_IS_000:
			for (i = 0; i < l; i++)
				parts[2][i] = '0';
			break;

		case FIRST_VOLUME_IS_001:
			for (i = 0; i < l; i++)
				parts[2][i] = (i < l - 1) ? '0' : '1';
			break;

		case FIRST_VOLUME_IS_RAR:
			if (g_str_has_suffix (parts[1], "r")) {
				parts[2][0] = 'a';
				parts[2][1] = 'r';
			}
			else {
				parts[2][0] = 'A';
				parts[2][1] = 'R';
			}
			break;
		}

		volume_name = g_strjoinv ("", parts);

		g_strfreev (parts);
	}
	g_regex_unref (re);

	if (volume_name != NULL) {
		char *tmp;

		tmp = volume_name;
		volume_name = g_filename_from_utf8 (tmp, -1, NULL, NULL, NULL);
		g_free (tmp);
	}

	return volume_name;
}


void
rar_check_multi_volume (FrCommand *comm)
{
	GFile *file;
	char   buffer[11];

	file = g_file_new_for_path (comm->filename);
	if (! g_load_file_in_buffer (file, buffer, 11, NULL)) {
		g_object_unref (file);
		return;
	}

	if (memcmp (buffer, "Rar!", 4) != 0)
		return;

	if ((buffer[10] & 0x01) == 0x01) {
		char   *volume_name = NULL;
		char   *name;

		name = g_filename_to_utf8 (file_name_from_path (comm->filename), -1, NULL, NULL, NULL);

		volume_name = get_first_volume_name (name, "^(.*\\.part)([0-9]+)(\\.rar)$", FIRST_VOLUME_IS_001);
		if (volume_name == NULL)
			volume_name = get_first_volume_name (name, "^(.*\\.r)([0-9]+)$", FIRST_VOLUME_IS_RAR);
		if (volume_name == NULL)
			volume_name = get_first_volume_name (name, "^(.*\\.)([0-9]+)$", FIRST_VOLUME_IS_001);

		if (volume_name != NULL) {
			GFile *parent;
			GFile *volume_file;

			parent = g_file_get_parent (file);
			volume_file = g_file_get_child (parent, volume_name);
			fr_command_set_multi_volume (comm, volume_file);

			g_object_unref (volume_file);
			g_object_unref (parent);
		}

		g_free (name);
	}

	g_object_unref (file);
}
