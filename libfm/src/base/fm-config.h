/*
 *      fm-config.h
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2009 Juergen Hoetzel <juergen@archlinux.org>
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


#ifndef __FM_CONFIG_H__
#define __FM_CONFIG_H__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define FM_CONFIG_TYPE              (fm_config_get_type())
#define FM_CONFIG(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            FM_CONFIG_TYPE, FmConfig))
#define FM_CONFIG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),\
            FM_CONFIG_TYPE, FmConfigClass))
#define FM_IS_CONFIG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            FM_CONFIG_TYPE))
#define FM_IS_CONFIG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass),\
            FM_CONFIG_TYPE))

typedef struct _FmConfig            FmConfig;
typedef struct _FmConfigClass       FmConfigClass;

#define     FM_CONFIG_DEFAULT_SINGLE_CLICK      FALSE
#define     FM_CONFIG_DEFAULT_USE_TRASH         TRUE
#define     FM_CONFIG_DEFAULT_CONFIRM_DEL       TRUE
#define     FM_CONFIG_DEFAULT_CONFIRM_TRASH     TRUE
#define     FM_CONFIG_DEFAULT_NO_USB_TRASH      TRUE

#define     FM_CONFIG_DEFAULT_BIG_ICON_SIZE     48
#define     FM_CONFIG_DEFAULT_SMALL_ICON_SIZE   16
#define     FM_CONFIG_DEFAULT_PANE_ICON_SIZE    16
#define     FM_CONFIG_DEFAULT_THUMBNAIL_SIZE    128

#define     FM_CONFIG_DEFAULT_SHOW_THUMBNAIL    TRUE
#define     FM_CONFIG_DEFAULT_THUMBNAIL_LOCAL   TRUE
#define     FM_CONFIG_DEFAULT_THUMBNAIL_MAX     2048

#define     FM_CONFIG_DEFAULT_FORCE_S_NOTIFY    TRUE
#define     FM_CONFIG_DEFAULT_BACKUP_HIDDEN     TRUE
#define     FM_CONFIG_DEFAULT_NO_EXPAND_EMPTY   FALSE
#define     FM_CONFIG_DEFAULT_SHOW_FULL_NAMES   FALSE
#define     FM_CONFIG_DEFAULT_ONLY_USER_TEMPLATES FALSE
#define     FM_CONFIG_DEFAULT_TEMPLATE_RUN_APP  FALSE
#define     FM_CONFIG_DEFAULT_TEMPL_TYPE_ONCE   FALSE
#define     FM_CONFIG_DEFAULT_SHADOW_HIDDEN     FALSE
#define     FM_CONFIG_DEFAULT_DEFER_CONTENT_TEST FALSE
#define     FM_CONFIG_DEFAULT_QUICK_EXEC        FALSE
#define     FM_CONFIG_DEFAULT_SMART_DESKTOP_AUTODROP TRUE

#define     FM_CONFIG_DEFAULT_PLACES_HOME       TRUE
#define     FM_CONFIG_DEFAULT_PLACES_DESKTOP    TRUE
#define     FM_CONFIG_DEFAULT_PLACES_ROOT       FALSE
#define     FM_CONFIG_DEFAULT_PLACES_COMPUTER   FALSE
#define     FM_CONFIG_DEFAULT_PLACES_TRASH      TRUE
#define     FM_CONFIG_DEFAULT_PLACES_APPLICATIONS TRUE
#define     FM_CONFIG_DEFAULT_PLACES_NETWORK    FALSE
#define     FM_CONFIG_DEFAULT_PLACES_UNMOUNTED  TRUE

#define     FM_CONFIG_DEFAULT_AUTO_SELECTION_DELAY 600

/* this enum is used by FmDndDest but we save it nicely in config so have it here */

/**
 * FmDndDestDropAction:
 * @FM_DND_DEST_DROP_AUTO: move if source and destination are on the same file system, copy otherwise
 * @FM_DND_DEST_DROP_COPY: copy
 * @FM_DND_DEST_DROP_MOVE: move
 * @FM_DND_DEST_DROP_ASK: open popup to let user select desired action
 *
 * selected behavior when files are dropped on destination widget.
 */
typedef enum
{
    FM_DND_DEST_DROP_AUTO,
    FM_DND_DEST_DROP_COPY,
    FM_DND_DEST_DROP_MOVE,
    FM_DND_DEST_DROP_ASK
} FmDndDestDropAction;

