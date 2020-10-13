/*
 * fr-command-lrzip.c
 *
 *  Created on: 10.04.2010
 *      Author: Alexander Saprykin
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <glib.h>

#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-lrzip.h"

static void fr_command_lrzip_class_init  (FrCommandLrzipClass *class);
static void fr_command_lrzip_init        (FrCommand         *afile);
static void fr_command_lrzip_finalize    (GObject           *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;


/* -- list -- */


static void
list__process_line (char     *line,
		    gpointer  data)
{
	FileData  *fdata;
	FrCommand *comm = FR_COMMAND (data);

	g_return_if_fail (line != NULL);

	if (strlen (line) == 0)
		return;

	if (! g_str_has_prefix (line, "Decompressed file size:"))
		return;

	fdata = file_data_new ();
	fdata->size = g_ascii_strtoull (get_last_field (line, 4), NULL, 10);

	struct stat st;
	
	if (stat (comm->filename, &st) == 0)
		fdata->modified = st.st_mtim.tv_sec;
	else
		time(&(fdata->modified));

	fdata->encrypted = FALSE;

	char *new_fname = g_strdup (file_name_from_path (comm->filename));
	if (g_str_has_suffix (new_fname, ".lrz"))
		new_fname[strlen (new_fname) - 4] = '\0';

	if (*new_fname == '/') {
		fdata->full_path = g_strdup (new_fname);
		fdata->original_path = fdata->full_path;
	}
	else {
		fdata->full_path = g_strconcat ("/", new_fname, NULL);
		fdata->original_path = fdata->full_path + 1;
	}
	fdata->path = remove_level_from_path (fdata->full_path);
	fdata->name = new_fname;
	fdata->dir = FALSE;
	fdata->link = NULL;

	if (fdata->name == 0)
		file_data_free (fdata);
	else
		fr_command_add_file (comm, fdata);
}


static void
fr_command_lrzip_list (FrCommand  *comm)
{
	fr_process_set_err_line_func (comm->process, list__process_line, comm);

	fr_process_begin_command (comm->process, "lrzip");
	fr_process_add_arg (comm->process, "-i");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
	fr_process_start (comm->process);
}


static void
fr_command_lrzip_add (FrCommand  *comm,
		      const char *from_file,
		      GList      *file_list,
		      const char *base_dir,
		      gboolean    update,
		      gboolean    recursive)
{
	fr_process_begin_command (comm->process, "lrzip");

	if (base_dir != NULL)
		fr_process_set_working_dir (comm->process, base_dir);

	/* preserve links. */

	switch (comm->compression) {
	case FR_COMPRESSION_VERY_FAST:
		fr_process_add_arg (comm->process, "-l"); break;
	case FR_COMPRESSION_FAST:
		fr_process_add_arg (comm->process, "-g"); break;
	case FR_COMPRESSION_NORMAL:
		fr_process_add_arg (comm->process, "-b"); break;
	case FR_COMPRESSION_MAXIMUM:
		fr_process_add_arg (comm->process, "-z"); break;
	}

	fr_process_add_arg (comm->process, "-o");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_add_arg (comm->process, (char *) file_list->data);

	fr_process_end_command (comm->process);
}

static void
fr_command_lrzip_extract (FrCommand  *comm,
			  const char *from_file,
			  GList      *file_list,
			  const char *dest_dir,
			  gboolean    overwrite,
			  gboolean    skip_older,
			  gboolean    junk_paths)
{
	fr_process_begin_command (comm->process, "lrzip");
	fr_process_add_arg (comm->process, "-d");

	if (dest_dir != NULL) {
		fr_process_add_arg (comm->process, "-O");
		fr_process_add_arg (comm->process, dest_dir);
	}
	if (overwrite)
		fr_process_add_arg (comm->process, "-f");

	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
}


/*
static void
fr_command_lrzip_test (FrCommand   *comm)
{
	fr_process_begin_command (comm->process, "lrzip");
	fr_process_add_arg (comm->process, "-t");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
}
*/


const char *lrzip_mime_type[] = { "application/x-lrzip", NULL };


static const char **
fr_command_lrzip_get_mime_types (FrCommand *comm)
{
	return lrzip_mime_type;
}


static FrCommandCap
fr_command_lrzip_get_capabilities (FrCommand  *comm,
				   const char *mime_type,
				   gboolean    check_command)
{
	FrCommandCap capabilities = FR_COMMAND_CAN_DO_NOTHING;

	if (is_program_available ("lrzip", check_command))
		capabilities |= FR_COMMAND_CAN_READ_WRITE;

	return capabilities;
}


static const char *
fr_command_lrzip_get_packages (FrCommand  *comm,
			       const char *mime_type)
{
	return PACKAGES ("lrzip");
}


static void
fr_command_lrzip_class_init (FrCommandLrzipClass *class)
{
	GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
	FrCommandClass *afc;

	parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_lrzip_finalize;

	afc->list             = fr_command_lrzip_list;
	afc->add              = fr_command_lrzip_add;
	afc->extract          = fr_command_lrzip_extract;
	afc->get_mime_types   = fr_command_lrzip_get_mime_types;
	afc->get_capabilities = fr_command_lrzip_get_capabilities;
	afc->get_packages     = fr_command_lrzip_get_packages;
}


static void
fr_command_lrzip_init (FrCommand *comm)
{
	comm->propAddCanUpdate             = FALSE;
	comm->propAddCanReplace            = FALSE;
	comm->propAddCanStoreFolders       = FALSE;
	comm->propExtractCanAvoidOverwrite = TRUE;
	comm->propExtractCanSkipOlder      = FALSE;
	comm->propExtractCanJunkPaths      = FALSE;
	comm->propPassword                 = FALSE;
	comm->propTest                     = FALSE;
}


static void
fr_command_lrzip_finalize (GObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (FR_IS_COMMAND_LRZIP (object));

	/* Chain up */
	if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_lrzip_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FrCommandLrzipClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_lrzip_class_init,
			NULL,
			NULL,
			sizeof (FrCommandLrzip),
			0,
			(GInstanceInitFunc) fr_command_lrzip_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandLrzip",
					       &type_info,
					       0);
	}

	return type;
}
