/*
 *      fm-vfs-menu.c
 *      VFS for "menu://applications/" path using menu-cache library.
 *
 *      Copyright 2012-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-file.h"
#include "glib-compat.h"
#include "fm-utils.h"
#include "fm-xml-file.h"

#include <glib/gi18n-lib.h>
#include <menu-cache.h>

/* support for libmenu-cache 0.4.x */
#ifndef MENU_CACHE_CHECK_VERSION
# ifdef HAVE_MENU_CACHE_DIR_LIST_CHILDREN
#  define MENU_CACHE_CHECK_VERSION(_a,_b,_c) (_a == 0 && _b < 5) /* < 0.5.0 */
# else
#  define MENU_CACHE_CHECK_VERSION(_a,_b,_c) 0 /* not even 0.4.0 */
# endif
#endif

/* libmenu-cache is multithreaded since 0.4.x */
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
# define RUN_WITH_MENU_CACHE(__func,__data) __func(__data)
#else
# define RUN_WITH_MENU_CACHE(__func,__data) fm_run_in_default_main_context(__func,__data)
#endif

/* beforehand declarations */
static GFile *_fm_vfs_menu_new_for_uri(const char *uri);


/* ---- applications.menu manipulations ---- */
typedef struct _FmMenuMenuTree          FmMenuMenuTree;

struct _FmMenuMenuTree
{
    FmXmlFile *menu; /* composite tree to analyze */
    char *file_path; /* current file */
    GCancellable *cancellable;
    gint line, pos; /* we remember position in deepest file */
};

G_LOCK_DEFINE_STATIC(menuTree); /* locks all .menu file access data below */
static FmXmlFileTag menuTag_Menu = 0; /* tags that are supported */
static FmXmlFileTag menuTag_Include = 0;
static FmXmlFileTag menuTag_Exclude = 0;
static FmXmlFileTag menuTag_Filename = 0;
static FmXmlFileTag menuTag_Category = 0;
static FmXmlFileTag menuTag_MergeFile = 0;
static FmXmlFileTag menuTag_Directory = 0;
static FmXmlFileTag menuTag_Name = 0;
static FmXmlFileTag menuTag_Deleted = 0;
static FmXmlFileTag menuTag_NotDeleted = 0;

/* this handler does nothing, used just to remember its id */
static gboolean _menu_xml_handler_pass(FmXmlFileItem *item, GList *children,
                                       char * const *attribute_names,
                                       char * const *attribute_values,
                                       guint n_attributes, gint line, gint pos,
                                       GError **error, gpointer user_data)
{
    FmMenuMenuTree *data = user_data;
    return !g_cancellable_set_error_if_cancelled(data->cancellable, error);
}

/* FIXME: handle <Move><Old>...</Old><New>...</New></Move> */

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

static FmXmlFileItem *_find_in_children(GList *list, const char *path)
{
    const char *ptr;
    char *_ptr;

    if (list == NULL)
        return NULL;
    g_debug("menu tree: searching for '%s'", path);
    ptr = strchr(path, '/');
    if (ptr == NULL)
    {
        ptr = path;
        path = _ptr = NULL;
    }
    else
    {
        _ptr = g_strndup(path, ptr - path);
        path = ptr + 1;
        ptr = _ptr;
    }
    while (list)
    {
        const char *elem_name = _get_menu_name(list->data);
        /* g_debug("got child %d: %s", fm_xml_file_item_get_tag(list->data), elem_name); */
        if (g_strcmp0(elem_name, ptr) == 0)
            break;
        else
            list = list->next;
    }
    g_free(_ptr);
    if (list && path)
    {
        FmXmlFileItem *item;

        list = fm_xml_file_item_get_children(list->data);
        item = _find_in_children(list, path);
        g_list_free(list);
        return item;
    }
    return list ? list->data : NULL;
}

/* returns only <Menu> child */
static FmXmlFileItem *_set_default_contents(FmXmlFile *file, const char *basename)
{
    FmXmlFileItem *item, *child;
    char *path;

    /* set DTD */
    fm_xml_file_set_dtd(file,
                        "Menu PUBLIC '-//freedesktop//DTD Menu 1.0//EN'\n"
                        " 'http://www.freedesktop.org/standards/menu-spec/menu-1.0.dtd'",
                        NULL);
    /* set content:
        <Menu>
            <Name>Applications</Name>
            <MergeFile type='parent'>/etc/xgd/menus/%s</MergeFile>
        </Menu> */
    item = fm_xml_file_item_new(menuTag_Menu);
    fm_xml_file_insert_first(file, item);
    child = fm_xml_file_item_new(menuTag_Name);
    fm_xml_file_item_append_text(child, "Applications", -1, FALSE);
    fm_xml_file_item_append_child(item, child);
    child = fm_xml_file_item_new(menuTag_MergeFile);
    fm_xml_file_item_set_attribute(child, "type", "parent");
    /* FIXME: what is correct way to handle this? is it required at all? */
    path = g_strdup_printf("/etc/xgd/menus/%s", basename);
    fm_xml_file_item_append_text(child, path, -1, FALSE);
    g_free(path);
    fm_xml_file_item_append_child(item, child);
    return item;
    /* FIXME: can errors happen above? */
}

/* tries to create <Menu>... path in children list and returns <Menu> for it */
static FmXmlFileItem *_create_path_in_tree(FmXmlFileItem *parent, const char *path)
{
    const char *ptr;
    char *_ptr;
    GList *list, *l;
    FmXmlFileItem *item, *name;

    if (path == NULL)
        return NULL;
    list = fm_xml_file_item_get_children(parent);
    /* g_debug("menu tree: creating '%s'", path); */
    ptr = strchr(path, '/');
    if (ptr == NULL)
    {
        ptr = path;
        path = _ptr = NULL;
    }
    else
    {
        _ptr = g_strndup(path, ptr - path);
        path = ptr + 1;
        ptr = _ptr;
    }
    for (l = list; l; l = l->next)
    {
        const char *elem_name = _get_menu_name(l->data);
        /* g_debug("got child %d: %s", fm_xml_file_item_get_tag(list->data), elem_name); */
        if (g_strcmp0(elem_name, ptr) == 0)
            break;
    }
    if (l) /* subpath already exists */
    {
        item = l->data;
        g_list_free(list);
        g_free(_ptr);
        return _create_path_in_tree(item, path);
    }
    g_list_free(list);
    /* create subtag <Name> */
    name = fm_xml_file_item_new(menuTag_Name);
    fm_xml_file_item_append_text(name, ptr, -1, FALSE);
    g_free(_ptr);
    /* create <Menu> and insert it */
    item = fm_xml_file_item_new(menuTag_Menu);
    if (!fm_xml_file_item_append_child(parent, item) ||
        !fm_xml_file_item_append_child(item, name))
    {
        /* FIXME: it cannot fail on newly created items! */
        fm_xml_file_item_destroy(name);
        fm_xml_file_item_destroy(item);
        return NULL;
    }
    /* path is NULL if it is a final subpath */
    return path ? _create_path_in_tree(item, path) : item;
}

/* locks menuTree, sets fields in data, sets gf
   returns "Applications" menu on success and NULL on failure */