/**
 * FmConfig:
 * @terminal: command line to launch terminal emulator
 * @archiver: desktop_id of the archiver used
 * @big_icon_size: size of big icons
 * @small_icon_size: size of small icons
 * @pane_icon_size: size of side pane icons
 * @thumbnail_size: size of thumbnail icons
 * @thumbnail_max: show thumbnails only for files not bigger than this, in KB or Kpix
 * @auto_selection_delay: (since 1.2.0) delay for autoselection in single-click mode, in ms
 * @drop_default_action: (since 1.2.0) default action on drop (see #FmDndDestDropAction)
 * @single_click: single click to open file
 * @use_trash: delete file to trash can
 * @confirm_del: ask before deleting files
 * @confirm_trash: (since 1.2.0) ask before moving files to trash can
 * @show_thumbnail: show thumbnails
 * @thumbnail_local: show thumbnails for local files only
 * @show_internal_volumes: show system internal volumes in side pane. (udisks-only)
 * @si_unit: use SI prefix for file sizes
 * @advanced_mode: enable advanced features for experienced user
 * @force_startup_notify: (since 1.0.1) use startup notify by default
 * @backup_as_hidden: (since 1.0.1) treat backup files as hidden
 * @no_usb_trash: (since 1.0.1) don't create trash folder on removable media
 * @no_child_non_expandable: (since 1.0.1) hide expanders on empty folder
 * @show_full_names: (since 1.2.0) always show full names in Icon View mode
 * @shadow_hidden: (since 1.2.0) show icons of hidden files shadowed in the view
 * @places_home: (since 1.2.0) show 'Home' item in Places
 * @places_desktop: (since 1.2.0) show 'Desktop' item in Places
 * @places_applications: (since 1.2.0) show 'Applications' item in Places
 * @places_trash: (since 1.2.0) show 'Trash' item in Places
 * @places_root: (since 1.2.0) show '/' item in Places
 * @places_computer: (since 1.2.0) show 'My computer' item in Places
 * @places_network: (since 1.2.0) show 'Network' item in Places
 * @places_unmounted: (since 1.2.0) show unmounted internal volumes in Places
 * @only_user_templates: (since 1.2.0) show only user defined templates in 'Create...' menu
 * @template_run_app: (since 1.2.0) run default application after creation from template
 * @template_type_once: (since 1.2.0) use only one template of each MIME type
 * @defer_content_test: (since 1.2.0) defer test for content type on folder loading
 * @quick_exec: (since 1.2.0) don't ask user for action on executable launch
 * @modules_blacklist: (since 1.2.0) list of modules (mask in form "type:name") to never load
 * @modules_whitelist: (since 1.2.0) list of excemptions from @modules_blacklist
 * @list_view_size_units: (since 1.2.0) file size units in list view: h, k, M, G
 * @format_cmd: (since 1.2.0) command to format the volume (device will be added)
 * @smart_desktop_autodrop: (since 1.2.0) enable "smart shortcut" auto-action for ~/Desktop
 * @saved_search: (since 1.2.0) internal saved data of fm_launch_search_simple()
 */
struct _FmConfig
{
    /*< private >*/
    GObject parent;
    char *_cfg_name;

    /*< public >*/
    char* terminal;
    char* archiver;

    gint big_icon_size;
    gint small_icon_size;
    gint pane_icon_size;
    gint thumbnail_size;
    gint thumbnail_max;
    gint auto_selection_delay;
    gint drop_default_action;

    gboolean single_click;
    gboolean use_trash;
    gboolean confirm_del;
    gboolean confirm_trash;
    gboolean show_thumbnail;
    gboolean thumbnail_local;
    gboolean show_internal_volumes;
    gboolean si_unit;
    gboolean advanced_mode;
    gboolean force_startup_notify;
    gboolean backup_as_hidden;
    gboolean no_usb_trash;
    gboolean no_child_non_expandable;
    gboolean show_full_names;
    gboolean shadow_hidden;

    gboolean places_home;
    gboolean places_desktop;
    gboolean places_applications;
    gboolean places_trash;
    gboolean places_root;
    gboolean places_computer;
    gboolean places_network;
    gboolean places_unmounted;

    gboolean only_user_templates;
    gboolean template_run_app;
    gboolean template_type_once;
    gboolean defer_content_test;
    gboolean quick_exec;

    gchar **modules_blacklist;
    gchar **modules_whitelist;
    /*< private >*/
    gchar **system_modules_blacklist; /* concatenated from system, don't save! */
    /*< public >*/

    gchar *list_view_size_units;
    gchar *format_cmd;

    gboolean smart_desktop_autodrop;
    gchar *saved_search;
    /*< private >*/
    gpointer _reserved1; /* reserved space for updates until next ABI */
    gpointer _reserved2;
    gpointer _reserved3;
    gpointer _reserved4;
    gpointer _reserved5;
    gpointer _reserved6;
    gpointer _reserved7;
    GFileMonitor *_cfg_mon;
};

/**
 * FmConfigClass
 * @parent_class: the parent class
 * @changed: the class closure for the #FmConfig::changed signal
 */
struct _FmConfigClass
{
    GObjectClass parent_class;
    void (*changed)(FmConfig* cfg);
};

/* global config object */
extern FmConfig* fm_config;

GType       fm_config_get_type      (void);
FmConfig*   fm_config_new           (void);

void fm_config_load_from_file(FmConfig* cfg, const char* name);

void fm_config_load_from_key_file(FmConfig* cfg, GKeyFile* kf);

void fm_config_save(FmConfig* cfg, const char* name);

void fm_config_emit_changed(FmConfig* cfg, const char* changed_key);

/* internal for libfm */
const char *_fm_config_get_name(FmConfig *cfg);

G_END_DECLS

#endif /* __FM_CONFIG_H__ */
