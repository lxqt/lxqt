/*
 *      menu-compose.c : scans appropriate .desktop files and composes cache file.
 *
 *      Copyright 2014-2015 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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
#include "version.h"

#include <string.h>
#include <stdio.h>
#include <glib/gstdio.h>

#define NONULL(a) (a == NULL) ? "" : a

static GSList *DEs = NULL;

static guint req_version = 1; /* old compatibility default */

static void menu_app_reset(MenuApp *app)
{
    g_free(app->filename);
    g_free(app->title);
    g_free(app->key);
    app->key = NULL;
    g_free(app->comment);
    g_free(app->icon);
    g_free(app->generic_name);
    g_free(app->exec);
    g_free(app->try_exec);
    g_free(app->wd);
    g_free(app->categories);
    g_free(app->keywords);
    g_free(app->show_in);
    g_free(app->hide_in);
}

static void menu_app_free(gpointer data)
{
    MenuApp *app = data;

    menu_app_reset(app);
    g_free(app->id);
    g_list_free(app->dirs);
    g_list_free(app->menus);
    g_slice_free(MenuApp, app);
}

static char *_escape_lf(char *str)
{
    char *c;

    if (str != NULL && (c = strchr(str, '\n')) != NULL)
    {
        GString *s = g_string_new_len(str, c - str);

        while (*c)
        {
            if (*c == '\n')
                g_string_append(s, "\\n");
            else
                g_string_append_c(s, *c);
            c++;
        }
        g_free(str);
        str = g_string_free(s, FALSE);
    }
    return str;
}

static char *_get_string(GKeyFile *kf, const char *key)
{
    return _escape_lf(g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP, key, NULL));
}

/* g_key_file_get_locale_string is too much limited so implement replacement */
static char *_get_language_string(GKeyFile *kf, const char *key)
{
    char **lang;
    char *try_key, *str;

    for (lang = languages; lang[0] != NULL; lang++)
    {
        try_key = g_strdup_printf("%s[%s]", key, lang[0]);
        str = _get_string(kf, try_key);
        g_free(try_key);
        if (str != NULL)
            return str;
    }
    return _escape_lf(g_key_file_get_locale_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                                   key, languages[0], NULL));
}

static char **_get_string_list(GKeyFile *kf, const char *key, gsize *lp)
{
    char **str, **p;

    str = g_key_file_get_string_list(kf, G_KEY_FILE_DESKTOP_GROUP, key, lp, NULL);
    if (str != NULL)
        for (p = str; p[0] != NULL; p++)
            p[0] = _escape_lf(p[0]);
    return str;
}

static char **_get_language_string_list(GKeyFile *kf, const char *key, gsize *lp)
{
    char **lang;
    char *try_key, **str;

    for (lang = languages; lang[0] != NULL; lang++)
    {
        try_key = g_strdup_printf("%s[%s]", key, lang[0]);
        str = _get_string_list(kf, try_key, lp);
        g_free(try_key);
        if (str != NULL)
            return str;
    }
    str = g_key_file_get_locale_string_list(kf, G_KEY_FILE_DESKTOP_GROUP, key,
                                            languages[0], lp, NULL);
    if (str != NULL)
        for (lang = str; lang[0] != NULL; lang++)
            lang[0] = _escape_lf(lang[0]);
    return str;
}

static void _fill_menu_from_file(MenuMenu *menu, const char *path)
{
    GKeyFile *kf;

    if (!g_str_has_suffix(path, ".directory")) /* ignore random names */
        return;
    kf = g_key_file_new();
    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
        goto exit;
    menu->title = _get_language_string(kf, G_KEY_FILE_DESKTOP_KEY_NAME);
    menu->comment = _get_language_string(kf, G_KEY_FILE_DESKTOP_KEY_COMMENT);
    menu->icon = _get_string(kf, G_KEY_FILE_DESKTOP_KEY_ICON);
    menu->layout.nodisplay = g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                                    G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY, NULL);
    menu->layout.is_set = TRUE;
exit:
    g_key_file_free(kf);
}

static const char **menu_app_intern_key_file_list(GKeyFile *kf, const char *key,
                                                  gboolean localized,
                                                  gboolean add_to_des)
{
    gsize len, i;
    char **val;
    const char **res;

    if (localized)
        val = _get_language_string_list(kf, key, &len);
    else
        val = _get_string_list(kf, key, &len);
    if (val == NULL)
        return NULL;
    res = (const char **)g_new(char *, len + 1);
    for (i = 0; i < len; i++)
    {
        res[i] = g_intern_string(val[i]);
        if (add_to_des && g_slist_find(DEs, res[i]) == NULL)
            DEs = g_slist_append(DEs, (gpointer)res[i]);
    }
    res[i] = NULL;
    g_strfreev(val);
    return res;
}

