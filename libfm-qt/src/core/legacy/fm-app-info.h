/*
 *      fm-app-info.h
 *
 *      Copyright 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef __FM_APP_INFO_H__
#define __FM_APP_INFO_H__

#include <gio/gio.h>

G_BEGIN_DECLS

gboolean fm_app_info_launch(GAppInfo *appinfo, GList *files,
                            GAppLaunchContext *launch_context, GError **error);

gboolean fm_app_info_launch_uris(GAppInfo *appinfo, GList *uris,
                                 GAppLaunchContext *launch_context, GError **error);

gboolean fm_app_info_launch_default_for_uri(const char *uri,
                                            GAppLaunchContext *launch_context,
                                            GError **error);

GAppInfo* fm_app_info_create_from_commandline(const char *commandline,
                                              const char *application_name,
                                              GAppInfoCreateFlags flags,
                                              GError **error);

G_END_DECLS

#endif /* __FM_APP_INFO_H__ */
