/*
 *      Copyright 2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This file is a part of libmenu-cache package and created program
 *      should be not used without the library.
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

#include <libfm/fm-extra.h>
#include <menu-cache.h>

FmXmlFileTag menuTag_Menu;
FmXmlFileTag menuTag_AppDir;
FmXmlFileTag menuTag_DefaultAppDirs;
FmXmlFileTag menuTag_DirectoryDir;
FmXmlFileTag menuTag_DefaultDirectoryDirs;
FmXmlFileTag menuTag_Include;
FmXmlFileTag menuTag_Exclude;
FmXmlFileTag menuTag_Filename;
FmXmlFileTag menuTag_Or;
FmXmlFileTag menuTag_And;
FmXmlFileTag menuTag_Not;
FmXmlFileTag menuTag_Category;
FmXmlFileTag menuTag_MergeFile;
FmXmlFileTag menuTag_MergeDir;
FmXmlFileTag menuTag_DefaultMergeDirs;
FmXmlFileTag menuTag_Directory;
FmXmlFileTag menuTag_Name;
FmXmlFileTag menuTag_Deleted;
FmXmlFileTag menuTag_NotDeleted;
FmXmlFileTag menuTag_OnlyUnallocated;
FmXmlFileTag menuTag_NotOnlyUnallocated;
FmXmlFileTag menuTag_All;
FmXmlFileTag menuTag_LegacyDir;
FmXmlFileTag menuTag_KDELegacyDirs;
FmXmlFileTag menuTag_Move;
FmXmlFileTag menuTag_Old;
FmXmlFileTag menuTag_New;
FmXmlFileTag menuTag_Layout;
FmXmlFileTag menuTag_DefaultLayout;
FmXmlFileTag menuTag_Menuname;
FmXmlFileTag menuTag_Separator;
FmXmlFileTag menuTag_Merge;

typedef enum {
    MERGE_NONE, /* starting value */
    MERGE_FILES, /* first set */
    MERGE_MENUS,
    MERGE_ALL, /* only set */
    MERGE_FILES_MENUS, /* second set */
    MERGE_MENUS_FILES
} MenuMergeType;

typedef struct {
    MenuCacheType type : 2; /* used by MenuMenu, MENU_CACHE_TYPE_DIR */
    gboolean only_unallocated : 1; /* for Menuname: TRUE if show_empty is set */
    gboolean is_set : 1; /* used by MenuMenu, for Menuname: TRUE if allow_inline is set */
    gboolean show_empty : 1;
    gboolean allow_inline : 1;
    gboolean inline_header : 1;
    gboolean inline_alias : 1;
    gboolean inline_header_is_set : 1; /* for Menuname */
    gboolean inline_alias_is_set : 1; /* for Menuname */
    gboolean inline_limit_is_set : 1; /* for Menuname; for MenuMenu is Legacy mark */
    gboolean nodisplay : 1;
    GList *items; /* items are MenuItem : Menuname or Filename or Separator or Merge */
    int inline_limit;
} MenuLayout;

/* Menuname item */
typedef struct {
    MenuLayout layout;
    char *name;
} MenuMenuname;

/* Filename item in layout */
typedef struct {
    MenuCacheType type : 2; /* MENU_CACHE_TYPE_APP */
    char *id;
} MenuFilename;

/* Separator item */
typedef struct {
    MenuCacheType type : 2; /* MENU_CACHE_TYPE_SEP */
} MenuSep;

/* Merge item */
typedef struct {
    MenuCacheType type : 2; /* MENU_CACHE_TYPE_NONE */
    MenuMergeType merge_type;
} MenuMerge;

/* Menu item */
typedef struct {
    MenuLayout layout; /* copied from hash on </Menu> */
    char *name;
    /* next fields are only for Menu */
    char *key; /* for sorting */
    GList *id; /* <Directory> for <Menu>, may be NULL, first is most relevant */
    /* next fields are only for composer */
    GList *children; /* items are MenuItem : MenuApp, MenuMenu, MenuSep, MenuRule */
    char *title;
    char *comment;
    char *icon;
    const char *dir;
} MenuMenu;

/* File item in menu */
typedef struct {
    MenuCacheType type : 2; /* MENU_CACHE_TYPE_APP */
    gboolean excluded : 1;
    gboolean allocated : 1;
    gboolean matched : 1;
    gboolean use_terminal : 1;
    gboolean use_notification : 1;
    gboolean hidden : 1;
    GList *dirs; /* can be reordered until allocated */
    GList *menus;
    char *filename; /* if NULL then is equal to id */
    char *key; /* for sorting */
    char *id;
    char *title;
    char *comment;
    char *icon;
    char *generic_name;
    char *exec;
    char *try_exec;
    char *wd;
    const char **categories; /* all char ** keep interned values */
    const char **keywords;
    const char **show_in;
    const char **hide_in;
} MenuApp;

/* a placeholder for matching */
typedef struct {
    MenuCacheType type : 2; /* MENU_CACHE_TYPE_NONE */
    FmXmlFileItem *rule;
} MenuRule;

/* requested language(s) */
char **languages;

/* list of menu files to monitor */
GSList *MenuFiles;

/* list of menu dirs to monitor */
GSList *MenuDirs;

/* list of available app dirs */
GSList *AppDirs;

/* list of available dir dirs */
GSList *DirDirs;

/* parse and merge menu files */
MenuMenu *get_merged_menu(const char *file, FmXmlFile **xmlfile, GError **error);

/* parse all files into layout and save cache file */
gboolean save_menu_cache(MenuMenu *layout, const char *menuname, const char *file,
                         gboolean with_hidden);

/* free MenuLayout data */
void _free_layout_items(GList *data);

/* verbosity level */
gint verbose;

#define DBG if (verbose) g_debug
#define VDBG if (verbose > 1) g_debug
#define VVDBG if (verbose > 2) g_debug
