/*
 * fm-thumbnail-loader.h
 *
 * Copyright 2013 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This file is a part of the Libfm library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __FM_THUMBNAIL_LOADER_H__
#define __FM_THUMBNAIL_LOADER_H__

#include <glib.h>
#include "fm-file-info.h"

/* If we're not using GNU C, elide __attribute__ */
#ifndef __GNUC__
# define __attribute__(x)
#endif

G_BEGIN_DECLS

typedef struct _FmThumbnailLoader FmThumbnailLoader;

/**
 * FmThumbnailLoaderCallback:
 * @req: request descriptor
 * @data: user data provided when request was made
 *
 * The callback to requestor when thumbnail is ready.
 * Note that this call is done outside of GTK loop so if the callback
 * wants to use any GTK API it should call gdk_threads_enter() and
 * gdk_threads_leave() for safety.
 *
 * Since: 1.2.0
 */
typedef void (*FmThumbnailLoaderCallback)(FmThumbnailLoader *req, gpointer data);

void _fm_thumbnail_loader_init();

void _fm_thumbnail_loader_finalize();

FmThumbnailLoader* fm_thumbnail_loader_load(FmFileInfo* src_file,
                                            guint size,
                                            FmThumbnailLoaderCallback callback,
                                            gpointer user_data);

void fm_thumbnail_loader_cancel(FmThumbnailLoader* req);

GObject* fm_thumbnail_loader_get_data(FmThumbnailLoader* req);

FmFileInfo* fm_thumbnail_loader_get_file_info(FmThumbnailLoader* req);

guint fm_thumbnail_loader_get_size(FmThumbnailLoader* req);

/* for toolkit-specific image loading code */

typedef struct _FmThumbnailLoaderBackend FmThumbnailLoaderBackend;

/**
 * FmThumbnailLoaderBackend:
 * @read_image_from_file: callback to read image by file path
 * @read_image_from_stream: callback to read image by opened #GInputStream
 * @write_image: callback to write thumbnail file from image
 * @scale_image: callback to change image sizes
 * @rotate_image: callback to change image orientation
 * @get_image_width: callback to retrieve width from image
 * @get_image_height: callback to retrieve height from image
 * @get_image_text: callback to retrieve custom attributes text from image
 * @set_image_text: callback to set custom attributes text into image
 *
 * Abstract backend callbacks list.
 */
struct _FmThumbnailLoaderBackend {
    GObject* (*read_image_from_file)(const char* filename);
    GObject* (*read_image_from_stream)(GInputStream* stream, guint64 len, GCancellable* cancellable);
    gboolean (*write_image)(GObject* image, const char* filename);
    GObject* (*scale_image)(GObject* ori_pix, int new_width, int new_height);
    GObject* (*rotate_image)(GObject* image, int degree);
    int (*get_image_width)(GObject* image);
    int (*get_image_height)(GObject* image);
    char* (*get_image_text)(GObject* image, const char* key);
    gboolean (*set_image_text)(GObject* image, const char* key, const char* val);
    // const char* (*get_image_orientation)(GObject* image);
    // GObject* (*apply_orientation)(GObject* image);
};

gboolean fm_thumbnail_loader_set_backend(FmThumbnailLoaderBackend* _backend)
                                __attribute__((warn_unused_result,nonnull(1)));

G_END_DECLS

#endif /* __FM_THUMBNAIL_LOADER_H__ */
