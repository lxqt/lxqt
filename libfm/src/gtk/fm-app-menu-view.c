/*
 *      fm-app-menu-view.c
 *
 *      Copyright 2010 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2013-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

/**
 * SECTION:fm-app-menu-view
 * @short_description: Applications tree for application selection dialogs.
 * @title: Application chooser tree
 *
 * @include: libfm/fm-gtk.h
 *
 * The widget to represent known applications as a tree.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "../glib-compat.h"
#include "fm-app-menu-view.h"
#include "fm-icon.h"
#include <menu-cache.h>
#include <glib/gi18n-lib.h>
#include <gio/gdesktopappinfo.h>
#include <string.h>

/* support for libmenu-cache 0.4.x */
#ifndef MENU_CACHE_CHECK_VERSION
# ifdef HAVE_MENU_CACHE_DIR_LIST_CHILDREN
#  define MENU_CACHE_CHECK_VERSION(_a,_b,_c) (_a == 0 && _b < 5) /* < 0.5.0 */
# else
#  define MENU_CACHE_CHECK_VERSION(_a,_b,_c) 0 /* not even 0.4.0 */
# endif
#endif

enum
{
    COL_ICON,
    COL_TITLE,
    COL_ITEM,
    N_COLS
};

static GtkTreeStore* store = NULL;
static MenuCache* menu_cache = NULL;
static gpointer menu_cache_reload_notify = NULL;

static void destroy_store(gpointer user_data, GObject *obj)
{
    menu_cache_remove_reload_notify(menu_cache, menu_cache_reload_notify);
    menu_cache_reload_notify = NULL;
    menu_cache_unref(menu_cache);
    menu_cache = NULL;
    store = NULL;
}

/* called with lock held */
static void add_menu_items(GtkTreeIter* parent_it, MenuCacheDir* dir)
{
    GtkTreeIter it;
    GSList * l;
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    GSList *list;
#endif
    GIcon* gicon;
    /* Iterate over all menu items in this directory. */
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    for (l = list = menu_cache_dir_list_children(dir); l != NULL; l = l->next)
#else
    for (l = menu_cache_dir_get_children(dir); l != NULL; l = l->next)
#endif
    {
        /* Get the menu item. */
        MenuCacheItem* item = MENU_CACHE_ITEM(l->data);
        switch(menu_cache_item_get_type(item))
        {
            case MENU_CACHE_TYPE_NONE:
            case MENU_CACHE_TYPE_SEP:
                break;
            case MENU_CACHE_TYPE_APP:
            case MENU_CACHE_TYPE_DIR:
                if(menu_cache_item_get_icon(item))
                    gicon = G_ICON(fm_icon_from_name(menu_cache_item_get_icon(item)));
                else
                    gicon = NULL;
                gtk_tree_store_append(store, &it, parent_it);
                gtk_tree_store_set(store, &it,
                                   COL_ICON, gicon,
                                   COL_TITLE, menu_cache_item_get_name(item),
                                   COL_ITEM, item, -1);
                if(gicon)
                    g_object_unref(gicon);

                if(menu_cache_item_get_type(item) == MENU_CACHE_TYPE_DIR)
                    add_menu_items(&it, MENU_CACHE_DIR(item));
                break;
        }
    }
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    g_slist_free_full(list, (GDestroyNotify)menu_cache_item_unref);
#endif
}

#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
static void on_menu_cache_reload(MenuCache* mc, gpointer user_data)
#else
static void on_menu_cache_reload(gpointer mc, gpointer user_data)
#endif
{
    g_return_if_fail(store);
    gtk_tree_store_clear(store);
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
    MenuCacheDir* dir = menu_cache_dup_root_dir(mc);
#else
    MenuCacheDir* dir = menu_cache_get_root_dir(menu_cache);
#endif
    /* FIXME: preserve original selection */
    if(dir)
    {
        add_menu_items(NULL, dir);
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
        menu_cache_item_unref(MENU_CACHE_ITEM(dir));
#endif
    }
}

