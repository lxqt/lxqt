/*
 *      menu-file.c : parses <name>.menu file and merges all XML tags.
 *
 *      Copyright 2013-2017 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "menu-tags.h"

#include <string.h>
#include <stdlib.h>
#include <gio/gio.h>

#define _(...) __VA_ARGS__

/* ---- applications.menu manipulations ---- */
typedef struct _MenuTreeData MenuTreeData;

struct _MenuTreeData
{
    FmXmlFile *menu; /* composite tree to analyze */
    const char *file_path; /* current file */
    gint line, pos; /* we remember position in deepest file */
};

FmXmlFileTag menuTag_Menu = 0; /* tags that are supported */
FmXmlFileTag menuTag_Include = 0;
FmXmlFileTag menuTag_Exclude = 0;
FmXmlFileTag menuTag_Filename = 0;
FmXmlFileTag menuTag_Or = 0;
FmXmlFileTag menuTag_And = 0;
FmXmlFileTag menuTag_Not = 0;
FmXmlFileTag menuTag_Category = 0;
FmXmlFileTag menuTag_MergeFile = 0;
FmXmlFileTag menuTag_MergeDir = 0;
FmXmlFileTag menuTag_DefaultMergeDirs = 0;
FmXmlFileTag menuTag_Directory = 0;
FmXmlFileTag menuTag_Name = 0;
FmXmlFileTag menuTag_Deleted = 0;
FmXmlFileTag menuTag_NotDeleted = 0;
FmXmlFileTag menuTag_AppDir = 0;
FmXmlFileTag menuTag_DefaultAppDirs = 0;
FmXmlFileTag menuTag_DirectoryDir = 0;
FmXmlFileTag menuTag_DefaultDirectoryDirs = 0;
FmXmlFileTag menuTag_OnlyUnallocated = 0;
FmXmlFileTag menuTag_NotOnlyUnallocated = 0;
FmXmlFileTag menuTag_All = 0;
FmXmlFileTag menuTag_LegacyDir = 0;
FmXmlFileTag menuTag_KDELegacyDirs = 0;
FmXmlFileTag menuTag_Move = 0;
FmXmlFileTag menuTag_Old = 0;
FmXmlFileTag menuTag_New = 0;
FmXmlFileTag menuTag_Layout = 0;
FmXmlFileTag menuTag_DefaultLayout = 0;
FmXmlFileTag menuTag_Menuname = 0;
FmXmlFileTag menuTag_Separator = 0;
FmXmlFileTag menuTag_Merge = 0;

/* list of available app dirs */
GSList *AppDirs = NULL;

/* list of available dir dirs */
GSList *DirDirs = NULL;

/* list of menu dirs to monitor */
GSList *MenuDirs = NULL;

/* we keep all the unfinished items in the hash */
static GHashTable *layout_hash = NULL;

#define RETURN_TRUE_AND_DESTROY_IF_QUIET(a) do { \
    if (verbose == 0) { \
        fm_xml_file_item_destroy(a); \
        return TRUE; \
    } } while (0)

static gboolean _fail_if_in_layout(FmXmlFileItem *item, GError **error)
{
    FmXmlFileItem *parent;

    parent = fm_xml_file_item_get_parent(item);
    if (parent && (fm_xml_file_item_get_tag(parent) == menuTag_Layout ||
                   fm_xml_file_item_get_tag(parent) == menuTag_DefaultLayout))
    {
        g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                    _("Tag <%s> is invalid below <%s>"),
                    fm_xml_file_item_get_tag_name(item),
                    fm_xml_file_item_get_tag_name(parent));
        return TRUE;
    }
    return FALSE;
}

#define RETURN_IF_IN_LAYOUT(i,e) do { \
    if (_fail_if_in_layout(i, e)) { \
        if (verbose > 0) \
            return FALSE; \
        fm_xml_file_item_destroy(i); \
        g_clear_error(e); \
        return TRUE; \
    } } while (0)

/* this handler does nothing, used just to remember its id */
static gboolean _menu_xml_handler_pass(FmXmlFileItem *item, GList *children,
                                       char * const *attribute_names,
                                       char * const *attribute_values,
                                       guint n_attributes, gint line, gint pos,
                                       GError **error, gpointer user_data)
{
    RETURN_IF_IN_LAYOUT(item, error);
    return TRUE;
}

/* checks the tag */
static gboolean _menu_xml_handler_Name(FmXmlFileItem *item, GList *children,
                                       char * const *attribute_names,
                                       char * const *attribute_values,
                                       guint n_attributes, gint line, gint pos,
                                       GError **error, gpointer user_data)
{
    FmXmlFileItem *name_item;
    const char *name;

    RETURN_IF_IN_LAYOUT(item, error);
    name_item = fm_xml_file_item_find_child(item, FM_XML_FILE_TEXT);
    if (name_item == NULL ||
        (name = fm_xml_file_item_get_data(name_item, NULL)) == NULL ||
        strchr(name, '/') != NULL) /* empty or invalid tag */
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Invalid <Name> tag"));
        return FALSE;
    }
    return TRUE;
}

static gboolean _menu_xml_handler_Not(FmXmlFileItem *item, GList *children,
                                      char * const *attribute_names,
                                      char * const *attribute_values,
                                      guint n_attributes, gint line, gint pos,
                                      GError **error, gpointer user_data)
{
    FmXmlFileTag tag;
    GList *child;

    RETURN_IF_IN_LAYOUT(item, error);
    if (verbose > 0) for (child = children; child; child = child->next)
    {
        tag = fm_xml_file_item_get_tag(child->data);
        if (tag != menuTag_And && tag == menuTag_Or && tag == menuTag_Filename &&
            tag != menuTag_Category && tag != menuTag_All)
        {
            g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                                _("Tag <Not> may contain only <And>, <Or>,"
                                  " <Filename>, <Category> or <All> child"));
            return FALSE;
        }
    }
    return TRUE;
}

static void _add_app_dir(const char *app_dir)
{
    const char *str = g_intern_string(app_dir);
    GSList *l;
    GDir *dir;
    char *path;

    for (l = AppDirs; l; l = l->next)
        if (l->data == str)
            break;
    if (l == NULL)
        AppDirs = g_slist_append(AppDirs, (gpointer)str);
    /* recursively scan the directory now */
    dir = g_dir_open(app_dir, 0, NULL);
    if (dir)
    {
        while ((str = g_dir_read_name(dir)) != NULL) /* reuse pointer */
        {
            path = g_build_filename(app_dir, str, NULL);
            if (g_file_test(path, G_FILE_TEST_IS_DIR))
                _add_app_dir(path);
            g_free(path);
        }
        g_dir_close(dir);
    }
}

static gboolean _menu_xml_handler_AppDir(FmXmlFileItem *item, GList *children,
                                         char * const *attribute_names,
                                         char * const *attribute_values,
                                         guint n_attributes, gint line, gint pos,
                                         GError **error, gpointer user_data)
{
    MenuTreeData *data = user_data;
    FmXmlFileItem *parent, *name;
    const char *path;
    char *_path;

    parent = fm_xml_file_item_get_parent(item);
    if (parent == NULL || fm_xml_file_item_get_tag(parent) != menuTag_Menu)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Tag <AppDir> can appear only below <Menu>"));
        return FALSE;
    }
    if (children == NULL ||
        fm_xml_file_item_get_tag((name = children->data)) != FM_XML_FILE_TEXT ||
        (path = fm_xml_file_item_get_data(name, NULL)) == NULL)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Invalid <AppDir> tag"));
        return FALSE;
    }
    if (g_path_is_absolute(path))
        _path = NULL;
    else
    {
        char *_dir = g_path_get_dirname(data->file_path);
        path = _path = g_build_filename(_dir, path, NULL);
        g_free(_dir);
        /* FIXME: canonicalize path */
        fm_xml_file_item_destroy(name);
        fm_xml_file_item_append_text(item, _path, -1, FALSE);
    }
    _add_app_dir(path);
    g_free(_path);
    /* contents of the directory will be parsed later */
    return TRUE;
}