static FmXmlFileItem *_prepare_contents(FmMenuMenuTree *data, GCancellable *cancellable,
                                        GError **error, GFile **gf)
{
    const char *xdg_menu_prefix;
    char *contents;
    gsize len;
    GList *xml = NULL;
    FmXmlFileItem *apps;
    gboolean ok;

    /* do it in compatibility with lxpanel */
    xdg_menu_prefix = g_getenv("XDG_MENU_PREFIX");
    contents = g_strdup_printf("%sapplications.menu",
                               xdg_menu_prefix ? xdg_menu_prefix : "lxde-");
    data->file_path = g_build_filename(g_get_user_config_dir(), "menus",
                                       contents, NULL);
    *gf = g_file_new_for_path(data->file_path);
    data->menu = fm_xml_file_new(NULL);
    data->line = data->pos = -1;
    data->cancellable = cancellable;
    G_LOCK(menuTree);
    /* set tags, ignore errors */
    menuTag_Menu = fm_xml_file_set_handler(data->menu, "Menu",
                                           &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Name = fm_xml_file_set_handler(data->menu, "Name",
                                           &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Deleted = fm_xml_file_set_handler(data->menu, "Deleted",
                                              &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_NotDeleted = fm_xml_file_set_handler(data->menu, "NotDeleted",
                                                 &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Directory = fm_xml_file_set_handler(data->menu, "Directory",
                                                &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Include = fm_xml_file_set_handler(data->menu, "Include",
                                              &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Exclude = fm_xml_file_set_handler(data->menu, "Exclude",
                                              &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Filename = fm_xml_file_set_handler(data->menu, "Filename",
                                               &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_MergeFile = fm_xml_file_set_handler(data->menu, "MergeFile",
                                                &_menu_xml_handler_pass, FALSE, NULL);
    menuTag_Category = fm_xml_file_set_handler(data->menu, "Category",
                                               &_menu_xml_handler_pass, FALSE, NULL);
    if (!g_file_query_exists(*gf, cancellable))
    {
        /* if file doesn't exist then it should be created with default contents */
        apps = _set_default_contents(data->menu, contents);
        g_free(contents);
        return apps;
    }
    g_free(contents); /* we used it temporarily */
    contents = NULL;
    ok = g_file_load_contents(*gf, cancellable, &contents, &len, NULL, error);
    if (!ok)
        return NULL;
    ok = fm_xml_file_parse_data(data->menu, contents, len, error, data);
    g_free(contents);
    if (ok)
        xml = fm_xml_file_finish_parse(data->menu, error);
    if (xml == NULL) /* error is set by failed function */
    {
        if (data->line == -1)
            data->line = fm_xml_file_get_current_line(data->menu, &data->pos);
        g_prefix_error(error, _("XML file '%s' error (%d:%d): "), data->file_path,
                       data->line, data->pos);
    }
    else
    {
        apps = _find_in_children(xml, "Applications");
        g_list_free(xml);
        if (apps)
            return apps;
        g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_NOENT,
                            _("XML file doesn't contain Applications root"));
    }
    return NULL;
}

/* replaces invalid chars in path with minus sign */
static inline char *_get_pathtag_for_path(const char *path)
{
    char *pathtag, *c;

    pathtag = g_strdup(path);
    for (c = pathtag; *c; c++)
        if (*c == '/' || *c == '\t' || *c == '\n' || *c == '\r' || *c == ' ')
            *c = '-';
    return pathtag;
}

static gboolean _save_new_menu_file(GFile *gf, FmXmlFile *file,
                                    GCancellable *cancellable,
                                    GError **error)
{
    gsize len;
    char *contents = fm_xml_file_to_data(file, &len, error);
    gboolean result = FALSE;

    if (contents == NULL)
        return FALSE;
    /* g_debug("new menu file: %s", contents); */
    result = g_file_replace_contents(gf, contents, len, NULL, FALSE,
                                     G_FILE_CREATE_REPLACE_DESTINATION, NULL,
                                     cancellable, error);
    g_free(contents);
    return result;
}

#if MENU_CACHE_CHECK_VERSION(0, 5, 0)
/* changes .menu XML file */
static gboolean _remove_directory(const char *path, GCancellable *cancellable,
                                  GError **error)
{
    GList *xml = NULL, *it;
    GFile *gf;
    FmXmlFileItem *apps, *item;
    FmMenuMenuTree data;
    gboolean ok = TRUE;

    /* g_debug("deleting menu folder '%s'", path); */
    /* FIXME: check if there is that path in the XML tree before doing anything */
    apps = _prepare_contents(&data, cancellable, error, &gf);
    if (apps == NULL)
    {
        /* either failed to load contents or cancelled */
        ok = FALSE;
    }
    else if ((xml = fm_xml_file_item_get_children(apps)) != NULL &&
             (item = _find_in_children(xml, path)) != NULL)
    {
        /* if path is found and has <NotDeleted/> then replace it with <Deleted/> */
        g_list_free(xml);
        xml = fm_xml_file_item_get_children(item);
        for (it = xml; it; it = it->next)
        {
            FmXmlFileTag tag = fm_xml_file_item_get_tag(it->data);
            if (tag == menuTag_Deleted || tag == menuTag_NotDeleted)
                fm_xml_file_item_destroy(it->data);
        }
        goto _add_deleted_tag;
    }
    else
    {
        /* else create path and add <Deleted/> to it */
        item = _create_path_in_tree(apps, path);
        if (item == NULL)
        {
            g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                        _("Cannot create XML definition for '%s'"), path);
            ok = FALSE;
        }
        else
        {
            FmXmlFileItem *item2;

_add_deleted_tag:
            item2 = fm_xml_file_item_new(menuTag_Deleted);
            fm_xml_file_item_set_comment(item2, "deleted by LibFM");
            fm_xml_file_item_append_child(item, item2); /* NOTE: it cannot fail */
        }
    }
    if (ok)
        ok = _save_new_menu_file(gf, data.menu, cancellable, error);
    G_UNLOCK(menuTree);
    g_object_unref(gf);
    g_object_unref(data.menu);
    g_free(data.file_path);
    g_list_free(xml);
    return ok;
}

/* changes .menu XML file */
static gboolean _add_directory(const char *path, GCancellable *cancellable,
                               GError **error)
{
    GList *xml = NULL, *it;
    GFile *gf;
    FmXmlFileItem *apps, *item, *child;
    FmMenuMenuTree data;
    gboolean ok = TRUE;

    /* g_debug("adding menu folder '%s'", path); */
    /* FIXME: fail if such Menu Name already not deleted in XML tree */
    apps = _prepare_contents(&data, cancellable, error, &gf);
    if (apps == NULL)
    {
        /* either failed to load contents or cancelled */
        ok = FALSE;
    }
    else if ((xml = fm_xml_file_item_get_children(apps)) != NULL &&
             (item = _find_in_children(xml, path)) != NULL)
    {
        /* "undelete" the directory: */
        /* if path is found and has <Deleted/> then replace it with <NotDeleted/> */
        g_list_free(xml);
        xml = fm_xml_file_item_get_children(item);
        ok = FALSE; /* it should be Deleted, otherwise error, see FIXME above */
        for (it = xml; it; it = it->next)
        {
            FmXmlFileTag tag = fm_xml_file_item_get_tag(it->data);
            if (tag == menuTag_Deleted)
            {
                fm_xml_file_item_destroy(it->data);
                ok = TRUE; /* see FIXME above */
            }
            else if (tag == menuTag_NotDeleted)
            {
                fm_xml_file_item_destroy(it->data);
                ok = FALSE; /* see FIXME above */
            }
        }
        if (!ok) /* see FIXME above */
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_EXISTS,
                        _("Menu path '%s' already exists"), path);
        else
        {
            child = fm_xml_file_item_new(menuTag_NotDeleted);
            fm_xml_file_item_set_comment(child, "undeleted by LibFM");
            fm_xml_file_item_append_child(item, child); /* NOTE: it cannot fail */
        }
    }
    else
    {
        /* else create path and add content to it */
        item = _create_path_in_tree(apps, path);
        if (item == NULL)
        {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_EXISTS,
                        _("Cannot create XML definition for '%s'"), path);
            ok = FALSE;
        }
        else
        {
            char *pathtag, *dir, *contents;
            GString *str;

            /* add <NotDeleted/> */
            child = fm_xml_file_item_new(menuTag_NotDeleted);
            fm_xml_file_item_append_child(item, child);
            /* touch .directory file, ignore errors */
            dir = strrchr(path, '/'); /* use it as a storage for basename */
            if (dir)
                dir++;
            else
                dir = (char *)path;
            contents = g_strdup_printf("[" G_KEY_FILE_DESKTOP_GROUP "]\n"
                                       G_KEY_FILE_DESKTOP_KEY_TYPE "=" G_KEY_FILE_DESKTOP_TYPE_DIRECTORY "\n"
                                       G_KEY_FILE_DESKTOP_KEY_NAME "=%s", dir);
            pathtag = _get_pathtag_for_path(path);
            dir = g_build_filename(g_get_user_data_dir(), "desktop-directories",
                                   pathtag, NULL);
            str = g_string_new(dir);
            g_free(dir);
            g_string_append(str, ".directory");
            g_file_set_contents(str->str, contents, -1, NULL);
            /* FIXME: report errors */
            g_free(contents);
            /* add <Directory>.....</Directory> */
            child = fm_xml_file_item_new(menuTag_Directory);
            g_string_printf(str, "%s.directory", pathtag);
            fm_xml_file_item_append_text(child, str->str, str->len, FALSE);
            fm_xml_file_item_append_child(item, child);
            /* add <Include><Category>......</Category></Include> */
            child = fm_xml_file_item_new(menuTag_Include);
            fm_xml_file_item_append_child(item, child);
            g_string_printf(str, "X-%s", pathtag);
            g_free(pathtag);
            item = fm_xml_file_item_new(menuTag_Category); /* reuse item var. */
            fm_xml_file_item_append_text(item, str->str, str->len, FALSE);
            fm_xml_file_item_append_child(child, item);
            g_string_free(str, TRUE);
            /* ignoring errors since new created items cannot fail on append */
        }
    }
    if (ok)
        ok = _save_new_menu_file(gf, data.menu, cancellable, error);
    G_UNLOCK(menuTree);
    g_object_unref(gf);
    g_object_unref(data.menu);
    g_free(data.file_path);
    g_list_free(xml);
    return ok;
}
#endif

/* changes .menu XML file */
static gboolean _add_application(const char *path, GCancellable *cancellable,
                                 GError **error)
{
    const char *id;
    char *dir;
    GList *xml = NULL, *it;
    GFile *gf;
    FmXmlFileItem *apps, *item, *child;
    FmMenuMenuTree data;
    gboolean ok = TRUE;

    id = strrchr(path, '/');
    if (id == NULL)
    {
        dir = NULL;
        id = path;
    }
    else
    {
        dir = g_strndup(path, id - path);
        id++;
    }
    apps = _prepare_contents(&data, cancellable, error, &gf);
    if (apps == NULL)
    {
        /* either failed to load contents or cancelled */
        ok = FALSE;
    }
    else if (dir == NULL) /* adding to root, use apps as target */
    {
        item = apps;
        goto _set;
    }
    else if ((xml = fm_xml_file_item_get_children(apps)) != NULL &&
             (item = _find_in_children(xml, dir)) != NULL)
    {
        /* already found that path */
        goto _set;
    }
    else
    {
        /* else create path and add content to it */
        item = _create_path_in_tree(apps, dir);
        if (item == NULL)
        {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_EXISTS,
                        _("Cannot create XML definition for '%s'"), path);
            ok = FALSE;
        }
        else
        {
_set:
            g_list_free(xml);
            xml = fm_xml_file_item_get_children(item);
            ok = FALSE; /* check if Include is already there */
            /* remove <Exclude><Filename>id</Filename></Exclude> */
            for (it = xml; it; it = it->next)
            {
                FmXmlFileTag tag = fm_xml_file_item_get_tag(it->data);
                if (tag == menuTag_Exclude)
                {
                    /* get Filename tag */
                    child = fm_xml_file_item_find_child(it->data, menuTag_Filename);
                    if (child == NULL)
                        continue;
                    /* get contents of the tag */
                    child = fm_xml_file_item_find_child(child, FM_XML_FILE_TEXT);
                    if (child == NULL)
                        continue;
                    if (strcmp(fm_xml_file_item_get_data(child, NULL), id) != 0)
                        continue;
                    fm_xml_file_item_destroy(it->data);
                    /* it was excluded before so removing exclude will add it */
                    ok = TRUE;
                }
                else if (!ok && tag == menuTag_Include)
                {
                    /* get Filename tag */
                    child = fm_xml_file_item_find_child(it->data, menuTag_Filename);
                    if (child == NULL)
                        continue;
                    /* get contents of the tag */
                    child = fm_xml_file_item_find_child(child, FM_XML_FILE_TEXT);
                    if (child == NULL)
                        continue;
                    if (strcmp(fm_xml_file_item_get_data(child, NULL), id) == 0)
                        ok = TRUE; /* found! */
                }
            }
            if (!ok)
            {
                /* add <Include><Filename>id</Filename></Include> */
                child = fm_xml_file_item_new(menuTag_Include);
                fm_xml_file_item_set_comment(child, "added by LibFM");
                fm_xml_file_item_append_child(item, child);
                item = fm_xml_file_item_new(menuTag_Filename);
                fm_xml_file_item_append_text(item, id, -1, FALSE);
                fm_xml_file_item_append_child(child, item);
                ok = TRUE;
            }
        }
    }
    if (ok)
        ok = _save_new_menu_file(gf, data.menu, cancellable, error);
    G_UNLOCK(menuTree);
    g_object_unref(gf);
    g_object_unref(data.menu);
    g_free(data.file_path);
    g_list_free(xml);
    g_free(dir);
    return ok;
}

/* changes .menu XML file */
static gboolean _remove_application(const char *path, GCancellable *cancellable,
                                    GError **error)
{
    const char *id;
    char *dir;
    GList *xml = NULL, *it;
    GFile *gf;
    FmXmlFileItem *apps, *item, *child;
    FmMenuMenuTree data;
    gboolean ok = TRUE;

    id = strrchr(path, '/');
    if (id == NULL)
    {
        dir = NULL;
        id = path;
    }
    else
    {
        dir = g_strndup(path, id - path);
        id++;
    }
    apps = _prepare_contents(&data, cancellable, error, &gf);
    if (apps == NULL)
    {
        /* either failed to load contents or cancelled */
        ok = FALSE;
    }
    else if (dir == NULL) /* removing from root, use apps as target */
    {
        item = apps;
        goto _set;
    }
    else if ((xml = fm_xml_file_item_get_children(apps)) != NULL &&
             (item = _find_in_children(xml, dir)) != NULL)
    {
        /* already found that path */
        goto _set;
    }
    else
    {
        /* else create path and add content to it */
        item = _create_path_in_tree(apps, dir);
        if (item == NULL)
        {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_EXISTS,
                        _("Cannot create XML definition for '%s'"), path);
            ok = FALSE;
        }
        else
        {
_set:
            g_list_free(xml);
            xml = fm_xml_file_item_get_children(item);
            ok = FALSE; /* check if Include is already there */
            /* remove <Include><Filename>id</Filename></Include> */
            for (it = xml; it; it = it->next)
            {
                FmXmlFileTag tag = fm_xml_file_item_get_tag(it->data);
                if (tag == menuTag_Include)
                {
                    /* get Filename tag */
                    child = fm_xml_file_item_find_child(it->data, menuTag_Filename);
                    if (child == NULL)
                        continue;
                    /* get contents of the tag */
                    child = fm_xml_file_item_find_child(child, FM_XML_FILE_TEXT);
                    if (child == NULL)
                        continue;
                    if (strcmp(fm_xml_file_item_get_data(child, NULL), id) != 0)
                        continue;
                    fm_xml_file_item_destroy(it->data);
                    /* it was included before so removing include will remove it */
                    ok = TRUE;
                }
                else if (!ok && tag == menuTag_Exclude)
                {
                    /* get Filename tag */
                    child = fm_xml_file_item_find_child(it->data, menuTag_Filename);
                    if (child == NULL)
                        continue;
                    /* get contents of the tag */
                    child = fm_xml_file_item_find_child(child, FM_XML_FILE_TEXT);
                    if (child == NULL)
                        continue;
                    if (strcmp(fm_xml_file_item_get_data(child, NULL), id) == 0)
                        ok = TRUE; /* found! */
                }
            }
            if (!ok)
            {
                /* add <Exclude><Filename>id</Filename></Exclude> */
                child = fm_xml_file_item_new(menuTag_Exclude);
                fm_xml_file_item_set_comment(child, "deleted by LibFM");
                fm_xml_file_item_append_child(item, child);
                item = fm_xml_file_item_new(menuTag_Filename);
                fm_xml_file_item_append_text(item, id, -1, FALSE);
                fm_xml_file_item_append_child(child, item);
                ok = TRUE;
            }
        }
    }
    if (ok)
        ok = _save_new_menu_file(gf, data.menu, cancellable, error);
    G_UNLOCK(menuTree);
    g_object_unref(gf);
    g_object_unref(data.menu);
    g_free(data.file_path);
    g_list_free(xml);
    g_free(dir);
    return ok;
}


/* ---- FmMenuVFile class ---- */
#define FM_TYPE_MENU_VFILE             (fm_vfs_menu_file_get_type())
#define FM_MENU_VFILE(o)               (G_TYPE_CHECK_INSTANCE_CAST((o), \
                                        FM_TYPE_MENU_VFILE, FmMenuVFile))

typedef struct _FmMenuVFile             FmMenuVFile;
typedef struct _FmMenuVFileClass        FmMenuVFileClass;

static GType fm_vfs_menu_file_get_type  (void);

struct _FmMenuVFile
{
    GObject parent_object;

    char *path;
};

struct _FmMenuVFileClass
{
  GObjectClass parent_class;
};

static void fm_menu_g_file_init(GFileIface *iface);
static void fm_menu_fm_file_init(FmFileInterface *iface);

G_DEFINE_TYPE_WITH_CODE(FmMenuVFile, fm_vfs_menu_file, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(G_TYPE_FILE, fm_menu_g_file_init)
                        G_IMPLEMENT_INTERFACE(FM_TYPE_FILE, fm_menu_fm_file_init))

static void fm_vfs_menu_file_finalize(GObject *object)
{
    FmMenuVFile *item = FM_MENU_VFILE(object);

    g_free(item->path);

    G_OBJECT_CLASS(fm_vfs_menu_file_parent_class)->finalize(object);
}

static void fm_vfs_menu_file_class_init(FmMenuVFileClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = fm_vfs_menu_file_finalize;
}

static void fm_vfs_menu_file_init(FmMenuVFile *item)
{
    /* nothing */
}

static FmMenuVFile *_fm_menu_vfile_new(void)
{
    return (FmMenuVFile*)g_object_new(FM_TYPE_MENU_VFILE, NULL);
}


/* ---- menu enumerator class ---- */
#define FM_TYPE_VFS_MENU_ENUMERATOR        (fm_vfs_menu_enumerator_get_type())
#define FM_VFS_MENU_ENUMERATOR(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), \
                            FM_TYPE_VFS_MENU_ENUMERATOR, FmVfsMenuEnumerator))

