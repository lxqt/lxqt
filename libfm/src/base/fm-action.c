/*
 *      fm-action.c
 *
 *      Copyright 2014-2020 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *      Copyright 2020 Leonardo Citrolo <leoc@users.sourceforge.net>
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

/**
 * SECTION:fm-action
 * @short_description: Support for DES-EMA extension for menus and actions.
 * @title: FmAction
 *
 * @include: libfm/fm.h
 *
 * The FmActionCache object represents a cache for user-defined menus and
 * actions that can be used in the file manager.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-action.h"
#include "fm-terminal.h"
#include "glib-compat.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#include <string.h>
#include <stdlib.h>

typedef struct _FmActionCondition FmActionCondition;
typedef struct _FmActionInfo FmActionInfo;
typedef struct _FmActionDir FmActionDir;

typedef enum
{
    CONDITION_CAPS_OWNER = (1<<0), /* Owner */
    CONDITION_CAPS_READABLE = (1<<1), /* Readable */
    CONDITION_CAPS_WRITABLE = (1<<2), /* Writable */
    CONDITION_CAPS_EXECUTABLE = (1<<3), /* Executable */
    CONDITION_CAPS_LOCAL = (1<<4) /* Local */
} FmActionConditionCaps;

#define CONDITION_CAPS_SHIFT_NEGATE 5 /* bits to negate */

typedef enum
{
    /* numerics */
    CONDITION_TYPE_COUNT = 0x0, /* SelectionCount = */
    CONDITION_TYPE_COUNT_LESS, /* SelectionCount < */
    CONDITION_TYPE_COUNT_MORE, /* SelectionCount > */
    CONDITION_TYPE_CAPS, /* Capabilities */
    /* strings */
    CONDITION_TYPE_RUN = 0x10, /* ShowIfRunning */
    CONDITION_TYPE_TRY_EXEC, /* TryExec */
    CONDITION_TYPE_DBUS, /* ShowIfRegistered */
    CONDITION_TYPE_OUT_TRUE, /* ShowIfTrue */
    /* string lists */
    CONDITION_TYPE_MIME = 0x20, /* MimeTypes */
    CONDITION_TYPE_BASENAME, /* Basenames with Matchcase=true */
    CONDITION_TYPE_NCASE_BASENAME, /* the same with Matchcase=false */
    CONDITION_TYPE_SCHEME, /* Schemes */
    CONDITION_TYPE_FOLDER, /* Folders */
} FmActionConditionType;

#define CONDITION_TYPE_MASK             0xf0 /* see above */
#define CONDITION_TYPE_STRING           0x10
#define CONDITION_TYPE_STRING_LIST      0x20

struct _FmActionCondition
{
    FmActionCondition *next;
    FmActionConditionType type;
    union {
        int num;        /* 0x0X */
        char *str;      /* 0x1X */
        char **list;    /* 0x2X */
    };
};

typedef enum
{
    ACTION_TARGET_CONTEXT = (1 << 0),
    ACTION_TARGET_LOCATION = (1 << 1),
    ACTION_TARGET_TOOLBAR = (1 << 2)
} FmActionTarget;

struct _FmActionInfo
{
    gint n_ref;
    GSList *profiles;   /* list of FmAction, noref */
    GFile *dir;         /* for monitoring, noref */
    char *id;           /* file basename */
    FmActionCondition *conditions; /* list of FmActionCondition, allocated */
    char *name;         /* Name field */
    char *desc;         /* Description field */
    char *icon_name;    /* Icon field */
    char *tooltip;      /* Tooltip field */
    char *toolbar_label; /* ToolbarLabel field */
    char *shortcut;     /* SuggestedShortcut field */
    FmActionTarget target;
    gboolean enabled : 1; /* Enabled field */
};

typedef enum
{
    EXEC_MODE_NORMAL,
    EXEC_MODE_TERMINAL,
    EXEC_MODE_EMBEDDED,
    EXEC_MODE_DISPLAY_OUTPUT
} FmActionExecMode;

struct _FmAction
{
    GObject parent_object;
    FmActionInfo *info; /* main data, ref */
    FmActionCondition *conditions; /* list of FmActionCondition, allocated */
    char *exec;         /* Exec field */
    char *binary;       /* only executable name from exec */
    char *path;         /* Path field */
    char *wm_class;     /* StartupWMClass field */
    char *exec_as;      /* ExecuteAs field */
    FmActionMenu *menu; /* menu this item was created for, noref */
    FmActionExecMode exec_mode; /* ExecutionMode field */
    gboolean use_sn : 1; /* StartupNotify field */
    gboolean hidden : 1; /* if denied by OnlyShowIn & NotShowIn */
};

struct _FmActionDir
{
    FmActionDir *next;
    GFile *dir;
    GFileMonitor *mon;
};

struct _FmActionMenu
{
    GObject parent_object;
    GFile *dir;         /* for monitoring, noref */
    char *id;           /* file basename */
    FmActionCondition *conditions; /* list of FmActionCondition, allocated */
    char **ids;         /* ItemsList field */
    char *name;         /* Name field */
    char *desc;         /* Description field */
    char *icon_name;    /* Icon field */
    char *tooltip;      /* Tooltip field */
    char *shortcut;     /* SuggestedShortcut field */
    FmActionCache *cache; /* cache this menu belongs to, noref */
    GList *children;    /* FmAction / FmActionMenu, ref - NULL for cache-only */
    FmActionMenu *parent; /* menu this item was created for, noref */
    FmFileInfoList *files; /* only for top menu - starting conditions */
    FmActionTarget target;
    gboolean enabled : 1; /* Enabled field */
};

/* references schema:
 *
 * FmActionCache
 *  +--> FmActionMenu
 *  +-----+--> FmAction
 *              +--> FmActionInfo
 */
struct _FmActionCache
{
    GObject parent_object;
};

static FmActionDir *cache_dirs; /* list of FmActionDir, allocated */
static GList *cache_actions; /* list of FmAction, ref */
static GList *cache_menus; /* list of FmActionMenu, ref */
static GSList *cache_to_update; /* strings list, allocated, updater_data invalid if NULL */
static FmActionCache **cache_updater_data; /* locked, freed by idle function */

#if GLIB_CHECK_VERSION(2, 32, 0)
static GWeakRef singleton;
#else
static int cache_n_ref = 0;
#endif

G_LOCK_DEFINE_STATIC(update); /* protects all lists */


/* ---- FmActionInfo manipulations */
static void _fm_action_conditions_free(FmActionCondition *this)
{
    while (this != NULL)
    {
        FmActionCondition *next = this->next;
        if ((this->type & CONDITION_TYPE_MASK) == CONDITION_TYPE_STRING)
            g_free(this->str);
        else if ((this->type & CONDITION_TYPE_MASK) == CONDITION_TYPE_STRING_LIST)
            g_strfreev(this->list);
        g_slice_free(FmActionCondition, this);
        this = next;
    }
}

/* lock is on, don't do any unref! */
static FmActionInfo *_fm_action_info_ref(FmActionInfo *info, FmAction *action)
{
    g_assert (g_slist_find(info->profiles, action) == NULL);
    info->profiles = g_slist_append(info->profiles, action);
    info->n_ref++;
    return info;
}

/* lock is on */
static void _fm_action_info_unref(FmActionInfo *info, FmAction *action)
{
    GSList *link = g_slist_find(info->profiles, action);

    g_return_if_fail(link != NULL);
    info->profiles = g_slist_delete_link(info->profiles, link);
    info->n_ref--;
    if (info->n_ref > 0)
        return;
    g_assert(info->profiles == NULL);
    _fm_action_conditions_free(info->conditions);
    g_free(info->id);
    g_free(info->name);
    g_free(info->desc);
    g_free(info->icon_name);
    g_free(info->tooltip);
    g_free(info->toolbar_label);
    g_free(info->shortcut);
    g_slice_free(FmActionInfo, info);
}


/* ---- Parameters expansion ---- */
static inline FmActionMenu *_get_top_menu(FmActionMenu *menu)
{
    if (menu != NULL)
        while (menu->parent != NULL)
            menu = menu->parent;
    return menu;
}