static gboolean _menu_xml_handler_DefaultAppDirs(FmXmlFileItem *item, GList *children,
                                                 char * const *attribute_names,
                                                 char * const *attribute_values,
                                                 guint n_attributes, gint line, gint pos,
                                                 GError **error, gpointer user_data)
{
    FmXmlFileItem *parent;
    static gboolean added = FALSE;

    parent = fm_xml_file_item_get_parent(item);
    if (parent == NULL || fm_xml_file_item_get_tag(parent) != menuTag_Menu)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Tag <DefaultAppDirs> can appear only below <Menu>"));
        return FALSE;
    }
    if (!added)
    {
        const gchar* const * dirs = g_get_system_data_dirs();
        char *dir = g_build_filename(g_get_user_data_dir(), "applications", NULL);
        _add_app_dir(dir);
        g_free(dir);
        if (dirs) while (dirs[0] != NULL)
        {
            dir = g_build_filename(*dirs++, "applications", NULL);
            _add_app_dir(dir);
            g_free(dir);
        }
        added = TRUE;
    }
    /* contents of the directories will be parsed later */
    return TRUE;
}

static void _add_dir_dir(const char *dir_dir)
{
    const char *str = g_intern_string(dir_dir);
    GSList *l;

    for (l = DirDirs; l; l = l->next)
        if (l->data == str)
            return;
    DirDirs = g_slist_append(DirDirs, (gpointer)str);
}

static gboolean _menu_xml_handler_DirectoryDir(FmXmlFileItem *item, GList *children,
                                               char * const *attribute_names,
                                               char * const *attribute_values,
                                               guint n_attributes, gint line, gint pos,
                                               GError **error, gpointer user_data)
{
    MenuTreeData *data = user_data;
    FmXmlFileItem *parent, *name;
    const char *path;

    parent = fm_xml_file_item_get_parent(item);
    if (parent == NULL || fm_xml_file_item_get_tag(parent) != menuTag_Menu)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Tag <DirectoryDir> can appear only below <Menu>"));
        return FALSE;
    }
    if (children == NULL ||
        fm_xml_file_item_get_tag((name = children->data)) != FM_XML_FILE_TEXT ||
        (path = fm_xml_file_item_get_data(name, NULL)) == NULL)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Invalid <DirectoryDir> tag"));
        return FALSE;
    }
    if (g_path_is_absolute(path))
        _add_dir_dir(path);
    else
    {
        char *_dir = g_path_get_dirname(data->file_path);
        char *_path = g_build_filename(_dir, path, NULL);

        g_free(_dir);
        fm_xml_file_item_destroy(name);
        fm_xml_file_item_append_text(item, _path, -1, FALSE);
        _add_dir_dir(_path);
        g_free(_path);
    }
    /* contents of the directory will be parsed later */
    return TRUE;
}

static gboolean _menu_xml_handler_DefaultDirectoryDirs(FmXmlFileItem *item, GList *children,
                                                       char * const *attribute_names,
                                                       char * const *attribute_values,
                                                       guint n_attributes, gint line, gint pos,
                                                       GError **error, gpointer user_data)
{
    FmXmlFileItem *parent;
    static gboolean added = FALSE;

    parent = fm_xml_file_item_get_parent(item);
    if (parent == NULL || fm_xml_file_item_get_tag(parent) != menuTag_Menu)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Tag <DefaultDirectoryDirs> can appear only below <Menu>"));
        return FALSE;
    }
    if (!added)
    {
        const gchar* const * dirs = g_get_system_data_dirs();
        char *dir = g_build_filename(g_get_user_data_dir(), "desktop-directories", NULL);
        _add_dir_dir(dir);
        g_free(dir);
        if (dirs) while (dirs[0] != NULL)
        {
            dir = g_build_filename(*dirs++, "desktop-directories", NULL);
            _add_dir_dir(dir);
            g_free(dir);
        }
        added = TRUE;
    }
    /* contents of the directories will be parsed later */
    return TRUE;
}

/* adds .menu file contents next to current item */
static gboolean _menu_xml_handler_MergeFile(FmXmlFileItem *item, GList *children,
                                            char * const *attribute_names,
                                            char * const *attribute_values,
                                            guint n_attributes, gint line, gint pos,
                                            GError **error, gpointer user_data)
{
    MenuTreeData *data = user_data;
    FmXmlFileItem *name;
    const char *path;

    name = fm_xml_file_item_get_parent(item);
    if (name == NULL || fm_xml_file_item_get_tag(name) != menuTag_Menu)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Tag <MergeFile> can appear only below <Menu>"));
        return FALSE;
    }
    if (children == NULL ||
        fm_xml_file_item_get_tag((name = children->data)) != FM_XML_FILE_TEXT ||
        (path = fm_xml_file_item_get_data(name, NULL)) == NULL)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Invalid <MergeFile> tag"));
        return FALSE;
    }
    if (attribute_names) while (attribute_names[0])
    {
        if (strcmp(attribute_names[0], "type") == 0)
        {
            if (strcmp(attribute_values[0], "parent") == 0)
            {
                const gchar* const *dirs = g_get_system_config_dirs();
                const gchar* const *dir;
                const char *rel_path;
                char *file;

                /* scan whole config dirs for matching, ignoring current path */
                for (dir = dirs; dir[0]; dir++)
                    if (g_str_has_prefix(data->file_path, dir[0]))
                    {
                        rel_path = data->file_path + strlen(dir[0]);
                        goto replace_from_system_config_dirs;
                    }
                /* not found in XDG_CONFIG_DIRS, test for XDG_CONFIG_HOME */
                if (g_str_has_prefix(data->file_path, g_get_user_config_dir()))
                {
                    rel_path = data->file_path + strlen(g_get_user_config_dir());
replace_from_system_config_dirs:
                    fm_xml_file_item_destroy(name);
                    while (*rel_path == G_DIR_SEPARATOR) rel_path++;
                    while (dirs[0] != NULL)
                    {
                        if (dirs[0] == dir[0])
                            continue;
                        file = g_build_filename(dirs[0], rel_path, NULL);
                        if (g_file_test(file, G_FILE_TEST_IS_REGULAR))
                        {
                            fm_xml_file_item_append_text(item, file, -1, FALSE);
                            g_free(file);
                            break;
                        }
                        g_free(file);
                        dirs++;
                    }
                    if (dirs[0] != NULL) /* a file for merge was found */
                        return TRUE;
                }
                /* FIXME: what to do if parsed file is not in some config dirs? */
                VDBG("No file for <MergeFile type=\"parent\"/> found, ignoring");
                fm_xml_file_item_destroy(item);
                return TRUE;
            }
            break;
        }
        attribute_names++;
        attribute_values++;
    }
    if (!g_path_is_absolute(path))
    {
        char *_dir = g_path_get_dirname(data->file_path);
        char *_path = g_build_filename(_dir, path, NULL);

        g_free(_dir);
        fm_xml_file_item_destroy(name);
        fm_xml_file_item_append_text(item, _path, -1, FALSE);
        g_free(_path);
    }
    /* actual merge will be done in next stage */
    return TRUE;
}

/* adds all .menu files in directory */
static gboolean _menu_xml_handler_MergeDir(FmXmlFileItem *item, GList *children,
                                           char * const *attribute_names,
                                           char * const *attribute_values,
                                           guint n_attributes, gint line, gint pos,
                                           GError **error, gpointer user_data)
{
    MenuTreeData *data = user_data;
    FmXmlFileItem *name;
    const char *path;

    name = fm_xml_file_item_get_parent(item);
    if (name == NULL || fm_xml_file_item_get_tag(name) != menuTag_Menu)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Tag <MergeDir> can appear only below <Menu>"));
        return FALSE;
    }
    if (children == NULL ||
        fm_xml_file_item_get_tag((name = children->data)) != FM_XML_FILE_TEXT ||
        (path = fm_xml_file_item_get_data(name, NULL)) == NULL)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Invalid <MergeDir> tag"));
        return FALSE;
    }
    if (!g_path_is_absolute(path))
    {
        char *_dir = g_path_get_dirname(data->file_path);
        char *_path = g_build_filename(_dir, path, NULL);

        g_free(_dir);
        fm_xml_file_item_destroy(name);
        fm_xml_file_item_append_text(item, _path, -1, FALSE);
        g_free(_path);
    }
    /* actual merge will be done in next stage */
    return TRUE;
}

