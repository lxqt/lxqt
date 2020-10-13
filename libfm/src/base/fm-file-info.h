/*
 *      fm-file-info.h
 *
 *      Copyright 2009 - 2012 PCMan <pcman.tw@gmail.com>
 *      Copyright 2012-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifndef _FM_FILE_INFO_H_
#define _FM_FILE_INFO_H_

#include <glib.h>
#include <gio/gio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fm-icon.h"
#include "fm-list.h"
#include "fm-path.h"
#include "fm-mime-type.h"

G_BEGIN_DECLS

typedef struct _FmFileInfo FmFileInfo;
//typedef struct _FmFileInfoList FmFileInfoList; // defined in fm-path.h

struct _MenuCacheItem;/* forward declaration for MenuCacheItem */

/* intialize the file info system */
void _fm_file_info_init();
void _fm_file_info_finalize();

FmFileInfo* fm_file_info_new();
#ifndef FM_DISABLE_DEPRECATED
FmFileInfo* fm_file_info_new_from_gfileinfo(FmPath* path, GFileInfo* inf);
void fm_file_info_set_from_gfileinfo(FmFileInfo* fi, GFileInfo* inf);
#endif
FmFileInfo *fm_file_info_new_from_g_file_data(GFile *gf, GFileInfo *inf, FmPath *path);
void fm_file_info_set_from_g_file_data(FmFileInfo* fi, GFile* gf, GFileInfo* inf);

FmFileInfo* fm_file_info_new_from_menu_cache_item(FmPath* path, struct _MenuCacheItem* item);
#ifndef FM_DISABLE_DEPRECATED
void fm_file_info_set_from_menu_cache_item(FmFileInfo* fi, struct _MenuCacheItem* item);
#endif

gboolean fm_file_info_set_from_native_file(FmFileInfo* fi, const char* path, GError** err);
FmFileInfo *fm_file_info_new_from_native_file(FmPath *path, const char *path_str, GError **err);

FmFileInfo* fm_file_info_ref( FmFileInfo* fi );
void fm_file_info_unref( FmFileInfo* fi );

void fm_file_info_update(FmFileInfo* fi, FmFileInfo* src);

/** returned FmPath shouldn't be unref by caller */
FmPath* fm_file_info_get_path( FmFileInfo* fi );
const char* fm_file_info_get_name( FmFileInfo* fi );
const char* fm_file_info_get_disp_name( FmFileInfo* fi );
//const char* fm_file_info_get_edit_name(FmFileInfo *fi);

void fm_file_info_set_path(FmFileInfo* fi, FmPath* path);
void fm_file_info_set_disp_name( FmFileInfo* fi, const char* name );
void fm_file_info_set_icon(FmFileInfo *fi, GIcon *icon);

goffset fm_file_info_get_size( FmFileInfo* fi );
const char* fm_file_info_get_disp_size( FmFileInfo* fi );

goffset fm_file_info_get_blocks( FmFileInfo* fi );

mode_t fm_file_info_get_mode( FmFileInfo* fi );

gboolean fm_file_info_is_native(FmFileInfo* fi);

FmMimeType* fm_file_info_get_mime_type( FmFileInfo* fi );

gboolean fm_file_info_is_dir( FmFileInfo* fi );

gboolean fm_file_info_is_symlink( FmFileInfo* fi );

gboolean fm_file_info_is_shortcut( FmFileInfo* fi );

gboolean fm_file_info_is_mountable( FmFileInfo* fi );

gboolean fm_file_info_is_image( FmFileInfo* fi );

gboolean fm_file_info_is_text( FmFileInfo* fi );

gboolean fm_file_info_is_desktop_entry( FmFileInfo* fi );

gboolean fm_file_info_is_unknown_type( FmFileInfo* fi );

gboolean fm_file_info_is_hidden(FmFileInfo* fi);
gboolean fm_file_info_is_backup(FmFileInfo* fi);

/* if the mime-type is executable, such as shell script, python script, ... */
gboolean fm_file_info_is_executable_type( FmFileInfo* fi);
gboolean fm_file_info_is_accessible(FmFileInfo* fi);
gboolean fm_file_info_is_writable_directory(FmFileInfo* fi);