static gboolean _expand_params(GString *str, const char *line,
                               FmActionMenu *at, gboolean do_quote, GError **error)
{
    FmActionMenu *menu;
    FmFileInfoList *files;
    FmFileInfo *file;
    GList *head, *l;
    char *tmp, *c;
    const char *scheme;
    char *(*quote_func)(const char *);
    gsize len;

    if (line == NULL || line[0] == '\0')
    {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("Empty value"));
        return FALSE;
    }

    menu = _get_top_menu(at);
    if (menu == NULL)
    {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("Invalid selection"));
        return FALSE;
    }
    files = menu->files;
    if (menu->files == NULL) /* it's valid case if not context menu */
        menu->files = fm_file_info_list_new();
    head = fm_file_info_list_peek_head_link(menu->files);
    file = head->data;
    if (do_quote)
        quote_func = g_shell_quote;
    else
        quote_func = g_strdup;
    while (*line)
    {
        if (*line == '%') switch (*++line)
        {
        case '%':
            g_string_append_c(str, '%');
            break;
        case 'b':
            tmp = quote_func(fm_file_info_get_name(file));
            g_string_append(str, tmp);
            g_free(tmp);
            break;
        case 'B':
            tmp = quote_func(fm_file_info_get_name(file));
            g_string_append(str, tmp);
            g_free(tmp);
            for (l = head->next; l != NULL; l = l->next)
            {
                g_string_append_c(str, ' ');
                tmp = quote_func(fm_file_info_get_name(l->data));
                g_string_append(str, tmp);
                g_free(tmp);
            }
            break;
        case 'c':
            g_string_append_printf(str, "%d", fm_file_info_list_get_length(files));
            break;
        case 'd':
            if (menu->target == ACTION_TARGET_CONTEXT)
               tmp = fm_path_to_str(fm_path_get_parent(fm_file_info_get_path(file)));
            else
               tmp = fm_path_to_str(fm_file_info_get_path(file));
            c = quote_func(tmp);
            g_string_append(str, c);
            g_free(c);
            g_free(tmp);
            break;
        case 'D':
            if (menu->target == ACTION_TARGET_CONTEXT)
               tmp = fm_path_to_str(fm_path_get_parent(fm_file_info_get_path(file)));
            else
               tmp = fm_path_to_str(fm_file_info_get_path(file));
            c = quote_func(tmp);
            g_string_append(str, c);
            g_free(c);
            g_free(tmp);
            for (l = head->next; l != NULL; l = l->next)
            {
                g_string_append_c(str, ' ');
                tmp = fm_path_to_str(fm_path_get_parent(fm_file_info_get_path(l->data)));
                c = quote_func(tmp);
                g_string_append(str, c);
                g_free(c);
                g_free(tmp);
            }
            break;
        case 'f':
            tmp = fm_path_to_str(fm_file_info_get_path(file));
            c = quote_func(tmp);
            g_string_append(str, c);
            g_free(c);
            g_free(tmp);
            break;
        case 'F':
            tmp = fm_path_to_str(fm_file_info_get_path(file));
            c = quote_func(tmp);
            g_string_append(str, c);
            g_free(c);
            g_free(tmp);
            for (l = head->next; l != NULL; l = l->next)
            {
                g_string_append_c(str, ' ');
                tmp = fm_path_to_str(fm_file_info_get_path(l->data));
                c = quote_func(tmp);
                g_string_append(str, c);
                g_free(c);
                g_free(tmp);
            }
            break;
        case 'h':
            scheme = fm_path_get_basename(fm_path_get_scheme_path(fm_file_info_get_path(file)));
            len = strlen(scheme) - 1; /* it's at last '/' now */
            tmp = (char *)&scheme[len]; /* implement GNU-only memrchr() */
            while (tmp > scheme && *--tmp != '/'); /* it's at host now */
            if (*tmp != '/') /* it's not remote path */
                break;
            len -= (tmp - scheme);
            if (len == 0)
                break;
            c = &tmp[len];
            while (c > tmp && *--c != '@'); /* skip non-host part */
            if (*c == '@')
            {
                len -= ((++c) - tmp);
                if (len == 0)
                    break;
                tmp = c;
            }
            c = memchr(tmp, ':', len); /* discard port part */
            if (c != NULL)
                len = c - tmp;
            if (len > 0)
            {
                c = g_strndup(tmp, len);
                tmp = quote_func(c);
                g_free(c);
                g_string_append(str, tmp);
                g_free(tmp);
            }
            break;
        case 'm':
            g_string_append(str, fm_mime_type_get_type(fm_file_info_get_mime_type(file)));
            break;
        case 'M':
            g_string_append(str, fm_mime_type_get_type(fm_file_info_get_mime_type(file)));
            for (l = head->next; l; l = l->next)
            {
                g_string_append_c(str, ' ');
                g_string_append(str, fm_mime_type_get_type(fm_file_info_get_mime_type(l->data)));
            }
            break;
        case 'n':
            scheme = fm_path_get_basename(fm_path_get_scheme_path(fm_file_info_get_path(file)));
            len = strlen(scheme) - 1; /* it's at last '/' now */
            tmp = (char *)&scheme[len]; /* implement GNU-only memrchr() */
            while (tmp > scheme && *--tmp != '/'); /* it's at host now */
            if (*tmp != '/') /* it's not remote path */
                break;
            len -= (tmp - scheme);
            if (len == 0)
                break;
            c = &tmp[len];
            while (c > tmp && *--c != '@'); /* skip non-host part */
            if (*c == '@')
            {
                len = c - tmp;
                if (len == 0)
                    break;
                c = memchr(tmp, ':', len); /* URI might contain password? */
                if (c != NULL)
                    len = c - tmp;
                if (len > 0)
                    g_string_append_len(str, tmp, len);
            }
            break;
        case 'o':
            /* FIXME */
            break;
        case 'O':
            /* FIXME */
            break;
        case 'p':
            scheme = fm_path_get_basename(fm_path_get_scheme_path(fm_file_info_get_path(file)));
            len = strlen(scheme) - 1; /* it's at last '/' now */
            tmp = (char *)&scheme[len]; /* implement GNU-only memrchr() */
            while (tmp > scheme && *--tmp != '/'); /* it's at host now */
            if (*tmp != '/') /* it's not remote path */
                break;
            len -= (tmp - scheme);
            if (len == 0)
                break;
            c = &tmp[len];
            while (c > tmp && *--c != '@'); /* skip non-host part */
            if (*c == '@')
            {
                len -= ((++c) - tmp);
                if (len == 0)
                    break;
                tmp = c;
            }
            c = memchr(tmp, ':', len); /* find port part */
            if (c != NULL && &c[1] < &tmp[len])
                g_string_append_len(str, &c[1], (c - tmp) + 1);
            break;
        case 's':
            scheme = fm_path_get_basename(fm_path_get_scheme_path(fm_file_info_get_path(file)));
            c = strchr(scheme, ':');
            if (c == NULL || c == scheme)
                g_string_append(str, "file");
            else
                g_string_append_len(str, scheme, c - scheme);
            break;
        case 'u':
            tmp = fm_path_to_uri(fm_file_info_get_path(file));
            c = quote_func(tmp);
            g_string_append(str, c);
            g_free(c);
            g_free(tmp);
            break;
        case 'U':
            tmp = fm_path_to_uri(fm_file_info_get_path(file));
            c = quote_func(tmp);
            g_string_append(str, c);
            g_free(c);
            g_free(tmp);
            for (l = head->next; l != NULL; l = l->next)
            {
                g_string_append_c(str, ' ');
                tmp = fm_path_to_uri(fm_file_info_get_path(l->data));
                c = quote_func(tmp);
                g_string_append(str, c);
                g_free(c);
                g_free(tmp);
            }
            break;
        case 'w':
            scheme = fm_file_info_get_name(file);
            tmp = quote_func(scheme);
            c = strrchr(tmp, '.');
            if (c != NULL)
                g_string_append_len(str, tmp, c - tmp);
            else
                g_string_append(str, tmp);
            g_free(tmp);
            break;
        case 'W':
            scheme = fm_file_info_get_name(file);
            tmp = quote_func(scheme);
            c = strrchr(tmp, '.');
            if (c != NULL)
                g_string_append_len(str, tmp, c - tmp);
            else
                g_string_append(str, tmp);
            g_free(tmp);
            for (l = head->next; l != NULL; l = l->next)
            {
                g_string_append_c(str, ' ');
                scheme = fm_file_info_get_name(l->data);
                tmp = quote_func(scheme);
                c = strrchr(tmp, '.');
                if (c != NULL)
                    g_string_append_len(str, tmp, c - tmp);
                else
                    g_string_append(str, tmp);
                g_free(tmp);
            }
            break;
        case 'x':
            scheme = fm_file_info_get_name(file);
            c = strrchr(scheme, '.');
            if (c != NULL)
                g_string_append(str, &c[1]);
            break;
        case 'X':
            scheme = fm_file_info_get_name(file);
            c = strrchr(scheme, '.');
            if (c != NULL)
                g_string_append(str, &c[1]);
            for (l = head->next; l != NULL; l = l->next)
            {
                g_string_append_c(str, ' ');
                scheme = fm_file_info_get_name(l->data);
                c = strrchr(scheme, '.');
                if (c != NULL)
                    g_string_append(str, &c[1]);
            }
            break;
        default: ; /* ignore invalid */
        }
        else
            g_string_append_c(str, *line);
        line++;
    }
    return TRUE;
}


/* ---- FmAction class ---- */
static void fm_action_g_app_info_init(GAppInfoIface *iface);

G_DEFINE_TYPE_WITH_CODE(FmAction, fm_action, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(G_TYPE_APP_INFO,
                                              fm_action_g_app_info_init))

static void fm_action_finalize(GObject *object)
{
    FmAction *action = FM_ACTION(object);

    G_LOCK(update);
    _fm_action_info_unref(action->info, action);
    G_UNLOCK(update);
    _fm_action_conditions_free(action->conditions);
    g_free(action->exec);
    g_free(action->binary);
    g_free(action->path);
    g_free(action->wm_class);
    g_free(action->exec_as);
    g_assert(action->menu == NULL); /* it should've been reset by menu */

    G_OBJECT_CLASS(fm_action_parent_class)->finalize(object);
}

static void fm_action_class_init(FmActionClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = fm_action_finalize;
}

static void fm_action_init(FmAction *item)
{
    /* nothing */
}

/* lock is on */
static FmAction * _fm_action_new(void)
{
    return (FmAction *)g_object_new(FM_TYPE_ACTION, NULL);
}

static GAppInfo * _fm_action_dup(GAppInfo *appinfo)
{
    FmAction *action = FM_ACTION(appinfo);
    FmAction *dup = _fm_action_new();

    G_LOCK(update);
    dup->info = _fm_action_info_ref(action->info, dup);
    G_UNLOCK(update);
    /* conditions for duplicate should be NULL */
    dup->exec = g_strdup(action->exec);
    dup->binary = g_strdup(action->binary);
    dup->path = g_strdup(action->path);
    dup->wm_class = g_strdup(action->wm_class);
    dup->exec_as = g_strdup(action->exec_as);
    /* menu for duplicate should be NULL */
    dup->exec_mode = action->exec_mode;
    dup->use_sn = action->use_sn;
    return (GAppInfo *)dup;
}

static gboolean _fm_action_equal(GAppInfo *appinfo1, GAppInfo *appinfo2)
{
    return (FM_ACTION(appinfo1)->info == FM_ACTION(appinfo2)->info);
}

static const char * _fm_action_get_id(GAppInfo *appinfo)
{
    return FM_ACTION(appinfo)->info->id;
}

static const char * _fm_action_get_name(GAppInfo *appinfo)
{
//    _expand_params(str, action->info->name, action->menu, NULL);
    return FM_ACTION(appinfo)->info->name;
}

static const char * _fm_action_get_description(GAppInfo *appinfo)
{
    return FM_ACTION(appinfo)->info->desc;
}

static const char * _fm_action_get_executable(GAppInfo *appinfo)
{
    return FM_ACTION(appinfo)->binary;
}

static GIcon * _fm_action_get_icon(GAppInfo *appinfo)
{
    GString *str = g_string_sized_new(64);
    FmAction *action = FM_ACTION(appinfo);
    GIcon *icon = NULL;

    _expand_params(str, action->info->icon_name, action->menu, FALSE, NULL);
    if (str->len > 0)
        icon = (GIcon *)fm_icon_from_name(str->str);
    g_string_free(str, TRUE);
    return icon;
}

struct ChildSetup
{
    char *display;
    char *sn_id;
};

static void child_setup(gpointer user_data)
{
    struct ChildSetup* data = (struct ChildSetup*)user_data;
    if (data->display)
        g_setenv("DISPLAY", data->display, TRUE);
    if (data->sn_id)
        g_setenv("DESKTOP_STARTUP_ID", data->sn_id, TRUE);
}