static void _fill_app_from_key_file(MenuApp *app, GKeyFile *kf)
{
    app->title = _get_language_string(kf, G_KEY_FILE_DESKTOP_KEY_NAME);
    app->comment = _get_language_string(kf, G_KEY_FILE_DESKTOP_KEY_COMMENT);
    app->icon = _get_string(kf, G_KEY_FILE_DESKTOP_KEY_ICON);
    app->generic_name = _get_language_string(kf, G_KEY_FILE_DESKTOP_KEY_GENERIC_NAME);
    app->exec = _get_string(kf, G_KEY_FILE_DESKTOP_KEY_EXEC);
    app->try_exec = _get_string(kf, G_KEY_FILE_DESKTOP_KEY_TRY_EXEC);
    app->wd = _get_string(kf, G_KEY_FILE_DESKTOP_KEY_PATH);
    app->categories = menu_app_intern_key_file_list(kf, G_KEY_FILE_DESKTOP_KEY_CATEGORIES,
                                                    FALSE, FALSE);
    app->keywords = menu_app_intern_key_file_list(kf, "Keywords", TRUE, FALSE);
    app->show_in = menu_app_intern_key_file_list(kf, G_KEY_FILE_DESKTOP_KEY_ONLY_SHOW_IN,
                                                 FALSE, TRUE);
    app->hide_in = menu_app_intern_key_file_list(kf, G_KEY_FILE_DESKTOP_KEY_NOT_SHOW_IN,
                                                 FALSE, TRUE);
    app->use_terminal = g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                               G_KEY_FILE_DESKTOP_KEY_TERMINAL, NULL);
    app->use_notification = g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                                   G_KEY_FILE_DESKTOP_KEY_STARTUP_NOTIFY, NULL);
    app->hidden = g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                         G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY, NULL);
}

static GHashTable *all_apps = NULL;

static GSList *loaded_dirs = NULL;

static GList *_make_def_layout(void)
{
    MenuMerge *mm;
    GList *layout;

    mm = g_slice_new(MenuMerge);
    mm->type = MENU_CACHE_TYPE_NONE;
    mm->merge_type = MERGE_FILES;
    layout = g_list_prepend(NULL, mm);
    mm = g_slice_new(MenuMerge);
    mm->type = MENU_CACHE_TYPE_NONE;
    mm->merge_type = MERGE_MENUS;
    return g_list_prepend(layout, mm);
}

static void _fill_apps_from_dir(MenuMenu *menu, GList *lptr, GString *prefix,
                                gboolean is_legacy)
{
    const char *dir = lptr->data;
    GDir *gd;
    const char *name;
    char *filename, *id;
    gsize prefix_len = prefix->len;
    MenuApp *app;
    GKeyFile *kf;

    if (g_slist_find(loaded_dirs, dir) == NULL)
        loaded_dirs = g_slist_prepend(loaded_dirs, (gpointer)dir);
    /* the directory might be scanned with different prefix already */
    else if (prefix->str[0] == '\0')
        return;
    gd = g_dir_open(dir, 0, NULL);
    if (gd == NULL)
        return;
    kf = g_key_file_new();
    DBG("fill apps from dir [%s]%s", prefix->str, dir);
    /* Scan the directory with subdirs,
       ignore not .desktop files,
       ignore already present files that are allocated */
    while ((name = g_dir_read_name(gd)) != NULL)
    {
        filename = g_build_filename(dir, name, NULL);
        if (g_file_test(filename, G_FILE_TEST_IS_DIR))
        {
            /* recursion */
            if (is_legacy)
            {
                MenuMenu *submenu = g_slice_new0(MenuMenu);
                submenu->layout = menu->layout; /* copy all */
                submenu->layout.items = _make_def_layout(); /* default layout */
                submenu->layout.inline_limit_is_set = TRUE; /* marker */
                submenu->name = g_strdup(name);
                submenu->dir = g_intern_string(filename);
                menu->children = g_list_append(menu->children, submenu);
            }
            else
            {
                g_string_append(prefix, name);
                g_string_append_c(prefix, '-');
                name = g_intern_string(filename);
                /* a little trick here - we insert new node after this one */
                lptr = g_list_insert_before(lptr, lptr->next, (gpointer)name);
                _fill_apps_from_dir(menu, lptr->next, prefix, FALSE);
                g_string_truncate(prefix, prefix_len);
            }
        }
        else if (!g_str_has_suffix(name, ".desktop") ||
                 !g_file_test(filename, G_FILE_TEST_IS_REGULAR) ||
                 !g_key_file_load_from_file(kf, filename,
                                            G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
            ; /* ignore not key files */
        else if ((id = g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                             G_KEY_FILE_DESKTOP_KEY_TYPE, NULL)) == NULL ||
                 strcmp(id, G_KEY_FILE_DESKTOP_TYPE_APPLICATION) != 0)
            /* ignore non-applications */
            g_free(id);
        else
        {
            g_free(id);
            if (prefix_len > 0)
            {
                g_string_append(prefix, name);
                app = g_hash_table_lookup(all_apps, prefix->str);
            }
            else
                app = g_hash_table_lookup(all_apps, name);
            if (app == NULL)
            {
                if (!g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                            G_KEY_FILE_DESKTOP_KEY_HIDDEN, NULL))
                {
                    /* deleted file should be ignored */
                    app = g_slice_new0(MenuApp);
                    app->type = MENU_CACHE_TYPE_APP;
                    app->filename = (prefix_len > 0) ? g_strdup(name) : NULL;
                    app->id = g_strdup((prefix_len > 0) ? prefix->str : name);
                    VDBG("found app id=%s", app->id);
                    g_hash_table_insert(all_apps, app->id, app);
                    app->dirs = g_list_prepend(NULL, (gpointer)dir);
                    _fill_app_from_key_file(app, kf);
                }
            }
            else if (app->allocated)
                g_warning("id '%s' already allocated for %s and requested to"
                          " change to %s, ignoring request", name,
                          (const char *)app->dirs->data, dir);
            else if (g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                            G_KEY_FILE_DESKTOP_KEY_HIDDEN, NULL))
            {
                VDBG("removing app id=%s", app->id),
                g_hash_table_remove(all_apps, name);
            }
            else
            {
                /* reset the data */
                menu_app_reset(app);
                app->filename = (prefix_len > 0) ? g_strdup(name) : NULL;
                /* reorder dirs list */
                app->dirs = g_list_remove(app->dirs, dir);
                app->dirs = g_list_prepend(app->dirs, (gpointer)dir);
                _fill_app_from_key_file(app, kf);
                /* FIXME: conform to spec about Legacy in Categories field */
            }
            if (prefix_len > 0)
                g_string_truncate(prefix, prefix_len);
        }
        g_free(filename);
    }
    g_dir_close(gd);
    g_key_file_free(kf);
}