/* used for validating DefaultMergeDirs and KDELegacyDirs */
static gboolean _menu_xml_handler_DefaultMergeDirs(FmXmlFileItem *item, GList *children,
                                                   char * const *attribute_names,
                                                   char * const *attribute_values,
                                                   guint n_attributes, gint line, gint pos,
                                                   GError **error, gpointer user_data)
{
    FmXmlFileItem *parent = fm_xml_file_item_get_parent(item);

    if (parent == NULL || fm_xml_file_item_get_tag(parent) != menuTag_Menu)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                    _("Tag <%s> can appear only below <Menu>"),
                    fm_xml_file_item_get_tag_name(item));
        return FALSE;
    }
    /* actual merge will be done in next stage */
    return TRUE;
}

static gboolean _menu_xml_handler_LegacyDir(FmXmlFileItem *item, GList *children,
                                            char * const *attribute_names,
                                            char * const *attribute_values,
                                            guint n_attributes, gint line, gint pos,
                                            GError **error, gpointer user_data)
{
    MenuTreeData *data = user_data;
    FmXmlFileItem *name;
    const char *path;

    name = fm_xml_file_item_get_parent(item);
    if (name == NULL || fm_xml_file_item_get_tag(name) != menuTag_Menu)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Tag <LegacyDir> can appear only below <Menu>"));
        return FALSE;
    }
    if (children == NULL ||
        fm_xml_file_item_get_tag((name = children->data)) != FM_XML_FILE_TEXT ||
        (path = fm_xml_file_item_get_data(name, NULL)) == NULL)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Invalid <LegacyDir> tag"));
        return FALSE;
    }
    if (!g_path_is_absolute(path))
    {
        char *_dir = g_path_get_dirname(data->file_path);
        char *_path = g_build_filename(_dir, path, NULL);

        g_free(_dir);
        fm_xml_file_item_destroy(name);
        fm_xml_file_item_append_text(item, _path, -1, FALSE);
        g_free(_path);
    }
    /* handle "prefix" attribute! */
    path = 0;
    if (attribute_names) while (attribute_names[0])
    {
        if (strcmp(attribute_names[0], "prefix") == 0)
            path = attribute_values[0];
        attribute_names++;
        attribute_values++;
    }
    fm_xml_file_item_set_comment(item, path);
    /* actual merge will be done in next stage */
    return TRUE;
}

static MenuLayout *_find_layout(FmXmlFileItem *item, gboolean create)
{
    MenuLayout *layout = g_hash_table_lookup(layout_hash, item);

    if (layout == NULL && create)
    {
        layout = g_slice_new0(MenuLayout);
        /* set defaults */
        layout->inline_header = TRUE;
        layout->inline_limit = 4;
        g_hash_table_insert(layout_hash, item, layout);
    }
    return layout;
}

static gboolean _menu_xml_handler_Filename(FmXmlFileItem *item, GList *children,
                                           char * const *attribute_names,
                                           char * const *attribute_values,
                                           guint n_attributes, gint line, gint pos,
                                           GError **error, gpointer user_data)
{
    FmXmlFileItem *parent;
    const char *id;
    MenuLayout *layout;
    MenuFilename *app;
    FmXmlFileTag tag = 0;

    if (children == NULL ||
        fm_xml_file_item_get_tag(children->data) != FM_XML_FILE_TEXT ||
        (id = fm_xml_file_item_get_data(children->data, NULL)) == NULL)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Empty <Filename> tag"));
        return FALSE;
    }
    parent = fm_xml_file_item_get_parent(item);
    if (parent)
        tag = fm_xml_file_item_get_tag(parent);
    if (tag == menuTag_Layout || tag == menuTag_DefaultLayout)
    {
        layout = _find_layout(parent, TRUE);
        app = g_slice_new0(MenuFilename);
        app->type = MENU_CACHE_TYPE_APP;
        app->id = g_strdup(id);
        layout->items = g_list_append(layout->items, app);
    }
    return TRUE;
}

static gboolean _menu_xml_handler_Menuname(FmXmlFileItem *item, GList *children,
                                           char * const *attribute_names,
                                           char * const *attribute_values,
                                           guint n_attributes, gint line, gint pos,
                                           GError **error, gpointer user_data)
{
    FmXmlFileItem *parent;
    const char *name;
    MenuLayout *layout;
    MenuMenuname *menu;
    FmXmlFileTag tag = 0;

    if (children == NULL ||
        fm_xml_file_item_get_tag(children->data) != FM_XML_FILE_TEXT ||
        (name = fm_xml_file_item_get_data(children->data, NULL)) == NULL)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Empty <Menuname> tag"));
        return FALSE;
    }
    parent = fm_xml_file_item_get_parent(item);
    if (parent)
        tag = fm_xml_file_item_get_tag(parent);
    if (tag != menuTag_Layout && tag != menuTag_DefaultLayout)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Tag <Menuname> may only appear below <Layout> or"
                              " <DefaultLayout>"));
        return FALSE;
    }
    layout = _find_layout(parent, TRUE);
    menu = g_slice_new0(MenuMenuname);
    menu->layout.type = MENU_CACHE_TYPE_DIR;
    menu->name = g_strdup(name);
    if (attribute_names) while (attribute_names[0])
    {
        if (strcmp(attribute_names[0], "show_empty") == 0)
        {
            menu->layout.show_empty = (g_ascii_strcasecmp(attribute_values[0],
                                                          "true") == 0);
            menu->layout.only_unallocated = TRUE;
        }
        else if (strcmp(attribute_names[0], "inline") == 0)
        {
            menu->layout.allow_inline = (g_ascii_strcasecmp(attribute_values[0],
                                                            "true") == 0);
            menu->layout.is_set = TRUE;
        }
        else if (strcmp(attribute_names[0], "inline_header") == 0)
        {
            menu->layout.inline_header = (g_ascii_strcasecmp(attribute_values[0],
                                                             "true") == 0);
            menu->layout.inline_header_is_set = TRUE;
        }
        else if (strcmp(attribute_names[0], "inline_alias") == 0)
        {
            menu->layout.inline_alias = (g_ascii_strcasecmp(attribute_values[0],
                                                            "true") == 0);
            menu->layout.inline_alias_is_set = TRUE;
        }
        else if (strcmp(attribute_names[0], "inline_limit") == 0)
        {
            menu->layout.inline_limit = atoi(attribute_values[0]);
            menu->layout.inline_limit_is_set = TRUE;
        }
        attribute_names++;
        attribute_values++;
    }
    layout->items = g_list_append(layout->items, menu);
    return TRUE;
}

static gboolean _menu_xml_handler_Separator(FmXmlFileItem *item, GList *children,
                                            char * const *attribute_names,
                                            char * const *attribute_values,
                                            guint n_attributes, gint line, gint pos,
                                            GError **error, gpointer user_data)
{
    FmXmlFileItem *parent;
    MenuLayout *layout;
    MenuSep *sep;
    FmXmlFileTag tag = 0;

    parent = fm_xml_file_item_get_parent(item);
    if (parent)
        tag = fm_xml_file_item_get_tag(parent);
    if (tag != menuTag_Layout && tag != menuTag_DefaultLayout)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Tag <Separator> may only appear below <Layout> or"
                              " <DefaultLayout>"));
        return FALSE;
    }
    layout = _find_layout(parent, TRUE);
    sep = g_slice_new0(MenuSep);
    sep->type = MENU_CACHE_TYPE_SEP;
    layout->items = g_list_append(layout->items, sep);
    return TRUE;
}