static gboolean _do_launch(FmAction *action, GAppLaunchContext *launch_context,
                           GError **error)
{
    GString *str = g_string_sized_new(64);
    char** argv;
    int argc;
    gboolean ok = FALSE;

/*
    if (action->exec_mode == EXEC_MODE_EMBEDDED && menu->exec_embedded != NULL)
    {
        if (_expand_params(str, action->exec, action->menu, error)
            ok = menu->exec_embedded(str->str, menu->embedded_data); // launch embedded
        goto finish;
    }
*/
    if (action->exec_mode != EXEC_MODE_NORMAL)
    {
        FmTerminal* term;
        static FmTerminal xterm_def = { .program = "xterm", .open_arg = "-e" };

        /* prepend terminal to the line */
        term = fm_terminal_dup_default(NULL);
        if (!term) /* fallback to xterm if a terminal emulator is not found. */
            term = &xterm_def;
        g_string_assign(str, term->program);
        g_string_append_c(str, ' ');
        if (term->custom_args)
        {
            g_string_append(str, term->custom_args);
            g_string_append_c(str, ' ');
        }
        if (action->exec_mode == EXEC_MODE_DISPLAY_OUTPUT && term->noclose_arg)
            g_string_append(str, term->noclose_arg);
        else
            g_string_append(str, term->open_arg);
        g_string_append_c(str, ' ');
        if (term != &xterm_def)
            g_object_unref(term);
    }
    if (!_expand_params(str, action->exec, action->menu, TRUE, error))
        goto finish;
    if (g_shell_parse_argv(str->str, &argc, &argv, error))
    {
        FmActionMenu *menu;
        GList *launched_files, *l;
        struct ChildSetup data;

        data.display = NULL;
        data.sn_id = NULL;
        if (launch_context)
        {
            menu = _get_top_menu(action->menu); /* validated by _expand_params */
            launched_files = NULL;
            for (l = fm_file_info_list_peek_head_link(menu->files); l; l = l->next)
                launched_files = g_list_prepend(launched_files,
                                fm_path_to_gfile(fm_file_info_get_path(l->data)));
            launched_files = g_list_reverse(launched_files);
            data.display = g_app_launch_context_get_display(launch_context,
                                                            G_APP_INFO(action),
                                                            launched_files);
            if (action->use_sn)
                data.sn_id = g_app_launch_context_get_startup_notify_id(launch_context,
                                                                        G_APP_INFO(action),
                                                                        launched_files);
            g_list_free_full(launched_files, g_object_unref);
        }
        g_string_truncate(str, 0);
        if (action->path)
            _expand_params(str, action->path, action->menu, FALSE, NULL);
        if (str->str[0] != '/')
            _expand_params(str, "%d", action->menu, FALSE, NULL); /* see the spec */
        ok = g_spawn_async(str->str, argv, NULL,
                           G_SPAWN_SEARCH_PATH, child_setup, &data, NULL, error);
        if (!ok)
        {
            if (data.sn_id)
                g_app_launch_context_launch_failed(launch_context, data.sn_id);
        }
        g_strfreev(argv);
    }

finish:
    g_string_free(str, TRUE);
    return ok;
}

static gboolean _fm_action_launch(GAppInfo *appinfo,
                                  GList *files,
                                  GAppLaunchContext *launch_context,
                                  GError **error)
{
    /* NOTE: we always launch selection not argument */
    return _do_launch(FM_ACTION(appinfo), launch_context, error);
}

static gboolean _fm_action_supports_uris(GAppInfo *appinfo)
{
    const char *exec = FM_ACTION(appinfo)->exec;

    while (*exec)
    {
        if (*exec++ != '%')
            continue;
        if (*exec == 'u' || *exec == 'U')
            return TRUE;
        exec++;
    }
    return FALSE;
}

static gboolean _fm_action_true(GAppInfo *appinfo)
{
    return TRUE;
}

static gboolean _fm_action_launch_uris(GAppInfo *appinfo,
                                       GList *uris,
                                       GAppLaunchContext *launch_context,
                                       GError **error)
{
    /* NOTE: we always launch selection not argument */
    return _do_launch(FM_ACTION(appinfo), launch_context, error);
}

static gboolean _fm_action_should_show(GAppInfo *appinfo)
{
    return FM_ACTION(appinfo)->info->enabled;
}

#define ERROR_UNSUPPORTED(err) g_set_error_literal(err, G_IO_ERROR, \
                        G_IO_ERROR_NOT_SUPPORTED, _("Operation not supported"))

/* For changing associations */
static gboolean _fm_action_return_false_unsupported(GAppInfo *appinfo,
                                                    const char *content_type,
                                                    GError **error)
{
    ERROR_UNSUPPORTED(error);
    return FALSE;
}

static gboolean _fm_action_false(GAppInfo *appinfo)
{
    return FALSE;
}

static gboolean _fm_action_do_delete(GAppInfo *appinfo)
{
    /* FIXME: TODO */
    return FALSE;
}

static const char * _fm_action_get_commandline(GAppInfo *appinfo)
{
    return FM_ACTION(appinfo)->exec;
}

#if GLIB_CHECK_VERSION(2, 24, 0)
static const char * _fm_action_get_display_name(GAppInfo *appinfo)
{
//    _expand_params(str, action->info->desc, action->menu, NULL);
    if (FM_ACTION(appinfo)->info->tooltip != NULL)
        return FM_ACTION(appinfo)->info->tooltip;
    return _fm_action_get_name(appinfo);
}
#endif

static void fm_action_g_app_info_init(GAppInfoIface *iface)
{
    iface->dup = _fm_action_dup;
    iface->equal = _fm_action_equal;
    iface->get_id = _fm_action_get_id;
    iface->get_name = _fm_action_get_name;
    iface->get_description = _fm_action_get_description;
    iface->get_executable = _fm_action_get_executable;
    iface->get_icon = _fm_action_get_icon;
    iface->launch = _fm_action_launch;
    iface->supports_uris = _fm_action_supports_uris;
    iface->supports_files = _fm_action_true; /* FIXME: is this right? */
    iface->launch_uris = _fm_action_launch_uris;
    iface->should_show = _fm_action_should_show;
    iface->set_as_default_for_type = _fm_action_return_false_unsupported;
    iface->set_as_default_for_extension = _fm_action_return_false_unsupported;
    iface->add_supports_type = _fm_action_return_false_unsupported;
    iface->can_remove_supports_type = _fm_action_false;
    iface->remove_supports_type = _fm_action_return_false_unsupported;
    iface->can_delete = _fm_action_false;
    iface->do_delete = _fm_action_do_delete;
    iface->get_commandline = _fm_action_get_commandline;
#if GLIB_CHECK_VERSION(2, 24, 0)
    iface->get_display_name = _fm_action_get_display_name;
#endif
#if GLIB_CHECK_VERSION(2, 28, 0)
    iface->set_as_last_used_for_type = _fm_action_return_false_unsupported;
#endif
}


/* ---- FmActionMenu class ---- */
static void _fm_action_menu_free_children(FmActionMenu *menu)
{
    /* recursively free the allocated menu tree */
    while (menu->children)
    {
        if (menu->children->data == NULL) ; /* separator */
        else if (FM_IS_ACTION(menu->children->data))
        {
            FmAction *action = menu->children->data;
            G_LOCK(update);
            g_assert(action->menu == menu);
            action->menu = NULL;
            G_UNLOCK(update);
        }
        else
        {
            FmActionMenu *submenu = menu->children->data;
            g_assert(FM_IS_ACTION_MENU(submenu));
            g_assert(submenu->parent == menu);
            G_LOCK(update);
            submenu->parent = NULL;
            G_UNLOCK(update);
            _fm_action_menu_free_children(submenu);
        }
        if (menu->children->data != NULL)
            g_object_unref(menu->children->data);
        menu->children = g_list_delete_link(menu->children, menu->children);
    }
}

static void fm_action_menu_g_app_info_init(GAppInfoIface *iface);

G_DEFINE_TYPE_WITH_CODE(FmActionMenu, fm_action_menu, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(G_TYPE_APP_INFO,
                                              fm_action_menu_g_app_info_init))

static void fm_action_menu_finalize(GObject *object)
{
    FmActionMenu *menu = FM_ACTION_MENU(object);

    g_free(menu->id);
    _fm_action_conditions_free(menu->conditions);
    g_strfreev(menu->ids);
    g_free(menu->name);
    g_free(menu->desc);
    g_free(menu->icon_name);
    g_free(menu->tooltip);
    g_free(menu->shortcut);
    _fm_action_menu_free_children(menu);
    g_assert(menu->parent == NULL); /* it should've been reset by parent menu */
    if (menu->files != NULL)
        fm_file_info_list_unref(menu->files);

    G_OBJECT_CLASS(fm_action_menu_parent_class)->finalize(object);
}

static void fm_action_menu_class_init(FmActionMenuClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = fm_action_menu_finalize;
}

static void fm_action_menu_init(FmActionMenu *item)
{
    /* nothing */
}

/* lock is on */
static FmActionMenu * _fm_action_menu_new(void)
{
    return (FmActionMenu *)g_object_new(FM_TYPE_ACTION_MENU, NULL);
}

static GAppInfo * _fm_action_menu_dup(GAppInfo *appinfo)
{
    FmActionMenu *menu = FM_ACTION_MENU(appinfo);
    FmActionMenu *dup = _fm_action_menu_new();

    /* conditions, ids, dir for duplicate should be NULL */
    dup->id = g_strdup(menu->id);
    dup->name = g_strdup(menu->name);
    dup->desc = g_strdup(menu->desc);
    dup->icon_name = g_strdup(menu->icon_name);
    dup->tooltip = g_strdup(menu->tooltip);
    dup->shortcut = g_strdup(menu->shortcut);
    /* duplicate isn't in cache nor in any menu tree */
    dup->enabled = menu->enabled;
    return (GAppInfo *)dup;
}

static gboolean _fm_action_menu_equal(GAppInfo *appinfo1, GAppInfo *appinfo2)
{
    return (g_strcmp0(FM_ACTION_MENU(appinfo1)->id, FM_ACTION_MENU(appinfo2)->id) == 0);
}

static const char * _fm_action_menu_get_id(GAppInfo *appinfo)
{
    return FM_ACTION_MENU(appinfo)->id;
}

static const char * _fm_action_menu_get_name(GAppInfo *appinfo)
{
//    _expand_params(str, menu->name, menu, NULL);
    return FM_ACTION_MENU(appinfo)->name;
}