static int _compare_items(gconstpointer a, gconstpointer b)
{
    /* return negative value to reverse sort list */
    return -strcmp(((MenuApp*)a)->type == MENU_CACHE_TYPE_APP ? ((MenuApp*)a)->key
                                                              : ((MenuMenu*)a)->key,
                   ((MenuApp*)b)->type == MENU_CACHE_TYPE_APP ? ((MenuApp*)b)->key
                                                              : ((MenuMenu*)b)->key);
}

static gboolean menu_app_match_tag(MenuApp *app, FmXmlFileItem *it)
{
    FmXmlFileTag tag = fm_xml_file_item_get_tag(it);
    GList *children, *child;
    gboolean ok = FALSE;

    VVDBG("menu_app_match_tag: entering <%s>", fm_xml_file_item_get_tag_name(it));
    children = fm_xml_file_item_get_children(it);
    if (tag == menuTag_Or)
    {
        for (child = children; child; child = child->next)
            if (menu_app_match_tag(app, child->data))
                break;
        ok = (child != NULL);
    }
    else if (tag == menuTag_And)
    {
        for (child = children; child; child = child->next)
            if (!menu_app_match_tag(app, child->data))
                break;
        ok = (child == NULL);
    }
    else if (tag == menuTag_Not)
    {
        for (child = children; child; child = child->next)
            if (menu_app_match_tag(app, child->data))
                break;
        ok = (child == NULL);
    }
    else if (tag == menuTag_All)
        ok = TRUE;
    else if (tag == menuTag_Filename)
    {
        register const char *id = fm_xml_file_item_get_data(children->data, NULL);
        ok = (g_strcmp0(id, app->id) == 0);
    }
    else if (tag == menuTag_Category)
    {
        if (app->categories != NULL)
        {
            const char *cat = g_intern_string(fm_xml_file_item_get_data(children->data, NULL));
            const char **cats = app->categories;
            while (*cats)
                if (*cats == cat)
                    break;
                else
                    cats++;
            ok = (*cats != NULL);
        }
    }
    g_list_free(children);
    VVDBG("menu_app_match_tag %s: leaving <%s>: %d", app->id, fm_xml_file_item_get_tag_name(it), ok);
    return ok;
}

static gboolean menu_app_match_excludes(MenuApp *app, GList *rules);

static gboolean menu_app_match(MenuApp *app, GList *rules, gboolean do_all)
{
    MenuRule *rule;
    GList *children, *child;

    for (; rules != NULL; rules = rules->next)
    {
        rule = rules->data;
        if (rule->type != MENU_CACHE_TYPE_NONE ||
            fm_xml_file_item_get_tag(rule->rule) != menuTag_Include)
            continue;
        children = fm_xml_file_item_get_children(rule->rule);
        for (child = children; child; child = child->next)
            if (menu_app_match_tag(app, child->data))
                break;
        g_list_free(children);
        if (child != NULL)
            return (!do_all || !menu_app_match_excludes(app, rules->next));
    }
    return FALSE;
}

static gboolean menu_app_match_excludes(MenuApp *app, GList *rules)
{
    MenuRule *rule;
    GList *children, *child;

    for (; rules != NULL; rules = rules->next)
    {
        rule = rules->data;
        if (rule->type != MENU_CACHE_TYPE_NONE ||
            fm_xml_file_item_get_tag(rule->rule) != menuTag_Exclude)
            continue;
        children = fm_xml_file_item_get_children(rule->rule);
        for (child = children; child; child = child->next)
            if (menu_app_match_tag(app, child->data))
                break;
        g_list_free(children);
        if (child != NULL)
            /* application might be included again later so check for it */
            return !menu_app_match(app, rules->next, TRUE);
    }
    return FALSE;
}

static void _free_leftovers(GList *item);

