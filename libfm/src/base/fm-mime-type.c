/*
 *      fm-mime-type.c
 *
 *      Copyright 2009 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2009 Juergen Hoetzel <juergen@archlinux.org>
 *      Copyright 2012-2016 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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
 * SECTION:fm-mime-type
 * @short_description: Extended MIME types support.
 * @title: FmMimeType
 *
 * @include: libfm/fm.h
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-mime-type.h"

#include <glib/gi18n-lib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

G_LOCK_DEFINE_STATIC(thumbnailers);

struct _FmMimeType
{
    char* type; /* mime type name */
    char* description;  /* description of the mime type */
    FmIcon* icon;

    /* thumbnailers installed for the mime-type - locked here not there! */
    GList* thumbnailers; /* FmMimeType does "not" own the FmThumbnailer objects */

    int n_ref;
};

/* FIXME: how can we handle reload of xdg mime? */

static GHashTable *mime_hash = NULL;
G_LOCK_DEFINE_STATIC(mime_hash);

static FmMimeType* directory_type = NULL;
static FmMimeType* mountable_type = NULL;
static FmMimeType* shortcut_type = NULL;
static FmMimeType* desktop_entry_type = NULL;

static FmMimeType* fm_mime_type_new(const char* type_name);

void _fm_mime_type_init()
{
    mime_hash = g_hash_table_new_full(g_str_hash, g_str_equal,
                                       NULL, fm_mime_type_unref);

    /* since those are frequently used, we store them to save hash table lookup. */
    directory_type = fm_mime_type_from_name("inode/directory");
    mountable_type = fm_mime_type_from_name("inode/mount-point");
    desktop_entry_type = fm_mime_type_from_name("application/x-desktop");

    /* fake mime-type for shortcuts */
    shortcut_type = fm_mime_type_from_name("inode/x-shortcut");
    shortcut_type->description = g_strdup(_("shortcut to URI"));
}

void _fm_mime_type_finalize()
{
    fm_mime_type_unref(directory_type);
    fm_mime_type_unref(shortcut_type);
    fm_mime_type_unref(mountable_type);
    fm_mime_type_unref(desktop_entry_type);
    g_hash_table_destroy(mime_hash);
}

/**
 * fm_mime_type_from_file_name
 * @ufile_name: file name to guess
 *
 * Finds #FmMimeType descriptor guessing type from @ufile_name.
 *
 * Before 1.0.0 this API had name fm_mime_type_get_for_file_name.
 *
 * Returns: (transfer full): a #FmMimeType object.
 *
 * Since: 0.1.0
 */
FmMimeType* fm_mime_type_from_file_name(const char* ufile_name)
{
    FmMimeType* mime_type;
    char * type;
    gboolean uncertain;
    /* let skip scheme and host from non-native names */
    type = g_strstr_len(ufile_name, -1, "://");
    if (type != NULL)
        ufile_name = strchr(&type[3], '/');
    if (ufile_name == NULL)
        ufile_name = "unknown";
    type = g_content_type_guess(ufile_name, NULL, 0, &uncertain);
    mime_type = fm_mime_type_from_name(type);
    g_free(type);
    return mime_type;
}

/**
 * fm_mime_type_from_native_file
 * @file_path: full path to file
 * @base_name: file basename
 * @pstat: (allow-none): file atrributes
 *
 * Finds #FmMimeType descriptor for provided data. If file does not exist
 * then returns %NULL.
 *
 * Before 1.0.0 this API had name fm_mime_type_get_for_native_file.
 *
 * Note that this call does I/O and therefore can block.
 *
 * Returns: (transfer full): a #FmMimeType object.
 *
 * Since: 0.1.0
 */
FmMimeType* fm_mime_type_from_native_file(const char* file_path,
                                        const char* base_name,
                                        struct stat* pstat)
{
    FmMimeType* mime_type;
    struct stat st;

    if(!pstat)
    {
        pstat = &st;
        if(stat(file_path, &st) == -1)
            return NULL;
    }