static const char * _fm_action_menu_get_description(GAppInfo *appinfo)
{
    return FM_ACTION_MENU(appinfo)->desc;
}

static const char * _fm_action_menu_get_executable(GAppInfo *appinfo)
{
    return NULL;
}

static GIcon * _fm_action_menu_get_icon(GAppInfo *appinfo)
{
    GString *str = g_string_sized_new(64);
    FmActionMenu *menu = FM_ACTION_MENU(appinfo);
    GIcon *icon = NULL;

    _expand_params(str, menu->icon_name, menu, FALSE, NULL);
    if (str->len > 0)
        icon = (GIcon *)fm_icon_from_name(str->str);
    g_string_free(str, TRUE);
    return icon;
}

static gboolean _fm_action_menu_launch(GAppInfo *appinfo,
                                  GList *files,
                                  GAppLaunchContext *launch_context,
                                  GError **error)
{
    ERROR_UNSUPPORTED(error);
    return FALSE;
}

static gboolean _fm_action_menu_should_show(GAppInfo *appinfo)
{
    return FM_ACTION_MENU(appinfo)->enabled;
}

static gboolean _fm_action_menu_do_delete(GAppInfo *appinfo)
{
    /* FIXME: TODO */
    return FALSE;
}

#if GLIB_CHECK_VERSION(2, 24, 0)
static const char * _fm_action_menu_get_display_name(GAppInfo *appinfo)
{
//    _expand_params(str, menu->desc, menu, NULL);
    return FM_ACTION_MENU(appinfo)->tooltip;
}
#endif

static void fm_action_menu_g_app_info_init(GAppInfoIface *iface)
{
    iface->dup = _fm_action_menu_dup;
    iface->equal = _fm_action_menu_equal;
    iface->get_id = _fm_action_menu_get_id;
    iface->get_name = _fm_action_menu_get_name;
    iface->get_description = _fm_action_menu_get_description;
    iface->get_executable = _fm_action_menu_get_executable;
    iface->get_icon = _fm_action_menu_get_icon;
    iface->launch = _fm_action_menu_launch;
    iface->supports_uris = _fm_action_false;
    iface->supports_files = _fm_action_false;
    iface->launch_uris = _fm_action_menu_launch;
    iface->should_show = _fm_action_menu_should_show;
    iface->set_as_default_for_type = _fm_action_return_false_unsupported;
    iface->set_as_default_for_extension = _fm_action_return_false_unsupported;
    iface->add_supports_type = _fm_action_return_false_unsupported;
    iface->can_remove_supports_type = _fm_action_false;
    iface->remove_supports_type = _fm_action_return_false_unsupported;
    iface->can_delete = _fm_action_false;
    iface->do_delete = _fm_action_menu_do_delete;
    iface->get_commandline = _fm_action_menu_get_executable;
#if GLIB_CHECK_VERSION(2, 24, 0)
    iface->get_display_name = _fm_action_menu_get_display_name;
#endif
#if GLIB_CHECK_VERSION(2, 28, 0)
    iface->set_as_last_used_for_type = _fm_action_return_false_unsupported;
#endif
}


/* ---- FmActionCache class ---- */
static void _action_cache_monitor_event(GFileMonitor *mon, GFile *gf, GFile *other,
                                        GFileMonitorEvent evt, FmActionCache *cache);

G_DEFINE_TYPE(FmActionCache, fm_action_cache, G_TYPE_OBJECT);

static void _fm_action_cache_finalize(GObject *object)
{
    FmActionCache *cache = FM_ACTION_CACHE(object);
    GSList *l_to_update;
    GList *l_actions, *l_menus;
    FmActionDir *l_dirs;

    G_LOCK(update); /* lock it ahead to prevent race condition */
#if !GLIB_CHECK_VERSION(2, 32, 0)
    if (!g_atomic_int_dec_and_test(&cache_n_ref))
    {
        G_UNLOCK(update);
        goto finish;
    }
#endif
    if (cache_to_update != NULL)
        *cache_updater_data = NULL; /* terminate the updater */
    /* updater is finished now, can free everything */
    l_to_update = cache_to_update;
    cache_to_update = NULL;
    l_actions = cache_actions;
    cache_actions = NULL;
    l_menus = cache_menus;
    cache_menus = NULL;
    l_dirs = cache_dirs;
    cache_dirs = NULL;
    G_UNLOCK(update);
    g_slist_free_full(l_to_update, g_free);
    g_list_free_full(l_actions, g_object_unref);
    g_list_free_full(l_menus, g_object_unref);
    while (l_dirs != NULL)
    {
        FmActionDir *dir = l_dirs;
        l_dirs = dir->next;
        g_signal_handlers_disconnect_by_func(dir->mon, _action_cache_monitor_event,
                                             cache);
        g_object_unref(dir->mon);
        g_object_unref(dir->dir);
        g_slice_free(FmActionDir, dir);
    }
#if !GLIB_CHECK_VERSION(2, 32, 0)
finish:
#endif

    G_OBJECT_CLASS(fm_action_cache_parent_class)->finalize(object);
}

static void fm_action_cache_class_init(FmActionCacheClass *klass)
{
    GObjectClass *g_object_class;

    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->finalize = _fm_action_cache_finalize;
}

static void fm_action_cache_init(FmActionCache *cache)
{
    /* nothing */
}


/* ---- internal API ---- */
static inline FmActionCondition *_add_condition_int(FmActionCondition *clist,
                                                    FmActionConditionType type,
                                                    gint val)
{
    FmActionCondition *cond = g_slice_new(FmActionCondition);
    cond->next = clist;
    cond->type = type;
    cond->num = val;
    return cond;
}

static inline FmActionCondition *_add_condition_str(FmActionCondition *clist,
                                                    FmActionConditionType type,
                                                    gchar *val)
{
    FmActionCondition *cond = g_slice_new(FmActionCondition);
    cond->next = clist;
    cond->type = type;
    cond->str = val;
    return cond;
}

static inline FmActionCondition *_add_condition_str_list(FmActionCondition *clist,
                                                         FmActionConditionType type,
                                                         gchar **val)
{
    FmActionCondition *cond = g_slice_new(FmActionCondition);
    cond->next = clist;
    cond->type = type;
    cond->list = val;
    return cond;
}

/* lock is on, don't do any unref! */
static FmActionCondition *_g_key_file_get_conditions(GKeyFile *kf, const char *group)
{
    FmActionCondition *clist = NULL;
    char *cval, *c;
    int ival;
    char **lval, **x;
    GError *error;
    gboolean bval;
    FmActionConditionType type;

    /* string lists */
    lval = g_key_file_get_string_list(kf, group, "MimeTypes", NULL, NULL);
    if (lval)
        clist = _add_condition_str_list(clist, CONDITION_TYPE_MIME, lval);
    lval = g_key_file_get_string_list(kf, group, "Basenames", NULL, NULL);
    if (lval)
    {
        type = CONDITION_TYPE_BASENAME;
        error = NULL;
        bval = g_key_file_get_boolean(kf, group, "Matchcase", &error);
        if (error)
            g_error_free(error);
        else if (!bval)
        {
            type = CONDITION_TYPE_NCASE_BASENAME;
            for (x = lval; x[0] != NULL; x++)
            {
                c = x[0];
                x[0] = g_utf8_casefold(c, -1);
                g_free(c);
            }
        }
        clist = _add_condition_str_list(clist, type, lval);
    }
    lval = g_key_file_get_string_list(kf, group, "Schemes", NULL, NULL);
    if (lval)
        clist = _add_condition_str_list(clist, CONDITION_TYPE_SCHEME, lval);
    lval = g_key_file_get_string_list(kf, group, "Folders", NULL, NULL);
    if (lval)
    {
        /* add pattern* for each pattern that don't ends with '*' */
        ival = g_strv_length(lval);
        lval = g_renew(char *, lval, ival * 2 + 1);
        for (x = lval; x[0] != NULL; x++)
        {
            c = x[0] + strlen(x[0]);
            if (c == x[0] || c[-1] != '*')
                lval[ival++] = g_strconcat(x[0], "/*", NULL);
        }
        lval[ival] = NULL;
        clist = _add_condition_str_list(clist, CONDITION_TYPE_FOLDER, lval);
    }
    /* strings */
    cval = g_key_file_get_string(kf, group, "ShowIfRunning", NULL);
    if (cval)
        clist = _add_condition_str(clist, CONDITION_TYPE_RUN, cval);
    cval = g_key_file_get_string(kf, group, "TryExec", NULL);
    if (cval)
        clist = _add_condition_str(clist, CONDITION_TYPE_TRY_EXEC, cval);
    cval = g_key_file_get_string(kf, group, "ShowIfRegistered", NULL);
    if (cval)
        clist = _add_condition_str(clist, CONDITION_TYPE_DBUS, cval);
    cval = g_key_file_get_string(kf, group, "ShowIfTrue", NULL);
    if (cval)
        clist = _add_condition_str(clist, CONDITION_TYPE_OUT_TRUE, cval);
    /* integers */
    lval = g_key_file_get_string_list(kf, group, "Capabilities", NULL, NULL);
    if (lval)
    {
        ival = 0;
        for (x = lval; x[0] != NULL; x++)
        {
            if (x[0][0] == '!')
            {
                if (strcmp(x[0] + 1, "Owner") == 0)
                    ival |= (CONDITION_CAPS_OWNER << CONDITION_CAPS_SHIFT_NEGATE);
                else if (strcmp(x[0] + 1, "Readable") == 0)
                    ival |= (CONDITION_CAPS_READABLE << CONDITION_CAPS_SHIFT_NEGATE);
                else if (strcmp(x[0] + 1, "Writable") == 0)
                    ival |= (CONDITION_CAPS_WRITABLE << CONDITION_CAPS_SHIFT_NEGATE);
                else if (strcmp(x[0] + 1, "Executable") == 0)
                    ival |= (CONDITION_CAPS_EXECUTABLE << CONDITION_CAPS_SHIFT_NEGATE);
                else if (strcmp(x[0] + 1, "Local") == 0)
                    ival |= (CONDITION_CAPS_LOCAL << CONDITION_CAPS_SHIFT_NEGATE);
            }
            else if (strcmp(x[0], "Owner") == 0)
                ival |= CONDITION_CAPS_OWNER;
            else if (strcmp(x[0], "Readable") == 0)
                ival |= CONDITION_CAPS_READABLE;
            else if (strcmp(x[0], "Writable") == 0)
                ival |= CONDITION_CAPS_WRITABLE;
            else if (strcmp(x[0], "Executable") == 0)
                ival |= CONDITION_CAPS_EXECUTABLE;
            else if (strcmp(x[0], "Local") == 0)
                ival |= CONDITION_CAPS_LOCAL;
        }
        if (ival != 0)
            clist = _add_condition_int(clist, CONDITION_TYPE_CAPS, ival);
        g_strfreev(lval);
    }
    cval = g_key_file_get_string(kf, group, "SelectionCount", NULL);
    if (cval)
    {
        c = cval;
        if (c[0] == '<')
            c++, type = CONDITION_TYPE_COUNT_LESS;
        else if (c[0] == '>')
            c++, type = CONDITION_TYPE_COUNT_MORE;
        else if (c[0] == '=')
            c++, type = CONDITION_TYPE_COUNT;
        else
            type = CONDITION_TYPE_COUNT;
        while (c[0] == ' ') c++;
        ival = strtoul(c, &c, 10);
        while (c[0] == ' ') c++;
        if (c[0] == '\0')
            clist = _add_condition_int(clist, type, ival);
        g_free(cval);
    }
    return clist;
}

