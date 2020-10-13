/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2006 The Free Software Foundation, Inc.
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

#include <glib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "file-utils.h"
#include "fr-command.h"
#include "fr-command-zip.h"
#include "fr-command-jar.h"
#include "java-utils.h"


typedef struct {
	char *filename;
	char *rel_path;
	char *package_minus_one_level;
	char *link_name;		/* package dir = package_minus_one_level + '/' + link_name */
} JarData;


static void fr_command_jar_class_init  (FrCommandJarClass *class);
static void fr_command_jar_init        (FrCommand         *afile);
static void fr_command_jar_finalize    (GObject           *object);


static FrCommandClass *parent_class = NULL;


static void
fr_command_jar_add (FrCommand     *comm,
		    const char    *from_file,
		    GList         *file_list,
		    const char    *base_dir,
		    gboolean       update,
		    gboolean       recursive)
{
	FrProcess *proc = comm->process;
	GList     *zip_list = NULL, *jardata_list = NULL, *jar_list = NULL;
	GList     *scan;
	char      *tmp_dir;

	for (scan = file_list; scan; scan = scan->next) {
		char *filename = scan->data;
		char *path = build_uri (base_dir, filename, NULL);
		char *package = NULL;

		if (file_extension_is (filename, ".java"))
			package = get_package_name_from_java_file (path);
		else if (file_extension_is (filename, ".class"))
			package = get_package_name_from_class_file (path);

		if ((package == NULL) || (strlen (package) == 0))
			zip_list = g_list_append (zip_list, g_strdup (filename));
		else {
			JarData *newdata = g_new0 (JarData, 1);

			newdata->package_minus_one_level = remove_level_from_path (package);
			newdata->link_name = g_strdup (file_name_from_path (package));
			newdata->rel_path = remove_level_from_path (filename);
			newdata->filename = g_strdup (file_name_from_path (filename));
			jardata_list = g_list_append (jardata_list, newdata);
		}

		g_free (package);
		g_free (path);
	}

	tmp_dir = get_temp_work_dir (NULL);
	for (scan = jardata_list; scan ; scan = scan->next) {
		JarData *jdata = scan->data;
		char    *pack_path;
		char    *old_link;
		char    *link_name;
		int      retval;

		pack_path = build_uri (tmp_dir, jdata->package_minus_one_level, NULL);
		if (! make_directory_tree_from_path (pack_path, 0755, NULL)) {
			g_free (pack_path);
			continue;
		}

		old_link = build_uri (base_dir, jdata->rel_path, NULL);
		link_name = g_build_filename (pack_path, jdata->link_name, NULL);

		retval = symlink (old_link, link_name);
		if ((retval != -1) || (errno == EEXIST))
			jar_list = g_list_append (jar_list,
						  g_build_filename (jdata->package_minus_one_level,
							            jdata->link_name,
						      	            jdata->filename,
						      	            NULL));

		g_free (link_name);
		g_free (old_link);
		g_free (pack_path);
	}

	if (zip_list != NULL)
		parent_class->add (comm, NULL, zip_list, base_dir, update, FALSE);

	if (jar_list != NULL)
		parent_class->add (comm, NULL, jar_list, tmp_dir, update, FALSE);

	fr_process_begin_command (proc, "rm");
	fr_process_set_working_dir (proc, "/");
	fr_process_add_arg (proc, "-r");
	fr_process_add_arg (proc, "-f");
	fr_process_add_arg (proc, tmp_dir);
	fr_process_end_command (proc);
	fr_process_set_sticky (proc, TRUE);

	for (scan = jardata_list; scan ; scan = scan->next) {
		JarData *jdata = scan->data;
		g_free (jdata->filename);
		g_free (jdata->package_minus_one_level);
		g_free (jdata->link_name);
		g_free (jdata->rel_path);
	}

	path_list_free (jardata_list);
	path_list_free (jar_list);
	path_list_free (zip_list);
	g_free (tmp_dir);
}


const char *jar_mime_type[] = { "application/x-java-archive",
				NULL };


static const char **
fr_command_jar_get_mime_types (FrCommand *comm)
{
	return jar_mime_type;
}


static FrCommandCap
fr_command_jar_get_capabilities (FrCommand  *comm,
			         const char *mime_type,
				 gboolean    check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES;
	if (is_program_available ("zip", check_command))
		capabilities |= FR_COMMAND_CAN_READ_WRITE;

	return capabilities;
}


static const char *
fr_command_jar_get_packages (FrCommand  *comm,
			     const char *mime_type)
{
	return PACKAGES ("zip,unzip");
}


static void
fr_command_jar_class_init (FrCommandJarClass *class)
{
	GObjectClass   *gobject_class = G_OBJECT_CLASS(class);
	FrCommandClass *afc = FR_COMMAND_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	gobject_class->finalize = fr_command_jar_finalize;

	afc->add = fr_command_jar_add;
	afc->get_mime_types = fr_command_jar_get_mime_types;
	afc->get_capabilities = fr_command_jar_get_capabilities;
	afc->get_packages     = fr_command_jar_get_packages;
}


static void
fr_command_jar_init (FrCommand *comm)
{
}


static void
fr_command_jar_finalize (GObject *object)
{
        g_return_if_fail (object != NULL);
        g_return_if_fail (FR_IS_COMMAND_JAR (object));

	/* Chain up */
        if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_jar_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (FrCommandJarClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_jar_class_init,
			NULL,
			NULL,
			sizeof (FrCommandJar),
			0,
			(GInstanceInitFunc) fr_command_jar_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND_ZIP,
					       "FRCommandJar",
					       &type_info,
					       0);
        }

        return type;
}
