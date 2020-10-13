/*
 *      fm-path.h
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2012 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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


#ifndef __FM_PATH_H__
#define __FM_PATH_H__

#include <glib.h>
#include <gio/gio.h>

#include "fm-list.h"

G_BEGIN_DECLS

#define FM_PATH(path)   ((FmPath*)path)

typedef struct _FmPath FmPath;
typedef struct _FmPathList FmPathList;

/**
 * FmPathFlags:
 * @FM_PATH_NONE: -
 * @FM_PATH_IS_NATIVE: This is a native path to UNIX, like /home
 * @FM_PATH_IS_LOCAL: This path refers to a file on local filesystem
 * @FM_PATH_IS_VIRTUAL: This path is virtual and it doesn't exist on real filesystem
 * @FM_PATH_IS_TRASH: This path is under trash:///
 * @FM_PATH_IS_XDG_MENU: This path is under menu:///
 *
 * Flags of #FmPath object.
 *
 * FM_PATH_IS_VIRTUAL and FM_PATH_IS_XDG_MENU are deprecated since 1.0.2
 * and FM_PATH_IS_LOCAL is deprecated since 1.2.0,
 * and should not be used in newly written code.
 */
typedef enum
{
    FM_PATH_NONE = 0,
    FM_PATH_IS_NATIVE = 1<<0,
    FM_PATH_IS_LOCAL = 1<<1,
    FM_PATH_IS_VIRTUAL = 1<<2,
    FM_PATH_IS_TRASH = 1<<3,
    FM_PATH_IS_XDG_MENU = 1<<4,

    /*< private >*/
    /* reserved for future use */
    FM_PATH_IS_RESERVED1 = 1<<5,
    FM_PATH_IS_RESERVED2 = 1<<6,
    FM_PATH_IS_RESERVED3 = 1<<7,
} FmPathFlags;

typedef struct _FmFileInfoList FmFileInfoList; /* fm-file-info.h includes this too */

void _fm_path_init(void);
void _fm_path_finalize(void);

FmPath* fm_path_new_for_path(const char* path_name);
FmPath* fm_path_new_for_uri(const char* uri);
FmPath* fm_path_new_for_display_name(const char* path_name);
FmPath* fm_path_new_for_str(const char* path_str);
FmPath* fm_path_new_for_commandline_arg(const char* arg);

FmPath* fm_path_new_child(FmPath* parent, const char* basename);
FmPath* fm_path_new_child_len(FmPath* parent, const char* basename, int name_len);
FmPath* fm_path_new_relative(FmPath* parent, const char* rel);
FmPath* fm_path_new_for_gfile(GFile* gf);

/* predefined paths */
FmPath* fm_path_get_root(void); /* / */
FmPath* fm_path_get_home(void); /* home directory */
FmPath* fm_path_get_desktop(void); /* $HOME/Desktop */
FmPath* fm_path_get_trash(void); /* trash:/// */
FmPath* fm_path_get_apps_menu(void); /* menu://applications.menu/ */

FmPath* fm_path_ref(FmPath* path);
void fm_path_unref(FmPath* path);

FmPath* fm_path_get_parent(FmPath* path);
const char* fm_path_get_basename(FmPath* path);
FmPathFlags fm_path_get_flags(FmPath* path);
gboolean fm_path_has_prefix(FmPath* path, FmPath* prefix);
FmPath* fm_path_get_scheme_path(FmPath* path);

#define fm_path_is_native(path) ((fm_path_get_flags(path)&FM_PATH_IS_NATIVE)!=0)
#define fm_path_is_trash(path) ((fm_path_get_flags(path)&FM_PATH_IS_TRASH)!=0)
#define fm_path_is_trash_root(path) (path == fm_path_get_trash())
#define fm_path_is_native_or_trash(path) ((fm_path_get_flags(path)&(FM_PATH_IS_NATIVE|FM_PATH_IS_TRASH))!=0)
#ifndef FM_DISABLE_DEPRECATED
#define fm_path_is_local(path) ((fm_path_get_flags(path)&FM_PATH_IS_LOCAL)!=0)
#define fm_path_is_virtual(path) ((fm_path_get_flags(path)&FM_PATH_IS_VIRTUAL)!=0)
#define fm_path_is_xdg_menu(path) ((fm_path_get_flags(path)&FM_PATH_IS_XDG_MENU)!=0)
#endif

char* fm_path_to_str(FmPath* path);
char* fm_path_to_uri(FmPath* path);
GFile* fm_path_to_gfile(FmPath* path);

char* fm_path_display_name(FmPath* path, gboolean human_readable);
char* fm_path_display_basename(FmPath* path);

/* for usage by FmFileInfo handlers - never use in applications */
void _fm_path_set_display_name(FmPath *path, const char *disp_name);
const char *_fm_path_get_display_name(FmPath *path);

/* For used in hash tables */
guint fm_path_hash(FmPath* path);
gboolean fm_path_equal(FmPath* p1, FmPath* p2);

/* can be used for sorting */
int fm_path_compare(FmPath* p1, FmPath* p2);

/* used for completion in fm_path_entry */
gboolean fm_path_equal_str(FmPath *path, const gchar *str, int n);

/* calculate how many elements are in this path. */
int fm_path_depth(FmPath* path);

/* path list */
FmPathList* fm_path_list_new(void);
FmPathList* fm_path_list_new_from_uri_list(const char* uri_list);
FmPathList* fm_path_list_new_from_uris(char* const* uris);
FmPathList* fm_path_list_new_from_file_info_list(FmFileInfoList* fis);
FmPathList* fm_path_list_new_from_file_info_glist(GList* fis);
FmPathList* fm_path_list_new_from_file_info_gslist(GSList* fis);

#ifndef __GTK_DOC_IGNORE__
static inline FmPathList* fm_path_list_ref(FmPathList* list)
{
    return list ? (FmPathList*)fm_list_ref((FmList*)list) : NULL;
}
static inline void fm_path_list_unref(FmPathList* list)
{
    g_return_if_fail(list);
    fm_list_unref((FmList*)list);
}

static inline guint fm_path_list_get_length(FmPathList* list)
{
    return fm_list_get_length((FmList*)list);
}
static inline gboolean fm_path_list_is_empty(FmPathList* list)
{
    return fm_list_is_empty((FmList*)list);
}
static inline FmPath* fm_path_list_peek_head(FmPathList* list)
{
    return (FmPath*)fm_list_peek_head((FmList*)list);
}
static inline GList* fm_path_list_peek_head_link(FmPathList* list)
{
    return fm_list_peek_head_link((FmList*)list);
}

static inline void fm_path_list_push_tail(FmPathList* list, FmPath* d)
{
    fm_list_push_tail((FmList*)list,d);
}
#endif /* __GTK_DOC_IGNORE__ */

char* fm_path_list_to_uri_list(FmPathList* pl);
/* char** fm_path_list_to_uris(FmPathList* pl); */
void fm_path_list_write_uri_list(FmPathList* pl, GString* buf);

G_END_DECLS

#endif /* __FM_PATH_H__ */