/* lock is on, don't do any unref! */
static void fm_actions_add_action_from_keyfile(FmActionCache *cache,
                                               GKeyFile *kf, GFile *parent,
                                               const char *id, const char *name)
{
    FmActionInfo *info;
    GString *key;
    char **profiles;
    char **profile;
    char *exec, *c;
    FmAction *action;

    if (g_key_file_get_boolean(kf, "Desktop Entry", "Hidden", NULL)) /* deleted */
        return;

    info = g_slice_new0(FmActionInfo);
    key = g_string_sized_new(32);
    profiles = g_key_file_get_string_list(kf, "Desktop Entry", "Profiles", NULL, NULL);
    if (profiles) for (profile = profiles; *profile; profile++)
    {
        g_string_printf(key, "X-Action-Profile %s", *profile);
        exec = g_key_file_get_string(kf, key->str, "Exec", NULL);
        if (exec == NULL)
            continue;
        action = _fm_action_new();
        action->exec = exec;
        c = strchr(exec, ' ');
        if (c == NULL)
            action->binary = g_strdup(exec);
        else
            action->binary = g_strndup(exec, c - exec);
        action->info = _fm_action_info_ref(info, action);
        action->path = g_key_file_get_string(kf, key->str, "Path", NULL);
        action->wm_class = g_key_file_get_string(kf, key->str, "StartupWMClass", NULL);
        action->exec_as = g_key_file_get_string(kf, key->str, "ExecuteAs", NULL);
        action->use_sn = g_key_file_get_boolean(kf, key->str, "StartupNotify", NULL);
        c = g_key_file_get_string(kf, key->str, "ExecutionMode", NULL);
        if (c != NULL)
        {
            if (strcmp(c, "Terminal") == 0)
                action->exec_mode = EXEC_MODE_TERMINAL;
            else if (strcmp(c, "Embedded") == 0)
                action->exec_mode = EXEC_MODE_EMBEDDED;
            else if (strcmp(c, "DisplayOutput") == 0)
                action->exec_mode = EXEC_MODE_DISPLAY_OUTPUT;
            g_free(c);
        }
        action->conditions = _g_key_file_get_conditions(kf, key->str);
        cache_actions = g_list_prepend(cache_actions, action);
        /* FIXME: check OnlyShowIn and NotShowIn into ->hidden */
    }
    g_strfreev(profiles);
    g_string_free(key, TRUE);
    if (info->profiles == NULL)
    {
        g_slice_free(FmActionInfo, info);
        return;
    }
    info->dir = parent;
    info->id = g_strdup(id);
    info->name = g_strdup(name);
    info->tooltip = g_key_file_get_locale_string(kf, "Desktop Entry", "Tooltip", NULL, NULL);
    info->icon_name = g_key_file_get_locale_string(kf, "Desktop Entry", "Icon", NULL, NULL);
    info->desc = g_key_file_get_locale_string(kf, "Desktop Entry", "Description", NULL, NULL);
    info->shortcut = g_key_file_get_string(kf, "Desktop Entry", "SuggestedShortcut", NULL);
    info->enabled = TRUE;
    /* FIXME: if enabled then check OnlyShowIn and NotShowIn */
    if (g_key_file_has_key(kf, "Desktop Entry", "Enabled", NULL))
        info->enabled = g_key_file_get_boolean(kf, "Desktop Entry", "Enabled", NULL);
    info->toolbar_label = g_key_file_get_locale_string(kf, "Desktop Entry",
                                                       "ToolbarLabel", NULL, NULL);
    info->conditions = _g_key_file_get_conditions(kf, "Desktop Entry");
    info->target = ACTION_TARGET_CONTEXT;
    if (g_key_file_has_key(kf, "Desktop Entry", "TargetContext", NULL) &&
        !g_key_file_get_boolean(kf, "Desktop Entry", "TargetContext", NULL))
        info->target = 0;
    else
        info->target = ACTION_TARGET_CONTEXT;
    if (g_key_file_get_boolean(kf, "Desktop Entry", "TargetLocation", NULL))
        info->target |= ACTION_TARGET_LOCATION;
    if (g_key_file_get_boolean(kf, "Desktop Entry", "TargetToolbar", NULL))
        info->target |= ACTION_TARGET_TOOLBAR;
}

/* lock is on, don't do any unref! */
static void fm_actions_add_menu_from_keyfile(FmActionCache *cache,
                                             GKeyFile *kf, GFile *parent,
                                             const char *id, const char *name)
{
    FmActionMenu *menu;
    char **ids;
    int i;

    if (g_key_file_get_boolean(kf, "Desktop Entry", "Hidden", NULL)) /* deleted */
        return;

    ids = g_key_file_get_string_list(kf, "Desktop Entry", "ItemsList", NULL, NULL);
    if (ids == NULL || ids[0] == NULL)
    {
        g_strfreev(ids);
        return;
    }

    menu = _fm_action_menu_new();
    menu->ids = ids;
    /* arggh! crazy Windows style "no-extension"! */
    for (i = 0; ids[i] != NULL; i++)
    {
        char *orig = ids[i];
        if (strcmp(orig, "SEPARATOR") == 0)
            ids[i] = g_strdup("");
        else
            ids[i] = g_strconcat(orig, ".desktop", NULL);
        g_free(orig);
    }
    menu->cache = cache;
    menu->dir = parent;
    menu->id = g_strdup(id);
    menu->name = g_strdup(name);
    menu->tooltip = g_key_file_get_locale_string(kf, "Desktop Entry", "Tooltip", NULL, NULL);
    menu->icon_name = g_key_file_get_locale_string(kf, "Desktop Entry", "Icon", NULL, NULL);
    menu->desc = g_key_file_get_locale_string(kf, "Desktop Entry", "Description", NULL, NULL);
    menu->shortcut = g_key_file_get_string(kf, "Desktop Entry", "SuggestedShortcut", NULL);
    menu->enabled = TRUE;
    if (g_key_file_has_key(kf, "Desktop Entry", "Enabled", NULL))
        menu->enabled = g_key_file_get_boolean(kf, "Desktop Entry", "Enabled", NULL);
    /* FIXME: if enabled then check OnlyShowIn and NotShowIn */
    menu->conditions = _g_key_file_get_conditions(kf, "Desktop Entry");
    cache_menus = g_list_prepend(cache_menus, menu);
}

/* lock is on, don't do any unref! */
static void fm_actions_add_for_keyfile(FmActionCache *cache, GKeyFile *kf,
                                       GFile *parent, const char *id)
{
    char *name = g_key_file_get_locale_string(kf, "Desktop Entry", "Name", NULL, NULL);
    if (name != NULL)
    {
        char *type = g_key_file_get_string(kf, "Desktop Entry", "Type", NULL);
        if (type != NULL && strcmp(type, "Menu") == 0)
            fm_actions_add_menu_from_keyfile(cache, kf, parent, id, name);
        else
            fm_actions_add_action_from_keyfile(cache, kf, parent, id, name);
        g_free(type);
        g_free(name);
    }
}

/* lock is on, don't do any unref! */
/* caller should free id */
static gboolean fm_action_file_may_update(FmActionCache *cache, const char *file,
                                          GFile **gfp, char **idp, GList **to_drop)
{
    GFile *gf, *parent;
    FmActionDir *dir, *test;
    char *id;
    GList *l;
    FmAction *action;
    FmActionMenu *menu;

    /* find parent GFile in the cache */
    gf = g_file_new_for_path(file);
    parent = g_file_get_parent(gf);
    *idp = id = g_file_get_basename(gf);
    g_object_unref(gf);
    for (dir = cache_dirs; dir; dir = dir->next)
        if (g_file_equal(parent, dir->dir))
            break;
    g_object_unref(parent);
    if (dir == NULL) /* invalid path */
        return FALSE;
    *gfp = dir->dir;
    /* check if id already is in the cache - either skip or drop it */
    for (l = cache_menus; l; l = l->next)
    {
        menu = l->data;
        if (strcmp(menu->id, id) == 0)
            break;
    }
    if (l != NULL)
    {
        for (test = cache_dirs; test != dir; test = test->next)
            if (test->dir == menu->dir)
                break;
        if (test != dir) /* found one earlier, don't replace */
            return FALSE;
        cache_menus = g_list_remove_link(cache_menus, l);
        /* file should be removed from cache but not unref since lock is on */
        *to_drop = g_list_concat(l, *to_drop);
        return TRUE;
    }
    for (l = cache_actions; l; l = l->next)
    {
        action = l->data;
        if (strcmp(action->info->id, id) == 0)
            break;
    }
    if (l == NULL) /* not found, safe to insert new */
        return TRUE;
    for (test = cache_dirs; test != dir; test = test->next)
        if (test->dir == action->info->dir)
            break;
    if (test != dir) /* found one earlier, don't replace */
        return FALSE;
    while (l != NULL) /* remove all actions of this id */
    {
        GList *next = l->next;

        cache_actions = g_list_remove_link(cache_actions, l);
        /* file should be removed from cache but not unref since lock is on */
        *to_drop = g_list_concat(l, *to_drop);
        while (next != NULL) /* find next profile of the same id */
            if (((FmAction *)next->data)->info == action->info)
                break;
            else
                next = next->next;
        l = next;
    }
    return TRUE;
}