/**
 * fm_app_menu_view_new
 *
 * Creates new application tree widget.
 *
 * Returns: (transfer full): a new widget.
 *
 * Since: 0.1.0
 */
GtkTreeView *fm_app_menu_view_new(void)
{
    GtkTreeView* view;
    GtkTreeViewColumn* col;
    GtkCellRenderer* render;

    if(!store)
    {
        static GType menu_cache_item_type = 0;
        char* oldenv;
        if(G_UNLIKELY(!menu_cache_item_type))
            menu_cache_item_type = g_boxed_type_register_static("MenuCacheItem",
                                            (GBoxedCopyFunc)menu_cache_item_ref,
                                            (GBoxedFreeFunc)menu_cache_item_unref);
        store = gtk_tree_store_new(N_COLS, G_TYPE_ICON, /*GDK_TYPE_PIXBUF, */G_TYPE_STRING, menu_cache_item_type);
        g_object_weak_ref(G_OBJECT(store), destroy_store, NULL);

        /* ensure that we're using lxmenu-data */
        oldenv = g_strdup(g_getenv("XDG_MENU_PREFIX"));
        g_setenv("XDG_MENU_PREFIX", "lxde-", TRUE);
        menu_cache = menu_cache_lookup("applications.menu");
        if(oldenv)
        {
            g_setenv("XDG_MENU_PREFIX", oldenv, TRUE);
            g_free(oldenv);
        }
        else
            g_unsetenv("XDG_MENU_PREFIX");

        if(menu_cache)
        {
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
            MenuCacheDir* dir = menu_cache_dup_root_dir(menu_cache);
#else
            MenuCacheDir* dir = menu_cache_get_root_dir(menu_cache);
#endif
            menu_cache_reload_notify = menu_cache_add_reload_notify(menu_cache, on_menu_cache_reload, NULL);
            if(dir) /* content of menu is already loaded */
            {
                add_menu_items(NULL, dir);
#if MENU_CACHE_CHECK_VERSION(0, 4, 0)
                menu_cache_item_unref(MENU_CACHE_ITEM(dir));
#endif
            }
        }
    }
    else
        g_object_ref(store);

    view = (GtkTreeView*)gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

    render = gtk_cell_renderer_pixbuf_new();
    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, _("Installed Applications"));
    gtk_tree_view_column_pack_start(col, render, FALSE);
    gtk_tree_view_column_set_attributes(col, render, "gicon", COL_ICON, NULL);

    render = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, render, TRUE);
    gtk_tree_view_column_set_attributes(col, render, "text", COL_TITLE, NULL);

    gtk_tree_view_append_column(view, col);

    g_object_unref(store);
    return view;
}

/**
 * fm_app_menu_view_dup_selected_app
 * @view: a widget
 *
 * Retrieves selected application from the widget.
 * The returned data should be freed with g_object_unref() after usage.
 *
 * Before 1.0.0 this call had name fm_app_menu_view_get_selected_app.
 *
 * Returns: (transfer full): selected application descriptor.
 *
 * Since: 0.1.0
 */
GAppInfo* fm_app_menu_view_dup_selected_app(GtkTreeView* view)
{
    char* id = fm_app_menu_view_dup_selected_app_desktop_id(view);
    if(id)
    {
        GDesktopAppInfo* app = g_desktop_app_info_new(id);
        g_free(id);
        return G_APP_INFO(app);
    }
    return NULL;
}

/**
 * fm_app_menu_view_dup_selected_app_desktop_id
 * @view: a widget
 *
 * Retrieves name of selected application from the widget.
 * The returned data should be freed with g_free() after usage.
 *
 * Before 1.0.0 this call had name fm_app_menu_view_get_selected_app_desktop_id.
 *
 * Returns: (transfer full): selected application name.
 *
 * Since: 0.1.0
 */
