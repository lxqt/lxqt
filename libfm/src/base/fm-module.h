/*
 *      fm-module.h
 *
 *      This file is a part of the Libfm project.
 *
 *      Copyright 2013-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifndef __FM_MODULE_H__
#define __FM_MODULE_H__ 1

#include <gio/gio.h>

#define __FM_MODULE_VERSION__(xx) FM_MODULE_##xx##_VERSION
#define __FM_DEFINE_VERSION__(xx) __FM_MODULE_VERSION__(xx)

/**
 * FM_DEFINE_MODULE
 * @_type_: type of module (e.g. `vfs')
 * @_name_: module type specific key (e.g. `menu')
 *
 * Macro used in module definition. Module should have module specific
 * structure: if @type is vfs then it should be fm_module_init_vfs. See
 * specific header file for some `extern' definition.
 */
#define FM_DEFINE_MODULE(_type_, _name_) \
int module_##_type_##_version = __FM_DEFINE_VERSION__(_type_); \
char module_name[] = #_name_;

/**
 * FmModuleInitCallback
 * @key: the module name as key value for the type
 * @init_data: module type specific initialization data
 * @version: version of loaded module
 *
 * This API is used to make callback from the modules loader to the
 * implementation which uses module so the implementation may do required
 * checks and add module to own list of supported data.
 * This callback will be done in default main context.
 *
 * Returns: %TRUE if module was accepted by implementation.
 *
 * Since: 1.2.0
 */
typedef gboolean (*FmModuleInitCallback)(const char *key, gpointer init_data, int version);

/**
 * FM_MODULE_DEFINE_TYPE
 * @_type_: type of module (e.g. `vfs')
 * @_struct_: type of struct with module callbacks
 * @_minver_: minimum version supported
 *
 * Macro used in module caller. Callback is ran when matched module is
 * found, it should return %TRUE on success.
 */
#define FM_MODULE_DEFINE_TYPE(_type_, _struct_, _minver_) \
static gboolean fm_module_callback_##_type_(const char *, gpointer, int ver); \
\
static inline void fm_module_register_##_type_ (void) { \
    fm_module_register_type(#_type_, \
                            _minver_, __FM_DEFINE_VERSION__(_type_), \
                            fm_module_callback_##_type_); \
}

/* use this whenever extension is about to be used */
#define CHECK_MODULES(...) if(G_UNLIKELY(!fm_modules_loaded)) fm_modules_load()

G_BEGIN_DECLS

/* adds schedule */
void fm_module_register_type(const char *type, int minver, int maxver, FmModuleInitCallback cb);
/* removes schedule */
void fm_module_unregister_type(const char *type);
/* forces schedules */
void fm_modules_load(void);
/* checks if in use */
gboolean fm_module_is_in_use(const char *type, const char *name);
/* add application-dependent path for modules */
gboolean fm_modules_add_directory(const char *path);
/* the flag */
extern volatile gint fm_modules_loaded;

G_END_DECLS

#endif /* __FM_MODULE_H__ */