static gboolean _menu_xml_handler_Merge(FmXmlFileItem *item, GList *children,
                                        char * const *attribute_names,
                                        char * const *attribute_values,
                                        guint n_attributes, gint line, gint pos,
                                        GError **error, gpointer user_data)
{
    FmXmlFileItem *parent;
    MenuLayout *layout;
    MenuMerge *mm;
    FmXmlFileTag tag = 0;
    MenuMergeType type = MERGE_NONE;

    parent = fm_xml_file_item_get_parent(item);
    if (parent)
        tag = fm_xml_file_item_get_tag(parent);
    if (tag != menuTag_Layout && tag != menuTag_DefaultLayout)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Tag <Merge> may only appear below <Layout> or"
                              " <DefaultLayout>"));
        return FALSE;
    }
    if (attribute_names) while (attribute_names[0])
    {
        if (strcmp(attribute_names[0], "type") == 0)
        {
            if (strcmp(attribute_values[0], "menus") == 0)
                type = MERGE_MENUS;
            else if (strcmp(attribute_values[0], "files") == 0)
                type = MERGE_FILES;
            else if (strcmp(attribute_values[0], "all") == 0)
                type = MERGE_ALL;
            break;
        }
        attribute_names++;
        attribute_values++;
    }
    if (type == MERGE_NONE)
    {
        RETURN_TRUE_AND_DESTROY_IF_QUIET(item);
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                            _("Tag <Merge> should have attribute 'type' as"
                              " \"menus\", \"files\", or \"all\""));
        return FALSE;
    }
    layout = _find_layout(parent, TRUE);
    mm = g_slice_new0(MenuMerge);
    mm->type = MENU_CACHE_TYPE_NONE;
    mm->merge_type = type;
    layout->items = g_list_append(layout->items, mm);
    return TRUE;
}

static gboolean _menu_xml_handler_Layout(FmXmlFileItem *item, GList *children,
                                         char * const *attribute_names,
                                         char * const *attribute_values,
                                         guint n_attributes, gint line, gint pos,
                                         GError **error, gpointer user_data)
{
    /* ignore empty layout */
    return TRUE;
}

static gboolean _menu_xml_handler_DefaultLayout(FmXmlFileItem *item, GList *children,
                                                char * const *attribute_names,
                                                char * const *attribute_values,
                                                guint n_attributes, gint line, gint pos,
                                                GError **error, gpointer user_data)
{
    MenuLayout *layout;

    layout = _find_layout(item, TRUE);
    if (attribute_names) while (attribute_names[0])
    {
        if (strcmp(attribute_names[0], "show_empty") == 0)
            layout->show_empty = (g_ascii_strcasecmp(attribute_values[0],
                                                     "true") == 0);
        else if (strcmp(attribute_names[0], "inline") == 0)
            layout->allow_inline = (g_ascii_strcasecmp(attribute_values[0],
                                                       "true") == 0);
        else if (strcmp(attribute_names[0], "inline_header") == 0)
            layout->inline_header = (g_ascii_strcasecmp(attribute_values[0],
                                                        "true") == 0);
        else if (strcmp(attribute_names[0], "inline_alias") == 0)
            layout->inline_alias = (g_ascii_strcasecmp(attribute_values[0],
                                                       "true") == 0);
        else if (strcmp(attribute_names[0], "inline_limit") == 0)
            layout->inline_limit = atoi(attribute_values[0]);
        attribute_names++;
        attribute_values++;
    }
    return TRUE;
}

static gboolean _merge_xml_file(MenuTreeData *data, FmXmlFileItem *item,
                                const char *path, GList **m, GError **error,
                                gboolean add_to_list)
{
    FmXmlFile *menu = NULL;
    GList *xml = NULL, *it; /* loaded list */
    GFile *gf;
    GError *err = NULL;
    const char *save_path;
    char *contents;
    gsize len;
    gboolean ok;

    /* check for loops! */
    path = g_intern_string(path);
    it = *m;
    if (g_list_find(it, path) != NULL)
    {
        g_critical("merging loop detected for file '%s'", path);
        return TRUE;
    }
    *m = g_list_prepend(it, (gpointer)path);
    if (add_to_list && g_slist_find(MenuFiles, path) == NULL)
        MenuFiles = g_slist_append(MenuFiles, (gpointer)path);
    save_path = data->file_path;
    data->file_path = path;
    DBG("merging the XML file '%s'", data->file_path);
    gf = g_file_new_for_path(data->file_path);
    ok = g_file_load_contents(gf, NULL, &contents, &len, NULL, error);
    g_object_unref(gf);
    if (!ok)
    {
        /* replace the path with failed one */
        return FALSE;
    }
    menu = fm_xml_file_new(data->menu);
    /* g_debug("merging FmXmlFile %p into %p", menu, data->menu); */
    ok = fm_xml_file_parse_data(menu, contents, len, error, data);
    g_free(contents);
    if (ok)
    {
        xml = fm_xml_file_finish_parse(menu, &err);
        if (err && err->domain == G_MARKUP_ERROR &&
            err->code == G_MARKUP_ERROR_EMPTY)
        {
            /* NOTE: it should be legal case to have empty menu file.
               it may be not generally this but let it be */
            g_error_free(err);
            data->file_path = save_path;
            g_object_unref(menu);
            return TRUE;
        }
        if (err)
            g_propagate_error(error, err);
    }
    if (xml == NULL) /* error is set by failed function */
    {
        /* g_debug("freeing FmXmlFile %p (failed)", menu); */
        /* only this handler does recursion, therefore it is safe to set and
           and do check of data->line here */
        if (data->line == -1)
            data->line = fm_xml_file_get_current_line(menu, &data->pos);
        /* we do a little trick here - we don't restore previous fule but
           leave data->file_path for diagnostics in _update_categories() */
        g_object_unref(menu);
        return FALSE;
    }
    data->file_path = save_path;
    /* insert all children but Name before item */
    for (it = xml; it; it = it->next)
    {
        GList *xml_sub, *it_sub;

        if (fm_xml_file_item_get_tag(it->data) != menuTag_Menu)
        {
            if (verbose == 0)
                continue; /* just skip it in quiet mode */
            g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                        _("Merging file may contain only <Menu> top level tag,"
                          " got <%s>"), fm_xml_file_item_get_tag_name(it->data));
            /* FIXME: it will show error not for merged file but current */
            break;
        }
        xml_sub = fm_xml_file_item_get_children(it->data);
        for (it_sub = xml_sub; it_sub; it_sub = it_sub->next)
        {
            /* g_debug("merge: trying to insert %p into %p", it_sub->data,
                    fm_xml_file_item_get_parent(item)); */
            if (fm_xml_file_item_get_tag(it_sub->data) != menuTag_Name &&
                !fm_xml_file_insert_before(item, it_sub->data))
            {
                g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                            _("Failed to insert tag <%s> from merging file"),
                            fm_xml_file_item_get_tag_name(it_sub->data));
                /* FIXME: it will show error not for merged file but current */
                break;
            }
        }
        g_list_free(xml_sub);
        if (it_sub) /* failed above */
            break;
    }
    g_list_free(xml);
    ok = (it == NULL);
    /* g_debug("freeing FmXmlFile %p (success=%d)", menu, (int)ok); */
    g_object_unref(menu);
    return ok;
}

static gboolean _merge_menu_directory(MenuTreeData *data, FmXmlFileItem *item,
                                      const char *path, GList **m, GError **error,
                                      gboolean ignore_not_exist)
{
    char *child;
    GDir *dir;
    GError *err = NULL;
    const char *name;
    gboolean ok = TRUE;

    DBG("merging the XML directory '%s'", path);
    path = g_intern_string(path);
    if (g_slist_find(MenuDirs, path) == NULL)
        MenuDirs = g_slist_append(MenuDirs, (gpointer)path);
    dir = g_dir_open(path, 0, &err);
    if (dir)
    {
        while ((name = g_dir_read_name(dir)))
        {
            if (strlen(name) <= 5 || !g_str_has_suffix(name, ".menu"))
            {
                /* skip files that aren't *.menu */
                continue;
            }
            child = g_build_filename(path, name, NULL);
            ok = _merge_xml_file(data, item, child, m, &err, FALSE);
            if (!ok)
            {
                /*
                if (err->domain == G_IO_ERROR && err->code == G_IO_ERROR_PERMISSION_DENIED)
                {
                    g_warning("cannot merge XML file %s: no access", child);
                    g_clear_error(&err);
                    g_free(child);
                    ok = TRUE;
                    continue;
                }
                */
                g_free(child);
                if (ignore_not_exist && err->domain == G_IO_ERROR &&
                    (err->code == G_IO_ERROR_NOT_FOUND))
                {
                    g_clear_error(&err);
                    continue;
                }
                g_propagate_error(error, err);
                err = NULL;
                break;
            }
            g_free(child);
        }
        g_dir_close(dir);
    }
    else if (ignore_not_exist && err->domain == G_FILE_ERROR &&
             (err->code == G_FILE_ERROR_NOENT))
    {
        VDBG("_merge_menu_directory: dir %s does not exist", path);
        g_error_free(err);
    }
    else
    {
        g_propagate_error(error, err);
        ok = FALSE;
    }
    return ok;
}

