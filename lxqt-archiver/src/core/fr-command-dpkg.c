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

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>

#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-dpkg.h"

static void fr_command_dpkg_class_init  (FrCommandDpkgClass *class);
static void fr_command_dpkg_init        (FrCommand         *afile);
static void fr_command_dpkg_finalize    (GObject           *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;

static void
process_metadata_line (char      *line,
                       FrCommand *comm)
{
        FileData    *fdata;
        char       **fields;
        char        *name;

        g_return_if_fail (line != NULL);

        fields = split_line (line, 6);
        if (!fields[1] || !g_str_equal (fields[1], "bytes,")) {
                g_strfreev (fields);
                return;
        }

        fdata = file_data_new ();
        fdata->size = g_ascii_strtoull (fields[0], NULL, 10);

        if (fields[5] && g_str_equal (fields[4],"*")) {
                name = g_strdup (fields[5]);
        } else {
                name = g_strdup (get_last_field (line, 5));
        }
        g_strstrip (name);

        fdata->full_path = g_strconcat ("/DEBIAN/", name, NULL);
        fdata->original_path = fdata->full_path + 1;

        g_strfreev (fields);

        fdata->name = g_strdup (name);
        fdata->path = g_strdup ("DEBIAN");
  
        g_free (name);
  
        fr_command_add_file (comm, fdata);
}

static void
process_data_line (char     *line,
                   gpointer  data)
{
        FileData    *fdata;
        FrCommand   *comm = FR_COMMAND (data);
        char       **fields;
        char       **tmfields;
        struct tm    tm = {0, };
        const char  *name;

        g_return_if_fail (line != NULL);

        if (line[0] == ' ') {
                /* This is the output of dpkg-deb -I */
                process_metadata_line (line, comm);
                return;
        }

        fdata = file_data_new ();

        fields = split_line (line, 5);
        fdata->size = g_ascii_strtoull (fields[2], NULL, 10);
        tmfields = g_strsplit(fields[3], "-", 3);
        if (tmfields[2]) {
                tm.tm_year = atoi (tmfields[0]) - 1900;
                tm.tm_mon = atoi (tmfields[1]);
                tm.tm_mday = atoi (tmfields[2]);
        }
        g_strfreev (tmfields);
        tmfields = g_strsplit (fields[4], ":", 2);
        if (tmfields[1]) {
                tm.tm_hour = atoi (tmfields[0]);
                tm.tm_min = atoi (tmfields[1]);
        }
        g_strfreev (tmfields);
        fdata->modified = mktime (&tm);
        g_strfreev (fields);

        name = get_last_field (line, 6);
        fields = g_strsplit (name, " -> ", 2);

        fdata->dir = line[0] == 'd';
        name = fields[0];
        if (g_str_has_prefix (name, "./")) { /* Should generally be the case */
                fdata->full_path = g_strdup (name + 1);
                fdata->original_path = fdata->full_path + 1;
        } else if (name[0] == '/') {
                fdata->full_path = g_strdup (name);
                fdata->original_path = fdata->full_path;
        } else {
                fdata->full_path = g_strconcat ("/", name, NULL);
                fdata->original_path = fdata->full_path + 1;
        }
        if (fdata->dir && (name[strlen (name) - 1] != '/')) {
                char *old_full_path = fdata->full_path;
                fdata->full_path = g_strconcat (old_full_path, "/", NULL);
                g_free (old_full_path);
                fdata->original_path = g_strdup (name);
                fdata->free_original_path = TRUE;
        }

        if (fields[1] != NULL)
                fdata->link = g_strdup (fields[1]);
        g_strfreev (fields);

        if (fdata->dir)
                fdata->name = dir_name_from_path (fdata->full_path);
        else
                fdata->name = g_strdup (file_name_from_path (fdata->full_path));
        fdata->path = remove_level_from_path (fdata->full_path);

        if (*fdata->name == 0)
                file_data_free (fdata);
        else
                fr_command_add_file (comm, fdata);
}


static void
fr_command_dpkg_list (FrCommand *comm)
{
        fr_process_set_out_line_func (comm->process, process_data_line, comm);

        fr_process_begin_command (comm->process, "dpkg-deb");
        fr_process_add_arg (comm->process, "-I");
        fr_process_add_arg (comm->process, comm->filename);
        fr_process_end_command (comm->process);
        fr_process_start (comm->process);

        fr_process_begin_command (comm->process, "dpkg-deb");
        fr_process_add_arg (comm->process, "-c");
        fr_process_add_arg (comm->process, comm->filename);
        fr_process_end_command (comm->process);
        fr_process_start (comm->process);
}


static void
fr_command_dpkg_extract (FrCommand  *comm,
                         const char *from_file,
                         GList      *file_list,
                         const char *dest_dir,
                         gboolean    overwrite,
                         gboolean    skip_older,
                         gboolean    junk_paths)
{
        fr_process_begin_command (comm->process, "dpkg-deb");
        fr_process_add_arg (comm->process, "-x");
        fr_process_add_arg (comm->process, comm->filename);
        if (dest_dir != NULL) {
                fr_process_add_arg (comm->process, dest_dir);
        } else {
                fr_process_add_arg (comm->process, ".");
        }
        /* FIXME it is not possible to unpack only some files */
        fr_process_end_command (comm->process);

        /* Also extract metadata in DEBIAN/ */
        fr_process_begin_command (comm->process, "dpkg-deb");
        if (dest_dir != NULL) {
                fr_process_set_working_dir (comm->process, dest_dir);
        }
        fr_process_add_arg (comm->process, "-e");
        fr_process_add_arg (comm->process, comm->filename);
        fr_process_end_command (comm->process);
}


const char *dpkg_mime_type[] = { "application/x-deb", NULL };


static const char **
fr_command_dpkg_get_mime_types (FrCommand *comm)
{
        return dpkg_mime_type;
}


static FrCommandCap
fr_command_dpkg_get_capabilities (FrCommand  *comm,
                                  const char *mime_type,
                                  gboolean    check_command)
{
        FrCommandCap capabilities;

        capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES;
        if (is_program_available ("dpkg-deb", check_command))
                capabilities |= FR_COMMAND_CAN_READ;

        return capabilities;
}


static const char *
fr_command_dpkg_get_packages (FrCommand  *comm,
                              const char *mime_type)
{
        return PACKAGES ("dpkg");
}


static void
fr_command_dpkg_class_init (FrCommandDpkgClass *class)
{
        GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
        FrCommandClass *afc;

        parent_class = g_type_class_peek_parent (class);
        afc = (FrCommandClass*) class;

        gobject_class->finalize = fr_command_dpkg_finalize;

        afc->list             = fr_command_dpkg_list;
        afc->extract          = fr_command_dpkg_extract;
        afc->get_mime_types   = fr_command_dpkg_get_mime_types;
        afc->get_capabilities = fr_command_dpkg_get_capabilities;
        afc->get_packages     = fr_command_dpkg_get_packages;
}


static void
fr_command_dpkg_init (FrCommand *comm)
{
        comm->propAddCanUpdate             = FALSE;
        comm->propAddCanReplace            = FALSE;
        comm->propExtractCanAvoidOverwrite = FALSE;
        comm->propExtractCanSkipOlder      = FALSE;
        comm->propExtractCanJunkPaths      = FALSE;
        comm->propPassword                 = FALSE;
        comm->propTest                     = FALSE;
}


static void
fr_command_dpkg_finalize (GObject *object)
{
        g_return_if_fail (object != NULL);
        g_return_if_fail (FR_IS_COMMAND_DPKG (object));

        /* Chain up */
        if (G_OBJECT_CLASS (parent_class)->finalize)
                G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_dpkg_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
                        sizeof (FrCommandDpkgClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) fr_command_dpkg_class_init,
                        NULL,
                        NULL,
                        sizeof (FrCommandDpkg),
                        0,
                        (GInstanceInitFunc) fr_command_dpkg_init
                };

                type = g_type_register_static (FR_TYPE_COMMAND,
                                               "FRCommandDpkg",
                                               &type_info,
                                               0);
        }

        return type;
}
