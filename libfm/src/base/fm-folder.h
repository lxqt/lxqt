/*
 *      fm-folder.h
 *
 *      Copyright 2009-2012 PCMan <pcman.tw@gmail.com>
 *      Copyright 2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This file is a part of the Libfm library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __FM_FOLDER_H__
#define __FM_FOLDER_H__

#include <glib-object.h>
#include <gio/gio.h>
#include "fm-path.h"
#include "fm-dir-list-job.h"
#include "fm-file-info.h"
#include "fm-job.h"
#include "fm-file-info-job.h"

G_BEGIN_DECLS

#define FM_TYPE_FOLDER              (fm_folder_get_type())
#define FM_FOLDER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            FM_TYPE_FOLDER, FmFolder))
#define FM_FOLDER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),\
            FM_TYPE_FOLDER, FmFolderClass))
#define FM_IS_FOLDER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            FM_TYPE_FOLDER))
#define FM_IS_FOLDER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass),\
            FM_TYPE_FOLDER))

typedef struct _FmFolder            FmFolder;
typedef struct _FmFolderClass       FmFolderClass;

/**
 * FmFolderClass
 * @parent_class: the parent class
 * @files_added: the class closure for #FmFolder::files-added signal
 * @files_removed: the class closure for #FmFolder::files-removed signal
 * @files_changed: the class closure for #FmFolder::files-changed signal
 * @start_loading: the class closure for #FmFolder::start-loading signal
 * @finish_loading: the class closure for #FmFolder::finish-loading signal
 * @unmount: the class closure for #FmFolder::unmount signal
 * @changed: the class closure for #FmFolder::changed signal
 * @removed: the class closure for #FmFolder::removed signal
 * @content_changed: the class closure for #FmFolder::content-changed signal
 * @fs_info: the class closure for #FmFolder::fs-info signal
 * @error: the class closure for #FmFolder::error signal
 */
struct _FmFolderClass
{
    GObjectClass parent_class;

    void (*files_added)(FmFolder* dir, GSList* files);
    void (*files_removed)(FmFolder* dir, GSList* files);
    void (*files_changed)(FmFolder* dir, GSList* files);
    void (*start_loading)(FmFolder* dir);
    void (*finish_loading)(FmFolder* dir);
    void (*unmount)(FmFolder* dir);
    void (*changed)(FmFolder* dir);
    void (*removed)(FmFolder* dir);
    void (*content_changed)(FmFolder* dir);
    void (*fs_info)(FmFolder* dir);
    guint (*error)(FmFolder* dir, GError* err, guint severity);
    /*< private >*/
    gpointer _reserved1;
    gpointer _reserved2;
    gpointer _reserved3;
    gpointer _reserved4;
};

GType       fm_folder_get_type      (void);
FmFolder*   fm_folder_from_path(FmPath* path);
FmFolder*   fm_folder_from_gfile(GFile* gf);
FmFolder*   fm_folder_from_path_name(const char* path);
FmFolder*   fm_folder_from_uri(const char* uri);

FmFolder *fm_folder_find_by_path(FmPath *path);
void fm_folder_block_updates(FmFolder *folder);
void fm_folder_unblock_updates(FmFolder *folder);

FmFileInfo* fm_folder_get_info(FmFolder* folder);
FmPath* fm_folder_get_path(FmFolder* folder);

FmFileInfoList* fm_folder_get_files (FmFolder* folder);
gboolean fm_folder_is_empty(FmFolder* folder);
FmFileInfo* fm_folder_get_file_by_name(FmFolder* folder, const char* name);

gboolean fm_folder_is_loaded(FmFolder* folder);
gboolean fm_folder_is_valid(FmFolder* folder);
gboolean fm_folder_is_incremental(FmFolder* folder);

void fm_folder_reload(FmFolder* folder);

gboolean fm_folder_get_filesystem_info(FmFolder* folder, guint64* total_size, guint64* free_size);
void fm_folder_query_filesystem_info(FmFolder* folder);

/* internal event handling to workaroung GIO inotify delay */
gboolean _fm_folder_event_file_added(FmFolder *folder, FmPath *path);
gboolean _fm_folder_event_file_changed(FmFolder *folder, FmPath *path);
void _fm_folder_event_file_deleted(FmFolder *folder, FmPath *path);

gboolean fm_folder_make_directory(FmFolder *folder, const char *name, GError **error);

void _fm_folder_init();
void _fm_folder_finalize();

G_END_DECLS

#endif /* __FM_FOLDER_H__ */