static inline const char *_get_menu_name(FmXmlFileItem *item)
{
    if (fm_xml_file_item_get_tag(item) != menuTag_Menu) /* skip not menu */
        return NULL;
    item = fm_xml_file_item_find_child(item, menuTag_Name);
    if (item == NULL) /* no Name tag? */
        return NULL;
    item = fm_xml_file_item_find_child(item, FM_XML_FILE_TEXT);
    if (item == NULL) /* empty Name tag? */
        return NULL;
    return fm_xml_file_item_get_data(item, NULL);
}

/* merges subitems - consumes list */
/* NOTE: it will not delete duplicate elements other than Menu or Name */
static void _merge_level(GList *first)
{
    while (first)
    {
        if (first->data) /* we might merge this one already */
        {
            GList *next;

            if (first->next)
            {
                /* merge this item with identical ones */
                const char *name = _get_menu_name(first->data);

                if (name) /* not a menu tag */
                {
                    for (next = first->next; next; next = next->next)
                    {
                        if (next->data == NULL) /* already merged */
                            continue;
                        if (g_strcmp0(name, _get_menu_name(next->data)) == 0)
                        {
                            GList *children = fm_xml_file_item_get_children(next->data);
                            GList *l;

                            DBG("found two identical Menu '%s', merge them", name);
                            for (l = children; l; l = l->next) /* merge all but Name */
                                if (fm_xml_file_item_get_tag(l->data) != menuTag_Name)
                                    fm_xml_file_item_append_child(first->data, l->data);
                            g_list_free(children);
                            fm_xml_file_item_destroy(next->data);
                            next->data = NULL; /* we merged it so no data */
                        }
                    }
                }
            }
        }
        /* go to next item */
        first = g_list_delete_link(first, first);
    }
}

static FmXmlFileItem *_walk_path(GList *child, const char *path,
                                 FmXmlFileItem *parent, gboolean create)
{
    FmXmlFileItem *item = NULL;
    char *subpath = strchr(path, '/');

    if (subpath)
    {
        char *next = &subpath[1];
        subpath = g_strndup(path, subpath - path);
        path = next;
    }
    for (; child != NULL; child = child->next)
    {
        item = child->data;
        if (item == NULL)
            continue;
        if (g_strcmp0(subpath ? subpath : path, _get_menu_name(item)) == 0)
            break;
        item = NULL;
    }
    g_free(subpath); /* free but still use as marker */
    if (subpath != NULL && item != NULL)
    {
        child = fm_xml_file_item_get_children(item);
        item = _walk_path(child, path, item, create);
        g_list_free(child);
    }
    else if (subpath == NULL && item == NULL && create)
    {
        /* create new <Menu><Name>path</Name></Menu> and append it to parent */
        item = fm_xml_file_item_new(menuTag_Menu);
        if (!fm_xml_file_item_append_child(parent, item))
            fm_xml_file_item_destroy(item); /* FIXME: is it possible? */
        else
        {
            parent = fm_xml_file_item_new(menuTag_Name); /* reuse pointer */
            fm_xml_file_item_append_text(parent, path, -1, FALSE);
            fm_xml_file_item_append_child(item, parent);
        }
    }
    return item;
}

static FmXmlFileItem *_walk_children(GList *children, FmXmlFileItem *list,
                                     FmXmlFileTag tag, gboolean create)
{
    GList *sub, *l;
    FmXmlFileItem *item = NULL;

    sub = fm_xml_file_item_get_children(list);
    for (l = sub; l; l = l->next)
        if (fm_xml_file_item_get_tag((item = l->data)) == tag)
            break;
        else
            item = NULL;
    g_list_free(sub);
    if (item == NULL) /* no tag found */
        return NULL;
    item = fm_xml_file_item_find_child(item, FM_XML_FILE_TEXT);
    list = fm_xml_file_item_get_parent(list); /* it contains parent of <Move> now */
    if (item == NULL) /* empty tag, assume we are here */
        return list;
    return _walk_path(children, fm_xml_file_item_get_data(item, NULL), list, create);
}

