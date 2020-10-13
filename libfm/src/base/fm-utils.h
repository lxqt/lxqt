/*
 *      fm-utils.h
 *
 *      Copyright 2009 - 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2012-2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifndef __FM_UTILS_H__
#define __FM_UTILS_H__

#include <glib.h>
#include <gio/gio.h>
#include "fm-file-info.h"

G_BEGIN_DECLS

/**
 * FmAppCommandParseCallback:
 * @opt: key character
 * @user_data: data passed from caller of fm_app_command_parse()
 *
 * The handler which converts key char into string representation.
 *
 * See also: fm_app_command_parse().
 *
 * Returns: string representation.
 *
 * Since: 1.0.0
 */
typedef const char* (*FmAppCommandParseCallback)(char opt, gpointer user_data);

typedef struct _FmAppCommandParseOption FmAppCommandParseOption;

/**
 * FmAppCommandParseOption:
 * @opt: key character
 * @callback: subroutine to get string for substitution
 *
 * Element of correspondence for substitutions by fm_app_command_parse().
 */
struct _FmAppCommandParseOption
{
    char opt;
    FmAppCommandParseCallback callback;
};

int fm_app_command_parse(const char* cmd, const FmAppCommandParseOption* opts,
                         char** ret, gpointer user_data);

char* fm_file_size_to_str(char* buf, size_t buf_size, goffset size, gboolean si_prefix);
char *fm_file_size_to_str2(char *buf, size_t buf_size, goffset size, char size_units);

gboolean fm_key_file_get_int(GKeyFile* kf, const char* grp, const char* key, int* val);
gboolean fm_key_file_get_bool(GKeyFile* kf, const char* grp, const char* key, gboolean* val);

char* fm_canonicalize_filename(const char* filename, const char* cwd);

char* fm_strdup_replace(char* str, char* old_str, char* new_str);

gboolean fm_run_in_default_main_context(GSourceFunc func, gpointer data);

const char *fm_get_home_dir(void);

char *fm_uri_subpath_to_native_subpath(const char *subpath, GError **error);
void fm_strcatv(char ***strvp, char * const *astrv);

G_END_DECLS

#endif
