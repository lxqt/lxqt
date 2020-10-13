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

#include "fm-config.h"

/* global config object */
static FmConfig globalConfig_;
FmConfig* fm_config = &globalConfig_;

void fm_config_init() {
    FmConfig *self = fm_config;
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