static gboolean _activate_merges(MenuTreeData *data, FmXmlFileItem *item,
                                 GError **error)
{
    GList *children, *l, *l2, *merged = NULL;
    const char *path, *path2;
    FmXmlFileItem *sub;
    FmXmlFileTag tag;
    gboolean ok;

restart:
    children = fm_xml_file_item_get_children(item);
    /* expand DefaultMergeDirs first */
    for (l = children, sub = NULL; l; l = l->next)
    {
        if (fm_xml_file_item_get_tag(l->data) == menuTag_DefaultMergeDirs)
            sub = l->data;
    }
    if (sub != NULL)
    {
        const gchar * const *dirs = g_get_system_config_dirs();
        char *merged;
        FmXmlFileItem *it_sub;
        int i = g_strv_length((char **)dirs);

        /* insert in reverse order - see XDG menu specification */
        while (i > 0)
        {
            merged = g_build_filename(dirs[--i], "menus", "applications-merged", NULL);
            it_sub = fm_xml_file_item_new(menuTag_MergeDir);
            fm_xml_file_item_append_text(it_sub, merged, -1, FALSE);
            if (!fm_xml_file_insert_before(sub, it_sub) && verbose > 0)
            {
                g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                            _("Failed to insert tag <MergeDir>%s</MergeDir>"),
                            merged);
                g_free(merged);
                goto failed; /* failed to merge */
            }
            g_free(merged);
        }
        merged = g_build_filename(g_get_user_config_dir(), "menus", "applications-merged", NULL);
        it_sub = fm_xml_file_item_new(menuTag_MergeDir);
        fm_xml_file_item_append_text(it_sub, merged, -1, FALSE);
        if (!fm_xml_file_insert_before(sub, it_sub) && verbose > 0)
        {
            g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                        _("Failed to insert tag <MergeDir>%s</MergeDir>"),
                        merged);
            g_free(merged);
            goto failed; /* failed to merge */
        }
        g_free(merged);
        /* destroy all DefaultMergeDirs -- we replaced it already */
        for (l = children; l; l = l->next)
            if (fm_xml_file_item_get_tag(l->data) == menuTag_DefaultMergeDirs)
                fm_xml_file_item_destroy(l->data);
        /* restart merge again, we changed the list */
        g_list_free(children);
        goto restart;
    }
    /* do with MergeFile and MergeDir now */
    for (l = children; l; l = l->next)
    {
        tag = fm_xml_file_item_get_tag(l->data);
        if (tag == menuTag_MergeFile || tag == menuTag_MergeDir)
        {
            path = fm_xml_file_item_get_data(fm_xml_file_item_find_child(l->data,
                                                        FM_XML_FILE_TEXT), NULL);
            /* find duplicate - only last one should be used */
            for (l2 = l->next; l2; l2 = l2->next)
            {
                if (fm_xml_file_item_get_tag(l2->data) == tag)
                {
                    path2 = fm_xml_file_item_get_data(fm_xml_file_item_find_child(l2->data,
                                                        FM_XML_FILE_TEXT), NULL);
                    if (strcmp(path2, path) == 0)
                        break;
                }
            }
            if (l2 == NULL)
            {
                if (tag == menuTag_MergeFile)
                {
                    GError *err = NULL;
                    ok = _merge_xml_file(data, l->data, path, &merged, &err, TRUE);
                    if (ok) ;
                    else if (err->domain == G_IO_ERROR && err->code == G_IO_ERROR_NOT_FOUND)
                    {
                        g_error_free(err);
                        ok = TRUE;
                    }
                    else
                        g_propagate_error(error, err);
                }
                else
                    ok = _merge_menu_directory(data, l->data, path, &merged, error, TRUE);
                if (!ok)
                {
                    if (verbose > 0)
                    {
                        g_prefix_error(error, "failed on '%s': ", path);
                        goto failed; /* failed to merge */
                    }
                    g_clear_error(error);
                }
            }
            /* destroy item -- we replaced it already */
            fm_xml_file_item_destroy(l->data);
            if (l2 != NULL) /* it was a duplicate */
                continue;
            /* restart merge again, we could get new merges */
            g_list_free(children);
            goto restart;
        }
    }
    g_list_free(merged); /* we don't need it anymore */
    /* merge this level */
    _merge_level(children);
    children = fm_xml_file_item_get_children(item);
    /* expand DefaultAppDirs then supress duplicates on AppDir */
    for (l = children, sub = NULL; l; l = l->next)
    {
        if (fm_xml_file_item_get_tag(l->data) == menuTag_DefaultAppDirs)
            sub = l->data;
    }
    if (sub != NULL)
    {
        const gchar * const *dirs = g_get_system_data_dirs();
        char *merged;
        FmXmlFileItem *it_sub;
        int i = g_strv_length((char **)dirs);

        /* insert in reverse order - see XDG menu specification */
        while (i > 0)
        {
            merged = g_build_filename(dirs[--i], "applications", NULL);
            it_sub = fm_xml_file_item_new(menuTag_AppDir);
            fm_xml_file_item_append_text(it_sub, merged, -1, FALSE);
            if (!fm_xml_file_insert_before(sub, it_sub) && verbose > 0)
            {
                g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                            _("Failed to insert tag <AppDir>%s</AppDir>"),
                            merged);
                g_free(merged);
                goto failed; /* failed to merge */
            }
            g_free(merged);
        }
        merged = g_build_filename(g_get_user_data_dir(), "applications", NULL);
        it_sub = fm_xml_file_item_new(menuTag_AppDir);
        fm_xml_file_item_append_text(it_sub, merged, -1, FALSE);
        if (!fm_xml_file_insert_before(sub, it_sub) && verbose > 0)
        {
            g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                        _("Failed to insert tag <AppDir>%s</AppDir>"),
                        merged);
            g_free(merged);
            goto failed; /* failed to merge */
        }
        g_free(merged);
        /* destroy all DefaultAppDirs -- we replaced it already */
        for (l = children; l; l = l->next)
            if (fm_xml_file_item_get_tag(l->data) == menuTag_DefaultAppDirs)
            {
                fm_xml_file_item_destroy(l->data);
                l->data = NULL;
            }
    }
    for (l = children; l; l = l->next)
    {
        sub = l->data;
        if (sub == NULL || fm_xml_file_item_get_tag(sub) != menuTag_AppDir)
            continue;
        for (l2 = l->next; l2; l2 = l2->next)
            if (l2->data != NULL && fm_xml_file_item_get_tag(l2->data) == menuTag_AppDir)
                if (strcmp(fm_xml_file_item_get_data(fm_xml_file_item_find_child(l2->data, FM_XML_FILE_TEXT), NULL),
                           fm_xml_file_item_get_data(fm_xml_file_item_find_child(l->data, FM_XML_FILE_TEXT), NULL)) == 0)
                    break;
        if (l2 == NULL) /* no duplicates */
            continue;
        fm_xml_file_item_destroy(sub);
        l->data = NULL;
    }
    /* expand KDELegacyDirs then supress duplicates on LegacyDir */
    for (l = children, sub = NULL; l; l = l->next)
    {
        if (l->data == NULL)
            continue;
        if (fm_xml_file_item_get_tag(l->data) == menuTag_KDELegacyDirs)
            sub = l->data;
    }
    if (sub != NULL)
    {
        const gchar * const *dirs = g_get_system_data_dirs();
        char *merged;
        FmXmlFileItem *it_sub;
        int i = g_strv_length((char **)dirs);

        /* insert in reverse order - see XDG menu specification */
        while (i > 0)
        {
            merged = g_build_filename(dirs[--i], "applnk", NULL);
            it_sub = fm_xml_file_item_new(menuTag_LegacyDir);
            fm_xml_file_item_set_comment(it_sub, "kde-");
            fm_xml_file_item_append_text(it_sub, merged, -1, FALSE);
            if (!fm_xml_file_insert_before(sub, it_sub) && verbose > 0)
            {
                g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                            _("Failed to insert tag <LegacyDir>%s</LegacyDir>"),
                            merged);
                g_free(merged);
                goto failed; /* failed to merge */
            }
            g_free(merged);
        }
        merged = g_build_filename(g_get_user_data_dir(), "applnk", NULL);
        it_sub = fm_xml_file_item_new(menuTag_LegacyDir);
        fm_xml_file_item_set_comment(it_sub, "kde-");
        fm_xml_file_item_append_text(it_sub, merged, -1, FALSE);
        if (!fm_xml_file_insert_before(sub, it_sub) && verbose > 0)
        {
            g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                        _("Failed to insert tag <LegacyDir>%s</LegacyDir>"),
                        merged);
            g_free(merged);
            goto failed; /* failed to merge */
        }
        g_free(merged);
        /* destroy all KDELegacyDirs */
        for (l = children; l; l = l->next)
            if (l->data && fm_xml_file_item_get_tag(l->data) == menuTag_KDELegacyDirs)
            {
                fm_xml_file_item_destroy(l->data);
                l->data = NULL;
            }
    }
    for (l = children; l; l = l->next)
    {
        sub = l->data;
        if (sub == NULL || fm_xml_file_item_get_tag(sub) != menuTag_LegacyDir)
            continue;
        VDBG("check LegacyDir %s", fm_xml_file_item_get_data(fm_xml_file_item_find_child(l->data, FM_XML_FILE_TEXT), NULL));
        for (l2 = l->next; l2; l2 = l2->next)
            if (l2->data != NULL && fm_xml_file_item_get_tag(l2->data) == menuTag_LegacyDir)
                if (strcmp(fm_xml_file_item_get_data(fm_xml_file_item_find_child(l2->data, FM_XML_FILE_TEXT), NULL),
                           fm_xml_file_item_get_data(fm_xml_file_item_find_child(l->data, FM_XML_FILE_TEXT), NULL)) == 0)
                    break;
        if (l2 == NULL) /* no duplicates */
            continue;
        fm_xml_file_item_destroy(sub);
        l->data = NULL;
    }
    /* expand DefaultDirectoryDirs then supress duplicates on DirectoryDir */
    for (l = children, sub = NULL; l; l = l->next)
    {
        if (l->data == NULL)
            continue;
        if (fm_xml_file_item_get_tag(l->data) == menuTag_DefaultDirectoryDirs)
            sub = l->data;
    }
    if (sub != NULL)
    {
        const gchar * const *dirs = g_get_system_data_dirs();
        char *merged;
        FmXmlFileItem *it_sub;
        int i = g_strv_length((char **)dirs);

        /* insert in reverse order - see XDG menu specification */
        while (i > 0)
        {
            merged = g_build_filename(dirs[--i], "desktop-directories", NULL);
            it_sub = fm_xml_file_item_new(menuTag_DirectoryDir);
            fm_xml_file_item_append_text(it_sub, merged, -1, FALSE);
            if (!fm_xml_file_insert_before(sub, it_sub) && verbose > 0)
            {
                g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                            _("Failed to insert tag <DirectoryDir>%s</DirectoryDir>"),
                            merged);
                g_free(merged);
                goto failed; /* failed to merge */
            }
            g_free(merged);
        }
        merged = g_build_filename(g_get_user_data_dir(), "desktop-directories", NULL);
        it_sub = fm_xml_file_item_new(menuTag_DirectoryDir);
        fm_xml_file_item_append_text(it_sub, merged, -1, FALSE);
        if (!fm_xml_file_insert_before(sub, it_sub) && verbose > 0)
        {
            g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                        _("Failed to insert tag <DirectoryDir>%s</DirectoryDir>"),
                        merged);
            g_free(merged);
            goto failed; /* failed to merge */
        }
        g_free(merged);
        /* destroy all DefaultDirectoryDirs -- we replaced it already */
        for (l = children; l; l = l->next)
            if (l->data && fm_xml_file_item_get_tag(l->data) == menuTag_DefaultDirectoryDirs)
            {
                fm_xml_file_item_destroy(l->data);
                l->data = NULL;
            }
    }
    for (l = children; l; l = l->next)
    {
        sub = l->data;
        if (sub == NULL || fm_xml_file_item_get_tag(sub) != menuTag_DirectoryDir)
            continue;
        for (l2 = l->next; l2; l2 = l2->next)
            if (l2->data != NULL && fm_xml_file_item_get_tag(l2->data) == menuTag_DirectoryDir)
                if (strcmp(fm_xml_file_item_get_data(fm_xml_file_item_find_child(l2->data, FM_XML_FILE_TEXT), NULL),
                           fm_xml_file_item_get_data(fm_xml_file_item_find_child(l->data, FM_XML_FILE_TEXT), NULL)) == 0)
                    break;
        if (l2 == NULL) /* no duplicates */
            continue;
        fm_xml_file_item_destroy(sub);
        l->data = NULL;
    }
    /* support <Move><New>...</New><Old>...</Old></Move> for menus */
    for (l = children; l; l = l2)
    {
        l2 = l->next;
        sub = l->data;
        if (sub && fm_xml_file_item_get_tag(sub) == menuTag_Move)
        {
            FmXmlFileItem *old = _walk_children(children, sub, menuTag_Old, FALSE);
            sub = _walk_children(children, sub, menuTag_New, TRUE);
            if (old != NULL && sub != NULL)
            {
                GList *child = fm_xml_file_item_get_children(old);

                while (child != NULL)
                {
                    if (fm_xml_file_item_get_tag(child->data) != menuTag_Name)
                        fm_xml_file_item_append_child(sub, child->data);
                    child = g_list_delete_link(child, child);
                }
                fm_xml_file_item_destroy(old);
            }
            else
                DBG("invalid <Move> tag ignored");
        }
    }
    /* reload children, they might be changed after movements */
    g_list_free(children);
    children = fm_xml_file_item_get_children(item);
    /* do recursion for children Menu now */
    for (l = children; l; l = l->next)
    {
        sub = l->data;
        if (fm_xml_file_item_get_tag(sub) == menuTag_Menu &&
            !_activate_merges(data, sub, error))
            goto failed; /* failed to merge */
    }
    g_list_free(children);
    return TRUE;