    if(S_ISREG(pstat->st_mode))
    {
        gboolean uncertain;
        char* type = g_content_type_guess(base_name, NULL, 0, &uncertain);
        if(uncertain)
        {
            int fd, len;
            if(pstat->st_size == 0) /* empty file = text file with 0 characters in it. */
            {
                g_free(type);
                return fm_mime_type_from_name("text/plain");
            }
            fd = open(file_path, O_RDONLY);
            if(fd >= 0)
            {
                /* #3086703 - PCManFM crashes on non existent directories.
                 * http://sourceforge.net/tracker/?func=detail&aid=3086703&group_id=156956&atid=801864
                 *
                 * NOTE: do not use mmap here. Though we can get little
                 * performance gain, this makes our program more vulnerable
                 * to I/O errors. If the mapped file is truncated by other
                 * processes or I/O errors happen, we may receive SIGBUS.
                 * It's a pity that we cannot use mmap for speed up here. */
            /*
            #ifdef HAVE_MMAP
                const char* buf;
                len = pstat->st_size > 4096 ? 4096 : pstat->st_size;
                buf = (const char*)mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
                if(G_LIKELY(buf != MAP_FAILED))
                {
                    g_free(type);
                    type = g_content_type_guess(NULL, buf, len, &uncertain);
                    munmap(buf, len);
                }
            #else
            */
                char buf[4096];
                len = read(fd, buf, MIN(pstat->st_size, 4096));
                const char *tmp;
                char *qtype = type; /* questionable type */
                close(fd);
                type = g_content_type_guess(base_name, (guchar*)buf, len, &uncertain);
                /* we need more complicated guessing here: file may have some
                   wrong suffix or no suffix at all, and g_content_type_guess()
                   very probably will guess it wrong so let believe it only
                   if it insists on guessed type after testing its content,
                   otherwise discard name completely and analyze just content */
                if (g_strcmp0(qtype, type) != 0)
                {
                    g_free(type);
                    type = g_content_type_guess(NULL, (guchar*)buf, len, &uncertain);
                }
                g_free(qtype);
                /* bug: improperly named desktop entries are detected as text/plain */
                if (uncertain && len > 40 && (tmp = memchr(buf, '[', 40)) != NULL &&
                    strncmp(tmp, "[Desktop Entry]\n", 16) == 0)
                {
                    g_free(type);
                    return fm_mime_type_ref(desktop_entry_type);
                }
            /* #endif */
            }
        }
        mime_type = fm_mime_type_from_name(type);
        g_free(type);
        return mime_type;
    }

    if(S_ISDIR(pstat->st_mode))
        return fm_mime_type_ref(directory_type);
    if (S_ISCHR(pstat->st_mode))
        return fm_mime_type_from_name("inode/chardevice");
    if (S_ISBLK(pstat->st_mode))
        return fm_mime_type_from_name("inode/blockdevice");
    if (S_ISFIFO(pstat->st_mode))
        return fm_mime_type_from_name("inode/fifo");
    if (S_ISLNK(pstat->st_mode))
        return fm_mime_type_from_name("inode/symlink");
#ifdef S_ISSOCK
    if (S_ISSOCK(pstat->st_mode))
        return fm_mime_type_from_name("inode/socket");
#endif
    /* impossible */
    g_debug("Invalid stat mode: %d, %s", pstat->st_mode & S_IFMT, base_name);
    /* FIXME: some files under /proc/self has st_mode = 0, which causes problems.
     *        currently we treat them as files of unknown type. */
    return fm_mime_type_from_name("application/octet-stream");
}

/**
 * fm_mime_type_from_name
 * @type: MIME type name
 *
 * Finds #FmMimeType descriptor for @type.
 *
 * Before 1.0.0 this API had name fm_mime_type_get_for_type.
 *
 * Returns: (transfer full): a #FmMimeType object.
 *
 * Since: 0.1.0
 */
FmMimeType* fm_mime_type_from_name(const char* type)
{
    FmMimeType * mime_type;

    G_LOCK(mime_hash);
    mime_type = g_hash_table_lookup(mime_hash, type);
    if (!mime_type)
    {
        mime_type = fm_mime_type_new(type);
        g_hash_table_insert(mime_hash, mime_type->type, mime_type);
    }
    G_UNLOCK(mime_hash);
    fm_mime_type_ref(mime_type);
    return mime_type;
}

/**
 * fm_mime_type_new
 * @type_name: MIME type name
 *
 * Creates a new #FmMimeType descriptor for @type.
 *
 * Returns: (transfer full): new #FmMimeType object.
 *
 * Since: 0.1.0
 */
FmMimeType* fm_mime_type_new(const char* type_name)
{
    FmMimeType * mime_type = g_slice_new0(FmMimeType);
    GIcon* gicon;
    mime_type->type = g_strdup(type_name);
    mime_type->n_ref = 1;

    gicon = g_content_type_get_icon(mime_type->type);
    if(strcmp(mime_type->type, "inode/directory") == 0)
        g_themed_icon_prepend_name(G_THEMED_ICON(gicon), "folder");
    else if(g_content_type_can_be_executable(mime_type->type))
        g_themed_icon_append_name(G_THEMED_ICON(gicon), "application-x-executable");

    mime_type->icon = fm_icon_from_gicon(gicon);
    g_object_unref(gicon);

    return mime_type;
}

FmMimeType* _fm_mime_type_get_inode_directory()
{
    return directory_type;
}

FmMimeType* _fm_mime_type_get_inode_x_shortcut()
{
    return shortcut_type;
}

FmMimeType* _fm_mime_type_get_inode_mount_point()
{
    return mountable_type;
}

FmMimeType* _fm_mime_type_get_application_x_desktop()
{
    return desktop_entry_type;
}

/**
 * fm_mime_type_ref
 * @mime_type: a #FmMimeType descriptor
 *
 * Increments reference count on @mime_type.
 *
 * Returns: @mime_type.
 *
 * Since: 0.1.0
 */
FmMimeType* fm_mime_type_ref(FmMimeType* mime_type)
{
    g_atomic_int_inc(&mime_type->n_ref);
    return mime_type;
}