typedef struct _FmVfsMenuEnumerator         FmVfsMenuEnumerator;
typedef struct _FmVfsMenuEnumeratorClass    FmVfsMenuEnumeratorClass;

struct _FmVfsMenuEnumerator
{
    GFileEnumerator parent;

    MenuCache *mc;
    GSList *child;
    guint32 de_flag;
};

struct _FmVfsMenuEnumeratorClass
{
    GFileEnumeratorClass parent_class;
};

static GType fm_vfs_menu_enumerator_get_type   (void);

G_DEFINE_TYPE(FmVfsMenuEnumerator, fm_vfs_menu_enumerator, G_TYPE_FILE_ENUMERATOR)

static void _fm_vfs_menu_enumerator_dispose(GObject *object)
{
    FmVfsMenuEnumerator *enu = FM_VFS_MENU_ENUMERATOR(object);

    if(enu->mc)
    {
        menu_cache_unref(enu->mc);
        enu->mc = NULL;
    }

    G_OBJECT_CLASS(fm_vfs_menu_enumerator_parent_class)->dispose(object);
}

static GFileInfo *_g_file_info_from_menu_cache_item(MenuCacheItem *item,
//                                                    GFileAttributeMatcher *attribute_matcher,
                                                    guint32 de_flag)
{
    GFileInfo *fileinfo = g_file_info_new();
    const char *icon_name;
    GIcon* icon;

    /* FIXME: use g_uri_escape_string() for item name */
    g_file_info_set_name(fileinfo, menu_cache_item_get_id(item));
    if(menu_cache_item_get_name(item) != NULL)
        g_file_info_set_display_name(fileinfo, menu_cache_item_get_name(item));

    /* the setup below was in fm_file_info_set_from_menu_cache_item()
       so this setup makes latter API deprecated */
    icon_name = menu_cache_item_get_icon(item);
    if(icon_name)
    {
        icon = G_ICON(fm_icon_from_name(icon_name));
        if(G_LIKELY(icon))
        {
            g_file_info_set_icon(fileinfo, icon);
            g_object_unref(icon);
        }
    }
    if(menu_cache_item_get_type(item) == MENU_CACHE_TYPE_DIR)
    {
        g_file_info_set_file_type(fileinfo, G_FILE_TYPE_DIRECTORY);
#if MENU_CACHE_CHECK_VERSION(0, 5, 0)
        g_file_info_set_is_hidden(fileinfo,
                                  !menu_cache_dir_is_visible(MENU_CACHE_DIR(item)));
#else
        g_file_info_set_is_hidden(fileinfo, FALSE);
#endif
    }
    else /* MENU_CACHE_TYPE_APP */
    {
        char *path = menu_cache_item_get_file_path(item);
        g_file_info_set_file_type(fileinfo, G_FILE_TYPE_SHORTCUT);
        g_file_info_set_attribute_string(fileinfo,
                                         G_FILE_ATTRIBUTE_STANDARD_TARGET_URI,
                                         path);
        g_free(path);
        g_file_info_set_content_type(fileinfo, "application/x-desktop");
        g_file_info_set_is_hidden(fileinfo,
                                  !menu_cache_app_get_is_visible(MENU_CACHE_APP(item),
                                                                 de_flag));
    }
// FIXME: use attribute_matcher and set G_FILE_ATTRIBUTE_ACCESS_CAN_{WRITE,READ,EXECUTE,DELETE}
    g_file_info_set_attribute_string(fileinfo, G_FILE_ATTRIBUTE_ID_FILESYSTEM,
                                     "menu-Applications");
    g_file_info_set_attribute_boolean(fileinfo,
                                      G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME, TRUE);
    g_file_info_set_attribute_boolean(fileinfo,
                                      G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH, FALSE);
    return fileinfo;
}

typedef struct
{
    union
    {
        FmVfsMenuEnumerator *enumerator;
        const char *path_str;
    };
    union
    {
        FmMenuVFile *destination;
//        const char *attributes;
        const char *display_name;
        GFileInfo *info;
    };
//    GFileQueryInfoFlags flags;
    union
    {
        GCancellable *cancellable;
        GFile *file;
    };
    GError **error;
    gpointer result;
} FmVfsMenuMainThreadData;

static gboolean _fm_vfs_menu_enumerator_next_file_real(gpointer data)
{
    FmVfsMenuMainThreadData *init = data;
    FmVfsMenuEnumerator *enu = init->enumerator;
    GSList *child = enu->child;
    MenuCacheItem *item;

    init->result = NULL;

    if(child == NULL)
        goto done;

    for(; child; child = child->next)
    {
        if(g_cancellable_set_error_if_cancelled(init->cancellable, init->error))
            break;
        item = MENU_CACHE_ITEM(child->data);
        if(!item || menu_cache_item_get_type(item) == MENU_CACHE_TYPE_SEP ||
           menu_cache_item_get_type(item) == MENU_CACHE_TYPE_NONE)
            continue;
#if 0
        /* also hide menu items which should be hidden in current DE. */
        if(menu_cache_item_get_type(item) == MENU_CACHE_TYPE_APP
           && !menu_cache_app_get_is_visible(MENU_CACHE_APP(item), enu->de_flag))
            continue;
#endif

        init->result = _g_file_info_from_menu_cache_item(item, enu->de_flag);
        child = child->next;
        break;
    }
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    while(enu->child != child) /* free skipped/used elements */
    {
        GSList *ch = enu->child;
        enu->child = ch->next;
        menu_cache_item_unref(ch->data);
        g_slist_free_1(ch);
    }
#else
    enu->child = child;
#endif

done:
    return FALSE;
}

static GFileInfo *_fm_vfs_menu_enumerator_next_file(GFileEnumerator *enumerator,
                                                    GCancellable *cancellable,
                                                    GError **error)
{
    FmVfsMenuMainThreadData init;

    init.enumerator = FM_VFS_MENU_ENUMERATOR(enumerator);
    init.cancellable = cancellable;
    init.error = error;
    RUN_WITH_MENU_CACHE(_fm_vfs_menu_enumerator_next_file_real, &init);
    return init.result;
}

static gboolean _fm_vfs_menu_enumerator_close(GFileEnumerator *enumerator,
                                              GCancellable *cancellable,
                                              GError **error)
{
    FmVfsMenuEnumerator *enu = FM_VFS_MENU_ENUMERATOR(enumerator);

    if(enu->mc)
    {
        menu_cache_unref(enu->mc);
        enu->mc = NULL;
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
        g_slist_free_full(enu->child, (GDestroyNotify)menu_cache_item_unref);
#endif
        enu->child = NULL;
    }
    return TRUE;
}

static void fm_vfs_menu_enumerator_class_init(FmVfsMenuEnumeratorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GFileEnumeratorClass *enumerator_class = G_FILE_ENUMERATOR_CLASS(klass);

  gobject_class->dispose = _fm_vfs_menu_enumerator_dispose;

  enumerator_class->next_file = _fm_vfs_menu_enumerator_next_file;
  enumerator_class->close_fn = _fm_vfs_menu_enumerator_close;
}

static void fm_vfs_menu_enumerator_init(FmVfsMenuEnumerator *enumerator)
{
    /* nothing */
}

static MenuCacheItem *_vfile_path_to_menu_cache_item(MenuCache* mc, const char *path)
{
    MenuCacheItem *dir;
    char *unescaped, *tmp = NULL;

    unescaped = g_uri_unescape_string(path, NULL);
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    dir = MENU_CACHE_ITEM(menu_cache_dup_root_dir(mc));
#else
    dir = MENU_CACHE_ITEM(menu_cache_get_root_dir(mc));
#endif
    if(dir)
    {
#if !MENU_CACHE_CHECK_VERSION(0, 5, 0)
        char *id;
#if !MENU_CACHE_CHECK_VERSION(0, 4, 0)
        GSList *child;
#endif
#endif
        tmp = g_strconcat("/", menu_cache_item_get_id(dir), "/", unescaped, NULL);
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
        menu_cache_item_unref(dir);
        dir = menu_cache_item_from_path(mc, tmp);
#else
        /* access not dir is a bit tricky */
        id = strrchr(tmp, '/');
        *id++ = '\0';
        dir = MENU_CACHE_ITEM(menu_cache_get_dir_from_path(mc, tmp));
        child = menu_cache_dir_get_children(MENU_CACHE_DIR(dir));
        dir = NULL;
        while (child)
        {
            if (g_strcmp0(id, menu_cache_item_get_id(child->data)) == 0)
            {
                dir = child->data;
                break;
            }
            child = child->next;
        }
#endif
#if !MENU_CACHE_CHECK_VERSION(0, 5, 0)
        /* The menu-cache is buggy and returns parent for invalid path
           instead of failure so we check what we got here.
           Unfortunately we cannot detect if requested name is the same
           as its parent and menu-cache returned the parent. */
        id = strrchr(unescaped, '/');
        if(id)
            id++;
        else
            id = unescaped;
        if(dir != NULL && strcmp(id, menu_cache_item_get_id(dir)) != 0)
            dir = NULL;
#endif
    }
    g_free(unescaped);
    g_free(tmp);
    /* NOTE: returned value is referenced for >= 0.4.0 only */
    return dir;
}

static MenuCache *_get_menu_cache(GError **error)
{
    MenuCache *mc;
    static gboolean environment_tested = FALSE;
    static gboolean requires_prefix = FALSE;

    /* do it in compatibility with lxpanel */
    if(!environment_tested)
    {
        requires_prefix = (g_getenv("XDG_MENU_PREFIX") == NULL);
        environment_tested = TRUE;
    }
#if MENU_CACHE_CHECK_VERSION(0, 5, 0)
    mc = menu_cache_lookup_sync(requires_prefix ? "lxde-applications.menu+hidden" : "applications.menu+hidden");
#else
    mc = menu_cache_lookup_sync(requires_prefix ? "lxde-applications.menu" : "applications.menu");
#endif
    /* FIXME: may be it is reasonable to set XDG_MENU_PREFIX ? */

    if(mc == NULL) /* initialization failed */
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("Menu cache error"));
    return mc;
}

static gboolean _fm_vfs_menu_enumerator_new_real(gpointer data)
{
    FmVfsMenuMainThreadData *init = data;
    FmVfsMenuEnumerator *enumerator;
    MenuCache* mc;
    const char *de_name;
    MenuCacheItem *dir;

    mc = _get_menu_cache(init->error);

    if(mc == NULL) /* initialization failed */
        return FALSE;

    enumerator = g_object_new(FM_TYPE_VFS_MENU_ENUMERATOR, "container",
                              init->file, NULL);
    enumerator->mc = mc;
    de_name = g_getenv("XDG_CURRENT_DESKTOP");

    if(de_name)
        enumerator->de_flag = menu_cache_get_desktop_env_flag(mc, de_name);
    else
        enumerator->de_flag = (guint32)-1;

    /* the menu should be loaded now */
    if(init->path_str)
        dir = _vfile_path_to_menu_cache_item(mc, init->path_str);
    else
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
        dir = MENU_CACHE_ITEM(menu_cache_dup_root_dir(mc));
#else
        dir = MENU_CACHE_ITEM(menu_cache_get_root_dir(mc));
#endif
    if(dir)
    {
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
        enumerator->child = menu_cache_dir_list_children(MENU_CACHE_DIR(dir));
        menu_cache_item_unref(dir);
#else
        enumerator->child = menu_cache_dir_get_children(MENU_CACHE_DIR(dir));
#endif
    }
    /* FIXME: do something with attributes and flags */

    init->result = enumerator;
    return FALSE;
}

static GFileEnumerator *_fm_vfs_menu_enumerator_new(GFile *file,
                                                    const char *path_str,
                                                    const char *attributes,
                                                    GFileQueryInfoFlags flags,
                                                    GError **error)
{
    FmVfsMenuMainThreadData enu;

    enu.path_str = path_str;
//    enu.attributes = attributes;
//    enu.flags = flags;
    enu.file = file;
    enu.error = error;
    enu.result = NULL;
    RUN_WITH_MENU_CACHE(_fm_vfs_menu_enumerator_new_real, &enu);
    return enu.result;
}


/* ---- GFile implementation ---- */
static GFileAttributeInfoList *_fm_vfs_menu_settable_attributes = NULL;

#define ERROR_UNSUPPORTED(err) g_set_error_literal(err, G_IO_ERROR, \
                        G_IO_ERROR_NOT_SUPPORTED, _("Operation not supported"))

static GFile *_fm_vfs_menu_dup(GFile *file)
{
    FmMenuVFile *item, *new_item;

    item = FM_MENU_VFILE(file);
    new_item = _fm_menu_vfile_new();
    if(item->path)
        new_item->path = g_strdup(item->path);
    return (GFile*)new_item;
}

static guint _fm_vfs_menu_hash(GFile *file)
{
    return g_str_hash(FM_MENU_VFILE(file)->path ? FM_MENU_VFILE(file)->path : "/");
}

static gboolean _fm_vfs_menu_equal(GFile *file1, GFile *file2)
{
    char *path1 = FM_MENU_VFILE(file1)->path;
    char *path2 = FM_MENU_VFILE(file2)->path;

    return g_strcmp0(path1, path2) == 0;
}