failed:
    g_list_free(children);
    return FALSE;
}

static GList *_layout_items_copy(GList *orig)
{
    GList *copy = NULL;
    gpointer item;

    while (orig)
    {
        MenuSep *sep = orig->data;

        switch (sep->type) {
        case MENU_CACHE_TYPE_NONE:
            item = g_slice_new(MenuMerge);
            memcpy(item, sep, sizeof(MenuMerge));
            VVDBG("*** new menu layout: MenuMerge");
            break;
        case MENU_CACHE_TYPE_SEP:
            item = g_slice_new(MenuSep);
            memcpy(item, sep, sizeof(MenuSep));
            VVDBG("*** new menu layout: MenuSeparator");
            break;
        case MENU_CACHE_TYPE_APP:
            item = g_slice_new(MenuFilename);
            memcpy(item, sep, sizeof(MenuFilename));
            ((MenuFilename *)item)->id = g_strdup(((MenuFilename *)sep)->id);
            VVDBG("*** new menu layout: MenuFilename %s", ((MenuFilename *)item)->id);
            break;
        case MENU_CACHE_TYPE_DIR:
            item = g_slice_new(MenuMenuname);
            memcpy(item, sep, sizeof(MenuMenuname));
            ((MenuMenuname *)item)->name = g_strdup(((MenuMenuname *)sep)->name);
            VVDBG("*** new menu layout: MenuMenuname %s", ((MenuMenuname *)item)->name);
        }
        copy = g_list_prepend(copy, item);
        orig = orig->next;
    }
    return g_list_reverse(copy);
}

static MenuMenu *_make_menu_node(FmXmlFileItem *node, MenuLayout *def)
{
    FmXmlFileItem *item = NULL;
    MenuLayout *layout = NULL;
    GList *children, *l;
    MenuMenu *menu;
    FmXmlFileTag tag;
    gboolean ok = TRUE;

    if (fm_xml_file_item_find_child(node, menuTag_Name) == NULL)
    {
        g_warning("got a <Menu> without <Name>, ignored");
        return NULL;
    }
    children = fm_xml_file_item_get_children(node);
    /* check if it's deleted first */
    for (l = children; l; l = l->next)
    {
        tag = fm_xml_file_item_get_tag(l->data);
        if (tag == menuTag_Layout)
            item = l->data;
        else if (tag == menuTag_DefaultLayout)
            layout = _find_layout(l->data, FALSE);
        else if (tag == menuTag_Deleted)
            ok = FALSE;
        else if (tag == menuTag_NotDeleted)
            ok = TRUE;
    }
    if (!ok)
    {
        /* menu was disabled, ignore it */
        g_list_free(children);
        return NULL;
    }
    /* find default layout if any and subst */
    if (layout != NULL)
    {
        /* new DefaultLayout might be empty, ignore it then */
        if (layout->items == NULL)
            layout = NULL;
        else
            def = layout;
    }
    /* find layout, if not found then fill from default */
    if (item != NULL)
        layout = _find_layout(item, FALSE);
    if (layout == NULL)
        layout = def;
    menu = g_slice_new0(MenuMenu);
    menu->layout.type = MENU_CACHE_TYPE_DIR;
    menu->layout.show_empty = layout->show_empty;
    menu->layout.allow_inline = layout->allow_inline;
    menu->layout.inline_header = layout->inline_header;
    menu->layout.inline_alias = layout->inline_alias;
    menu->layout.inline_limit = layout->inline_limit;
    menu->layout.items = _layout_items_copy(layout->items);
    VDBG("*** starting new menu");
    /* gather all explicit data from XML */
    for (l = children; l; l = l->next)
    {
        tag = fm_xml_file_item_get_tag(l->data);
        /* we don't do any matching now, i.e. those tags will be processed later
           * Include Exclude Filename Category All And Not Or
           directory scannings will be processed later as well:
           * AppDir LegacyDir KDELegacyDirs */
        if (tag == menuTag_Menu)
        {
            MenuMenu *child = _make_menu_node(l->data, def);
            if (child != NULL)
            {
                VDBG("*** added submenu %s", child->name);
                menu->children = g_list_prepend(menu->children, child);
            }
        }
        else if (tag == menuTag_Directory)
        {
            item = fm_xml_file_item_find_child(l->data, FM_XML_FILE_TEXT);
            if (item != NULL)
                menu->id = g_list_prepend(menu->id,
                                g_strdup(fm_xml_file_item_get_data(item, NULL)));
        }
        else if (tag == menuTag_Name)
        {
            if (menu->name == NULL)
                menu->name = g_strdup(fm_xml_file_item_get_data(fm_xml_file_item_find_child(l->data,
                                                        FM_XML_FILE_TEXT), NULL));
        }
        else if (tag == menuTag_OnlyUnallocated)
            menu->layout.only_unallocated = TRUE;
        else if (tag == menuTag_NotOnlyUnallocated)
            menu->layout.only_unallocated = FALSE;
        else if (tag == menuTag_Include || tag == menuTag_Exclude ||
                 tag == menuTag_DirectoryDir || tag == menuTag_AppDir ||
                 tag == menuTag_LegacyDir)
                 /* FIXME: can those be here? Filename Category All And Not Or */
        {
            MenuRule *child = g_slice_new0(MenuRule);
            child->type = MENU_CACHE_TYPE_NONE;
            child->rule = l->data;
            VDBG("*** adding rule %s", fm_xml_file_item_get_tag_name(l->data));
            menu->children = g_list_prepend(menu->children, child);
        }
    }
    g_list_free(children);
    menu->children = g_list_reverse(menu->children);
    VDBG("*** done menu %s",menu->name);
    return menu;
}

static FmXmlFileItem *_find_in_children(GList *list, const char *name)
{
    while (list)
    {
        const char *elem_name = _get_menu_name(list->data);
        /* g_debug("got child %d: %s", fm_xml_file_item_get_tag(list->data), elem_name); */
        if (g_strcmp0(elem_name, name) == 0)
            return list->data;
        else
            list = list->next;
    }
    return NULL;
}

