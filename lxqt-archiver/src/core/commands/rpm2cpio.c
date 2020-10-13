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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>


static const char *
get_mime_type_from_magic_numbers (char *buffer)
{
	static struct {
		const char *mime_type;
		const char *first_bytes;
		int         offset;
		int         len;
	} sniffer_data [] = {
		{ "application/x-bzip2", "BZh", 0, 3 },
		{ "application/gzip", "\037\213", 0, 2 },
		{ "application/x-xz", "\3757zXZ\000", 0, 6 },
		{ NULL, NULL, 0 }
	};
	int  i;

	for (i = 0; sniffer_data[i].mime_type != NULL; i++)
		if (memcmp (sniffer_data[i].first_bytes,
			    buffer + sniffer_data[i].offset,
			    sniffer_data[i].len) == 0)
		{
			return sniffer_data[i].mime_type;
		}

	return NULL;
}


int
main (int argc, char **argv)
{
	const char *filename;
	GString    *cpio_args;
	int         i;
	FILE       *stream;
	guchar      bytes[8];
	int         il, dl, sigsize, offset;
	const char *mime_type;
	const char *archive_command;
	char       *command;

	if (argc < 3)
		return 0;

	filename = argv[1];
	cpio_args = g_string_new (argv[2]);
	for (i = 3; i < argc; i++) {
		g_string_append (cpio_args, " ");
		g_string_append (cpio_args, argv[i]);
	}

	stream = fopen (filename, "r");
	if (stream == NULL)
		return 1;

	if (fseek (stream, 104 , SEEK_CUR) != 0) {
		fclose (stream);
		return 1;
	}
	if (fread (bytes, 1, 8, stream) == 0) {
		fclose (stream);
		return 1;
	}
	il = 256 * (256 * (256 * bytes[0] + bytes[1]) + bytes[2]) + bytes[3];
	dl = 256 * (256 * (256 * bytes[4] + bytes[5]) + bytes[6]) + bytes[7];
	sigsize = 8 + 16 * il + dl;
	offset = 104 + sigsize + (8 - (sigsize % 8)) % 8 + 8;
	if (fseek (stream, offset, SEEK_SET) != 0) {
		fclose (stream);
		return 1;
	}
	if (fread (bytes, 1, 8, stream) == 0) {
		fclose (stream);
		return 1;
	}
	il = 256 * (256 * (256 * bytes[0] + bytes[1]) + bytes[2]) + bytes[3];
	dl = 256 * (256 * (256 * bytes[4] + bytes[5]) + bytes[6]) + bytes[7];
	sigsize = 8 + 16 * il + dl;
	offset = offset + sigsize;

	/* get the payload type */

	if (fseek (stream, offset, SEEK_SET) != 0) {
		fclose (stream);
		return 1;
	}
	if (fread (bytes, 1, 8, stream) == 0) {
		fclose (stream);
		return 1;
	}
	mime_type = get_mime_type_from_magic_numbers ((char *)bytes);
	if (mime_type == NULL)
		archive_command = "lzma -dc";
	else if (strcmp (mime_type, "application/x-xz") == 0)
		archive_command = "xz -dc";
	else if (strcmp (mime_type, "application/gzip") == 0)
		archive_command = "gzip -dc";
	else
		archive_command = "bzip2 -dc";
	fclose (stream);

	command = g_strdup_printf ("sh -c \"dd if=%s ibs=%u skip=1 2>/dev/null | %s | cpio %s\"", g_shell_quote (filename), offset, archive_command, cpio_args->str);

	return system (command);
}
