/*
 *      fm-thumbnailer.h
 *
 *      Copyright 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#ifndef __FM_THUMBNAILER_H__
#define __FM_THUMBNAILER_H__

#include <glib.h>

G_BEGIN_DECLS

#define	FM_THUMBNAILER(p)	((FmThumbnailer*)p)

typedef struct _FmThumbnailer	FmThumbnailer;

FmThumbnailer* fm_thumbnailer_new_from_keyfile(const char* id, GKeyFile* kf);

char* fm_thumbnailer_command_for_uri(FmThumbnailer* thumbnailer, const char* uri, const char* output_file, guint size);
GPid fm_thumbnailer_launch_for_uri_async(FmThumbnailer* thumbnailer,
                                         const char* uri,
                                         const char* output_file, guint size,
                                         GError** error);

#ifndef FM_DISABLE_DEPRECATED
gboolean fm_thumbnailer_launch_for_uri(FmThumbnailer* thumbnailer, const char* uri, const char* output_file, guint size);
void fm_thumbnailer_free(FmThumbnailer* thumbnailer);
#endif

FmThumbnailer* fm_thumbnailer_ref(FmThumbnailer* thumbnailer);
void fm_thumbnailer_unref(FmThumbnailer* thumbnailer);

/* reload the thumbnailers if needed */
void fm_thumbnailer_check_update();

void _fm_thumbnailer_init();
void _fm_thumbnailer_finalize();

G_END_DECLS

#endif /* __FM_THUMBNAILER_H__ */
