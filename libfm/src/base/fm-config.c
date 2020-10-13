/*
 *      fm-config.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2009 Juergen Hoetzel <juergen@archlinux.org>
 *      Copyright 2012-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *      Copyright 2016 Mamoru TASAKA <mtasaka@fedoraproject.org>
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
 * SECTION:fm-config
 * @short_description: Configuration file support for applications that use libfm.
 * @title: FmConfig
 *
 * @include: libfm/fm.h
 *
 * The #FmConfig represents basic configuration options that are used by
 * libfm classes and methods. Methods of class #FmConfig allow use either
 * default file (~/.config/libfm/libfm.conf) or another one to load the
 * configuration and to save it.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-config.h"
#include "fm-utils.h"
#include <stdio.h>

enum
{
    CHANGED,
    N_SIGNALS
};

/* global config object */
FmConfig* fm_config = NULL;

static guint signals[N_SIGNALS];

static void fm_config_finalize              (GObject *object);

G_DEFINE_TYPE(FmConfig, fm_config, G_TYPE_OBJECT);


static void fm_config_class_init(FmConfigClass *klass)
{
    GObjectClass *g_object_class;

    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->finalize = fm_config_finalize;

    /**
     * FmConfig::changed:
     * @config: configuration that was changed
     *
     * The #FmConfig::changed signal is emitted when a config key is changed.
     *
     * Since: 0.1.0
     */
    signals[CHANGED]=
        g_signal_new("changed",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST|G_SIGNAL_DETAILED,
                     G_STRUCT_OFFSET(FmConfigClass, changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

}

static void _on_cfg_file_changed(GFileMonitor *mon, GFile *gf, GFile *other,
                                 GFileMonitorEvent evt, FmConfig *cfg);

static inline void _cfg_monitor_free(FmConfig *cfg)
{
    if (cfg->_cfg_mon)
    {
        g_signal_handlers_disconnect_by_func(cfg->_cfg_mon, _on_cfg_file_changed, cfg);
        g_object_unref(cfg->_cfg_mon);
        cfg->_cfg_mon = NULL;
    }
}

static inline void _cfg_monitor_add(FmConfig *cfg, const char *path)
{
    GFile *gf = g_file_new_for_path(path);
    cfg->_cfg_mon = g_file_monitor_file(gf, G_FILE_MONITOR_NONE, NULL, NULL);
    g_object_unref(gf);
    if (cfg->_cfg_mon)
        g_signal_connect(cfg->_cfg_mon, "changed", G_CALLBACK(_on_cfg_file_changed), cfg);
}

static void fm_config_finalize(GObject *object)
{
    FmConfig* cfg;
    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_CONFIG(object));

    cfg = (FmConfig*)object;
    _cfg_monitor_free(cfg);
    g_free(cfg->_cfg_name);
    if(cfg->terminal)
        g_free(cfg->terminal);
    if(cfg->archiver)
        g_free(cfg->archiver);
    g_strfreev(cfg->system_modules_blacklist);
    g_strfreev(cfg->modules_blacklist);
    g_strfreev(cfg->modules_whitelist);
    g_free(cfg->format_cmd);
    g_free(cfg->list_view_size_units);
    g_free(cfg->saved_search);

    G_OBJECT_CLASS(fm_config_parent_class)->finalize(object);
}


static void fm_config_init(FmConfig *self)
{
    self->single_click = FM_CONFIG_DEFAULT_SINGLE_CLICK;
    self->auto_selection_delay = FM_CONFIG_DEFAULT_AUTO_SELECTION_DELAY;
    self->use_trash = FM_CONFIG_DEFAULT_USE_TRASH;
    self->confirm_del = FM_CONFIG_DEFAULT_CONFIRM_DEL;
    self->confirm_trash = FM_CONFIG_DEFAULT_CONFIRM_TRASH;
    self->big_icon_size = FM_CONFIG_DEFAULT_BIG_ICON_SIZE;
    self->small_icon_size = FM_CONFIG_DEFAULT_SMALL_ICON_SIZE;
    self->pane_icon_size = FM_CONFIG_DEFAULT_PANE_ICON_SIZE;
    self->thumbnail_size = FM_CONFIG_DEFAULT_THUMBNAIL_SIZE;
    self->show_thumbnail = FM_CONFIG_DEFAULT_SHOW_THUMBNAIL;
    self->thumbnail_local = FM_CONFIG_DEFAULT_THUMBNAIL_LOCAL;
    self->thumbnail_max = FM_CONFIG_DEFAULT_THUMBNAIL_MAX;
    /* show_internal_volumes defaulted to FALSE */
    /* si_unit defaulted to FALSE */
    /* terminal and archiver defaulted to NULL */
    /* drop_default_action defaulted to 0 */
    /* modules_blacklist and modules_whitelist defaulted to NULL */
    /* format_cmd defaulted to NULL */
    /* list_view_size_units defaulted to NULL */
    /* saved_search defaulted to NULL */
    self->advanced_mode = FALSE;
    self->force_startup_notify = FM_CONFIG_DEFAULT_FORCE_S_NOTIFY;
    self->backup_as_hidden = FM_CONFIG_DEFAULT_BACKUP_HIDDEN;
    self->no_usb_trash = FM_CONFIG_DEFAULT_NO_USB_TRASH;
    self->no_child_non_expandable = FM_CONFIG_DEFAULT_NO_EXPAND_EMPTY;
    self->show_full_names = FM_CONFIG_DEFAULT_SHOW_FULL_NAMES;
    self->shadow_hidden = FM_CONFIG_DEFAULT_SHADOW_HIDDEN;
    self->only_user_templates = FM_CONFIG_DEFAULT_ONLY_USER_TEMPLATES;
    self->template_run_app = FM_CONFIG_DEFAULT_TEMPLATE_RUN_APP;
    self->template_type_once = FM_CONFIG_DEFAULT_TEMPL_TYPE_ONCE;
    self->defer_content_test = FM_CONFIG_DEFAULT_DEFER_CONTENT_TEST;
    self->quick_exec = FM_CONFIG_DEFAULT_QUICK_EXEC;
    self->places_home = FM_CONFIG_DEFAULT_PLACES_HOME;
    self->places_desktop = FM_CONFIG_DEFAULT_PLACES_DESKTOP;
    self->places_root = FM_CONFIG_DEFAULT_PLACES_ROOT;
    self->places_computer = FM_CONFIG_DEFAULT_PLACES_COMPUTER;
    self->places_trash = FM_CONFIG_DEFAULT_PLACES_TRASH;
    self->places_applications = FM_CONFIG_DEFAULT_PLACES_APPLICATIONS;
    self->places_network = FM_CONFIG_DEFAULT_PLACES_NETWORK;
    self->places_unmounted = FM_CONFIG_DEFAULT_PLACES_UNMOUNTED;
    self->smart_desktop_autodrop = FM_CONFIG_DEFAULT_SMART_DESKTOP_AUTODROP;
}

/**
 * fm_config_new
 *
 * Creates a new configuration structure filled with default values.
 *
 * Return value: a new #FmConfig object.
 *
 * Since: 0.1.0
 */
FmConfig *fm_config_new(void)
{
    return (FmConfig*)g_object_new(FM_CONFIG_TYPE, NULL);
}

static void _on_cfg_file_changed(GFileMonitor *mon, GFile *gf, GFile *other,
                                 GFileMonitorEvent evt, FmConfig *cfg)
{
    if (evt == G_FILE_MONITOR_EVENT_DELETED)
        _cfg_monitor_free(cfg);
    else
        fm_config_load_from_file(cfg, cfg->_cfg_name);
}

/**
 * fm_config_emit_changed
 * @cfg: pointer to configuration
 * @changed_key: what was changed
 *
 * Causes the #FmConfig::changed signal to be emitted.
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Since: 0.1.0
 */
void fm_config_emit_changed(FmConfig* cfg, const char* changed_key)
{
    GQuark detail = changed_key ? g_quark_from_string(changed_key) : 0;
    g_signal_emit(cfg, signals[CHANGED], detail);
}

static void _parse_drop_default_action(GKeyFile *kf, gint *action)
{
    char *str = g_key_file_get_string(kf, "config", "drop_default_action", NULL);
    if (str)
    {
        switch (str[0])
        {
        case '0': case '1': case '2': case '3':
            /* backward compatibility */
            *action = (str[0] - '0');
            break;
        case 'a':
            if (str[1] == 'u') /* 'auto' */
                *action = FM_DND_DEST_DROP_AUTO;
            else if (str[1] == 's') /* 'ask' */
                *action = FM_DND_DEST_DROP_ASK;
            break;
        case 'c': /* 'copy' */
            *action = FM_DND_DEST_DROP_COPY;
            break;
        case 'm': /* 'move' */
            *action = FM_DND_DEST_DROP_MOVE;
        default: ; /* ignore invalid values */
        }
        g_free(str);
    }
}

/**
 * fm_config_load_from_key_file
 * @cfg: pointer to configuration
 * @kf: a #GKeyFile with configuration keys and values
 *
 * Fills configuration @cfg with data from #GKeyFile @kf.
 *
 * Since: 0.1.0
 */
void fm_config_load_from_key_file(FmConfig* cfg, GKeyFile* kf)
{
    char **strv;

    fm_key_file_get_bool(kf, "config", "use_trash", &cfg->use_trash);
    fm_key_file_get_bool(kf, "config", "single_click", &cfg->single_click);
    fm_key_file_get_int(kf, "config", "auto_selection_delay", &cfg->auto_selection_delay);
    fm_key_file_get_bool(kf, "config", "confirm_del", &cfg->confirm_del);
    fm_key_file_get_bool(kf, "config", "confirm_trash", &cfg->confirm_trash);
    if(cfg->terminal)
        g_free(cfg->terminal);
    cfg->terminal = g_key_file_get_string(kf, "config", "terminal", NULL);
    if(cfg->archiver)
        g_free(cfg->archiver);
    cfg->archiver = g_key_file_get_string(kf, "config", "archiver", NULL);
    fm_key_file_get_bool(kf, "config", "thumbnail_local", &cfg->thumbnail_local);
    fm_key_file_get_int(kf, "config", "thumbnail_max", &cfg->thumbnail_max);
    fm_key_file_get_bool(kf, "config", "advanced_mode", &cfg->advanced_mode);
    fm_key_file_get_bool(kf, "config", "si_unit", &cfg->si_unit);
    fm_key_file_get_bool(kf, "config", "force_startup_notify", &cfg->force_startup_notify);
    fm_key_file_get_bool(kf, "config", "backup_as_hidden", &cfg->backup_as_hidden);
    fm_key_file_get_bool(kf, "config", "no_usb_trash", &cfg->no_usb_trash);
    fm_key_file_get_bool(kf, "config", "no_child_non_expandable", &cfg->no_child_non_expandable);
    _parse_drop_default_action(kf, &cfg->drop_default_action);
    fm_key_file_get_bool(kf, "config", "show_full_names", &cfg->show_full_names);
    fm_key_file_get_bool(kf, "config", "only_user_templates", &cfg->only_user_templates);
    fm_key_file_get_bool(kf, "config", "template_run_app", &cfg->template_run_app);
    fm_key_file_get_bool(kf, "config", "template_type_once", &cfg->template_type_once);
    fm_key_file_get_bool(kf, "config", "defer_content_test", &cfg->defer_content_test);
    fm_key_file_get_bool(kf, "config", "quick_exec", &cfg->quick_exec);
    fm_key_file_get_bool(kf, "config", "smart_desktop_autodrop", &cfg->smart_desktop_autodrop);
    g_free(cfg->format_cmd);
    cfg->format_cmd = g_key_file_get_string(kf, "config", "format_cmd", NULL);
    /* append blacklist */
    strv = g_key_file_get_string_list(kf, "config", "modules_blacklist", NULL, NULL);
    fm_strcatv(&cfg->modules_blacklist, strv);
    g_strfreev(strv);
    /* replace whitelist */
    g_strfreev(cfg->modules_whitelist);
    cfg->modules_whitelist = g_key_file_get_string_list(kf, "config", "modules_whitelist", NULL, NULL);

#ifdef USE_UDISKS
    fm_key_file_get_bool(kf, "config", "show_internal_volumes", &cfg->show_internal_volumes);
#endif

    fm_key_file_get_int(kf, "ui", "big_icon_size", &cfg->big_icon_size);
    fm_key_file_get_int(kf, "ui", "small_icon_size", &cfg->small_icon_size);
    fm_key_file_get_int(kf, "ui", "pane_icon_size", &cfg->pane_icon_size);
    fm_key_file_get_int(kf, "ui", "thumbnail_size", &cfg->thumbnail_size);
    fm_key_file_get_bool(kf, "ui", "show_thumbnail", &cfg->show_thumbnail);
    fm_key_file_get_bool(kf, "ui", "shadow_hidden", &cfg->shadow_hidden);
    g_free(cfg->list_view_size_units);
    cfg->list_view_size_units = g_key_file_get_string(kf, "ui", "list_view_size_units", NULL);
    g_free(cfg->saved_search);
    cfg->saved_search = g_key_file_get_string(kf, "ui", "saved_search", NULL);

    fm_key_file_get_bool(kf, "places", "places_home", &cfg->places_home);
    fm_key_file_get_bool(kf, "places", "places_desktop", &cfg->places_desktop);
    fm_key_file_get_bool(kf, "places", "places_root", &cfg->places_root);
    fm_key_file_get_bool(kf, "places", "places_computer", &cfg->places_computer);
    fm_key_file_get_bool(kf, "places", "places_trash", &cfg->places_trash);
    fm_key_file_get_bool(kf, "places", "places_applications", &cfg->places_applications);
    fm_key_file_get_bool(kf, "places", "places_network", &cfg->places_network);
    fm_key_file_get_bool(kf, "places", "places_unmounted", &cfg->places_unmounted);
}

/**
 * fm_config_load_from_file
 * @cfg: pointer to configuration
 * @name: (allow-none): file name to load configuration
 *
 * Fills configuration @cfg with data from configuration file. The file
 * @name may be %NULL to load default configuration file. If @name is
 * full path then that file will be loaded. Otherwise @name will be
 * searched in system config directories and after that in ~/.config/
 * directory and all found files will be loaded, overwriting existing
 * data in @cfg.
 *
 * See also: fm_config_load_from_key_file()
 *
 * Since: 0.1.0
 */
void fm_config_load_from_file(FmConfig* cfg, const char* name)
{
    const gchar * const *dirs, * const *dir;
    char *path;
    char *old_cfg_name;
    GKeyFile* kf = g_key_file_new();

    old_cfg_name = cfg->_cfg_name;
    g_strfreev(cfg->modules_blacklist);
    g_strfreev(cfg->system_modules_blacklist);
    cfg->modules_blacklist = NULL;
    cfg->system_modules_blacklist = NULL;
    _cfg_monitor_free(cfg);
    if(G_LIKELY(!name))
        name = "libfm/libfm.conf";
    else
    {
        if(G_UNLIKELY(g_path_is_absolute(name)))
        {
            cfg->_cfg_name = g_strdup(name);
            if(g_key_file_load_from_file(kf, name, 0, NULL))
            {
                fm_config_load_from_key_file(cfg, kf);
                _cfg_monitor_add(cfg, name);
            }
            goto _out;
        }
    }

    cfg->_cfg_name = g_strdup(name);
    dirs = g_get_system_config_dirs();
    /* bug SF #887: first dir in XDG_CONFIG_DIRS is the most relevant
       so we shoult process the list in reverse order */
    dir = dirs;
    while (*dir)
        ++dir;
    while (dir-- != dirs)
    {
        path = g_build_filename(*dir, name, NULL);
        if(g_key_file_load_from_file(kf, path, 0, NULL))
            fm_config_load_from_key_file(cfg, kf);
        g_free(path);
    }
    /* we got all system blacklists, save them and get user's one */
    cfg->system_modules_blacklist = cfg->modules_blacklist;
    cfg->modules_blacklist = NULL;
    path = g_build_filename(g_get_user_config_dir(), name, NULL);
    if(g_key_file_load_from_file(kf, path, 0, NULL))
    {
        fm_config_load_from_key_file(cfg, kf);
        _cfg_monitor_add(cfg, path);
    }
    g_free(path);

_out:
    g_key_file_free(kf);
    g_free(old_cfg_name);
    g_signal_emit(cfg, signals[CHANGED], 0);
    /* FIXME: compare and send individual changes instead */
}

#define _save_config_bool(_str_,_cfg_,_name_) \
    g_string_append(_str_, #_name_); \
    g_string_append(_str_, _cfg_->_name_ ? "=1\n" : "=0\n")

#define _save_config_int(_str_,_cfg_,_name_) \
    g_string_append_printf(_str_, #_name_ "=%d\n", _cfg_->_name_)

#define _save_config_string(_str_,_cfg_,_name_) \
    if (_cfg_->_name_ != NULL) \
        g_string_append_printf(_str_, #_name_ "=%s\n", _cfg_->_name_)

#define _save_config_strv(_str_,_cfg_,_name_) do {\
    if(_cfg_->_name_ != NULL && _cfg_->_name_[0] != NULL) \
    { \
        char **list, *c; \
        g_string_append(_str_, #_name_ "="); \
        for (list = _cfg_->_name_; (c = *list); list++) \
        { \
            while (*c) \
            { \
                if (G_UNLIKELY(*c == '\\')) \
                    g_string_append_c(_str_, '\\'); \
                g_string_append_c(_str_, *c++); \
            } \
            g_string_append_c(_str_, ';'); \
        } \
        g_string_append_c(_str_, '\n'); \
    } \
} while(0)

#define _save_drop_action(_str_,_cfg_,_name_) do { \
    switch (_cfg_->_name_) \
    { \
    case FM_DND_DEST_DROP_AUTO: \
        g_string_append(_str_, #_name_ "=auto\n"); \
        break; \
    case FM_DND_DEST_DROP_COPY: \
        g_string_append(_str_, #_name_ "=copy\n"); \
        break; \
    case FM_DND_DEST_DROP_MOVE: \
        g_string_append(_str_, #_name_ "=move\n"); \
        break; \
    case FM_DND_DEST_DROP_ASK: \
        g_string_append(_str_, #_name_ "=ask\n"); \
        break; \
    } \
} while(0)

/**
 * fm_config_save
 * @cfg: pointer to configuration
 * @name: (allow-none): file name to save configuration
 *
 * Saves configuration into configuration file @name. If @name is %NULL
 * then configuration will be saved into default configuration file.
 * Otherwise it will be saved into file @name under directory ~/.config.
 *
 * Since: 0.1.0
 */
void fm_config_save(FmConfig* cfg, const char* name)
{
    char* path = NULL;;
    char* dir_path;
    FILE* f;
    GString *str;
    if(!name)
        name = path = g_build_filename(g_get_user_config_dir(), "libfm/libfm.conf", NULL);
    else if(!g_path_is_absolute(name))
        name = path = g_build_filename(g_get_user_config_dir(), name, NULL);

    dir_path = g_path_get_dirname(name);
    if(g_mkdir_with_parents(dir_path, 0700) != -1)
    {
        if (cfg->_cfg_mon)
            g_signal_handlers_block_by_func(cfg->_cfg_mon, _on_cfg_file_changed, cfg);
        f = fopen(name, "w");
        if(f)
        {
            str = g_string_new("# Configuration file for the libfm version " PACKAGE_VERSION ".\n"
                               "# Autogenerated file, don't edit, your changes will be overwritten.\n"
                               "\n[config]\n");
                _save_config_bool(str, cfg, single_click);
                _save_config_bool(str, cfg, use_trash);
                _save_config_bool(str, cfg, confirm_del);
                _save_config_bool(str, cfg, confirm_trash);
                _save_config_bool(str, cfg, advanced_mode);
                _save_config_bool(str, cfg, si_unit);
                _save_config_bool(str, cfg, force_startup_notify);
                _save_config_bool(str, cfg, backup_as_hidden);
                _save_config_bool(str, cfg, no_usb_trash);
                _save_config_bool(str, cfg, no_child_non_expandable);
                _save_config_bool(str, cfg, show_full_names);
                _save_config_bool(str, cfg, only_user_templates);
                _save_config_bool(str, cfg, template_run_app);
                _save_config_bool(str, cfg, template_type_once);
                _save_config_int(str, cfg, auto_selection_delay);
                _save_drop_action(str, cfg, drop_default_action);
                _save_config_bool(str, cfg, defer_content_test);
                _save_config_bool(str, cfg, quick_exec);
#ifdef USE_UDISKS
                _save_config_bool(str, cfg, show_internal_volumes);
#endif
                _save_config_string(str, cfg, terminal);
                _save_config_string(str, cfg, archiver);
                _save_config_string(str, cfg, format_cmd);
                _save_config_bool(str, cfg, thumbnail_local);
                _save_config_int(str, cfg, thumbnail_max);
                _save_config_strv(str, cfg, modules_blacklist);
                _save_config_strv(str, cfg, modules_whitelist);
                _save_config_bool(str, cfg, smart_desktop_autodrop);
            g_string_append(str, "\n[ui]\n");
                _save_config_int(str, cfg, big_icon_size);
                _save_config_int(str, cfg, small_icon_size);
                _save_config_int(str, cfg, pane_icon_size);
                _save_config_int(str, cfg, thumbnail_size);
                _save_config_bool(str, cfg, show_thumbnail);
                _save_config_bool(str, cfg, shadow_hidden);
                if (cfg->list_view_size_units && cfg->list_view_size_units[0])
                    cfg->list_view_size_units[1] = '\0'; /* leave only 1 char */
                _save_config_string(str, cfg, list_view_size_units);
                _save_config_string(str, cfg, saved_search);
            g_string_append(str, "\n[places]\n");
                _save_config_bool(str, cfg, places_home);
                _save_config_bool(str, cfg, places_desktop);
                _save_config_bool(str, cfg, places_root);
                _save_config_bool(str, cfg, places_computer);
                _save_config_bool(str, cfg, places_trash);
                _save_config_bool(str, cfg, places_applications);
                _save_config_bool(str, cfg, places_network);
                _save_config_bool(str, cfg, places_unmounted);
            fwrite(str->str, 1, str->len, f);
            fclose(f);
            g_string_free(str, TRUE);
        }
        if (cfg->_cfg_mon)
            g_signal_handlers_unblock_by_func(cfg->_cfg_mon, _on_cfg_file_changed, cfg);
    }
    g_free(dir_path);
    g_free(path);
}

const char *_fm_config_get_name(FmConfig *cfg)
{
    return cfg->_cfg_name;
}