static void menu_menu_free(MenuMenu *menu)
{
    g_free(menu->name);
    g_free(menu->key);
    g_list_foreach(menu->id, (GFunc)g_free, NULL);
    g_list_free(menu->id);
    _free_leftovers(menu->children);
    _free_layout_items(menu->layout.items);
    g_free(menu->title);
    g_free(menu->comment);
    g_free(menu->icon);
    g_slice_free(MenuMenu, menu);
}

static void _free_leftovers(GList *item)
{
    union {
        MenuMenu *menu;
        MenuRule *rule;
    } a = { NULL };

    while (item)
    {
        a.menu = item->data;
        if (a.rule->type == MENU_CACHE_TYPE_NONE)
            g_slice_free(MenuRule, a.rule);
        else if (a.rule->type == MENU_CACHE_TYPE_DIR)
            menu_menu_free(a.menu);
        /* MenuApp and MenuSep are not allocated in menu->children */
        item = g_list_delete_link(item, item);
    }
}

/* dirs are in order "first is more relevant" */
static void _stage1(MenuMenu *menu, GList *dirs, GList *apps, GList *legacy, GList *p)
{
    GList *child, *_dirs = NULL, *_apps = NULL, *_legs = NULL, *_lprefs = NULL;
    GList *l, *available = NULL, *result;
    const char *id;
    char *filename;
    GString *prefix;
    MenuApp *app;
    FmXmlFileTag tag;
    GHashTableIter iter;

    DBG("... entering %s (%d dirs %d apps)", menu->name, g_list_length(dirs), g_list_length(apps));
    /* Gather our dirs : DirectoryDir AppDir LegacyDir KDELegacyDirs */
    for (child = menu->children; child; child = child->next)
    {
        MenuRule *rule = child->data;
        if (rule->type != MENU_CACHE_TYPE_NONE)
            continue;
        tag = fm_xml_file_item_get_tag(rule->rule);
        if (tag == menuTag_DirectoryDir) {
            id = g_intern_string(fm_xml_file_item_get_data(fm_xml_file_item_find_child(rule->rule,
                                                           FM_XML_FILE_TEXT), NULL));
            DBG("new DirectoryDir %s", id);
            if (_dirs == NULL)
                _dirs = g_list_copy(dirs);
            /* replace and reorder the list */
            _dirs = g_list_remove(_dirs, id);
            _dirs = g_list_prepend(_dirs, (gpointer)id);
        } else if (tag == menuTag_AppDir) {
            id = g_intern_string(fm_xml_file_item_get_data(fm_xml_file_item_find_child(rule->rule,
                                                           FM_XML_FILE_TEXT), NULL));
            DBG("new AppDir %s", id);
            _apps = g_list_prepend(_apps, (gpointer)id);
        } else if (tag == menuTag_LegacyDir) {
            id = g_intern_string(fm_xml_file_item_get_data(fm_xml_file_item_find_child(rule->rule,
                                                           FM_XML_FILE_TEXT), NULL));
            DBG("new LegacyDir %s", id);
            _legs = g_list_prepend(_legs, (gpointer)id);
            _lprefs = g_list_prepend(_lprefs, (gpointer)fm_xml_file_item_get_comment(rule->rule));
        }
    }
    /* Gather data from files in the dirs */
    if (_dirs != NULL) dirs = _dirs;
    for (l = menu->id; l; l = l->next)
    {
        /* scan dirs now for availability of any of ids */
        filename = NULL;
        for (child = dirs; child; child = child->next)
        {
            filename = g_build_filename(child->data, l->data, NULL);
            if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
                break;
            g_free(filename);
            filename = NULL;
        }
        if (filename == NULL) for (child = _legs; child; child = child->next)
        {
            filename = g_build_filename(child->data, l->data, NULL);
            if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
                break;
            g_free(filename);
            filename = NULL;
        }
        if (filename == NULL) for (child = legacy; child; child = child->next)
        {
            filename = g_build_filename(child->data, l->data, NULL);
            if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
                break;
            g_free(filename);
            filename = NULL;
        }
        if (filename != NULL)
        {
            VVDBG("found dir file %s", filename);
            _fill_menu_from_file(menu, filename);
            g_free(filename);
            if (!menu->layout.is_set)
                continue;
            menu->dir = child->data;
            if (l != menu->id) /* relocate matched id to top if required */
            {
                menu->id = g_list_remove_link(menu->id, l);
                menu->id = g_list_concat(menu->id, l);
            }
            break;
        }
    }
    if (menu->layout.inline_limit_is_set && !menu->layout.is_set)
    {
        filename = g_build_filename(menu->dir, ".directory", NULL);
        _fill_menu_from_file(menu, filename);
        g_free(filename);
        filename = NULL;
    }
    prefix = g_string_new("");
    _apps = g_list_reverse(_apps);
    _legs = g_list_reverse(_legs);
    /* the same directory being scanned with different prefix should be denied */
    for (l = _apps; l; l = l->next)
    {
        for (child = _apps; child; child = result)
        {
            int len;

            result = child->next; /* it is not used yet so we can use it now */
            if (child == l)
                continue;
            len = strlen(l->data);
            if (strncmp(l->data, child->data, len) == 0 &&
                ((const char *)child->data)[len] == G_DIR_SEPARATOR)
                _apps = g_list_delete_link(_apps, child);
        }
        for (child = _legs; child; child = result)
        {
            int len;

            result = child->next;
            len = strlen(l->data);
            if (strncmp(l->data, child->data, len) == 0 &&
                ((const char *)child->data)[len] == G_DIR_SEPARATOR)
            {
                len = g_list_position(_legs, child);
                _legs = g_list_delete_link(_legs, child);
                child = g_list_nth(_lprefs, len);
                _lprefs = g_list_delete_link(_lprefs, child);
            }
        }
    }
    if (!menu->layout.inline_limit_is_set) for (l = _apps; l; l = l->next)
    {
        /* scan and fill the list */
        _fill_apps_from_dir(menu, l, prefix, FALSE);
    }
    if (_apps != NULL)
        apps = _apps = g_list_concat(g_list_copy(apps), _apps);
    for (l = _legs, child = _lprefs; l; l = l->next, child = child->next)
    {
        /* use prefix from <LegacyDir> attribute */
        g_string_assign(prefix, NONULL(child->data));
        VDBG("got legacy prefix %s", (char*)child->data);
        _fill_apps_from_dir(menu, l, prefix, TRUE);
        g_string_truncate(prefix, 0);
    }
    if (_legs != NULL)
        legacy = _legs = g_list_concat(g_list_copy(legacy), _legs);
    if (_lprefs != NULL)
        p = _lprefs = g_list_concat(g_list_copy(p), _lprefs);
    /* Gather all available files (some in $all_apps may be not in $apps) */
    VDBG("... do matching");
    g_hash_table_iter_init(&iter, all_apps);
    while (g_hash_table_iter_next(&iter, NULL, (gpointer *)&app))
    {
        app->matched = FALSE;
        /* check every dir if it is in $apps */
        if (menu->layout.inline_limit_is_set)
        {
            for (child = app->dirs; child; child = child->next)
                if (menu->dir == child->data)
                    break;
        }
        else for (child = app->dirs; child; child = child->next)
        {
            for (l = apps; l; l = l->next)
            {
                if (l->data == child->data)
                    break;
            }
            if (l != NULL) /* found one */
                break;
        }
        VVDBG("check %s in %s: %d", app->id, app->dirs ? (const char *)app->dirs->data : "(nil)", child != NULL);
        if (child == NULL) /* not matched */
            continue;
        /* Check matching : Include And Or Not All */
        if (menu->layout.inline_limit_is_set)
            app->matched = (app->categories == NULL); /* see the spec */
        else
            app->matched = menu_app_match(app, menu->children, FALSE);
        if (!app->matched)
            continue;
        app->allocated = TRUE;
        /* Mark it by Exclude And Or Not All */
        app->excluded = menu_app_match_excludes(app, menu->children);
        VVDBG("found match: %s excluded:%d", app->id, app->excluded);
        if (!app->excluded)
            available = g_list_prepend(available, app);
    }
    /* Compose layout using available list and replace menu->children */
    VDBG("... compose (available=%d)", g_list_length(available));
    result = NULL;
    for (child = menu->layout.items; child; child = child->next)
    {
        GList *next;
        app = child->data; /* either: MenuMenuname, MemuFilename, MenuSep, MenuMerge */
        switch (app->type) {
        case MENU_CACHE_TYPE_DIR: /* MenuMenuname */
            VDBG("composing Menuname %s", ((MenuMenuname *)app)->name);
            for (l = menu->children; l; l = l->next)
                if (((MenuMenu *)l->data)->layout.type == MENU_CACHE_TYPE_DIR &&
                    strcmp(((MenuMenuname *)app)->name, ((MenuMenu *)l->data)->name) == 0)
                    break;
            if (l != NULL) /* found such menu */
            {
                /* apply custom settings */
                if (((MenuMenuname *)app)->layout.only_unallocated)
                    ((MenuMenu *)l->data)->layout.show_empty = ((MenuMenuname *)app)->layout.show_empty;
                if (((MenuMenuname *)app)->layout.is_set)
                    ((MenuMenu *)l->data)->layout.allow_inline = ((MenuMenuname *)app)->layout.allow_inline;
                if (((MenuMenuname *)app)->layout.inline_header_is_set)
                    ((MenuMenu *)l->data)->layout.inline_header = ((MenuMenuname *)app)->layout.inline_header;
                if (((MenuMenuname *)app)->layout.inline_alias_is_set)
                    ((MenuMenu *)l->data)->layout.inline_alias = ((MenuMenuname *)app)->layout.inline_alias;
                if (((MenuMenuname *)app)->layout.inline_limit_is_set)
                    ((MenuMenu *)l->data)->layout.inline_limit = ((MenuMenuname *)app)->layout.inline_limit;
                /* remove from menu->children */
                menu->children = g_list_remove_link(menu->children, l);
                /* prepend to result */
                result = g_list_concat(l, result);
                /* ready for recursion now */
                _stage1(l->data, dirs, apps, legacy, p);
            }
            break;
        case MENU_CACHE_TYPE_APP: /* MemuFilename */
            VDBG("composing Filename %s", ((MenuFilename *)app)->id);
            app = g_hash_table_lookup(all_apps, ((MenuFilename *)app)->id);
            if (app == NULL)
                /* not available, ignoring it */
                break;
            l = g_list_find(result, app); /* this might be slow but we have
                                             to do this because app might be
                                             already added into result */
            if (l != NULL)
            {
                /* move it out to this place */
                result = g_list_remove_link(result, l);
                VVDBG("+++ composing app %s (move)", app->id);
            }
            else
            {
                l = g_list_find(available, app);
                VVDBG("+++ composing app %s%s", app->id, (l == NULL) ? " (add)" : "");
                if (l != NULL)
                    available = g_list_remove_link(available, l);
                else
                    l = g_list_prepend(NULL, app);
            }
            if (l != NULL)
                app->menus = g_list_prepend(app->menus, menu);
            result = g_list_concat(l, result);
            break;
        case MENU_CACHE_TYPE_SEP: /* MenuSep */
            VDBG("composing Separator");
            result = g_list_prepend(result, app);
            break;
        case MENU_CACHE_TYPE_NONE: /* MenuMerge */
            VDBG("composing Merge type %d", ((MenuMerge *)app)->merge_type);
            next = NULL;
            tag = 0;
            switch (((MenuMerge *)app)->merge_type) {
            case MERGE_FILES:
                tag = 1; /* use it as mark to not add dirs */
            case MERGE_ALL:
                for (l = available; l; l = l->next)
                {
                    app = l->data;
                    VVDBG("+++ composing app %s", app->id);
                    if (app->key == NULL)
                    {
                        if (app->title != NULL)
                            app->key = g_utf8_collate_key(app->title, -1);
                        else
                            g_warning("id %s has no Name", app->id),
                            app->key = g_utf8_collate_key(app->id, -1);
                    }
                    app->menus = g_list_prepend(app->menus, menu);
                }
                next = available;
                available = NULL;
                /* continue with menus */
            case MERGE_MENUS:
                if (tag != 1) for (l = menu->children; l; )
                {
                    if (((MenuMenu *)l->data)->layout.type == MENU_CACHE_TYPE_DIR)
                    {
                        GList *this = l;

                        /* find it in the rest of layout and skip if it's found */
                        for (l = child->next; l; l = l->next)
                            if (((MenuMenuname *)l->data)->layout.type == MENU_CACHE_TYPE_DIR &&
                                strcmp(((MenuMenuname *)l->data)->name, ((MenuMenu *)this->data)->name) == 0)
                                break;
                        if (l != NULL)
                        {
                            /* it will be added later by MenuMenuname handler */
                            l = this->next;
                            continue;
                        }
                        _stage1(this->data, dirs, apps, legacy, p); /* it's time for recursion */
                        VVDBG("+++ composing menu %s (%s)", ((MenuMenu *)this->data)->name, ((MenuMenu *)this->data)->title);
                        if (((MenuMenu *)this->data)->key == NULL)
                        {
                            if (((MenuMenu *)this->data)->title != NULL)
                                ((MenuMenu *)this->data)->key = g_utf8_collate_key(((MenuMenu *)this->data)->title, -1);
                            else
                                ((MenuMenu *)this->data)->key = g_utf8_collate_key(((MenuMenu *)this->data)->name, -1);
                        }
                        l = this->next;
                        /* move out from menu->children into result */
                        menu->children = g_list_remove_link(menu->children, this);
                        next = g_list_concat(this, next);
                    }
                    else
                        l = l->next;
                }
                result = g_list_concat(g_list_sort(next, _compare_items), result);
                break;
            default: ;
            }
        }
    }
    VDBG("... cleanup");
    _free_leftovers(menu->children);
    menu->children = g_list_reverse(result);
    for (child = menu->children; child; )
    {
        MenuMenu *submenu = child->data;

        child = child->next;
        if (submenu->layout.type == MENU_CACHE_TYPE_DIR &&
            submenu->layout.allow_inline &&
            (submenu->layout.inline_limit == 0 ||
             (int)g_list_length(submenu->children) <= submenu->layout.inline_limit))
        {
            DBG("*** got some inline!");
            if (submenu->layout.inline_alias && g_list_length(submenu->children) == 1)
            {
                /* replace name of single child with name of submenu */
                VDBG("replacing title of single child of %s due to inline_alias",
                     submenu->name);
                app = submenu->children->data;
                if (app->type == MENU_CACHE_TYPE_DIR)
                {
                    g_free(((MenuMenu *)app)->title);
                    ((MenuMenu *)app)->title = g_strdup(submenu->title ? submenu->title : submenu->name);
                }
                else if (app->type == MENU_CACHE_TYPE_APP)
                {
                    g_free(app->title);
                    app->title = g_strdup(submenu->title ? submenu->title : submenu->name);
                }
            }
            /* FIXME: inline the submenu... how to use inline_header? */
            submenu->children = g_list_reverse(submenu->children);
            while (submenu->children != NULL)
            {
                menu->children = g_list_insert_before(menu->children, child,
                                                      submenu->children->data);
                submenu->children = g_list_delete_link(submenu->children, submenu->children);
            }
            menu->children = g_list_remove(menu->children, submenu);
            menu_menu_free(submenu);
        }
    }
    /* NOTE: now only menus are allocated in menu->children */
    DBG("... done %s", menu->name);
    /* Do cleanup */
    g_list_free(available);
    g_list_free(_dirs);
    g_list_free(_apps);
    g_list_free(_legs);
    g_list_free(_lprefs);
    g_string_free(prefix, TRUE);
}