static gboolean _fm_vfs_menu_is_native(GFile *file)
{
    return FALSE;
}

static gboolean _fm_vfs_menu_has_uri_scheme(GFile *file, const char *uri_scheme)
{
    return g_ascii_strcasecmp(uri_scheme, "menu") == 0;
}

static char *_fm_vfs_menu_get_uri_scheme(GFile *file)
{
    return g_strdup("menu");
}

static char *_fm_vfs_menu_get_basename(GFile *file)
{
    /* g_debug("_fm_vfs_menu_get_basename %s", FM_MENU_VFILE(file)->path); */
    if(FM_MENU_VFILE(file)->path == NULL)
        return g_strdup("/");
    return g_path_get_basename(FM_MENU_VFILE(file)->path);
}

static char *_fm_vfs_menu_get_path(GFile *file)
{
    return NULL;
}

static char *_fm_vfs_menu_get_uri(GFile *file)
{
    return g_strconcat("menu://applications/", FM_MENU_VFILE(file)->path, NULL);
}

static char *_fm_vfs_menu_get_parse_name(GFile *file)
{
    char *unescaped, *path;

    /* g_debug("_fm_vfs_menu_get_parse_name %s", FM_MENU_VFILE(file)->path); */
    unescaped = g_uri_unescape_string(FM_MENU_VFILE(file)->path, NULL);
    path = g_strconcat("menu://applications/", unescaped, NULL);
    g_free(unescaped);
    return path;
}

static GFile *_fm_vfs_menu_get_parent(GFile *file)
{
    char *path = FM_MENU_VFILE(file)->path;
    char *dirname;
    GFile *parent;

    /* g_debug("_fm_vfs_menu_get_parent %s", path); */
    if(path)
    {
        dirname = g_path_get_dirname(path);
        if(strcmp(dirname, ".") == 0)
        {
            g_free(dirname);
            path = NULL;
        }
        else
            path = dirname;
    }
    parent = _fm_vfs_menu_new_for_uri(path);
    if(path)
        g_free(path);
    return parent;
}

/* this function is taken from GLocalFile implementation */
static const char *match_prefix (const char *path, const char *prefix)
{
  int prefix_len;

  prefix_len = strlen (prefix);
  if (strncmp (path, prefix, prefix_len) != 0)
    return NULL;

  if (prefix_len > 0 && (prefix[prefix_len-1]) == '/')
    prefix_len--;

  return path + prefix_len;
}

static gboolean _fm_vfs_menu_prefix_matches(GFile *prefix, GFile *file)
{
    const char *path = FM_MENU_VFILE(file)->path;
    const char *pp = FM_MENU_VFILE(prefix)->path;
    const char *remainder;

    if(pp == NULL)
        return TRUE;
    if(path == NULL)
        return FALSE;
    remainder = match_prefix(path, pp);
    if(remainder != NULL && *remainder == '/')
        return TRUE;
    return FALSE;
}

static char *_fm_vfs_menu_get_relative_path(GFile *parent, GFile *descendant)
{
    const char *path = FM_MENU_VFILE(descendant)->path;
    const char *pp = FM_MENU_VFILE(parent)->path;
    const char *remainder;

    if(pp == NULL)
        return g_strdup(path);
    if(path == NULL)
        return NULL;
    remainder = match_prefix(path, pp);
    if(remainder != NULL && *remainder == '/')
        return g_uri_unescape_string(&remainder[1], NULL);
    return NULL;
}

static GFile *_fm_vfs_menu_resolve_relative_path(GFile *file, const char *relative_path)
{
    const char *path = FM_MENU_VFILE(file)->path;
    FmMenuVFile *new_item = _fm_menu_vfile_new();

    /* g_debug("_fm_vfs_menu_resolve_relative_path %s %s", path, relative_path); */
    /* FIXME: handle if relative_path is invalid */
    if(relative_path == NULL || *relative_path == '\0')
        new_item->path = g_strdup(path);
    else if(path == NULL)
        new_item->path = g_strdup(relative_path);
    else
    {
        /* relative_path is the most probably unescaped string (at least GFVS
           works such way) so we have to escape invalid chars here. */
        char *escaped = g_uri_escape_string(relative_path,
                                            G_URI_RESERVED_CHARS_ALLOWED_IN_PATH,
                                            TRUE);
        new_item->path = g_strconcat(path, "/", relative_path, NULL);
        g_free(escaped);
    }
    return (GFile*)new_item;
}

static gboolean _fm_vfs_menu_get_child_for_display_name_real(gpointer data)
{
    FmVfsMenuMainThreadData *init = data;
    MenuCache *mc;
    MenuCacheItem *dir;
    gboolean is_invalid = FALSE;

    init->result = NULL;
    mc = _get_menu_cache(init->error);
    if(mc == NULL)
        goto _mc_failed;

    if(init->path_str)
    {
        dir = _vfile_path_to_menu_cache_item(mc, init->path_str);
        if(dir == NULL || menu_cache_item_get_type(dir) != MENU_CACHE_TYPE_DIR)
            is_invalid = TRUE;
    }
    else
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
        dir = MENU_CACHE_ITEM(menu_cache_dup_root_dir(mc));
#else
        dir = MENU_CACHE_ITEM(menu_cache_get_root_dir(mc));
#endif
    if(is_invalid)
        g_set_error_literal(init->error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                            _("Invalid menu directory"));
    else if(dir)
    {
#if MENU_CACHE_CHECK_VERSION(0, 5, 0)
        MenuCacheItem *item = menu_cache_find_child_by_name(MENU_CACHE_DIR(dir),
                                                            init->display_name);
        g_debug("searched for child '%s' found '%s'", init->display_name,
                item ? menu_cache_item_get_id(item) : "(nil)");
        if (item == NULL)
            init->result = _fm_vfs_menu_resolve_relative_path(init->file,
                                                              init->display_name);
        else
        {
            init->result = _fm_vfs_menu_resolve_relative_path(init->file,
                                                menu_cache_item_get_id(item));
            menu_cache_item_unref(item);
        }
#else /* < 0.5.0 */
        GSList *l;
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
        GSList *children = menu_cache_dir_list_children(MENU_CACHE_DIR(dir));
#else
        GSList *children = menu_cache_dir_get_children(MENU_CACHE_DIR(dir));
#endif
        for (l = children; l; l = l->next)
            if (g_strcmp0(init->display_name, menu_cache_item_get_name(l->data)) == 0)
                break;
        if (l == NULL) /* not found */
            init->result = _fm_vfs_menu_resolve_relative_path(init->file,
                                                              init->display_name);
        else
            init->result = _fm_vfs_menu_resolve_relative_path(init->file,
                                                menu_cache_item_get_id(l->data));
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
        g_slist_free_full(children, (GDestroyNotify)menu_cache_item_unref);
#endif
#endif /* < 0.5.0 */
    }
    else /* menu_cache_get_root_dir failed */
        g_set_error_literal(init->error, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("Menu cache error"));

#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    if(dir)
        menu_cache_item_unref(dir);
#endif
    menu_cache_unref(mc);

_mc_failed:
    return FALSE;
}

static GFile *_fm_vfs_menu_get_child_for_display_name(GFile *file,
                                                      const char *display_name,
                                                      GError **error)
{
    FmMenuVFile *item = FM_MENU_VFILE(file);
    FmVfsMenuMainThreadData enu;

    /* g_debug("_fm_vfs_menu_get_child_for_display_name: '%s' '%s'", item->path, display_name); */
    if (display_name == NULL || *display_name == '\0')
    {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("Menu item name cannot be empty"));
        return NULL;
    }
    /* NOTE: this violates GFile requirement that this API should not do I/O but
       there is no way to do this without dirty tricks which may lead to failure */
    enu.path_str = item->path;
    enu.error = error;
    enu.display_name = display_name;
    enu.file = file;
    RUN_WITH_MENU_CACHE(_fm_vfs_menu_get_child_for_display_name_real, &enu);
    return enu.result;
}

static GFileEnumerator *_fm_vfs_menu_enumerate_children(GFile *file,
                                                        const char *attributes,
                                                        GFileQueryInfoFlags flags,
                                                        GCancellable *cancellable,
                                                        GError **error)
{
    const char *path = FM_MENU_VFILE(file)->path;

    return _fm_vfs_menu_enumerator_new(file, path, attributes, flags, error);
}

static gboolean _fm_vfs_menu_query_info_real(gpointer data)
{
    FmVfsMenuMainThreadData *init = data;
    MenuCache *mc;
    MenuCacheItem *dir;
    gboolean is_invalid = FALSE;

    init->result = NULL;
    mc = _get_menu_cache(init->error);
    if(mc == NULL)
        goto _mc_failed;

    if(init->path_str)
    {
        dir = _vfile_path_to_menu_cache_item(mc, init->path_str);
        if(dir == NULL)
            is_invalid = TRUE;
    }
    else
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
        dir = MENU_CACHE_ITEM(menu_cache_dup_root_dir(mc));
#else
        dir = MENU_CACHE_ITEM(menu_cache_get_root_dir(mc));
#endif
    if(is_invalid)
        g_set_error(init->error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                    _("Invalid menu directory '%s'"), init->path_str);
    else if(dir)
    {
        const char *de_name = g_getenv("XDG_CURRENT_DESKTOP");

        if(de_name)
            init->result = _g_file_info_from_menu_cache_item(dir,
                                menu_cache_get_desktop_env_flag(mc, de_name));
        else
            init->result = _g_file_info_from_menu_cache_item(dir, (guint32)-1);
    }
    else /* menu_cache_get_root_dir failed */
        g_set_error_literal(init->error, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("Menu cache error"));

#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    if(dir)
        menu_cache_item_unref(dir);
#endif
    menu_cache_unref(mc);

_mc_failed:
    return FALSE;
}

static GFileInfo *_fm_vfs_menu_query_info(GFile *file,
                                          const char *attributes,
                                          GFileQueryInfoFlags flags,
                                          GCancellable *cancellable,
                                          GError **error)
{
    FmMenuVFile *item = FM_MENU_VFILE(file);
    GFileInfo *info;
    GFileAttributeMatcher *matcher;
    char *basename, *id;
    FmVfsMenuMainThreadData enu;

    matcher = g_file_attribute_matcher_new(attributes);

    if(item->path == NULL) /* menu root */
    {
        info = g_file_info_new();
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_NAME))
            g_file_info_set_name(info, "/");
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_ID_FILESYSTEM))
            g_file_info_set_attribute_string(info, G_FILE_ATTRIBUTE_ID_FILESYSTEM,
                                             "menu-Applications");
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_TYPE))
            g_file_info_set_file_type(info, G_FILE_TYPE_DIRECTORY);
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_ICON))
        {
            GIcon *icon = g_themed_icon_new("system-software-install");
            g_file_info_set_icon(info, icon);
            g_object_unref(icon);
        }
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN))
            g_file_info_set_is_hidden(info, FALSE);
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME))
            g_file_info_set_display_name(info, _("Applications"));
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME))
            g_file_info_set_attribute_boolean(info, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME, FALSE);
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH))
            g_file_info_set_attribute_boolean(info, G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH, FALSE);
#if 0
        /* FIXME: is this ever needed? */
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE))
            g_file_info_set_attribute_boolean(info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE, TRUE);
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_ACCESS_CAN_READ))
            g_file_info_set_attribute_boolean(info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ, TRUE);
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE))
            g_file_info_set_attribute_boolean(info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE, FALSE);
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE))
            g_file_info_set_attribute_boolean(info, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE, FALSE);
#endif
    }
    else if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_TYPE) ||
            g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_ICON) ||
            g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI) ||
            g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE) ||
            g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN) ||
            g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME))
    {
        /* retrieve matching attributes from menu-cache */
        enu.path_str = item->path;
//        enu.attributes = attributes;
//        enu.flags = flags;
        enu.cancellable = cancellable;
        enu.error = error;
        RUN_WITH_MENU_CACHE(_fm_vfs_menu_query_info_real, &enu);
        info = enu.result;
    }
    else
    {
        info = g_file_info_new();
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_STANDARD_NAME))
        {
            basename = g_path_get_basename(item->path);
            id = g_uri_unescape_string(basename, NULL);
            g_free(basename);
            g_file_info_set_name(info, id);
            g_free(id);
        }
        if(g_file_attribute_matcher_matches(matcher, G_FILE_ATTRIBUTE_ID_FILESYSTEM))
            g_file_info_set_attribute_string(info, G_FILE_ATTRIBUTE_ID_FILESYSTEM,
                                             "menu-Applications");
    }

    g_file_attribute_matcher_unref(matcher);

    return info;
}