char* fm_app_menu_view_dup_selected_app_desktop_id(GtkTreeView* view)
{
    GtkTreeIter it;
    GtkTreeSelection* sel = gtk_tree_view_get_selection(view);
    /* FIXME: this should be checked if it's exactly app menu tree! */
    char* id = NULL;
    if(gtk_tree_selection_get_selected(sel, NULL, &it))
    {
        MenuCacheItem* item;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &it, COL_ITEM, &item, -1);
        if(item && menu_cache_item_get_type(item) == MENU_CACHE_TYPE_APP)
            id = g_strdup(menu_cache_item_get_id(item));
    }
    return id;
}

/**
 * fm_app_menu_view_dup_selected_app_desktop_file_path
 * @view: a widget
 *
 * Retrieves file path to selected application from the widget.
 * The returned data should be freed with g_free() after usage.
 *
 * Before 1.0.0 this call had name fm_app_menu_view_get_selected_app_desktop_file.
 *
 * Returns: (transfer full): path to selected application file.
 *
 * Since: 0.1.0
 */
char* fm_app_menu_view_dup_selected_app_desktop_file_path(GtkTreeView* view)
{
    GtkTreeIter it;
    GtkTreeSelection* sel = gtk_tree_view_get_selection(view);
    /* FIXME: this should be checked if it's exactly app menu tree! */
    if(gtk_tree_selection_get_selected(sel, NULL, &it))
    {
        MenuCacheItem* item;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &it, COL_ITEM, &item, -1);
        if(item && menu_cache_item_get_type(item) == MENU_CACHE_TYPE_APP)
        {
            char* path = menu_cache_item_get_file_path(item);
            return path;
        }
    }
    return NULL;
}

/**
 * fm_app_menu_view_dup_selected_app_desktop_path
 * @view: a widget
 *
 * Retrieves #FmPath to selected application from the widget as a child
 * below fm_path_get_apps_menu() root path. Return %NULL if there is no
 * application selected.
 * The returned data should be freed with fm_path_unref() after usage.
 *
 * Returns: (transfer full): path to selected application file.
 *
 * Since: 1.2.0
 */
FmPath * fm_app_menu_view_dup_selected_app_desktop_path(GtkTreeView* view)
{
    GtkTreeIter it;
    GtkTreeSelection* sel = gtk_tree_view_get_selection(view);
    /* FIXME: this should be checked if it's exactly app menu tree! */
    if(gtk_tree_selection_get_selected(sel, NULL, &it))
    {
        MenuCacheItem* item;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &it, COL_ITEM, &item, -1);
        if(item && menu_cache_item_get_type(item) == MENU_CACHE_TYPE_APP)
        {
            char *mpath = menu_cache_dir_make_path(MENU_CACHE_DIR(item));
            FmPath *path = fm_path_new_relative(fm_path_get_apps_menu(),
                                                mpath+13 /* skip "/Applications" */);
            g_free(mpath);
            return path;
        }
    }
    return NULL;
}

/**
 * fm_app_menu_view_is_item_app
 * @view: a widget
 * @it: tree iterator
 *
 * Checks if item at @it is an application.
 *
 * Returns: %TRUE if item is an application.
 *
 * Since: 0.1.0
 */
gboolean fm_app_menu_view_is_item_app(GtkTreeView* view, GtkTreeIter* it)
{
    MenuCacheItem* item;
    /* FIXME: this should be checked if it's exactly app menu tree! */
    gboolean ret = FALSE;
    gtk_tree_model_get(GTK_TREE_MODEL(store), it, COL_ITEM, &item, -1);
    if(item && menu_cache_item_get_type(item) == MENU_CACHE_TYPE_APP)
        ret = TRUE;
    return ret;
}

/**
 * fm_app_menu_view_is_app_selected
 * @view: a widget
 *
 * Checks if there is an application selected in @view.
 *
 * Returns: %TRUE if there is an application selected.
 *
 * Since: 0.1.0
 */
gboolean fm_app_menu_view_is_app_selected(GtkTreeView* view)
{
    GtkTreeIter it;
    GtkTreeSelection* sel = gtk_tree_view_get_selection(view);
    if(gtk_tree_selection_get_selected(sel, NULL, &it))
        return fm_app_menu_view_is_item_app(view, &it);
    return FALSE;
}