static void fm_action_cache_ensure_updates(FmActionCache *cache)
{
    GSList *to_update;
    GList *to_drop;
    GKeyFile *kf;

    G_LOCK(update);
    to_update = cache_to_update;
    if (to_update != NULL)
        *cache_updater_data = NULL; /* terminate the updater */
    cache_to_update = NULL;
    G_UNLOCK(update);
    if (to_update == NULL)
        return;
    kf = g_key_file_new();
    to_drop = NULL;
    G_LOCK(update);
    while (to_update != NULL)
    {
        GFile *parent;
        char *id;

        if (fm_action_file_may_update(cache, to_update->data, &parent, &id, &to_drop) &&
            g_key_file_load_from_file(kf, to_update->data,
            G_KEY_FILE_KEEP_TRANSLATIONS, NULL)) /* NOTE: ignoring errors */
            fm_actions_add_for_keyfile(cache, kf, parent, id);
        g_free(to_update->data);
        g_free(id);
        to_update = g_slist_delete_link(to_update, to_update);
    }
    G_UNLOCK(update);
    g_key_file_free(kf);
    g_list_free_full(to_drop, g_object_unref);
}


/* ---- idle updater ---- */
static gboolean fm_actions_update_idle(gpointer data)
{
    FmActionCache *cache;
    GKeyFile *kf;
    GFile *parent;
    char *id;
    GList *to_drop;

    G_LOCK(update);
    cache = *((FmActionCache **)data);
    if (cache == NULL || cache_to_update == NULL)
    {
        /* terminated */
        G_UNLOCK(update);
        return FALSE;
    }
    to_drop = NULL;
    if (fm_action_file_may_update(cache, cache_to_update->data, &parent, &id, &to_drop))
    {
        kf = g_key_file_new();
        if (g_key_file_load_from_file(kf, cache_to_update->data,
            G_KEY_FILE_KEEP_TRANSLATIONS, NULL)) /* NOTE: ignoring errors */
            fm_actions_add_for_keyfile(cache, kf, parent, id);
        g_key_file_free(kf);
    }
    g_free(cache_to_update->data);
    g_free(id);
    cache_to_update = g_slist_delete_link(cache_to_update, cache_to_update);
    if (cache_to_update == NULL)
        cache = NULL; /* mark it to stop updater */
    G_UNLOCK(update);
    g_list_free_full(to_drop, g_object_unref);
    return (cache != NULL);
}

/* lock is on, don't do any unref! */
/* consumes filename */
static void fm_actions_schedule_update(FmActionCache *cache, char *filename)
{
    if (cache_to_update == NULL) /* no updater running now */
    {
        cache_updater_data = g_new(FmActionCache *, 1);
        *cache_updater_data = cache;
        g_idle_add_full(G_PRIORITY_LOW, fm_actions_update_idle,
                        cache_updater_data, g_free);
    }
    cache_to_update = g_slist_prepend(cache_to_update, filename);
}

static void _action_cache_monitor_event(GFileMonitor *mon, GFile *gf,
                                        GFile *other, GFileMonitorEvent evt,
                                        FmActionCache *cache)
{
    FmActionDir *dir;
    char *basename, *filename;
    GList *l, *next, *to_drop = NULL;
    GSList *sl, *sn;
    FmAction *action;
    FmActionMenu *menu;

    /* find dir matching mon */
    G_LOCK(update);
    for (dir = cache_dirs; dir; dir = dir->next)
        if (dir->mon == mon)
            break;
    G_UNLOCK(update);
    g_return_if_fail(dir != NULL);
    if (g_file_equal(gf, dir->dir))
        /* it's event on folder itself, ignoring */
        return;
    switch (evt)
    {
    case G_FILE_MONITOR_EVENT_CHANGED:
    case G_FILE_MONITOR_EVENT_CREATED:
        filename = g_file_get_path(gf);
        if (!filename || !g_str_has_suffix(filename, ".desktop"))
            /* ignore non-desktop files */
            g_free(filename);
        else
        {
            /* just schedule update */
            G_LOCK(update);
            fm_actions_schedule_update(cache, filename);
            G_UNLOCK(update);
        }
        break;
    case G_FILE_MONITOR_EVENT_DELETED:
        basename = g_file_get_basename(gf);
        filename = g_file_get_path(gf);
        G_LOCK(update);
        /* remove file from cache */
        for (l = cache_actions; l; l = l->next)
        {
            action = l->data;
            if (action->info->dir == dir->dir && strcmp(action->info->id, basename) == 0)
                break;
        }
        if (l != NULL)
        {
            while (l != NULL)
            {
                next = l->next;
                cache_actions = g_list_remove_link(cache_actions, l);
                to_drop = g_list_concat(l, to_drop);
                while (next != NULL)
                    if (((FmAction *)next->data)->info == action->info)
                        break;
                    else
                        next = next->next;
                l = next;
            }
        }
        else for (l = cache_menus; l; l = l->next)
        {
            menu = l->data;
            if (menu->dir == dir->dir && strcmp(menu->id, basename) == 0)
            {
                cache_menus = g_list_remove_link(cache_menus, l);
                to_drop = g_list_concat(l, to_drop);
                break;
            }
        }
        /* remove from queue if it's there */
        if (filename) for (sl = cache_to_update; sl; sl = sn)
        {
            sn = sl->next;
            if (strcmp(sl->data, filename) == 0)
            {
                g_free(sl->data);
                cache_to_update = g_slist_delete_link(cache_to_update, sl);
            }
        }
        G_UNLOCK(update);
        g_list_free_full(to_drop, g_object_unref);
        g_free(basename);
        g_free(filename);
        break;
#if GLIB_CHECK_VERSION(2, 24, 0)
    case G_FILE_MONITOR_EVENT_MOVED:
#endif
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
    case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
    case G_FILE_MONITOR_EVENT_UNMOUNTED:
        /* ignore those */
        break;
    }
}

/* lock is on, don't do any unref! */
/* prepends path to monitoring list */
static void fm_action_cache_add_directory(FmActionCache *cache, const char *path)
{
    FmActionDir *fmdir;
    GDir *dir;
    const char *name;

    dir = g_dir_open(path, 0, NULL);
    if (dir == NULL) /* unobtainable */
        return;
    while ((name = g_dir_read_name(dir)) != NULL)
    {
        if (g_str_has_suffix(name, ".desktop"))
            /* ignore non-desktop files */
            fm_actions_schedule_update(cache, g_build_filename(path, name, NULL));
    }
    g_dir_close(dir);
    fmdir = g_slice_new(FmActionDir);
    fmdir->next = cache_dirs;
    cache_dirs = fmdir;
    fmdir->dir = g_file_new_for_path(path);
    fmdir->mon = g_file_monitor_directory(fmdir->dir, G_FILE_MONITOR_NONE, NULL, NULL);
    g_signal_connect(fmdir->mon, "changed", G_CALLBACK(_action_cache_monitor_event),
                     cache);
}