static gint _stage2(MenuMenu *menu, gboolean with_hidden)
{
    GList *child = menu->children, *next, *to_delete = NULL;
    MenuApp *app;
    gint count = 0;

    VVDBG("stage 2: entered '%s'", menu->name);
    while (child)
    {
        app = child->data;
        next = child->next;
        switch (app->type) {
        case MENU_CACHE_TYPE_APP: /* Menu App */
            if (menu->layout.only_unallocated && app->menus->next != NULL)
            {
                VDBG("removing from %s as only_unallocated %s",menu->name,app->id);
                /* it is more than in one menu */
                menu->children = g_list_delete_link(menu->children, child);
                app->menus = g_list_remove(app->menus, menu);
            }
            else if (app->hidden && !with_hidden)
                /* should be not displayed */
                menu->children = g_list_delete_link(menu->children, child);
            else
                count++;
            break;
        case MENU_CACHE_TYPE_DIR: /* MenuMenu */
            /* do recursion */
            if (_stage2(child->data, with_hidden) > 0)
                count++;
            else if (!with_hidden || req_version < 2)
                to_delete = g_list_prepend(to_delete, child);
            break;
        default:
            /* separator */
            if (child == menu->children || next == NULL ||
                (app = next->data)->type == MENU_CACHE_TYPE_SEP)
                menu->children = g_list_delete_link(menu->children, child);
        }
        child = next;
    }
    VVDBG("stage 2: counted '%s': %d", menu->name, count);
    if (count > 0 && with_hidden)
        g_list_free(to_delete);
    else while (to_delete) /* if no apps here then don't keep dirs as well */
    {
        child = to_delete->data;
        VVDBG("stage 2: deleting empty '%s'", ((MenuMenu *)child->data)->name);
        menu_menu_free(child->data);
        menu->children = g_list_delete_link(menu->children, child);
        to_delete = g_list_delete_link(to_delete, to_delete);
    }
    if (count == 0)
    {
        if (menu->layout.show_empty)
            count++;
        else
            menu->layout.nodisplay = TRUE;
    }
    return count;
}