const char* fm_file_info_get_target( FmFileInfo* fi );

const char* fm_file_info_get_collate_key( FmFileInfo* fi );
const char* fm_file_info_get_collate_key_nocasefold(FmFileInfo* fi);
const char* fm_file_info_get_desc( FmFileInfo* fi );
const char* fm_file_info_get_disp_mtime( FmFileInfo* fi );
time_t fm_file_info_get_mtime( FmFileInfo* fi );
time_t fm_file_info_get_atime( FmFileInfo* fi );
time_t fm_file_info_get_ctime(FmFileInfo *fi);
FmIcon* fm_file_info_get_icon( FmFileInfo* fi );
uid_t fm_file_info_get_uid( FmFileInfo* fi );
const char *fm_file_info_get_disp_owner(FmFileInfo *fi);
gid_t fm_file_info_get_gid( FmFileInfo* fi );
const char *fm_file_info_get_disp_group(FmFileInfo *fi);
const char* fm_file_info_get_fs_id( FmFileInfo* fi );
dev_t fm_file_info_get_dev( FmFileInfo* fi );

gboolean fm_file_info_can_thumbnail(FmFileInfo* fi);

gboolean fm_file_info_can_set_name(FmFileInfo *fi);
gboolean fm_file_info_can_set_icon(FmFileInfo *fi);
gboolean fm_file_info_can_set_hidden(FmFileInfo *fi);

FmFileInfoList* fm_file_info_list_new();
//FmFileInfoList* fm_file_info_list_new_from_glist();

#ifndef __GTK_DOC_IGNORE__
static inline FmFileInfoList* fm_file_info_list_ref(FmFileInfoList* list)
{
    return list ? (FmFileInfoList*)fm_list_ref((FmList*)list) : NULL;
}
static inline void fm_file_info_list_unref(FmFileInfoList* list)
{
    if(list == NULL) return;
    fm_list_unref((FmList*)list);
}

static inline gboolean fm_file_info_list_is_empty(FmFileInfoList* list)
{
    return fm_list_is_empty((FmList*)list);
}
static inline guint fm_file_info_list_get_length(FmFileInfoList* list)
{
    return fm_list_get_length((FmList*)list);
}
static inline FmFileInfo* fm_file_info_list_peek_head(FmFileInfoList* list)
{
    return (FmFileInfo*)fm_list_peek_head((FmList*)list);
}
static inline GList* fm_file_info_list_peek_head_link(FmFileInfoList* list)
{
    return fm_list_peek_head_link((FmList*)list);
}

static inline void fm_file_info_list_push_tail(FmFileInfoList* list, FmFileInfo* d)
{
    fm_list_push_tail((FmList*)list,d);
}
static inline void fm_file_info_list_push_tail_link(FmFileInfoList* list, GList* d)
{
    fm_list_push_tail_link((FmList*)list,d);
}
static inline void fm_file_info_list_push_tail_noref(FmFileInfoList* list, FmFileInfo* d)
{
    fm_list_push_tail_noref((FmList*)list,d);
}
static inline FmFileInfo* fm_file_info_list_pop_head(FmFileInfoList* list)
{
    return (FmFileInfo*)fm_list_pop_head((FmList*)list);
}
static inline void fm_file_info_list_delete_link(FmFileInfoList* list, GList* _l)
{
    fm_list_delete_link((FmList*)list,_l);
}
static inline void fm_file_info_list_delete_link_nounref(FmFileInfoList* list, GList* _l)
{
    fm_list_delete_link_nounref((FmList*)list,_l);
}
static inline void fm_file_info_list_clear(FmFileInfoList* list)
{
    fm_list_clear((FmList*)list);
}
#endif /* __GTK_DOC_IGNORE__ */

/* return TRUE if all files in the list are of the same type */
gboolean fm_file_info_list_is_same_type(FmFileInfoList* list);

/* return TRUE if all files in the list are on the same fs */
gboolean fm_file_info_list_is_same_fs(FmFileInfoList* list);

#define FM_FILE_INFO(ptr)    ((FmFileInfo*)ptr)

G_END_DECLS

#endif