/* ---- matching conditions (arggh!) ---- */
static gboolean _matches_cond(FmFileInfoList *files, FmFileInfo *location,
                              FmActionCondition *cond, FmActionMenu *root)
{
    GList *flist = NULL, *l;
    char **x, *tst, *s;
    const char *c;
    GString *str;
    GPatternSpec *pattern;
    gsize len;
    int num;
    gboolean match = TRUE, found, match_num;

    if (files != NULL)
        flist = fm_file_info_list_peek_head_link(files);
    num = g_list_length(flist);
    match_num = (num > 0); /* default */
    for ( ; match && cond; cond = cond->next)
    {
        found = TRUE;
        switch (cond->type)
        {
        case CONDITION_TYPE_COUNT:
            if (num != cond->num)
                match = FALSE;
            match_num = TRUE;
            break;
        case CONDITION_TYPE_COUNT_LESS:
            if (num >= cond->num)
                match = FALSE;
            match_num = TRUE;
            break;
        case CONDITION_TYPE_COUNT_MORE:
            if (num <= cond->num)
                match = FALSE;
            match_num = TRUE;
            break;
        case CONDITION_TYPE_CAPS: /* Capabilities */
            /* sanity check */
            if ((cond->num & (CONDITION_CAPS_OWNER | (CONDITION_CAPS_OWNER << CONDITION_CAPS_SHIFT_NEGATE)))
                    == (CONDITION_CAPS_OWNER | (CONDITION_CAPS_OWNER << CONDITION_CAPS_SHIFT_NEGATE)) ||
                (cond->num & (CONDITION_CAPS_READABLE | (CONDITION_CAPS_READABLE << CONDITION_CAPS_SHIFT_NEGATE)))
                    == (CONDITION_CAPS_READABLE | (CONDITION_CAPS_READABLE << CONDITION_CAPS_SHIFT_NEGATE)) ||
                (cond->num & (CONDITION_CAPS_WRITABLE | (CONDITION_CAPS_WRITABLE << CONDITION_CAPS_SHIFT_NEGATE)))
                    == (CONDITION_CAPS_WRITABLE | (CONDITION_CAPS_WRITABLE << CONDITION_CAPS_SHIFT_NEGATE)) ||
                (cond->num & (CONDITION_CAPS_EXECUTABLE | (CONDITION_CAPS_EXECUTABLE << CONDITION_CAPS_SHIFT_NEGATE)))
                    == (CONDITION_CAPS_EXECUTABLE | (CONDITION_CAPS_EXECUTABLE << CONDITION_CAPS_SHIFT_NEGATE)) ||
                (cond->num & (CONDITION_CAPS_LOCAL | (CONDITION_CAPS_LOCAL << CONDITION_CAPS_SHIFT_NEGATE)))
                    == (CONDITION_CAPS_LOCAL | (CONDITION_CAPS_LOCAL << CONDITION_CAPS_SHIFT_NEGATE)))
                match = FALSE;
            if (!match)
                break;
            if (cond->num & (CONDITION_CAPS_LOCAL | (CONDITION_CAPS_LOCAL << CONDITION_CAPS_SHIFT_NEGATE)))
            {
                match = fm_file_info_is_native(location);
                for (l = flist; match && l != NULL; l = l->next)
                    if (!fm_file_info_is_native(l->data))
                        match = FALSE;
                if ((cond->num & CONDITION_CAPS_LOCAL) == 0)
                    match = !match;
                if (!match)
                    break;
            }
            if (cond->num & (CONDITION_CAPS_READABLE | (CONDITION_CAPS_READABLE << CONDITION_CAPS_SHIFT_NEGATE)))
            {
                for (l = flist; match && l != NULL; l = l->next)
                    if (!fm_file_info_is_accessible(l->data))
                        match = FALSE;
                if ((cond->num & CONDITION_CAPS_READABLE) == 0)
                    match = !match;
                if (!match)
                    break;
            }
            if (cond->num & (CONDITION_CAPS_WRITABLE | (CONDITION_CAPS_WRITABLE << CONDITION_CAPS_SHIFT_NEGATE)))
            {
                match = fm_file_info_is_writable_directory(location);
                /* FIXME */
                //for (l == flist; match && l != NULL; l = l->next)
                //    if (!fm_file_info_is_writable(l->data))
                //        match = FALSE;
                if (!match)
                    break;
            }
            if (cond->num & (CONDITION_CAPS_EXECUTABLE | (CONDITION_CAPS_EXECUTABLE << CONDITION_CAPS_SHIFT_NEGATE)))
            {
                /* FIXME */
            }
            if (cond->num & (CONDITION_CAPS_OWNER | (CONDITION_CAPS_OWNER << CONDITION_CAPS_SHIFT_NEGATE)))
            {
                uid_t uid = geteuid();

                for (l = flist; match && l != NULL; l = l->next)
                    if (fm_file_info_get_uid(l->data) != uid)
                        match = FALSE;
                if ((cond->num & CONDITION_CAPS_OWNER) == 0)
                    match = !match;
            }
            break;
        case CONDITION_TYPE_RUN: /* ShowIfRunning */
            // FIXME: this is insane condition as it will require to scan whole /proc
//            _expand_params(str, cond->str, root, NULL);
            break;
        case CONDITION_TYPE_TRY_EXEC: /* TryExec */
            str = g_string_sized_new(64);
            _expand_params(str, cond->str, root, TRUE, NULL);
            s = NULL;
            match = FALSE;
            if (!g_path_is_absolute(str->str))
                s = g_find_program_in_path(str->str);
            if (s != NULL)
                match = g_file_test(s, G_FILE_TEST_IS_EXECUTABLE);
            g_free(s);
            g_string_free(str, TRUE);
            break;
        case CONDITION_TYPE_DBUS: /* ShowIfRegistered */
#if defined(ENABLE_DBUS) && GLIB_CHECK_VERSION(2, 24, 0)
            str = g_string_size_new(64);
            _expand_params(str, cond->str, root, TRUE, NULL);
            /* DBus call is taken from GLib sources: gio/tests/gdbus-names.c */
            conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
            result = g_dbus_connection_call_sync(conn,
                                                 "org.freedesktop.DBus",  /* bus name */
                                                 "/org/freedesktop/DBus", /* object path */
                                                 "org.freedesktop.DBus",  /* interface name */
                                                 "NameHasOwner",          /* method name */
                                                 g_variant_new("(s)", str->str),
                                                 G_VARIANT_TYPE("(b)"),
                                                 G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
            match = FALSE;
            if (result != NULL)
            {
                g_variant_get(result, "(b)", &match);
                g_variant_unref(result);
            }
            g_string_free(str, TRUE);
#endif
            break;
        case CONDITION_TYPE_OUT_TRUE: /* ShowIfTrue */
            // FIXME: another insane condition to execute a program to test
//            _expand_params(str, cond->str, root, NULL);
            break;
        case CONDITION_TYPE_MIME: /* MimeTypes */
            found = FALSE;
            for (x = cond->list; match && x[0] != NULL; x++)
            {
                tst = x[0];
                len = strlen(tst);
                if (len == 1 && tst[0] == '*')
                    found = TRUE; /* always matches */
                else if (tst[0] == '!') /* negate condition */
                {
                    if (strncmp(&tst[1], "all/", 4) == 0)
                    {
                        if (strcmp(&tst[5], "allfiles") == 0)
                        {
                            for (l = flist; match && l; l = l->next)
                                if (S_ISREG(fm_file_info_get_mode(l->data)))
                                    match = FALSE; /* found regular file */
                        }
                        else /* it's !all/all */
                            match = FALSE;
                    }
                    else
                    {
                        len--;
                        tst++;
                        if (len >= 3 && tst[len-1] == '*' && tst[len-2] == '/')
                            len -= 2;
                        for (l = flist; match && l; l = l->next)
                        {
                            FmMimeType *mime_type = fm_file_info_get_mime_type(l->data);
                            if (mime_type == NULL)
                                continue;
                            c = fm_mime_type_get_type(mime_type);
                            match = (strncmp(c, tst, len) != 0 ||
                                     c[len] != tst[len]);
                        }
                    }
                }
                else if (!found)
                {
                    if (strncmp(tst, "all/", 4) == 0)
                    {
                        if (strcmp(&tst[4], "allfiles") == 0)
                        {
                            for (l = flist; l; l = l->next)
                                if (!S_ISREG(fm_file_info_get_mode(l->data)))
                                    break; /* not regular file */
                            found = (flist != NULL && l == NULL);
                        }
                        else /* it's all/all */
                            found = TRUE;
                    }
                    else
                    {
                        if (len >= 3 && tst[len-1] == '*' && tst[len-2] == '/')
                            len -= 2;
                        for (l = flist; l; l = l->next)
                        {
                            FmMimeType *mime_type = fm_file_info_get_mime_type(l->data);
                            if (mime_type == NULL)
                                continue;
                            c = fm_mime_type_get_type(mime_type);
                            if (strncmp(c, tst, len) != 0 || c[len] != tst[len])
                                break; /* doesn't match */
                        }
                        found = (flist != NULL && l == NULL);
                    }
                }
            }
            break;
        case CONDITION_TYPE_BASENAME: /* Basenames with Matchcase=true */
        case CONDITION_TYPE_NCASE_BASENAME: /* the same with Matchcase=false */
            found = FALSE;
            for (x = cond->list; match && x[0] != NULL; x++)
            {
                tst = x[0];
                if (tst[0] == '*' && tst[1] == '\0')
                    found = TRUE; /* always matches */
                else if (tst[0] == '!') /* negate condition */
                {
                    pattern = g_pattern_spec_new(&tst[1]);
                    for (l = flist; match && l; l = l->next)
                    {
                        s = (char *)fm_file_info_get_name(l->data);
                        if (cond->type == CONDITION_TYPE_NCASE_BASENAME)
                            s = g_utf8_casefold(s, -1);
                        if (s != NULL && g_pattern_match_string(pattern, s))
                            match = FALSE;
                        if (cond->type == CONDITION_TYPE_NCASE_BASENAME)
                            g_free(s);
                    }
                    g_pattern_spec_free(pattern);
                }
                else if (!found)
                {
                    pattern = g_pattern_spec_new(tst);
                    for (l = flist; !found && l != NULL; l = l->next)
                    {
                        s = (char*)fm_file_info_get_name(l->data);
                        if (cond->type == CONDITION_TYPE_NCASE_BASENAME)
                            s = g_utf8_casefold(s, -1);
                        if (s == NULL || !g_pattern_match_string(pattern, s))
                            found = TRUE; /* mark to break cycle */
                        if (cond->type == CONDITION_TYPE_NCASE_BASENAME)
                            g_free(s);
                    }
                    found = (flist != NULL && !found); /* all match */
                    g_pattern_spec_free(pattern);
                }
            }
            break;
        case CONDITION_TYPE_SCHEME: /* Schemes */
            found = FALSE;
            for (x = cond->list; match && x[0] != NULL; x++)
            {
                tst = x[0];
                if (tst[0] == '*' && tst[1] == '\0')
                    found = TRUE; /* always matches */
                else if (tst[0] == '!') /* negate condition */
                {
                    pattern = g_pattern_spec_new(&tst[1]);
                    for (l = flist; match && l; l = l->next)
                    {
                        s = (char *)fm_path_get_basename(fm_path_get_scheme_path(l->data));
                        tst = strchr(s, ':');
                        if (tst == NULL || tst == s)
                            match = !g_pattern_match_string(pattern, "file");
                        else
                        {
                            s = g_strndup(s, tst - s);
                            match = !g_pattern_match_string(pattern, s);
                            g_free(s);
                        }
                    }
                    g_pattern_spec_free(pattern);
                }
                else if (!found)
                {
                    pattern = g_pattern_spec_new(tst);
                    for (l = flist; !found && l != NULL; l = l->next)
                    {
                        s = (char *)fm_path_get_basename(fm_path_get_scheme_path(l->data));
                        tst = strchr(s, ':');
                        if (tst == NULL || tst == s)
                            found = !g_pattern_match_string(pattern, "file");
                        else
                        {
                            s = g_strndup(s, tst - s);
                            found = !g_pattern_match_string(pattern, s);
                            g_free(s);
                        }
                    }
                    found = (flist != NULL && l == NULL); /* all match */
                    g_pattern_spec_free(pattern);
                }
            }
            break;
        case CONDITION_TYPE_FOLDER: /* Folders */
            found = FALSE;
            /* check location for pattern(s) match */
            s = fm_path_to_str(fm_file_info_get_path(location));
            for (x = cond->list; match && x[0] != NULL; x++)
            {
                tst = x[0];
                if (tst[0] == '!') /* negate condition */
                    match = !g_pattern_match_simple(&tst[1], s);
                else
                    found = g_pattern_match_simple(tst, s);
            }
            g_free(s);
            break;
        default:
            g_warn_if_reached();
        }
        if (match)
            match = found;
    }
    return (match && match_num);
}

static gboolean _menu_matches(FmActionMenu *menu, FmFileInfo *location,
                              FmFileInfoList *files, FmActionMenu *root)
{
    return _matches_cond(files, location, menu->conditions, root);
}

static FmAction *_action_matches(FmAction *action, FmFileInfo *location,
                                 FmFileInfoList *files, FmActionTarget target,
                                 FmActionMenu *root)
{
    FmActionInfo *info = action->info;
    GSList *l;

    g_return_val_if_fail(info->profiles != NULL, NULL);
    if (action != info->profiles->data) /* we scan all profiles at first only */
        return NULL;
    if ((info->target && target) == 0)
        return NULL;
    if (!_matches_cond(files, location, info->conditions, root))
        return NULL;
    for (l = info->profiles; l; l = l->next)
    {
        action = l->data;
        if (_matches_cond(files, location, action->conditions, root))
            return action;
    }
    return NULL;
}

/* moves elem to root->children after composing */
static void _insert_menu(FmActionMenu *root, GList *elem, GList **menus,
                         GList **actions, GList **zero_level_menus)
{
    FmActionMenu *menu = elem->data, *to_free = NULL;
    char **ids = menu->ids, **id;
    GList *l;
    FmAction *action;

    /* check if it's already allocated into another menu and create dup instead */
    if (menu->parent != NULL)
    {
        to_free = menu;
        elem->data = _fm_action_menu_dup((GAppInfo *)menu);
        menu = elem->data;
    }
    /* find matching ids in menus/actions and move them to menu->children */
    if (ids != NULL) for (id = ids; id[0]; id++)
    {
        if (id[0][0] == '\0')
        {
            /* just prepend a NULL item if separator */
            menu->children = g_list_prepend(menu->children, NULL);
            continue;
        }
        /* check in menus list */
        for (l = *menus; l; l = l->next)
        {
            if (strcmp(id[0], ((FmActionMenu *)l->data)->id) == 0)
            {
                /* use _insert_menu() for menus */
                *menus = g_list_remove_link(*menus, l);
                _insert_menu(menu, l, menus, actions, zero_level_menus);
                break;
            }
        }
        if (l != NULL) /* found in menus */
            continue;
        /* check in actions list */
        for (l = *actions; l; l = l->next)
        {
            if (strcmp(id[0], ((FmAction *)l->data)->info->id) == 0)
            {
                /* move and set parent for action */
                *actions = g_list_remove_link(*actions, l);
                action = l->data;
                /* check if it's already allocated elsewhere so create a dup */
                if (action->menu != NULL)
                {
                    l->data = _fm_action_dup((GAppInfo *)action);
                    g_object_unref(action); /* drop extra reference */
                    action = l->data;
                }
                action->menu = menu;
                menu->children = g_list_concat(l, menu->children);
                break;
            }
        }
        if (l != NULL) /* found in actions */
            continue;
        /* check in resolved zero-level menus list */
        for (l = *zero_level_menus; l; l = l->next)
        {
            if (strcmp(id[0], ((FmActionMenu *)l->data)->id) == 0)
            {
                /* menus in zero_level_menus are ready so just move and reparent */
                *zero_level_menus = g_list_remove_link(*zero_level_menus, l);
                ((FmActionMenu *)l->data)->parent = menu;
                menu->children = g_list_concat(l, menu->children);
                break;
            }
        }
    }
    menu->children = g_list_reverse(menu->children);
    /* drop extra ref from menu if need */
    if (to_free)
        g_object_unref(to_free);
    /* do this last to prevent any cycles */
    menu->parent = root;
    root->children = g_list_concat(elem, root->children);
}

static FmActionMenu *fm_action_get_for_content(FmActionCache *cache,
                                               FmFileInfo *location,
                                               FmFileInfoList *files,
                                               FmActionTarget target)
{
    GList *menus = NULL, *actions = NULL, *l;
    FmActionMenu *root;
    FmAction *action;

    g_return_val_if_fail(FM_IS_ACTION_CACHE(cache), NULL);
    root = _fm_action_menu_new();
    root->enabled = TRUE;
    root->target = target;
    if (target == ACTION_TARGET_CONTEXT)
        root->files = fm_file_info_list_ref(files);
    else
    {
        root->files = fm_file_info_list_new();
        fm_file_info_list_push_tail(root->files, location);
    }
    fm_action_cache_ensure_updates(cache);
    G_LOCK(update);
    /* collect matched menus */
    for (l = cache_menus; l; l = l->next)
        if (_menu_matches(l->data, location, root->files, root))
            menus = g_list_prepend(menus, g_object_ref(l->data));
    /* collect matched actions */
    for (l = cache_actions; l; l = l->next)
        if ((action = _action_matches(l->data, location, root->files, target, root)))
            actions = g_list_prepend(actions, g_object_ref(action));
    G_UNLOCK(update);
    actions = g_list_reverse(actions);
    /* compose menu - add each menu to the root recursively */
    while (menus != NULL)
    {
        l = menus;
        menus = g_list_remove_link(menus, l);
        _insert_menu(root, l, &menus, &actions, &root->children);
    }
    /* add leftovers to the root */
    if (actions != NULL)
    {
        for (l = actions; l; l = l->next)
        {
            action = l->data;
            /* check if it's already allocated elsewhere so create a dup */
            if (action->menu != NULL)
            {
                l->data = _fm_action_dup((GAppInfo *)action);
                g_object_unref(action); /* drop extra reference */
                action = l->data;
            }
            action->menu = root;
        }
        root->children = g_list_concat(root->children, actions);
    }
    return root;
}


/* ---- external API ---- */

/**
 * fm_action_get_for_context
 * @cache: actions cache
 * @location: path to current directory
 * @files: list of files for context menu
 *
 * Checks for files in @cache that mets conditions and returns the list.
 * Returned menu should be freed using g_object_unref(). Note that every
 * g_app_info_launch*() call on any of items found in the menu will be
 * always done against @files and arguments of such call will be always
 * ignored, therefore you should never launch any items found in the menu
 * after you free it or otherwise your call will fail.
 *
 * Returns: (transfer full): list of found actions.
 *
 * Since: 1.3.0
 */
FmActionMenu *fm_action_get_for_context(FmActionCache *cache, FmFileInfo *location,
                                        FmFileInfoList *files)
{
    if (fm_file_info_list_is_empty(files))
        return NULL;
    return fm_action_get_for_content(cache, location, files, ACTION_TARGET_CONTEXT);
}

/**
 * fm_action_get_for_location
 * @cache: actions cache
 * @location: path to current directory
 *
 * Checks for files in @cache that mets conditions and returns the list.
 * Only actions that targetted @location are returned. Returned menu should
 * be freed using g_object_unref(). Note that every g_app_info_launch*()
 * call on any of items found in the menu will be always done against
 * @location and arguments of such call will be always ignored.
 *
 * Returns: (transfer full): list of found actions.
 *
 * Since: 1.3.0
 */
FmActionMenu *fm_action_get_for_location(FmActionCache *cache, FmFileInfo *location)
{
    return fm_action_get_for_content(cache, location, NULL, ACTION_TARGET_LOCATION);
}

/**
 * fm_action_get_for_toolbar
 * @cache: actions cache
 * @location: path to current directory
 *
 * Checks for files in @cache that mets conditions and returns the list.
 * Only actions that target toolbar are returned. Returned menu should
 * be freed using g_object_unref(). Note that every g_app_info_launch*()
 * call on any of items found in the menu will be always done against
 * @location and arguments of such call will be always ignored.
 *
 * Returns: (transfer full): list of found actions.
 *
 * Since: 1.3.0
 */
FmActionMenu *fm_action_get_for_toolbar(FmActionCache *cache, FmFileInfo *location)
{
    return fm_action_get_for_content(cache, location, NULL, ACTION_TARGET_TOOLBAR);
}

/**
 * fm_action_menu_get_children
 * @menu: an action menu
 *
 * Returns list of elements that belong to this menu. If an element is
 * %NULL then element is a separator. Otherwise element may be either
 * #FmAction or #FmActionMenu. Returned list owned by @menu and should
 * not be freed by caller.
 *
 * Returns: (transfer none) (element-type GAppInfo): list of @menu elements.
 *
 * Since: 1.3.0
 */
const GList *fm_action_menu_get_children(FmActionMenu *menu)
{
    g_return_val_if_fail(FM_IS_ACTION_MENU(menu), NULL);

    return menu->children;
}

/**
 * fm_action_get_suggested_shortcut
 * @action: an action
 *
 * Returns suggested keyboard shortcut for @action if available. The format
 * may look like "&lt;Control&gt;a" or "&lt;Shift&gt;&lt;Alt&gt;F1".
 *
 * Returns: (transfer none): keyboard shortcut string.
 *
 * Since: 1.3.0
 */
const char *fm_action_get_suggested_shortcut(FmAction *action)
{
    g_return_val_if_fail(FM_IS_ACTION(action), NULL);

    return action->info->shortcut;
}

/**
 * fm_action_get_toolbar_label
 * @action: an action
 *
 * Returns toolbar label for @action or %NULL if not defined.
 *
 * Returns: (transfer none): toolbar label.
 *
 * Since: 1.3.0
 */
const char *fm_action_get_toolbar_label(FmAction *action)
{
    g_return_val_if_fail(FM_IS_ACTION(action), NULL);

//    _expand_params(str, action->info->toolbar_label, action->menu, NULL);
    return action->info->toolbar_label;
}

/**
 * fm_action_get_startup_wm_class
 * @action: an action
 *
 * Returns startup WM class for @action or %NULL if not defined.
 *
 * Returns: (transfer none): startup WM class.
 *
 * Since: 1.3.0
 */
const char *fm_action_get_startup_wm_class(FmAction *action)
{
    g_return_val_if_fail(FM_IS_ACTION(action), NULL);

    return action->wm_class;
}

/* typedef gboolean (*FmActionExecEmbedded)(const char *command, gpointer user_data);

gboolean fm_action_cache_set_execution_window(FmActionCache *cache,
                                              FmActionExecEmbedded callback,
                                              gpointer user_data) */

/**
 * fm_action_cache_new
 *
 * Creates and initializes an #FmActionCache object, or returns a reference
 * for existing one if it was already created. Newly created cache will be
 * collected on idle if not requested right away (what is highly unlikely).
 *
 * Returns: (transfer full): a cache object.
 *
 * Since: 1.3.0
 */
FmActionCache *fm_action_cache_new(void)
{
    FmActionCache *cache;
    const char * const *dirs;
    char *path;
    guint i;

#if GLIB_CHECK_VERSION(2, 32, 0)
    G_LOCK(update); /* lock it to prevent creation of two instances */
    cache = g_weak_ref_get(&singleton);
    if (cache != NULL)
    {
        G_UNLOCK(update);
        return cache;
    }
#endif
    /* Create an instance */
    cache = (FmActionCache *)g_object_new(FM_TYPE_ACTION_CACHE, NULL);
#if GLIB_CHECK_VERSION(2, 32, 0)
    g_weak_ref_set(&singleton, cache);
#else
    if (g_atomic_int_exchange_and_add(&cache_n_ref, 1) > 0)
        return cache;
    G_LOCK(update); /* wait if disposal of previous is in progress yet */
#endif
    /* Cleanup before setup */
    cache_dirs = NULL;
    cache_actions = NULL;
    cache_menus = NULL;
    cache_to_update = NULL;
    /* Schedule scan of actions dirs - see fm_action_cache_add_directory() */
    dirs = g_get_system_data_dirs();
    i = g_strv_length((char **)dirs);
    while (i-- > 0)
    {
        path = g_build_filename(dirs[i], "file-manager/actions", NULL);
        fm_action_cache_add_directory(cache, path);
        g_free(path);
    }
    path = g_build_filename(g_get_user_data_dir(), "file-manager/actions", NULL);
    fm_action_cache_add_directory(cache, path);
    G_UNLOCK(update);
    g_free(path);
    /* Return the instance */
    return cache;
}