static inline int _compose_flags(const char **f)
{
    int x = 0, i;

    while (*f)
    {
        i = g_slist_index(DEs, *f++);
        if (i >= 0)
            x |= 1 << i;
    }
    return x;
}

static gboolean write_app_extra(FILE *f, MenuApp *app)
{
    gboolean ret;
    char *cats, *keywords;
    char *null_list[] = { NULL };

    if (req_version < 2)
        return TRUE;
    cats = g_strjoinv(";", app->categories ? (char **)app->categories : null_list);
    keywords = g_strjoinv(",", app->keywords ? (char **)app->keywords : null_list);
    ret = fprintf(f, "%s\n%s\n%s\n%s\n", NONULL(app->try_exec), NONULL(app->wd),
                                         cats, keywords) > 0;
    g_free(cats);
    g_free(keywords);
    return ret;
}

static gboolean write_app(FILE *f, MenuApp *app, gboolean with_hidden)
{
    int index;
    MenuCacheItemFlag flags = 0;
    int show = 0;

    if (app->hidden && !with_hidden)
        return TRUE;
    index = MAX(g_slist_index(AppDirs, app->dirs->data), 0) + g_slist_length(DirDirs);
    if (app->use_terminal)
        flags |= FLAG_USE_TERMINAL;
    if (app->hidden)
        flags |= FLAG_IS_NODISPLAY;
    if (app->use_notification)
        flags |= FLAG_USE_SN;
    if (app->show_in)
        show = _compose_flags(app->show_in);
    else if (app->hide_in)
        show = ~_compose_flags(app->hide_in);
    return fprintf(f, "-%s\n%s\n%s\n%s\n%s\n%d\n%s\n%s\n%u\n%d\n", app->id,
                   NONULL(app->title), NONULL(app->comment), NONULL(app->icon),
                   NONULL(app->filename), index, NONULL(app->generic_name),
                   NONULL(app->exec), flags, show) > 0 && write_app_extra(f, app);
}