static GFileInfo *_fm_vfs_menu_query_filesystem_info(GFile *file,
                                                     const char *attributes,
                                                     GCancellable *cancellable,
                                                     GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static GMount *_fm_vfs_menu_find_enclosing_mount(GFile *file,
                                                 GCancellable *cancellable,
                                                 GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static gboolean _fm_vfs_menu_set_display_name_real(gpointer data)
{
    FmVfsMenuMainThreadData *init = data;
    MenuCache *mc;
    MenuCacheItem *dir;
    gboolean ok = FALSE;

    mc = _get_menu_cache(init->error);
    if(mc == NULL)
        goto _mc_failed;

    dir = _vfile_path_to_menu_cache_item(mc, init->path_str);
    if(dir == NULL)
        g_set_error_literal(init->error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                            _("Invalid menu item"));
    else if (menu_cache_item_get_file_basename(dir) == NULL ||
             menu_cache_item_get_file_dirname(dir) == NULL)
        g_set_error(init->error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                    _("The menu item '%s' doesn't have appropriate entry file"),
                    menu_cache_item_get_id(dir));
    else if (!g_cancellable_set_error_if_cancelled(init->cancellable, init->error))
    {
        char *path = menu_cache_item_get_file_path(dir);
        GKeyFile *kf = g_key_file_new();

        ok = g_key_file_load_from_file(kf, path, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                                       init->error);
        g_free(path);
        if (ok)
        {
            /* get locale name */
            const gchar * const *langs = g_get_language_names();
            char *contents;
            gsize length;

            if (strcmp(langs[0], "C") != 0)
            {
                char *lang;
                /* remove encoding from locale name */
                char *sep = strchr(langs[0], '.');

                if (sep)
                    lang = g_strndup(langs[0], sep - langs[0]);
                else
                    lang = g_strdup(langs[0]);
                g_key_file_set_locale_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                             G_KEY_FILE_DESKTOP_KEY_NAME, lang,
                                             init->display_name);
                g_free(lang);
            }
            else
                g_key_file_set_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                      G_KEY_FILE_DESKTOP_KEY_NAME, init->display_name);
            contents = g_key_file_to_data(kf, &length, init->error);
            if (contents == NULL)
                ok = FALSE;
            else
            {
                path = g_build_filename(g_get_user_data_dir(),
                                        (menu_cache_item_get_type(dir) == MENU_CACHE_TYPE_DIR) ? "desktop-directories" : "applications",
                                        menu_cache_item_get_file_basename(dir), NULL);
                ok = g_file_set_contents(path, contents, length, init->error);
                /* FIXME: handle case if directory doesn't exist */
                g_free(contents);
                g_free(path);
            }
        }
        g_key_file_free(kf);
    }

#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    if(dir)
        menu_cache_item_unref(dir);
#endif
    menu_cache_unref(mc);

_mc_failed:
    return ok;
}

static GFile *_fm_vfs_menu_set_display_name(GFile *file,
                                            const char *display_name,
                                            GCancellable *cancellable,
                                            GError **error)
{
    FmMenuVFile *item = FM_MENU_VFILE(file);
    FmVfsMenuMainThreadData enu;

    /* g_debug("_fm_vfs_menu_set_display_name: %s -> %s", item->path, display_name); */
    if (item->path == NULL)
    {
        ERROR_UNSUPPORTED(error);
        return NULL;
    }
    if (display_name == NULL || *display_name == '\0')
    {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("Menu item name cannot be empty"));
        return NULL;
    }
    enu.path_str = item->path;
    enu.display_name = display_name;
    enu.cancellable = cancellable;
    enu.error = error;
    if (RUN_WITH_MENU_CACHE(_fm_vfs_menu_set_display_name_real, &enu))
        return g_object_ref(file);
    return NULL;
}

static GFileAttributeInfoList *_fm_vfs_menu_query_settable_attributes(GFile *file,
                                                                      GCancellable *cancellable,
                                                                      GError **error)
{
    return g_file_attribute_info_list_ref(_fm_vfs_menu_settable_attributes);
}

static GFileAttributeInfoList *_fm_vfs_menu_query_writable_namespaces(GFile *file,
                                                                      GCancellable *cancellable,
                                                                      GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static gboolean _fm_vfs_menu_set_attributes_from_info_real(gpointer data)
{
    FmVfsMenuMainThreadData *init = data;
    MenuCache *mc;
    MenuCacheItem *item;
    gpointer value;
    const char *display_name = NULL;
    GIcon *icon = NULL;
    gint set_hidden = -1;
    gboolean ok = FALSE;

    /* check attributes first */
    if (g_file_info_get_attribute_data(init->info, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                       NULL, &value, NULL))
        display_name = value;
    if (g_file_info_get_attribute_data(init->info, G_FILE_ATTRIBUTE_STANDARD_ICON,
                                       NULL, &value, NULL))
        icon = value;
    if (g_file_info_get_attribute_data(init->info, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN,
                                       NULL, &value, NULL))
        set_hidden = (*(gboolean *)value) ? 1 : 0;
    if (display_name == NULL && icon == NULL && set_hidden < 0)
        return TRUE; /* nothing to do */
    /* now try access item */
    mc = _get_menu_cache(init->error);
    if(mc == NULL)
        goto _mc_failed;

    item = _vfile_path_to_menu_cache_item(mc, init->path_str);
    if(item == NULL)
        g_set_error_literal(init->error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                            _("Invalid menu item"));
    else if (menu_cache_item_get_file_basename(item) == NULL ||
             menu_cache_item_get_file_dirname(item) == NULL)
        g_set_error(init->error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                    _("The menu item '%s' doesn't have appropriate entry file"),
                    menu_cache_item_get_id(item));
    else if (!g_cancellable_set_error_if_cancelled(init->cancellable, init->error))
    {
        char *path;
        GKeyFile *kf;
        GError *err = NULL;
        gboolean no_error = TRUE;

        /* for hidden on directory: use _add_directory() or _remove_directory() */
        if (set_hidden >= 0 && menu_cache_item_get_type(item) == MENU_CACHE_TYPE_DIR)
        {
#if MENU_CACHE_CHECK_VERSION(0, 5, 0)
            char *unescaped = g_uri_unescape_string(init->path_str, NULL);
            if (set_hidden > 0)
                no_error = _remove_directory(unescaped, init->cancellable, init->error);
            else
                no_error = _add_directory(unescaped, init->cancellable, init->error);
            g_free(unescaped);
            ok = no_error;
#else
            g_set_error_literal(init->error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                                _("Change hidden status isn't supported for menu directory"));
            no_error = FALSE;
#endif
            if (display_name == NULL && icon == NULL) /* nothing else to update */
                goto _done;
            set_hidden = -1; /* don't set NoDisplay for a directory */
        }
        /* in all other cases - update Name, Icon or NoDisplay and save keyfile */
        path = menu_cache_item_get_file_path(item);
        kf = g_key_file_new();
        ok = g_key_file_load_from_file(kf, path, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                                       &err);
        g_free(path);
        if (ok) /* otherwise there is no reason to continue */
        {
            char *contents;
            gsize length;

            if (display_name)
            {
                /* get locale name */
                const gchar * const *langs = g_get_language_names();

                if (strcmp(langs[0], "C") != 0)
                {
                    char *lang;
                    /* remove encoding from locale name */
                    char *sep = strchr(langs[0], '.');

                    if (sep)
                        lang = g_strndup(langs[0], sep - langs[0]);
                    else
                        lang = g_strdup(langs[0]);
                    g_key_file_set_locale_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                                 G_KEY_FILE_DESKTOP_KEY_NAME, lang,
                                                 display_name);
                    g_free(lang);
                }
                else
                    g_key_file_set_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                          G_KEY_FILE_DESKTOP_KEY_NAME, display_name);
            }
            if (icon)
            {
                char *icon_str = g_icon_to_string(icon);
                /* FIXME: need to change encoding in some cases? */
                g_key_file_set_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                      G_KEY_FILE_DESKTOP_KEY_ICON, icon_str);
                g_free(icon_str);
            }
            if (set_hidden >= 0)
            {
                g_key_file_set_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                       G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY,
                                       (set_hidden > 0));
            }
            contents = g_key_file_to_data(kf, &length, &err);
            if (contents == NULL)
                ok = FALSE;
            else
            {
                path = g_build_filename(g_get_user_data_dir(),
                                        (menu_cache_item_get_type(item) == MENU_CACHE_TYPE_DIR) ? "desktop-directories" : "applications",
                                        menu_cache_item_get_file_basename(item), NULL);
                ok = g_file_set_contents(path, contents, length, &err);
                /* FIXME: handle case if directory doesn't exist */
                g_free(contents);
                g_free(path);
            }
        }
        g_key_file_free(kf);
        if (no_error && !ok) /* we got error in err */
            g_propagate_error(init->error, err);
        else if (!ok) /* both init->error and err contain error */
            g_error_free(err);
        else if (!no_error) /* we got error in init->error */
            ok = FALSE;
    }

_done:
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    if(item)
        menu_cache_item_unref(item);
#endif
    menu_cache_unref(mc);

_mc_failed:
    return ok;
}

static gboolean _fm_vfs_menu_set_attributes_from_info(GFile *file,
                                                      GFileInfo *info,
                                                      GFileQueryInfoFlags flags,
                                                      GCancellable *cancellable,
                                                      GError **error)
{
    FmMenuVFile *item = FM_MENU_VFILE(file);
    FmVfsMenuMainThreadData enu;

    if (item->path == NULL)
    {
        ERROR_UNSUPPORTED(error);
        return FALSE;
    }
    enu.path_str = item->path;
    enu.info = info;
//    enu.flags = flags;
    enu.cancellable = cancellable;
    enu.error = error;
    return (RUN_WITH_MENU_CACHE(_fm_vfs_menu_set_attributes_from_info_real, &enu));
}

static gboolean _fm_vfs_menu_set_attribute(GFile *file,
                                           const char *attribute,
                                           GFileAttributeType type,
                                           gpointer value_p,
                                           GFileQueryInfoFlags flags,
                                           GCancellable *cancellable,
                                           GError **error)
{
    FmMenuVFile *item = FM_MENU_VFILE(file);
    GFileInfo *info;
    gboolean result;

    g_debug("_fm_vfs_menu_set_attribute: %s on %s", attribute, item->path);
    if (item->path == NULL)
    {
        ERROR_UNSUPPORTED(error);
        return FALSE;
    }
    if (value_p == NULL)
        goto _invalid_arg;
    if (strcmp(attribute, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME) == 0)
    {
        if (type != G_FILE_ATTRIBUTE_TYPE_STRING)
            goto _invalid_arg;
        info = g_file_info_new();
        g_file_info_set_display_name(info, value_p);
    }
    else if (strcmp(attribute, G_FILE_ATTRIBUTE_STANDARD_ICON) == 0)
    {
        if (type != G_FILE_ATTRIBUTE_TYPE_OBJECT)
            goto _invalid_arg;
        if (!G_IS_ICON(value_p))
            goto _invalid_arg;
        info = g_file_info_new();
        g_file_info_set_icon(info, value_p);
    }
    else if (strcmp(attribute, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN) == 0)
    {
        if (type != G_FILE_ATTRIBUTE_TYPE_BOOLEAN)
            goto _invalid_arg;
        info = g_file_info_new();
        g_file_info_set_is_hidden(info, *(gboolean *)value_p);
    }
    else
    {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                    _("Setting attribute '%s' not supported"), attribute);
        return FALSE;
    }
    result = _fm_vfs_menu_set_attributes_from_info(file, info, flags,
                                                   cancellable, error);
    g_object_unref(info);
    return result;

_invalid_arg:
    g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                _("Invalid value for attribute '%s'"), attribute);
    return FALSE;
}

static inline GFile *_g_file_new_for_id(const char *id)
{
    char *file_path;
    GFile *file;

    file_path = g_build_filename(g_get_user_data_dir(), "applications", id, NULL);
    /* we can try to guess file path and make directories but it
       hardly worth the efforts so it's easier to just make new file
       by its ID since ID is unique thru all the menu */
    if (file_path == NULL)
        return NULL;
    file = g_file_new_for_path(file_path);
    g_free(file_path);
    return file;
}

static gboolean _fm_vfs_menu_read_fn_real(gpointer data)
{
    FmVfsMenuMainThreadData *init = data;
    MenuCache *mc;
    MenuCacheItem *item = NULL;

    init->result = NULL;
    mc = _get_menu_cache(init->error);
    if(mc == NULL)
        goto _mc_failed;

    if(init->path_str)
        item = _vfile_path_to_menu_cache_item(mc, init->path_str);

        /* If item wasn't found or isn't a file then we cannot read it. */
    if(item != NULL && menu_cache_item_get_type(item) == MENU_CACHE_TYPE_DIR)
        g_set_error(init->error, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY,
                    _("The '%s' is a menu directory"), init->path_str);
    else if(item == NULL || menu_cache_item_get_type(item) != MENU_CACHE_TYPE_APP)
        g_set_error(init->error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                    _("The '%s' isn't a menu item"),
                    init->path_str ? init->path_str : "/");
    else
    {
        char *file_path;
        GFile *gf;
        GError *err = NULL;

        file_path = menu_cache_item_get_file_path(item);
        if (file_path)
        {
            gf = g_file_new_for_path(file_path);
            g_free(file_path);
            if (gf)
            {
                init->result = g_file_read(gf, init->cancellable, &err);
                if (init->result == NULL)
                {
                    /* never return G_IO_ERROR_IS_DIRECTORY */
                    if (err->domain == G_IO_ERROR &&
                        err->code == G_IO_ERROR_IS_DIRECTORY)
                    {
                        g_error_free(err);
                        g_set_error(init->error, G_IO_ERROR, G_IO_ERROR_NOT_REGULAR_FILE,
                                    _("The '%s' entry file is broken"), init->path_str);
                    }
                    else
                        g_propagate_error(init->error, err);
                }
                g_object_unref(gf);
            }
        }
    }

#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    if(item)
        menu_cache_item_unref(item);
#endif
    menu_cache_unref(mc);

_mc_failed:
    return FALSE;
}