/**
 * fm_mime_type_unref
 * @mime_type_: a #FmMimeType descriptor
 *
 * Decrements reference count on @mime_type_.
 *
 * Since: 0.1.0
 */
void fm_mime_type_unref(gpointer mime_type_)
{
    FmMimeType* mime_type = (FmMimeType*)mime_type_;
    if (g_atomic_int_dec_and_test(&mime_type->n_ref))
    {
        g_free(mime_type->type);
        g_free(mime_type->description);
        if (mime_type->icon)
            g_object_unref(mime_type->icon);
        g_assert(mime_type->thumbnailers == NULL);
            /* Note: we do not own references for FmThumbnailer here.
               this list should be free already or else it's failure
               and fm-thumbnailer.c will try to unref this destroyed object */
        g_slice_free(FmMimeType, mime_type);
    }
}

/**
 * fm_mime_type_get_icon
 * @mime_type: a #FmMimeType descriptor
 *
 * Retrieves icon associated with @mime_type. Returned data are owned by
 * @mime_type and should be not freed by caller.
 *
 * Returns: icon.
 *
 * Since: 0.1.0
 */
FmIcon* fm_mime_type_get_icon(FmMimeType* mime_type)
{
    return mime_type->icon;
}

/**
 * fm_mime_type_get_type
 * @mime_type: a #FmMimeType descriptor
 *
 * Retrieves MIME type name of @mime_type. Returned data are owned by
 * @mime_type and should be not freed by caller.
 *
 * Returns: MIME type name.
 *
 * Since: 0.1.0
 */
const char* fm_mime_type_get_type(FmMimeType* mime_type)
{
    return mime_type->type;
}

/**
 * fm_mime_type_get_thumbnailers
 * @mime_type: a #FmMimeType descriptor
 *
 * Retrieves list of thumbnailers associated with @mime_type. Returned
 * data are owned by @mime_type and should be not altered by caller.
 *
 * Returns: (element-type gpointer) (transfer none): the list.
 *
 * Deprecated: 1.2.0: Use fm_mime_type_get_thumbnailers_list() instead.
 *
 * Since: 1.0.0
 */
const GList* fm_mime_type_get_thumbnailers(FmMimeType* mime_type)
{
    /* FIXME: need this be thread-safe? */
    return mime_type->thumbnailers;
}

/**
 * fm_mime_type_get_thumbnailers_list
 * @mime_type: a #FmMimeType descriptor
 *
 * Retrieves list of thumbnailers associated with @mime_type. Returned
 * data should be freed after usage.
 *
 * Returns: (transfer full) (element-type FmThumbnailer): the list.
 *
 * Since: 1.2.0
 */
GList* fm_mime_type_get_thumbnailers_list(FmMimeType* mime_type)
{
    GList *list = NULL, *l;

    G_LOCK(thumbnailers);
    for (l = mime_type->thumbnailers; l; l = l->next)
        list = g_list_prepend(list, fm_thumbnailer_ref(l->data));
    list = g_list_reverse(list);
    G_UNLOCK(thumbnailers);
    return list;
}

/**
 * fm_mime_type_add_thumbnailer
 * @mime_type: a #FmMimeType descriptor
 * @thumbnailer: anonymous thumbnailer pointer
 *
 * Adds @thumbnailer to list of thumbnailers associated with @mime_type.
 *
 * Since: 1.0.0
 */
void fm_mime_type_add_thumbnailer(FmMimeType* mime_type, gpointer thumbnailer)
{
    G_LOCK(thumbnailers);
    mime_type->thumbnailers = g_list_append(mime_type->thumbnailers, thumbnailer);
    G_UNLOCK(thumbnailers);
}

/**
 * fm_mime_type_remove_thumbnailer
 * @mime_type: a #FmMimeType descriptor
 * @thumbnailer: anonymous thumbnailer pointer
 *
 * Removes @thumbnailer from list of thumbnailers associated with
 * @mime_type.
 *
 * Since: 1.0.0
 */
void fm_mime_type_remove_thumbnailer(FmMimeType* mime_type, gpointer thumbnailer)
{
    G_LOCK(thumbnailers);
    mime_type->thumbnailers = g_list_remove(mime_type->thumbnailers, thumbnailer);
    G_UNLOCK(thumbnailers);
}

/**
 * fm_mime_type_get_desc
 * @mime_type: a #FmMimeType descriptor
 *
 * Retrieves human-readable description of MIME type. Returned data are
 * owned by @mime_type and should be not freed by caller.
 *
 * Returns: MIME type name.
 *
 * Since: 0.1.0
 */
const char* fm_mime_type_get_desc(FmMimeType* mime_type)
{
    /* FIXME: is locking needed here or not? */
    if (G_UNLIKELY(! mime_type->description))
    {
        mime_type->description = g_content_type_get_description(mime_type->type);
        /* FIXME: should handle this better */
        if (G_UNLIKELY(! mime_type->description || ! *mime_type->description))
            mime_type->description = g_content_type_get_description(mime_type->type);
    }
    return mime_type->description;
}