static gboolean write_menu(FILE *f, MenuMenu *menu, gboolean with_hidden)
{
    int index;
    GList *child;
    gboolean ok = TRUE;

    if (!with_hidden && !menu->layout.show_empty && menu->children == NULL)
        return TRUE;
    if (menu->layout.nodisplay && (!with_hidden || req_version < 2))
        return TRUE;
    index = g_slist_index(DirDirs, menu->dir);
    if (fprintf(f, "+%s\n%s\n%s\n%s\n%s\n%d\n", menu->name, NONULL(menu->title),
                NONULL(menu->comment), NONULL(menu->icon),
                menu->id ? (const char *)menu->id->data : "", index) < 0)
        return FALSE;
    /* pass show_empty into file if format is v.1.2 */
    if (req_version >= 2 &&
        fprintf(f, "%d\n", menu->layout.nodisplay ? FLAG_IS_NODISPLAY : 0) < 0)
        return FALSE;
    for (child = menu->children; ok && child != NULL; child = child->next)
    {
        index = ((MenuApp *)child->data)->type;
        if (index == MENU_CACHE_TYPE_DIR)
            ok = write_menu(f, child->data, with_hidden);
        else if (index == MENU_CACHE_TYPE_APP)
            ok = write_app(f, child->data, with_hidden);
        else if (child->next != NULL && child != menu->children &&
                 ((MenuApp *)child->next->data)->type != MENU_CACHE_TYPE_SEP)
            /* separator - not add duplicates nor at start nor at end */
            fprintf(f, "-\n");
    }
    fputc('\n', f);
    return ok;
}