void _free_layout_items(GList *data)
{
    union {
        MenuMenuname *menu;
        MenuFilename *file;
        MenuSep *sep;
        MenuMerge *merge;
    } a = { NULL };

    while (data != NULL)
    {
        a.menu = data->data;
        switch (a.menu->layout.type) {
        case MENU_CACHE_TYPE_DIR:
            g_free(a.menu->name);
            g_slice_free(MenuMenuname, a.menu);
            break;
        case MENU_CACHE_TYPE_APP:
            g_free(a.file->id);
            g_slice_free(MenuFilename, a.file);
            break;
        case MENU_CACHE_TYPE_SEP:
            g_slice_free(MenuSep, a.sep);
            break;
        case MENU_CACHE_TYPE_NONE:
            g_slice_free(MenuMerge, a.merge);
        }
        data = g_list_delete_link(data, data);
    }
}

static void _free_layout(gpointer data)
{
    MenuLayout *layout = data;

    _free_layout_items(layout->items);
    g_slice_free(MenuLayout, data);
}

MenuMenu *get_merged_menu(const char *file, FmXmlFile **xmlfile, GError **error)
{
    GFile *gf;
    char *contents;
    gsize len;
    MenuTreeData data;
    GList *xml = NULL;
    FmXmlFileItem *apps;
    MenuMenu *menu = NULL;
    MenuLayout default_layout;
    MenuMerge def_files = { .type = MENU_CACHE_TYPE_NONE, .merge_type = MERGE_FILES };
    MenuMerge def_menus = { .type = MENU_CACHE_TYPE_NONE, .merge_type = MERGE_MENUS };
    gboolean ok;

    /* Load the file */
    data.file_path = file;
    gf = g_file_new_for_path(file);
    contents = NULL;
    ok = g_file_load_contents(gf, NULL, &contents, &len, NULL, error);
    g_object_unref(gf);
    if (!ok)
        return NULL;
    /* Init layouts hash and all the data */
    layout_hash = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL,
                                        _free_layout);
    data.menu = fm_xml_file_new(NULL);
    data.line = data.pos = -1;
    /* g_debug("new FmXmlFile %p", data.menu); */
    menuTag_Menu = fm_xml_file_set_handler(data.menu, "Menu",
                                           &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Include = fm_xml_file_set_handler(data.menu, "Include",
                                              &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Exclude = fm_xml_file_set_handler(data.menu, "Exclude",
                                              &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Filename = fm_xml_file_set_handler(data.menu, "Filename",
                                               &_menu_xml_handler_Filename, FALSE, NULL);
    menuTag_Or = fm_xml_file_set_handler(data.menu, "Or",
                                         &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_And = fm_xml_file_set_handler(data.menu, "And",
                                          &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Not = fm_xml_file_set_handler(data.menu, "Not",
                                          &_menu_xml_handler_Not, FALSE, NULL);
    menuTag_Category = fm_xml_file_set_handler(data.menu, "Category",
                                               &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_MergeFile = fm_xml_file_set_handler(data.menu, "MergeFile",
                                                &_menu_xml_handler_MergeFile, FALSE, NULL);
    menuTag_MergeDir = fm_xml_file_set_handler(data.menu, "MergeDir",
                                               &_menu_xml_handler_MergeDir, FALSE, NULL);
    menuTag_DefaultMergeDirs = fm_xml_file_set_handler(data.menu, "DefaultMergeDirs",
                                                       &_menu_xml_handler_DefaultMergeDirs, FALSE, NULL);
    menuTag_KDELegacyDirs = fm_xml_file_set_handler(data.menu, "KDELegacyDirs",
                                                    &_menu_xml_handler_DefaultMergeDirs, FALSE, NULL);
    menuTag_Name = fm_xml_file_set_handler(data.menu, "Name",
                                           &_menu_xml_handler_Name, FALSE, NULL);
    menuTag_Deleted = fm_xml_file_set_handler(data.menu, "Deleted",
                                              &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_NotDeleted = fm_xml_file_set_handler(data.menu, "NotDeleted",
                                                 &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Directory = fm_xml_file_set_handler(data.menu, "Directory",
                                                &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_AppDir = fm_xml_file_set_handler(data.menu, "AppDir",
                                             &_menu_xml_handler_AppDir, FALSE, NULL);
    menuTag_DefaultAppDirs = fm_xml_file_set_handler(data.menu, "DefaultAppDirs",
                                                     &_menu_xml_handler_DefaultAppDirs, FALSE, NULL);
    menuTag_DirectoryDir = fm_xml_file_set_handler(data.menu, "DirectoryDir",
                                                   &_menu_xml_handler_DirectoryDir, FALSE, NULL);
    menuTag_DefaultDirectoryDirs = fm_xml_file_set_handler(data.menu, "DefaultDirectoryDirs",
                                                           &_menu_xml_handler_DefaultDirectoryDirs, FALSE, NULL);
    menuTag_OnlyUnallocated = fm_xml_file_set_handler(data.menu, "OnlyUnallocated",
                                                      &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_NotOnlyUnallocated = fm_xml_file_set_handler(data.menu, "NotOnlyUnallocated",
                                                         &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_All = fm_xml_file_set_handler(data.menu, "All",
                                          &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_LegacyDir = fm_xml_file_set_handler(data.menu, "LegacyDir",
                                                &_menu_xml_handler_LegacyDir, FALSE, NULL);
    menuTag_Move = fm_xml_file_set_handler(data.menu, "Move",
                                           &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Old = fm_xml_file_set_handler(data.menu, "Old",
                                          &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_New = fm_xml_file_set_handler(data.menu, "New",
                                          &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Layout = fm_xml_file_set_handler(data.menu, "Layout",
                                             &_menu_xml_handler_Layout, FALSE, NULL);
    menuTag_DefaultLayout = fm_xml_file_set_handler(data.menu, "DefaultLayout",
                                                    &_menu_xml_handler_DefaultLayout, FALSE, NULL);
    menuTag_Menuname = fm_xml_file_set_handler(data.menu, "Menuname",
                                               &_menu_xml_handler_Menuname, FALSE, NULL);
    menuTag_Separator = fm_xml_file_set_handler(data.menu, "Separator",
                                                &_menu_xml_handler_Separator, FALSE, NULL);
    menuTag_Merge = fm_xml_file_set_handler(data.menu, "Merge",
                                            &_menu_xml_handler_Merge, FALSE, NULL);
    /* Do parsing */
    ok = fm_xml_file_parse_data(data.menu, contents, len, error, &data);
    g_free(contents);
    if (ok)
        xml = fm_xml_file_finish_parse(data.menu, error);
    if (xml == NULL) /* error is set by failed function */
    {
        if (data.line == -1)
            data.line = fm_xml_file_get_current_line(data.menu, &data.pos);
        g_prefix_error(error, _("XML file '%s' error (%d:%d): "), data.file_path,
                       data.line, data.pos);
        goto _return_error;
    }
    /* Merge other files */
    apps = _find_in_children(xml, "Applications");
    g_list_free(xml);
    if (apps == NULL)
    {
        g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_NOENT,
                            _("XML file doesn't contain Applications root"));
        goto _return_error;
    }
    if (!_activate_merges(&data, apps, error))
        goto _return_error;
    /* FIXME: validate <Merge> tags */
    /* Create our menu tree -- no failures anymore! */
    memset(&default_layout, 0, sizeof(default_layout));
    default_layout.inline_header = TRUE;
    default_layout.inline_limit = 4;
    default_layout.items = g_list_prepend(g_list_prepend(NULL, &def_files), &def_menus);
    menu = _make_menu_node(apps, &default_layout);
    g_list_free(default_layout.items);
    if (verbose > 2)
    {
        char *dump = fm_xml_file_to_data(data.menu, NULL, NULL);
        g_print("%s", dump);
        g_free(dump);
    }
    /* Free layouts hash */
_return_error:
    if (menu == NULL)
        g_object_unref(data.menu);
    else
        /* keep XML file still since MenuRule elements use items in it */
        *xmlfile = data.menu;
    g_hash_table_destroy(layout_hash);
    return menu;
}
