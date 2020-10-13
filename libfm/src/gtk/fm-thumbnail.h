/*
 *      fm-thumbnail.h
 *      
 *      Copyright 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#ifndef __FM_THUMBNAIL_H__
#define __FM_THUMBNAIL_H__

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "fm-file-info.h"
#include "fm-thumbnail-loader.h"

G_BEGIN_DECLS

#define FmThumbnailRequest FmThumbnailLoader

/**
 * FmThumbnailReadyCallback:
 * @req: request descriptor
 * @data: user data provided when request was made
 *
 * The callback to requestor when thumbnail is ready.
 * Note that this call is done outside of GTK loop so if the callback
 * wants to use any GTK API it should call gdk_threads_enter() and
 * gdk_threads_leave() for safety.
 *
 * Since: 0.1.0
 */
typedef void (*FmThumbnailReadyCallback)(FmThumbnailRequest*req, gpointer data);

void _fm_thumbnail_init();

void _fm_thumbnail_finalize();

FmThumbnailRequest* fm_thumbnail_request(FmFileInfo* src_file,
                                    guint size,
                                    FmThumbnailReadyCallback callback,
                                    gpointer user_data);

void fm_thumbnail_request_cancel(FmThumbnailRequest* req);

GdkPixbuf* fm_thumbnail_request_get_pixbuf(FmThumbnailRequest* req);

FmFileInfo* fm_thumbnail_request_get_file_info(FmThumbnailRequest* req);

guint fm_thumbnail_request_get_size(FmThumbnailRequest* req);

G_END_DECLS

#endif /* __FM_THUMBNAIL_H__ */