/*
 * we handle here only:
 * - menuTag_DirectoryDir : for directory files list
 * - menuTag_AppDir menuTag_LegacyDir menuTag_KDELegacyDirs : for app files list
 * - menuTag_Include menuTag_Exclude menuTag_And menuTag_Or menuTag_Not menuTag_All :
 *      as matching rules
 */
gboolean save_menu_cache(MenuMenu *layout, const char *menuname, const char *file,
                         gboolean with_hidden)
{
    const char *de_names[N_KNOWN_DESKTOPS] = { "LXDE",
                                               "GNOME",
                                               "KDE",
                                               "XFCE",
                                               "ROX" };
    char *tmp;
    FILE *f = NULL;
    GSList *l;
    int i;
    gboolean ok = FALSE;

    tmp = (char *)g_getenv("CACHE_GEN_VERSION");
    if (tmp && sscanf(tmp, "%d.%u", &i, &req_version) == 2)
    {
        if (i != VER_MAJOR) /* unsupported format requested */
            return FALSE;
    }
    if (req_version < VER_MINOR_SUPPORTED) /* unsupported format requested */
        return FALSE;
    if (req_version > VER_MINOR) /* fallback to maximal supported format */
        req_version = VER_MINOR;
    all_apps = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, menu_app_free);
    for (i = 0; i < N_KNOWN_DESKTOPS; i++)
        DEs = g_slist_append(DEs, (gpointer)g_intern_static_string(de_names[i]));
    /* Recursively add files into layout, don't take OnlyUnallocated into account */
    _stage1(layout, NULL, NULL, NULL, NULL);
    /* Recursively remove non-matched files by OnlyUnallocated flag */
    _stage2(layout, with_hidden);
    /* Prepare temporary file for safe creation */
    tmp = strrchr(menuname, G_DIR_SEPARATOR);
    if (tmp)
        menuname = &tmp[1];
    tmp = g_path_get_dirname(file);
    if (tmp != NULL && !g_file_test(tmp, G_FILE_TEST_EXISTS))
        g_mkdir_with_parents(tmp, 0700);
    g_free(tmp);
    tmp = g_strdup_printf("%sXXXXXX", file);
    i = g_mkstemp(tmp);
    if (i < 0)
        goto failed;
    /* Compose created layout into output file */
    f = fdopen(i, "w");
    if (f == NULL)
        goto failed;
    /* Write common data */
    fprintf(f, "1.%d\n%s%s\n%d\n", req_version, /* use CACHE_GEN_VERSION */
            menuname, with_hidden ? "+hidden" : "",
            g_slist_length(DirDirs) + g_slist_length(AppDirs)
            + g_slist_length(MenuDirs) + g_slist_length(MenuFiles));
    VDBG("%d %d %d %d",g_slist_length(DirDirs),g_slist_length(AppDirs),g_slist_length(MenuDirs),g_slist_length(MenuFiles));
    for (l = DirDirs; l; l = l->next)
        if (fprintf(f, "D%s\n", (const char *)l->data) < 0)
            goto failed;
    for (l = AppDirs; l; l = l->next)
        if (fprintf(f, "D%s\n", (const char *)l->data) < 0)
            goto failed;
    for (l = MenuDirs; l; l = l->next)
        if (fprintf(f, "D%s\n", (const char *)l->data) < 0)
            goto failed;
    for (l = MenuFiles; l; l = l->next)
        if (fprintf(f, "F%s\n", (const char *)l->data) < 0)
            goto failed;
    for (l = g_slist_nth(DEs, 5); l; l = l->next)
        if (fprintf(f, "%s;", (const char *)l->data) < 0)
            goto failed;
    fputc('\n', f);
    /* Write the menu tree */
    ok = write_menu(f, layout, with_hidden);
failed:
    if (f != NULL)
        fclose(f);
    if (ok)
        ok = g_rename(tmp, file) == 0;
    else if (tmp)
        g_unlink(tmp);
    /* Free all the data */
    menu_menu_free(layout);
    g_free(tmp);
    g_hash_table_destroy(all_apps);
    g_slist_free(DEs);
    g_slist_free(loaded_dirs);
    return ok;
}