static GFileInputStream *_fm_vfs_menu_read_fn(GFile *file,
                                              GCancellable *cancellable,
                                              GError **error)
{
    FmMenuVFile *item = FM_MENU_VFILE(file);
    FmVfsMenuMainThreadData enu;

    /* g_debug("_fm_vfs_menu_read_fn %s", item->path); */
    enu.path_str = item->path;
    enu.cancellable = cancellable;
    enu.error = error;
    RUN_WITH_MENU_CACHE(_fm_vfs_menu_read_fn_real, &enu);
    return enu.result;
}

static GFileOutputStream *_fm_vfs_menu_append_to(GFile *file,
                                                 GFileCreateFlags flags,
                                                 GCancellable *cancellable,
                                                 GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}


/* ---- FmMenuVFileOutputStream class ---- */
#define FM_TYPE_MENU_VFILE_OUTPUT_STREAM  (fm_vfs_menu_file_output_stream_get_type())
#define FM_MENU_VFILE_OUTPUT_STREAM(o)    (G_TYPE_CHECK_INSTANCE_CAST((o), \
                                           FM_TYPE_MENU_VFILE_OUTPUT_STREAM, \
                                           FmMenuVFileOutputStream))

typedef struct _FmMenuVFileOutputStream      FmMenuVFileOutputStream;
typedef struct _FmMenuVFileOutputStreamClass FmMenuVFileOutputStreamClass;

struct _FmMenuVFileOutputStream
{
    GFileOutputStream parent;
    GOutputStream *real_stream;
    gchar *path; /* "Dir/App.desktop" */
    GString *content;
    gboolean do_close;
};

struct _FmMenuVFileOutputStreamClass
{
    GFileOutputStreamClass parent_class;
};

static GType fm_vfs_menu_file_output_stream_get_type  (void);

G_DEFINE_TYPE(FmMenuVFileOutputStream, fm_vfs_menu_file_output_stream, G_TYPE_FILE_OUTPUT_STREAM);

static void fm_vfs_menu_file_output_stream_finalize(GObject *object)
{
    FmMenuVFileOutputStream *stream = FM_MENU_VFILE_OUTPUT_STREAM(object);
    if(stream->real_stream)
        g_object_unref(stream->real_stream);
    g_free(stream->path);
    g_string_free(stream->content, TRUE);
    G_OBJECT_CLASS(fm_vfs_menu_file_output_stream_parent_class)->finalize(object);
}

static gssize fm_vfs_menu_file_output_stream_write(GOutputStream *stream,
                                                   const void *buffer, gsize count,
                                                   GCancellable *cancellable,
                                                   GError **error)
{
    if (g_cancellable_set_error_if_cancelled(cancellable, error))
        return -1;
    g_string_append_len(FM_MENU_VFILE_OUTPUT_STREAM(stream)->content, buffer, count);
    return (gssize)count;
}

static gboolean fm_vfs_menu_file_output_stream_close(GOutputStream *gos,
                                                     GCancellable *cancellable,
                                                     GError **error)
{
    FmMenuVFileOutputStream *stream = FM_MENU_VFILE_OUTPUT_STREAM(gos);
    GKeyFile *kf;
    gsize len = 0;
    gchar *content;
    gboolean ok;

    if (g_cancellable_set_error_if_cancelled(cancellable, error))
        return FALSE;
    if (!stream->do_close)
        return TRUE;
    kf = g_key_file_new();
    /* parse entered file content first */
    if (stream->content->len > 0)
        g_key_file_load_from_data(kf, stream->content->str, stream->content->len,
                                  G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                                  NULL); /* FIXME: don't ignore some errors? */
    /* correct invalid data in desktop entry file: Name and Exec are mandatory,
       Type must be Application, and Category should include requested one */
    if(!g_key_file_has_key(kf, G_KEY_FILE_DESKTOP_GROUP,
                           G_KEY_FILE_DESKTOP_KEY_NAME, NULL))
        g_key_file_set_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                              G_KEY_FILE_DESKTOP_KEY_NAME, "");
    if(!g_key_file_has_key(kf, G_KEY_FILE_DESKTOP_GROUP,
                           G_KEY_FILE_DESKTOP_KEY_EXEC, NULL))
        g_key_file_set_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                              G_KEY_FILE_DESKTOP_KEY_EXEC, "");
    g_key_file_set_string(kf, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_TYPE,
                          G_KEY_FILE_DESKTOP_TYPE_APPLICATION);
    content = g_key_file_to_data(kf, &len, error);
    g_key_file_free(kf);
    if (!content)
        return FALSE;
    ok = g_output_stream_write_all(stream->real_stream, content, len, &len,
                                   cancellable, error);
    g_free(content);
    if (!ok || !g_output_stream_close(stream->real_stream, cancellable, error))
        return FALSE;
    stream->do_close = FALSE;
    if (stream->path && !_add_application(stream->path, cancellable, error))
        return FALSE;
    return TRUE;
}

static void fm_vfs_menu_file_output_stream_class_init(FmMenuVFileOutputStreamClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GOutputStreamClass *stream_class = G_OUTPUT_STREAM_CLASS(klass);

    gobject_class->finalize = fm_vfs_menu_file_output_stream_finalize;
    stream_class->write_fn = fm_vfs_menu_file_output_stream_write;
    stream_class->close_fn = fm_vfs_menu_file_output_stream_close;
    /* we don't implement seek/truncate/etag/query so no GFileOutputStream funcs */
}

static void fm_vfs_menu_file_output_stream_init(FmMenuVFileOutputStream *stream)
{
    stream->content = g_string_sized_new(1024);
    stream->do_close = TRUE;
}

static FmMenuVFileOutputStream *_fm_vfs_menu_file_output_stream_new(const gchar *path)
{
    FmMenuVFileOutputStream *stream;

    stream = g_object_new(FM_TYPE_MENU_VFILE_OUTPUT_STREAM, NULL);
    if (path)
        stream->path = g_strdup(path);
    return stream;
}

static GFileOutputStream *_vfile_menu_create(GFile *file,
                                             GFileCreateFlags flags,
                                             GCancellable *cancellable,
                                             GError **error,
                                             const gchar *path)
{
    FmMenuVFileOutputStream *stream;
    GFileOutputStream *ostream;
    GError *err = NULL;
    GFile *parent;

    if (g_cancellable_set_error_if_cancelled(cancellable, error))
        return NULL;
//    g_file_delete(file, cancellable, NULL); /* remove old if there is any */
    ostream = g_file_create(file, flags, cancellable, &err);
    if (ostream == NULL)
    {
        if (g_cancellable_is_cancelled(cancellable) ||
            err->domain != G_IO_ERROR || err->code != G_IO_ERROR_NOT_FOUND)
        {
            g_propagate_error(error, err);
            return NULL;
        }
        /* .local/share/applications/ isn't found? make it! */
        g_clear_error(&err);
        parent = g_file_get_parent(file);
        if (!g_file_make_directory_with_parents(parent, cancellable, error))
        {
            g_object_unref(parent);
            return NULL;
        }
        g_object_unref(parent);
        ostream = g_file_create(file, flags, cancellable, error);
        if (ostream == NULL)
            return ostream;
    }
    stream = _fm_vfs_menu_file_output_stream_new(path);
    stream->real_stream = G_OUTPUT_STREAM(ostream);
    return (GFileOutputStream*)stream;
}

static GFileOutputStream *_vfile_menu_replace(GFile *file,
                                              const char *etag,
                                              gboolean make_backup,
                                              GFileCreateFlags flags,
                                              GCancellable *cancellable,
                                              GError **error,
                                              const gchar *path)
{
    FmMenuVFileOutputStream *stream;
    GFileOutputStream *ostream;

    if (g_cancellable_set_error_if_cancelled(cancellable, error))
        return NULL;
    stream = _fm_vfs_menu_file_output_stream_new(path);
    ostream = g_file_replace(file, etag, make_backup, flags, cancellable, error);
    if (ostream == NULL)
    {
        g_object_unref(stream);
        return NULL;
    }
    stream->real_stream = G_OUTPUT_STREAM(ostream);
    return (GFileOutputStream*)stream;
}

static gboolean _fm_vfs_menu_create_real(gpointer data)
{
    FmVfsMenuMainThreadData *init = data;
    MenuCache *mc;
    char *unescaped = NULL, *id;
    gboolean is_invalid = TRUE;

    init->result = NULL;
    if(init->path_str)
    {
        MenuCacheItem *item;
#if !MENU_CACHE_CHECK_VERSION(0, 5, 0)
        GSList *list, *l;
#endif

        mc = _get_menu_cache(init->error);
        if(mc == NULL)
            goto _mc_failed;
        unescaped = g_uri_unescape_string(init->path_str, NULL);
        /* ensure new menu item has suffix .desktop */
        if (!g_str_has_suffix(unescaped, ".desktop"))
        {
            id = unescaped;
            unescaped = g_strconcat(unescaped, ".desktop", NULL);
            g_free(id);
        }
        id = strrchr(unescaped, '/');
        if (id)
            id++;
        else
            id = unescaped;
#if MENU_CACHE_CHECK_VERSION(0, 5, 0)
        item = menu_cache_find_item_by_id(mc, id);
        if (item)
            menu_cache_item_unref(item); /* use item simply as marker */
#else
        list = menu_cache_list_all_apps(mc);
        for (l = list; l; l = l->next)
            if (strcmp(menu_cache_item_get_id(l->data), id) == 0)
                break;
        if (l)
            item = l->data;
        else
            item = NULL;
        g_slist_free_full(list, (GDestroyNotify)menu_cache_item_unref);
#endif
        if(item == NULL)
            is_invalid = FALSE;
        /* g_debug("create id %s, category %s", id, category); */
        menu_cache_unref(mc);
    }

    if(is_invalid)
        g_set_error(init->error, G_IO_ERROR, G_IO_ERROR_EXISTS,
                    _("Cannot create menu item '%s'"),
                    init->path_str ? init->path_str : "/");
    else
    {
        GFile *gf = _g_file_new_for_id(id);

        if (gf)
        {
            init->result = _vfile_menu_create(gf, G_FILE_CREATE_NONE,
                                              init->cancellable, init->error,
                                              unescaped);
            g_object_unref(gf);
        }
    }
    g_free(unescaped);

_mc_failed:
    return FALSE;
}

static GFileOutputStream *_fm_vfs_menu_create(GFile *file,
                                              GFileCreateFlags flags,
                                              GCancellable *cancellable,
                                              GError **error)
{
    FmMenuVFile *item = FM_MENU_VFILE(file);
    FmVfsMenuMainThreadData enu;

    /* g_debug("_fm_vfs_menu_create %s", item->path); */
    enu.path_str = item->path;
    enu.cancellable = cancellable;
    enu.error = error;
    // enu.flags = flags;
    RUN_WITH_MENU_CACHE(_fm_vfs_menu_create_real, &enu);
    return enu.result;
}

static gboolean _fm_vfs_menu_replace_real(gpointer data)
{
    FmVfsMenuMainThreadData *init = data;
    MenuCache *mc;
    char *unescaped = NULL, *id;
    gboolean is_invalid = TRUE;

    init->result = NULL;
    if(init->path_str)
    {
        MenuCacheItem *item, *item2;

        mc = _get_menu_cache(init->error);
        if(mc == NULL)
            goto _mc_failed;
        /* prepare id first */
        unescaped = g_uri_unescape_string(init->path_str, NULL);
        id = strrchr(unescaped, '/');
        if (id != NULL)
            id++;
        else
            id = unescaped;
        /* get existing item */
        item = _vfile_path_to_menu_cache_item(mc, init->path_str);
        if (item != NULL) /* item is there, OK, we'll replace it then */
            is_invalid = FALSE;
        /* if not found then check item by id to exclude conflicts */
        else
        {
#if MENU_CACHE_CHECK_VERSION(0, 5, 0)
            item2 = menu_cache_find_item_by_id(mc, id);
#else
            GSList *list = menu_cache_list_all_apps(mc), *l;
            for (l = list; l; l = l->next)
                if (strcmp(menu_cache_item_get_id(l->data), id) == 0)
                    break;
            if (l)
                item2 = menu_cache_item_ref(l->data);
            else
                item2 = NULL;
            g_slist_free_full(list, (GDestroyNotify)menu_cache_item_unref);
#endif
            if(item2 == NULL)
                is_invalid = FALSE;
            else /* item was found in another category */
                menu_cache_item_unref(item2);
        }
        menu_cache_unref(mc);
    }

    if(is_invalid)
        g_set_error(init->error, G_IO_ERROR, G_IO_ERROR_EXISTS,
                    _("Cannot create menu item '%s'"),
                    init->path_str ? init->path_str : "/");
    else
    {
        GFile *gf = _g_file_new_for_id(id);

        if (gf)
        {
            /* FIXME: use flags and make_backup */
            init->result = _vfile_menu_replace(gf, NULL, FALSE,
                                               G_FILE_CREATE_REPLACE_DESTINATION,
                                               init->cancellable, init->error,
                                               /* don't insert it into XML */
                                               NULL);
            g_object_unref(gf);
        }
    }
    g_free(unescaped);

_mc_failed:
    return FALSE;
}

static GFileOutputStream *_fm_vfs_menu_replace(GFile *file,
                                               const char *etag,
                                               gboolean make_backup,
                                               GFileCreateFlags flags,
                                               GCancellable *cancellable,
                                               GError **error)
{
    FmMenuVFile *item = FM_MENU_VFILE(file);
    FmVfsMenuMainThreadData enu;

    /* g_debug("_fm_vfs_menu_replace %s", item->path); */
    enu.path_str = item->path;
    enu.cancellable = cancellable;
    enu.error = error;
    // enu.flags = flags;
    // enu.make_backup = make_backup;
    RUN_WITH_MENU_CACHE(_fm_vfs_menu_replace_real, &enu);
    return enu.result;
}

/* not in main thread; returns NULL on failure */
static GKeyFile *_g_key_file_from_item(GFile *file, GCancellable *cancellable,
                                       GError **error)
{
    char *contents;
    gsize length;
    GKeyFile *kf;

    if (!g_file_load_contents(file, cancellable, &contents, &length, NULL, error))
        return NULL;
    kf = g_key_file_new();
    if (!g_key_file_load_from_data(kf, contents, length,
                                   G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                                   error))
    {
        g_key_file_free(kf);
        kf = NULL;
    }
    g_free(contents);
    return kf;
}

/* not in main thread; returns FALSE on failure, consumes key file */
static gboolean _g_key_file_into_item(GFile *file, GKeyFile *kf,
                                      GCancellable *cancellable, GError **error)
{
    char *contents;
    gsize length;
    gboolean result = FALSE;

    contents = g_key_file_to_data(kf, &length, error);
    g_key_file_free(kf);
    if (contents == NULL)
        return FALSE;
    result = g_file_replace_contents(file, contents, length, NULL, FALSE,
                                     G_FILE_CREATE_REPLACE_DESTINATION, NULL,
                                     cancellable, error);
    g_free(contents);
    return result;
}

static gboolean _fm_vfs_menu_delete_file(GFile *file,
                                         GCancellable *cancellable,
                                         GError **error)
{
    FmMenuVFile *item = FM_MENU_VFILE(file);
    GKeyFile *kf;
    GError *err = NULL;

    g_debug("_fm_vfs_menu_delete_file %s", item->path);
    /* load contents */
    kf = _g_key_file_from_item(file, cancellable, &err);
    if (kf == NULL)
    {
#if MENU_CACHE_CHECK_VERSION(0, 5, 0)
        /* it might be just a directory */
        if (err->domain == G_IO_ERROR && err->code == G_IO_ERROR_IS_DIRECTORY)
        {
            char *unescaped = g_uri_unescape_string(item->path, NULL);
            gboolean ok = _remove_directory(unescaped, cancellable, error);
            g_error_free(err);
            g_free(unescaped);
            return ok;
        }
        /* else it just failed */
#endif
        g_propagate_error(error, err);
        return FALSE;
    }
    /* set NoDisplay=true and save */
    g_key_file_set_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                           G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY, TRUE);
    return _g_key_file_into_item(file, kf, cancellable, error);
}

static gboolean _fm_vfs_menu_trash(GFile *file,
                                   GCancellable *cancellable,
                                   GError **error)
{
    ERROR_UNSUPPORTED(error);
    return FALSE;
}

static gboolean _fm_vfs_menu_make_directory(GFile *file,
                                            GCancellable *cancellable,
                                            GError **error)
{
#if !MENU_CACHE_CHECK_VERSION(0, 5, 0)
    /* creating a directory with libmenu-cache < 0.5.0 will lead to invisible
       directory; inexperienced user will be confused; therefore we disable
       such operation in such conditions */
    ERROR_UNSUPPORTED(error);
    return FALSE;
#else
    FmMenuVFile *item = FM_MENU_VFILE(file);
    char *unescaped;
    gboolean ok;

    /* XDG desktop menu specification: desktop-entry-id should be *.desktop */
    if (g_str_has_suffix(item->path, ".desktop"))
    {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME,
                            _("Name of menu directory should not end with"
                              " \".desktop\""));
        return FALSE;
    }
    unescaped = g_uri_unescape_string(item->path, NULL);
    ok = _add_directory(unescaped, cancellable, error);
    g_free(unescaped);
    return ok;
#endif
}

static gboolean _fm_vfs_menu_make_symbolic_link(GFile *file,
                                                const char *symlink_value,
                                                GCancellable *cancellable,
                                                GError **error)
{
    ERROR_UNSUPPORTED(error);
    return FALSE;
}

static gboolean _fm_vfs_menu_copy(GFile *source,
                                  GFile *destination,
                                  GFileCopyFlags flags,
                                  GCancellable *cancellable,
                                  GFileProgressCallback progress_callback,
                                  gpointer progress_callback_data,
                                  GError **error)
{
    ERROR_UNSUPPORTED(error);
    return FALSE;
}

static gboolean _fm_vfs_menu_move_real(gpointer data)
{
    FmVfsMenuMainThreadData *init = data;
    MenuCache *mc = NULL;
    MenuCacheItem *item = NULL, *item2;
    char *src_path, *dst_path;
    char *src_id, *dst_id;
    gboolean result = FALSE;

    dst_path = init->destination->path;
    if (init->path_str == NULL || dst_path == NULL)
    {
        g_set_error_literal(init->error, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("Invalid operation with menu root"));
        return FALSE;
    }
    /* make path strings */
    src_path = g_uri_unescape_string(init->path_str, NULL);
    dst_path = g_uri_unescape_string(dst_path, NULL);
    src_id = strrchr(src_path, '/');
    if (src_id)
        src_id++;
    else
        src_id = src_path;
    dst_id = strrchr(dst_path, '/');
    if (dst_id)
        dst_id++;
    else
        dst_id = dst_path;
    if (strcmp(src_id, dst_id))
    {
        /* ID change isn't supported now */
        ERROR_UNSUPPORTED(init->error);
        goto _failed;
    }
    if (strcmp(src_path, dst_path) == 0)
    {
        g_warning("menu: tried to move '%s' into itself", src_path);
        g_free(src_path);
        g_free(dst_path);
        return TRUE; /* nothing was changed */
    }
    /* do actual move */
    mc = _get_menu_cache(init->error);
    if(mc == NULL)
        goto _failed;
    item = _vfile_path_to_menu_cache_item(mc, init->path_str);
    /* TODO: if id changed then check for ID conflicts */
    /* TODO: save updated desktop entry for old ID (if different) */
    if(item == NULL || menu_cache_item_get_type(item) != MENU_CACHE_TYPE_APP)
    {
        /* FIXME: implement directories movement */
        g_set_error(init->error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                    _("The '%s' isn't a menu item"), init->path_str);
        goto _failed;
    }
    item2 = _vfile_path_to_menu_cache_item(mc, init->destination->path);
    if (item2)
    {
        g_set_error(init->error, G_IO_ERROR, G_IO_ERROR_EXISTS,
                    _("Menu path '%s' already exists"), dst_path);
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
        menu_cache_item_unref(item2);
#endif
        goto _failed;
    }
    /* do actual move */
    if (_add_application(dst_path, init->cancellable, init->error))
    {
        if (_remove_application(src_path, init->cancellable, init->error))
            result = TRUE;
        else /* failed, rollback */
            _remove_application(dst_path, init->cancellable, NULL);
    }

_failed:
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    if(item)
        menu_cache_item_unref(item);
#endif
    if(mc)
        menu_cache_unref(mc);
    g_free(src_path);
    g_free(dst_path);
    return result;
}

static gboolean _fm_vfs_menu_move(GFile *source,
                                  GFile *destination,
                                  GFileCopyFlags flags,
                                  GCancellable *cancellable,
                                  GFileProgressCallback progress_callback,
                                  gpointer progress_callback_data,
                                  GError **error)
{
    FmMenuVFile *item = FM_MENU_VFILE(source);
    FmVfsMenuMainThreadData enu;

    /* g_debug("_fm_vfs_menu_move"); */
    if(!FM_IS_FILE(destination))
    {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                            _("Invalid destination"));
        return FALSE;
    }
    enu.path_str = item->path;
    enu.cancellable = cancellable;
    enu.error = error;
    // enu.flags = flags;
    enu.destination = FM_MENU_VFILE(destination);
    /* FIXME: use progress_callback */
    return RUN_WITH_MENU_CACHE(_fm_vfs_menu_move_real, &enu);
}

/* ---- FmMenuVFileMonitor class ---- */
#define FM_TYPE_MENU_VFILE_MONITOR     (fm_vfs_menu_file_monitor_get_type())
#define FM_MENU_VFILE_MONITOR(o)       (G_TYPE_CHECK_INSTANCE_CAST((o), \
                                        FM_TYPE_MENU_VFILE_MONITOR, FmMenuVFileMonitor))

typedef struct _FmMenuVFileMonitor      FmMenuVFileMonitor;
typedef struct _FmMenuVFileMonitorClass FmMenuVFileMonitorClass;

static GType fm_vfs_menu_file_monitor_get_type  (void);

struct _FmMenuVFileMonitor
{
    GFileMonitor parent_object;

    FmMenuVFile *file;
    MenuCache *cache;
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    MenuCacheItem *item;
    MenuCacheNotifyId notifier;
#else
    GSList *items;
    gboolean stopped;
    gpointer notifier;
#endif
};

struct _FmMenuVFileMonitorClass
{
    GFileMonitorClass parent_class;
};

G_DEFINE_TYPE(FmMenuVFileMonitor, fm_vfs_menu_file_monitor, G_TYPE_FILE_MONITOR);

static void fm_vfs_menu_file_monitor_finalize(GObject *object)
{
    FmMenuVFileMonitor *mon = FM_MENU_VFILE_MONITOR(object);

    if(mon->cache)
    {
        if(mon->notifier)
            menu_cache_remove_reload_notify(mon->cache, mon->notifier);
        menu_cache_unref(mon->cache);
    }
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    if(mon->item)
        menu_cache_item_unref(mon->item);
#else
    g_slist_free_full(mon->items, (GDestroyNotify)menu_cache_item_unref);
#endif
    g_object_unref(mon->file);

    G_OBJECT_CLASS(fm_vfs_menu_file_monitor_parent_class)->finalize(object);
}

static gboolean fm_vfs_menu_file_monitor_cancel(GFileMonitor *monitor)
{
    FmMenuVFileMonitor *mon = FM_MENU_VFILE_MONITOR(monitor);

#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    if(mon->item)
        menu_cache_item_unref(mon->item); /* rest will be done in finalizer */
    mon->item = NULL;
#else
    mon->stopped = TRUE;
#endif
    return TRUE;
}

static void fm_vfs_menu_file_monitor_class_init(FmMenuVFileMonitorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GFileMonitorClass *gfilemon_class = G_FILE_MONITOR_CLASS (klass);

    gobject_class->finalize = fm_vfs_menu_file_monitor_finalize;
    gfilemon_class->cancel = fm_vfs_menu_file_monitor_cancel;
}

static void fm_vfs_menu_file_monitor_init(FmMenuVFileMonitor *item)
{
    /* nothing */
}

static FmMenuVFileMonitor *_fm_menu_vfile_monitor_new(void)
{
    return (FmMenuVFileMonitor*)g_object_new(FM_TYPE_MENU_VFILE_MONITOR, NULL);
}

#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
static void _reload_notify_handler(MenuCache* cache, gpointer user_data)
#else
static void _reload_notify_handler(gpointer cache, gpointer user_data)
#endif
{
    FmMenuVFileMonitor *mon = FM_MENU_VFILE_MONITOR(user_data);
    GSList *items, *new_items, *ol, *nl;
    MenuCacheItem *dir;
    GFile *file;
    const char *de_name;
    guint32 de_flag;

#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    if(mon->item == NULL) /* menu folder was destroyed or monitor cancelled */
        return;
    dir = mon->item;
    if(mon->file->path)
        mon->item = _vfile_path_to_menu_cache_item(cache, mon->file->path);
    else
        mon->item = MENU_CACHE_ITEM(menu_cache_dup_root_dir(cache));
    if(mon->item && menu_cache_item_get_type(mon->item) != MENU_CACHE_TYPE_DIR)
    {
        menu_cache_item_unref(mon->item);
        mon->item = NULL;
    }
    if(mon->item == NULL) /* folder was destroyed - emit event and exit */
    {
        menu_cache_item_unref(dir);
        g_file_monitor_emit_event(G_FILE_MONITOR(mon), G_FILE(mon->file), NULL,
                                  G_FILE_MONITOR_EVENT_DELETED);
        return;
    }
    items = menu_cache_dir_list_children(MENU_CACHE_DIR(dir));
    menu_cache_item_unref(dir);
    new_items = menu_cache_dir_list_children(MENU_CACHE_DIR(mon->item));
#else
    if(mon->stopped) /* menu folder was destroyed or monitor cancelled */
        return;
    if(mon->file->path)
        dir = _vfile_path_to_menu_cache_item(cache, mon->file->path);
    else
        dir = MENU_CACHE_ITEM(menu_cache_get_root_dir(cache));
    if(dir == NULL) /* folder was destroyed - emit event and exit */
    {
        mon->stopped = TRUE;
        g_file_monitor_emit_event(G_FILE_MONITOR(mon), G_FILE(mon->file), NULL,
                                  G_FILE_MONITOR_EVENT_DELETED);
        return;
    }
    /* emit change on the folder in any case */
    g_file_monitor_emit_event(G_FILE_MONITOR(mon), G_FILE(mon->file), NULL,
                              G_FILE_MONITOR_EVENT_CHANGED);
    items = mon->items;
    mon->items = g_slist_copy_deep(menu_cache_dir_get_children(MENU_CACHE_DIR(dir)),
                                   (GCopyFunc)menu_cache_item_ref, NULL);
    new_items = g_slist_copy_deep(mon->items, (GCopyFunc)menu_cache_item_ref, NULL);
#endif
    for (ol = items; ol; ) /* remove all separatorts first */
    {
        nl = ol->next;
        if (menu_cache_item_get_id(ol->data) == NULL)
        {
            menu_cache_item_unref(ol->data);
            items = g_slist_delete_link(items, ol);
        }
        ol = nl;
    }
    for (ol = new_items; ol; )
    {
        nl = ol->next;
        if (menu_cache_item_get_id(ol->data) == NULL)
        {
            menu_cache_item_unref(ol->data);
            new_items = g_slist_delete_link(new_items, ol);
        }
        ol = nl;
    }
    /* we have two copies of lists now, compare them and emit events */
    ol = items;
    de_name = g_getenv("XDG_CURRENT_DESKTOP");
    if(de_name)
        de_flag = menu_cache_get_desktop_env_flag(cache, de_name);
    else
        de_flag = (guint32)-1;
    while (ol)
    {
        for (nl = new_items; nl; nl = nl->next)
            if (strcmp(menu_cache_item_get_id(ol->data),
                       menu_cache_item_get_id(nl->data)) == 0)
                break; /* the same id found */
        if (nl)
        {
            /* check if any visible attribute of it was changed */
            if (g_strcmp0(menu_cache_item_get_name(ol->data),
                          menu_cache_item_get_name(nl->data)) == 0 ||
                g_strcmp0(menu_cache_item_get_icon(ol->data),
                          menu_cache_item_get_icon(nl->data)) == 0 ||
                menu_cache_app_get_is_visible(ol->data, de_flag) !=
                                menu_cache_app_get_is_visible(nl->data, de_flag))
            {
                file = _fm_vfs_menu_resolve_relative_path(G_FILE(mon->file),
                                             menu_cache_item_get_id(nl->data));
                g_file_monitor_emit_event(G_FILE_MONITOR(mon), file, NULL,
                                          G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED);
                g_object_unref(file);
            }
            /* free both new and old from the list */
            menu_cache_item_unref(nl->data);
            new_items = g_slist_delete_link(new_items, nl);
            nl = ol->next; /* use 'nl' as storage */
            menu_cache_item_unref(ol->data);
            items = g_slist_delete_link(items, ol);
            ol = nl;
        }
        else /* id not found (removed), go to next */
            ol = ol->next;
    }
    /* emit events for removed files */
    while (items)
    {
        file = _fm_vfs_menu_resolve_relative_path(G_FILE(mon->file),
                                             menu_cache_item_get_id(items->data));
        g_file_monitor_emit_event(G_FILE_MONITOR(mon), file, NULL,
                                  G_FILE_MONITOR_EVENT_DELETED);
        g_object_unref(file);
        menu_cache_item_unref(items->data);
        items = g_slist_delete_link(items, items);
    }
    /* emit events for added files */
    while (new_items)
    {
        file = _fm_vfs_menu_resolve_relative_path(G_FILE(mon->file),
                                     menu_cache_item_get_id(new_items->data));
        g_file_monitor_emit_event(G_FILE_MONITOR(mon), file, NULL,
                                  G_FILE_MONITOR_EVENT_CREATED);
        g_object_unref(file);
        menu_cache_item_unref(new_items->data);
        new_items = g_slist_delete_link(new_items, new_items);
    }
}

static gboolean _fm_vfs_menu_monitor_dir_real(gpointer data)
{
    FmVfsMenuMainThreadData *init = data;
    FmMenuVFileMonitor *mon;
#if !MENU_CACHE_CHECK_VERSION(0, 4, 0)
    MenuCacheItem *dir;
#endif

    init->result = NULL;
    if(g_cancellable_set_error_if_cancelled(init->cancellable, init->error))
        return FALSE;
    /* open menu cache instance */
    mon = _fm_menu_vfile_monitor_new();
    if(mon == NULL) /* out of memory! */
        return FALSE;
    mon->file = FM_MENU_VFILE(g_object_ref(init->destination));
    mon->cache = _get_menu_cache(init->error);
    if(mon->cache == NULL)
        goto _fail;
    /* check if requested path exists within cache */
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    if(mon->file->path)
        mon->item = _vfile_path_to_menu_cache_item(mon->cache, mon->file->path);
    else
        mon->item = MENU_CACHE_ITEM(menu_cache_dup_root_dir(mon->cache));
    if(mon->item == NULL || menu_cache_item_get_type(mon->item) != MENU_CACHE_TYPE_DIR)
#else
    if(mon->file->path)
        dir = _vfile_path_to_menu_cache_item(mon->cache, mon->file->path);
    else
        dir = MENU_CACHE_ITEM(menu_cache_get_root_dir(mon->cache));
    if(dir == NULL)
#endif
    {
        g_set_error(init->error, G_IO_ERROR, G_IO_ERROR_FAILED,
                    _("FmMenuVFileMonitor: folder '%s' not found in menu cache"),
                    mon->file->path);
        goto _fail;
    }
#if !MENU_CACHE_CHECK_VERSION(0, 4, 0)
    /* for old libmenu-cache we have no choice but copy all the data right now */
    mon->items = g_slist_copy_deep(menu_cache_dir_get_children(MENU_CACHE_DIR(dir)),
                                   (GCopyFunc)menu_cache_item_ref, NULL);
#endif
    if(g_cancellable_set_error_if_cancelled(init->cancellable, init->error))
        goto _fail;
    /* current directory contents belong to mon->item now */
    /* attach reload notify handler */
    mon->notifier = menu_cache_add_reload_notify(mon->cache,
                                                 &_reload_notify_handler, mon);
    init->result = mon;
    return TRUE;

_fail:
    g_object_unref(mon);
    return FALSE;
}

static GFileMonitor *_fm_vfs_menu_monitor_dir(GFile *file,
                                              GFileMonitorFlags flags,
                                              GCancellable *cancellable,
                                              GError **error)
{
    FmVfsMenuMainThreadData enu;

    /* g_debug("_fm_vfs_menu_monitor_dir %s", FM_MENU_VFILE(file)->path); */
    enu.cancellable = cancellable;
    enu.error = error;
    // enu.flags = flags;
    enu.destination = FM_MENU_VFILE(file);
    RUN_WITH_MENU_CACHE(_fm_vfs_menu_monitor_dir_real, &enu);
    return (GFileMonitor*)enu.result;
}

static GFileMonitor *_fm_vfs_menu_monitor_file(GFile *file,
                                               GFileMonitorFlags flags,
                                               GCancellable *cancellable,
                                               GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

#if GLIB_CHECK_VERSION(2, 22, 0)
static GFileIOStream *_fm_vfs_menu_open_readwrite(GFile *file,
                                                  GCancellable *cancellable,
                                                  GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static GFileIOStream *_fm_vfs_menu_create_readwrite(GFile *file,
                                                    GFileCreateFlags flags,
                                                    GCancellable *cancellable,
                                                    GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}

static GFileIOStream *_fm_vfs_menu_replace_readwrite(GFile *file,
                                                     const char *etag,
                                                     gboolean make_backup,
                                                     GFileCreateFlags flags,
                                                     GCancellable *cancellable,
                                                     GError **error)
{
    ERROR_UNSUPPORTED(error);
    return NULL;
}
#endif /* Glib >= 2.22 */

static void fm_menu_g_file_init(GFileIface *iface)
{
    GFileAttributeInfoList *list;

    iface->dup = _fm_vfs_menu_dup;
    iface->hash = _fm_vfs_menu_hash;
    iface->equal = _fm_vfs_menu_equal;
    iface->is_native = _fm_vfs_menu_is_native;
    iface->has_uri_scheme = _fm_vfs_menu_has_uri_scheme;
    iface->get_uri_scheme = _fm_vfs_menu_get_uri_scheme;
    iface->get_basename = _fm_vfs_menu_get_basename;
    iface->get_path = _fm_vfs_menu_get_path;
    iface->get_uri = _fm_vfs_menu_get_uri;
    iface->get_parse_name = _fm_vfs_menu_get_parse_name;
    iface->get_parent = _fm_vfs_menu_get_parent;
    iface->prefix_matches = _fm_vfs_menu_prefix_matches;
    iface->get_relative_path = _fm_vfs_menu_get_relative_path;
    iface->resolve_relative_path = _fm_vfs_menu_resolve_relative_path;
    iface->get_child_for_display_name = _fm_vfs_menu_get_child_for_display_name;
    iface->enumerate_children = _fm_vfs_menu_enumerate_children;
    iface->query_info = _fm_vfs_menu_query_info;
    iface->query_filesystem_info = _fm_vfs_menu_query_filesystem_info;
    iface->find_enclosing_mount = _fm_vfs_menu_find_enclosing_mount;
    iface->set_display_name = _fm_vfs_menu_set_display_name;
    iface->query_settable_attributes = _fm_vfs_menu_query_settable_attributes;
    iface->query_writable_namespaces = _fm_vfs_menu_query_writable_namespaces;
    iface->set_attribute = _fm_vfs_menu_set_attribute;
    iface->set_attributes_from_info = _fm_vfs_menu_set_attributes_from_info;
    iface->read_fn = _fm_vfs_menu_read_fn;
    iface->append_to = _fm_vfs_menu_append_to;
    iface->create = _fm_vfs_menu_create;
    iface->replace = _fm_vfs_menu_replace;
    iface->delete_file = _fm_vfs_menu_delete_file;
    iface->trash = _fm_vfs_menu_trash;
    iface->make_directory = _fm_vfs_menu_make_directory;
    iface->make_symbolic_link = _fm_vfs_menu_make_symbolic_link;
    iface->copy = _fm_vfs_menu_copy;
    iface->move = _fm_vfs_menu_move;
    iface->monitor_dir = _fm_vfs_menu_monitor_dir;
    iface->monitor_file = _fm_vfs_menu_monitor_file;
#if GLIB_CHECK_VERSION(2, 22, 0)
    iface->open_readwrite = _fm_vfs_menu_open_readwrite;
    iface->create_readwrite = _fm_vfs_menu_create_readwrite;
    iface->replace_readwrite = _fm_vfs_menu_replace_readwrite;
    iface->supports_thread_contexts = TRUE;
#endif /* Glib >= 2.22 */

    list = g_file_attribute_info_list_new();
    g_file_attribute_info_list_add(list, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN,
                                   G_FILE_ATTRIBUTE_TYPE_BOOLEAN,
                                   G_FILE_ATTRIBUTE_INFO_COPY_WHEN_MOVED);
    g_file_attribute_info_list_add(list, G_FILE_ATTRIBUTE_STANDARD_ICON,
                                   G_FILE_ATTRIBUTE_TYPE_OBJECT,
                                   G_FILE_ATTRIBUTE_INFO_COPY_WHEN_MOVED);
    _fm_vfs_menu_settable_attributes = list;
}


/* ---- FmFile implementation ---- */
static gboolean _fm_vfs_menu_wants_incremental(GFile* file)
{
    return FALSE;
}

static void fm_menu_fm_file_init(FmFileInterface *iface)
{
    iface->wants_incremental = _fm_vfs_menu_wants_incremental;
}


/* ---- interface for loading ---- */
static GFile *_fm_vfs_menu_new_for_uri(const char *uri)
{
    FmMenuVFile *item = _fm_menu_vfile_new();

    if(uri == NULL)
        uri = "";
    /* skip menu:/ */
    if(g_ascii_strncasecmp(uri, "menu:", 5) == 0)
        uri += 5;
    while(*uri == '/')
        uri++;
    /* skip "applications/" or "applications.menu/" */
    if(g_ascii_strncasecmp(uri, "applications", 12) == 0)
    {
        uri += 12;
        if(g_ascii_strncasecmp(uri, ".menu", 5) == 0)
            uri += 5;
    }
    while(*uri == '/') /* skip starting slashes */
        uri++;
    /* save the rest of path, NULL means the root path */
    if(*uri)
    {
        char *end;

        item->path = g_strdup(uri);
        for(end = item->path + strlen(item->path); end > item->path; end--)
            if(end[-1] == '/') /* skip trailing slashes */
                end[-1] = '\0';
            else
                break;
    }
    /* g_debug("_fm_vfs_menu_new_for_uri %s -> %s", uri, item->path); */
    return (GFile*)item;
}

FM_DEFINE_MODULE(vfs, menu)

FmFileInitTable fm_module_init_vfs =
{
    .new_for_uri = &_fm_vfs_menu_new_for_uri
};
