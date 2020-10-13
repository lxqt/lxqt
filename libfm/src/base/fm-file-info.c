/*
 *      fm-file-info.c
 *
 *      Copyright 2009 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2009 Juergen Hoetzel <juergen@archlinux.org>
 *      Copyright 2017 Tsu Jan <tsujan2000@gmail.com>
 *      Copyright 2012-2018 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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
 * SECTION:fm-file-info
 * @short_description: File information cache for libfm.
 * @title: FmFileInfo
 *
 * @include: libfm/fm.h
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <menu-cache.h>
#include "fm-file-info.h"
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <grp.h> /* Query group name */
#include <pwd.h> /* Query user name */
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "fm-config.h"
#include "fm-utils.h"

/* support for libmenu-cache 0.4.x */
#ifndef MENU_CACHE_CHECK_VERSION
# ifdef HAVE_MENU_CACHE_DIR_LIST_CHILDREN
#  define MENU_CACHE_CHECK_VERSION(_a,_b,_c) (_a == 0 && _b < 5) /* < 0.5.0 */
# else
#  define MENU_CACHE_CHECK_VERSION(_a,_b,_c) 0 /* not even 0.4.0 */
# endif
#endif

#define COLLATE_USING_DISPLAY_NAME    ((char*)-1)

static FmIcon* icon_locked_folder = NULL;

/* all of the user special dirs are direct child of home directory */
static gboolean special_dirs_all_in_home = TRUE;

typedef struct _SpecialDirInfo
{
    const char* path_str;
    const char* base_name;
    const char* icon_name;
}SpecialDirInfo;

/* information of the user special dirs defined by xdg */
static SpecialDirInfo special_dir_info[G_USER_N_DIRECTORIES] = {
    {NULL, NULL, "user-desktop"},
    {NULL, NULL, "folder-documents"},
    {NULL, NULL, "folder-download"},
    {NULL, NULL, "folder-music"},
    {NULL, NULL, "folder-pictures"},
    {NULL, NULL, "folder-publicshare"},
    {NULL, NULL, "folder-templates"},
    {NULL, NULL, "folder-videos"}
};

struct _FmFileInfo
{
    FmPath* path; /* path of the file */

    mode_t mode;
    union {
        const char* fs_id;
        dev_t dev;
    };
    uid_t uid;
    gid_t gid;
    goffset size;
    time_t mtime;
    time_t atime;
    time_t ctime;

    gulong blksize;
    goffset blocks;

    /* FIXME: caching the collate key can greatly speed up sorting.
     *        However, memory usage is greatly increased!.
     *        Is there a better alternative solution?
     */
    char* collate_key; /* used to sort files by name */
    char* collate_key_case; /* the same but case-sensitive */
    char* disp_size;  /* displayed human-readable file size */
    char* disp_mtime; /* displayed last modification time */
    char* disp_owner;
    char* disp_group;
    FmMimeType* mime_type;
    FmIcon* icon;

    char* target; /* target of shortcut or mountable. */

    gboolean shortcut : 1; /* TRUE if file is shortcut type */
    gboolean accessible : 1; /* TRUE if can be read by user */
    gboolean hidden : 1; /* TRUE if file is hidden */
    gboolean backup : 1; /* TRUE if file is backup */
    gboolean name_is_changeable : 1; /* TRUE if name can be changed */
    gboolean icon_is_changeable : 1; /* TRUE if icon can be changed */
    gboolean hidden_is_changeable : 1; /* TRUE if hidden can be changed */
    gboolean fs_is_ro : 1; /* TRUE if host FS is R/O */

    /*<private>*/
    int n_ref;
};

struct _FmFileInfoList
{
    FmList list;
};

/* intialize the file info system */
void _fm_file_info_init(void)
{
    const char* user_home = fm_get_home_dir();
    int home_dir_len = strlen(user_home);
    int i;
    icon_locked_folder = fm_icon_from_name("folder-locked");

    for(i = 0; i < G_USER_N_DIRECTORIES; ++i)
    {
        /* get information of every special dir */
        const char* path_str = g_get_user_special_dir(i);
        const char* base_name = NULL;
        if(path_str)
        {
            /* FIXME: will someone put / at the end of the path? */
            base_name = strrchr(path_str, '/'); /* find the last '/' in dir path */
            if(base_name)
            {
                int prefix_len = (base_name - path_str);
                /* check if the special dir has home dir prefix */
                if(prefix_len != home_dir_len || strncmp(path_str, user_home, home_dir_len))
                    special_dirs_all_in_home = FALSE; /* not all of the special dirs are in home dir */
                ++base_name; /* skip separator */
                special_dir_info[i].base_name = base_name;
            }
            special_dir_info[i].path_str = path_str;
        }
    }
}

void _fm_file_info_finalize()
{
    g_object_unref(icon_locked_folder);
}

/**
 * fm_file_info_new:
 *
 * Returns: a new FmFileInfo struct which needs to be freed with
 * fm_file_info_unref() when it's no more needed.
 */
FmFileInfo* fm_file_info_new ()
{
    FmFileInfo * fi = g_slice_new0(FmFileInfo);
    fi->n_ref = 1;
    return fi;
}

/**
 * _fm_file_info_set_emblems:
 * @fi:  A FmFileInfo struct
 * @inf: A GFileInfo object
 *
 * Read icon emblems metadata from the "metadata::emblems" attribute of
 * a GFileInfo object and make the GIcon store in the FmFileInfo a 
 * GEmblemedIcon object if the file has emblems.
 */
static void _fm_file_info_set_emblems(FmFileInfo* fi, GFileInfo* inf)
{
    char** emblem_names = g_file_info_get_attribute_stringv(inf, "metadata::emblems");
    if(emblem_names)
    {
        GIcon* gicon = g_emblemed_icon_new(G_ICON(fi->icon), NULL);
        char** emblem_name;
        for(emblem_name = emblem_names; *emblem_name; ++emblem_name)
        {
            GIcon* emblem_icon = g_themed_icon_new(*emblem_name);
            GEmblem* emblem = g_emblem_new(emblem_icon);
            g_object_unref(emblem_icon);
            g_emblemed_icon_add_emblem(G_EMBLEMED_ICON(gicon), emblem);
            g_object_unref(emblem);
        }
        /* replace the original GIcon with an GEmblemedIcon */
        g_object_unref(fi->icon);
        fi->icon = fm_icon_from_gicon(gicon);
        g_object_unref(gicon);
    }
}

/**
 * fm_file_info_set_from_native_file:
 * @fi:  A FmFileInfo struct
 * @path:  full path of the file
 * @err: a GError** to retrive errors
 *
 * Get file info of the specified native file and store it in
 * the FmFileInfo struct.
 * 
 * Prior to calling this function, the FmPath of FmFileInfo should
 * have been set with fm_file_info_set_path().
 *
 * Note that this call does I/O and therefore can block.
 *
 * Returns: TRUE if no error happens.
 */
gboolean _fm_file_info_set_from_native_file(FmFileInfo* fi, const char* path,
                                            GError** err, gboolean get_fast)
{
    struct stat st;
    char *dname;

    g_return_val_if_fail(fi && fi->path, FALSE);
    if(lstat(path, &st) == 0)
    {
        GFile* gfile;
        GFileInfo* inf;

        fi->mode = st.st_mode;
        fi->mtime = st.st_mtime;
        fi->atime = st.st_atime;
        fi->ctime = st.st_ctime;
        fi->size = st.st_size;
        fi->dev = st.st_dev;
        fi->uid = st.st_uid;
        fi->gid = st.st_gid;

        /* handle symlinks: use target to retrieve its info */
        if(S_ISLNK(st.st_mode))
        {
            if (stat(path, &st) < 0)
            {
                /* g_debug("invalid symlink: %s", strerror(errno)); */
                fi->icon = fm_icon_from_name("dialog-warning");
                /* we cannot test broken symlink so skip all tests */
                get_fast = TRUE;
            }
            fi->target = g_file_read_link(path, NULL);
        }

        /* files with . prefix or ~ suffix are regarded as hidden files.
         * dirs with . prefix are regarded as hidden dirs. */
        dname = (char*)fm_path_get_basename(fi->path);
        fi->hidden = (dname[0] == '.');
        fi->backup = (!S_ISDIR(st.st_mode) && g_str_has_suffix(dname, "~"));
        dname = NULL;

        if (get_fast && S_ISREG(st.st_mode)) /* do rough estimation */
        {
            /* for non-regular files fm_mime_type_from_native_file() is fast */
            if ((st.st_mode & S_IXUSR) == S_IXUSR) /* executable */
                fi->mime_type = fm_mime_type_from_name("application/x-executable");
            else
                fi->mime_type = fm_mime_type_from_file_name(fm_path_get_basename(fi->path));
        }
        else
        {
            fi->mime_type = fm_mime_type_from_native_file(path, fm_path_get_basename(fi->path), &st);
            if (G_UNLIKELY(fi->mime_type == NULL))
                /* file might be deleted while we test it but we assume mime_type is not NULL */
                fi->mime_type = fm_mime_type_from_name("application/octet-stream");
        }

        if (get_fast) /* do rough estimation */
            fi->accessible = ((st.st_mode & S_IRUSR) == S_IRUSR);
        else
            fi->accessible = (g_access(path, R_OK) == 0);

        /* special handling for desktop entry files */
        if(G_UNLIKELY(!get_fast && fm_file_info_is_desktop_entry(fi)))
        {
            GKeyFile* kf = g_key_file_new();
            FmIcon* icon = NULL;
            char* icon_name;
            char* type;

            if(g_key_file_load_from_file(kf, path, 0, NULL))
            {
                /* check if type is correct and supported */
                type = g_key_file_get_string(kf, "Desktop Entry", "Type", NULL);
                if(type)
                {
                    /* g_debug("got desktop entry with type %s", type); */
                    if(strcmp(type, G_KEY_FILE_DESKTOP_TYPE_LINK) == 0)
                    {
                        char *uri = g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                                          G_KEY_FILE_DESKTOP_KEY_URL, NULL);
                        /* handle Type=Link, those are shortcuts
                           therefore set ->shortcut, ->target, ->mime_type */
                        if (uri)
                        {
                            FmMimeType *new_mime_type = fm_mime_type_from_file_name(uri);

                            /* g_debug("got type %s for URL %s", fm_mime_type_get_type(new_mime_type), uri); */
                            if (strcmp(fm_mime_type_get_type(new_mime_type),
                                       "application/octet-stream") == 0 ||
                                /* actually remote links should never be
                                   directories so let treat them as unknown */
                                (new_mime_type == _fm_mime_type_get_inode_directory()
                                 && !g_str_has_prefix(uri, "file:/")))
                            {
                                /* NOTE: earlier we classified all links to
                                   desktop entry as inode/x-shortcut too but
                                   that would require a lot of special support
                                   therefore we set to inode/x-shortcut only
                                   those shortcuts that we fail to determine */
                                fm_mime_type_unref(new_mime_type);
                                new_mime_type = fm_mime_type_ref(_fm_mime_type_get_inode_x_shortcut());
                            }
                            fm_mime_type_unref(fi->mime_type);
                            fi->mime_type = new_mime_type;
                            fi->shortcut = TRUE;
                            fi->target = uri;
                        }
                        else
                        {
                            /* otherwise it's error, Link should have URL */
                            g_free(type);
                            goto _not_desktop_entry;
                        }
                    }
                    /* FIXME: fail if Type isn't Application or Directory */
                    g_free(type);
                }
                else
                    goto _not_desktop_entry;
                icon_name = g_key_file_get_string(kf, "Desktop Entry", "Icon", NULL);
                if(icon_name)
                {
                    icon = fm_icon_from_name(icon_name);
                    g_free(icon_name);
                }
                /* Use title of the desktop entry for display */
                dname = g_key_file_get_locale_string(kf, "Desktop Entry", "Name", NULL, NULL);
                /* handle 'Hidden' key to set hidden attribute */
                if (!fi->hidden)
                    fi->hidden = g_key_file_get_boolean(kf, "Desktop Entry", "Hidden", NULL);
            }
            else
            {
                /* otherwise it's error so treat the file as simple text */
_not_desktop_entry:
                fm_mime_type_unref(fi->mime_type);
                fi->mime_type = fm_mime_type_from_name("text/plain");
            }
            if(icon)
                fi->icon = icon;
            else
                fi->icon = g_object_ref(fm_mime_type_get_icon(fi->mime_type));
            g_key_file_free(kf);
        }
        else if(!S_ISDIR(st.st_mode))
            ;
        /* set "locked" icon on unaccesible folder */
        else if(!fi->accessible)
            fi->icon = g_object_ref(icon_locked_folder);
        else if(!get_fast && S_ISDIR(st.st_mode)) /* special handling for folder icons */
        {
            FmPath* fmpath = fi->path;

            if(fm_path_equal(fmpath, fm_path_get_home())) /* this file is the home dir */
                fi->icon = fm_icon_from_name("user-home");
            else
            {
                FmPath* parent_path = fm_path_get_parent(fmpath);
                SpecialDirInfo* si;
                int i;
                if(special_dirs_all_in_home)
                {
                    /* pcman: This is a little trick for optimization.
                     * In most normal cases, all of the special folders are
                     * by default the direct child of user home dir,
                     * If this is the case, we can skip the check if our file
                     * is not in home dir since it's impossible for it to be
                     * a special dir.
                     * Without this trick, we do all the strcmp() calls
                     * for every single file found in the dir, which is expansive.
                     * With this trck, we only do this if we're in the home dir.
                     */
                    if(fm_path_equal(parent_path, fm_path_get_home()))
                    {
                        /* special dirs are all in home dir and we're in home dir, too */
                        const char* base_name = fm_path_get_basename(fmpath);
                        for(i = 0; i < G_USER_N_DIRECTORIES; ++i)
                        {
                            si = &special_dir_info[i];
                            if(si->base_name && strcmp(si->base_name, base_name) == 0)
                            {
                                fi->icon = fm_icon_from_name(si->icon_name);
                                break;
                            }
                        }
                    }
                    /* if all special dirs are in home dir and this file is not, it can't be a special folder */
                }
                else
                {
                    const char* base_name = fm_path_get_basename(fmpath);
                    for(i = 0; i < G_USER_N_DIRECTORIES; ++i)
                    {
                        si = &special_dir_info[i];
                        /* compare base name first, and then prefix if needed. */
                        if(si->base_name && strcmp(si->base_name, base_name) == 0
                            && strncmp(si->path_str, path, (si->base_name - si->path_str)) == 0)
                        {
                            fi->icon = fm_icon_from_name(si->icon_name);
                            break;
                        }
                    }
                }
            }
        }
        if(!fi->icon)
            fi->icon = g_object_ref(fm_mime_type_get_icon(fi->mime_type));

        gfile = g_file_new_for_path(path);

        /* get emblems using gio/gvfs-metadata */
        inf = g_file_query_info(gfile, "metadata::emblems,standard::icon", G_FILE_QUERY_INFO_NONE, NULL, NULL);
        if(inf)
        {
            _fm_file_info_set_emblems(fi, inf);
            g_object_unref(inf);
        }

        if (!dname)
            dname = g_filename_display_basename(path);
        _fm_path_set_display_name(fi->path, dname);
        g_free(dname);

        /* check if directory's file system is read-only, default is FALSE */
        fi->fs_is_ro = FALSE;
        if (S_ISDIR(st.st_mode))
        {
            inf = g_file_query_filesystem_info(gfile, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY,
                                               NULL, NULL);
            if (inf)
            {
                fi->fs_is_ro = g_file_info_get_attribute_boolean(inf, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY);
                g_object_unref(inf);
            }
        }
        g_object_unref(gfile);
    }
    else
    {
        g_set_error(err, G_IO_ERROR, g_io_error_from_errno(errno),
                    "%s: %s", path, g_strerror(errno));
        return FALSE;
    }
    /* name is changeable for native files */
    fi->name_is_changeable = TRUE;
    /* hidden attribute is immutable for native files */
    fi->hidden_is_changeable = FALSE;
    /* we can change icon only for accessible desktop entry */
    fi->icon_is_changeable = fm_file_info_is_desktop_entry(fi);
        /* FIXME: add support for icon change on directories too */
    return TRUE;
}

gboolean fm_file_info_set_from_native_file(FmFileInfo* fi, const char* path, GError** err)
{
    return _fm_file_info_set_from_native_file(fi, path, err, FALSE);
}

/**
 * fm_file_info_new_from_native_file
 * @path: (allow-none): path descriptor
 * @path_str: full path to the file
 * @err: (allow-none) (out): pointer to receive error
 *
 * Create a new #FmFileInfo for file pointed by @path. Returned data
 * should be freed with fm_file_info_unref() after usage.
 *
 * Returns: (transfer full): new file info or %NULL in case of error.
 *
 * Since: 1.2.0
 */
FmFileInfo *fm_file_info_new_from_native_file(FmPath *path, const char *path_str, GError **err)
{
    FmFileInfo* fi = fm_file_info_new();
    if (path)
        fi->path = fm_path_ref(path);
    else
        fi->path = fm_path_new_for_path(path_str);
    if (_fm_file_info_set_from_native_file(fi, path_str, err, TRUE))
        return fi;
    fm_file_info_unref(fi);
    return NULL;
}

/**
 * fm_file_info_set_from_gfileinfo:
 * @fi:  A FmFileInfo struct
 * @inf: a GFileInfo object
 *
 * Get file info from the GFileInfo object and store it in
 * the FmFileInfo struct.
 *
 * Deprecated: 1.2.0: Use fm_file_info_set_from_g_file_data() instead.
 */
void fm_file_info_set_from_gfileinfo(FmFileInfo* fi, GFileInfo* inf)
{
    fm_file_info_set_from_g_file_data(fi, NULL, inf);
}

/**
 * fm_file_info_set_from_g_file_data
 * @fi: a #FmFileInfo struct to update
 * @gf: (allow-none): a #GFile object to inspect
 * @inf: a #GFileInfo object to inspect
 *
 * Get file info from the #GFile and #GFileInfo objects and sets data in
 * the #FmFileInfo struct appropriately.
 *
 * Since: 1.2.0
 */
void fm_file_info_set_from_g_file_data(FmFileInfo *fi, GFile *gf, GFileInfo *inf)
{
    const char *tmp, *uri;
    GIcon* gicon;
    GFile *_gf = NULL;
    GFileAttributeInfoList *list;
    GFileType type;

    g_return_if_fail(fi->path);

    tmp = g_file_info_get_edit_name(inf);
    if (!tmp || strcmp(tmp, "/") == 0)
        tmp = g_file_info_get_display_name(inf);
    _fm_path_set_display_name(fi->path, tmp);

    fi->size = g_file_info_get_size(inf);

    tmp = g_file_info_get_content_type(inf);
    if(tmp)
        fi->mime_type = fm_mime_type_from_name(tmp);

    fi->mode = g_file_info_get_attribute_uint32(inf, G_FILE_ATTRIBUTE_UNIX_MODE);

    fi->uid = fi->gid = -1;
    if(g_file_info_has_attribute(inf, G_FILE_ATTRIBUTE_UNIX_UID))
        fi->uid = g_file_info_get_attribute_uint32(inf, G_FILE_ATTRIBUTE_UNIX_UID);
    if(g_file_info_has_attribute(inf, G_FILE_ATTRIBUTE_UNIX_GID))
        fi->gid = g_file_info_get_attribute_uint32(inf, G_FILE_ATTRIBUTE_UNIX_GID);

    type = g_file_info_get_file_type(inf);
    if(0 == fi->mode) /* if UNIX file mode is not available, compose a fake one. */
    {
        switch(type)
        {
        case G_FILE_TYPE_REGULAR:
            fi->mode |= S_IFREG;
            break;
        case G_FILE_TYPE_DIRECTORY:
            fi->mode |= S_IFDIR;
            break;
        case G_FILE_TYPE_SYMBOLIC_LINK:
            fi->mode |= S_IFLNK;
            break;
        case G_FILE_TYPE_SHORTCUT:
            break;
        case G_FILE_TYPE_MOUNTABLE:
            break;
        case G_FILE_TYPE_SPECIAL:
            if(fi->mode)
                break;
        /* if it's a special file but it doesn't have UNIX mode, compose a fake one. */
            if(strcmp(tmp, "inode/chardevice")==0)
                fi->mode |= S_IFCHR;
            else if(strcmp(tmp, "inode/blockdevice")==0)
                fi->mode |= S_IFBLK;
            else if(strcmp(tmp, "inode/fifo")==0)
                fi->mode |= S_IFIFO;
#ifdef S_IFSOCK
            else if(strcmp(tmp, "inode/socket")==0)
                fi->mode |= S_IFSOCK;
#endif
            break;
        case G_FILE_TYPE_UNKNOWN:
            ;
        }
    }

    if(g_file_info_has_attribute(inf, G_FILE_ATTRIBUTE_ACCESS_CAN_READ))
        fi->accessible = g_file_info_get_attribute_boolean(inf, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
    else
        /* assume it's accessible */
        fi->accessible = TRUE;

    /* special handling for symlinks */
    if (g_file_info_get_is_symlink(inf))
    {
        fi->mode &= ~S_IFMT; /* reset type */
        fi->mode |= S_IFLNK; /* set type to symlink */
        goto _file_is_symlink;
    }

    switch(type)
    {
    case G_FILE_TYPE_SHORTCUT:
        fi->shortcut = TRUE;
    case G_FILE_TYPE_MOUNTABLE:
        uri = g_file_info_get_attribute_string(inf, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI);
        if(uri)
        {
            if(g_str_has_prefix(uri, "file:///"))
                fi->target = g_filename_from_uri(uri, NULL, NULL);
            else
                fi->target = g_strdup(uri);
            if(!fi->mime_type)
                fi->mime_type = fm_mime_type_from_file_name(fi->target);
        }

        /* if the mime-type is not determined or is unknown */
        if(G_UNLIKELY(!fi->mime_type || g_content_type_is_unknown(fm_mime_type_get_type(fi->mime_type))))
        {
            /* FIXME: is this appropriate? */
            if(type == G_FILE_TYPE_SHORTCUT)
                fi->mime_type = fm_mime_type_ref(_fm_mime_type_get_inode_x_shortcut());
            else
                fi->mime_type = fm_mime_type_ref(_fm_mime_type_get_inode_mount_point());
        }
        break;
    case G_FILE_TYPE_DIRECTORY:
        if(!fi->mime_type)
            fi->mime_type = fm_mime_type_ref(_fm_mime_type_get_inode_directory());
        fi->fs_is_ro = FALSE; /* default is R/W */
        if (g_file_info_has_attribute(inf, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY))
            fi->fs_is_ro = g_file_info_get_attribute_boolean(inf, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY);
        break;
    case G_FILE_TYPE_SYMBOLIC_LINK:
_file_is_symlink:
        uri = g_file_info_get_symlink_target(inf);
        if(uri)
        {
            if(g_str_has_prefix(uri, "file:///"))
                fi->target = g_filename_from_uri(uri, NULL, NULL);
            else
                fi->target = g_strdup(uri);
            if(!fi->mime_type)
                fi->mime_type = fm_mime_type_from_file_name(fi->target);
        }
        /* continue with absent mime type */
    default: /* G_FILE_TYPE_UNKNOWN G_FILE_TYPE_REGULAR G_FILE_TYPE_SPECIAL */
        if(G_UNLIKELY(!fi->mime_type))
        {
            uri = g_file_info_get_name(inf);
            fi->mime_type = fm_mime_type_from_file_name(uri);
        }
    }

    /* try file-specific icon first */
    gicon = g_file_info_get_icon(inf);
    if(gicon)
        fi->icon = fm_icon_from_gicon(gicon);
        /* g_object_unref(gicon); this is not needed since
         * g_file_info_get_icon didn't increase ref_count.
         * the object returned by g_file_info_get_icon is
         * owned by GFileInfo. */
    /* set "locked" icon on unaccesible folder */
    else if(!fi->accessible && type == G_FILE_TYPE_DIRECTORY)
        fi->icon = g_object_ref(icon_locked_folder);
    else
        fi->icon = g_object_ref(fm_mime_type_get_icon(fi->mime_type));

    /* if the file has emblems, add them to the icon */
    _fm_file_info_set_emblems(fi, inf);

    if(fm_path_is_native(fi->path))
    {
        fi->dev = g_file_info_get_attribute_uint32(inf, G_FILE_ATTRIBUTE_UNIX_DEVICE);
    }
    else
    {
        tmp = g_file_info_get_attribute_string(inf, G_FILE_ATTRIBUTE_ID_FILESYSTEM);
        fi->fs_id = g_intern_string(tmp);
    }

    fi->mtime = g_file_info_get_attribute_uint64(inf, G_FILE_ATTRIBUTE_TIME_MODIFIED);
    fi->atime = g_file_info_get_attribute_uint64(inf, G_FILE_ATTRIBUTE_TIME_ACCESS);
    fi->ctime = g_file_info_get_attribute_uint64(inf, G_FILE_ATTRIBUTE_TIME_CHANGED);
    fi->hidden = g_file_info_get_is_hidden(inf);
    fi->backup = g_file_info_get_is_backup(inf);
    fi->name_is_changeable = TRUE; /* GVFS tends to ignore this attribute */
    fi->icon_is_changeable = fi->hidden_is_changeable = FALSE;
    if (g_file_info_has_attribute(inf, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME))
        fi->name_is_changeable = g_file_info_get_attribute_boolean(inf, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME);
    if (G_UNLIKELY(gf == NULL))
        gf = _gf = fm_path_to_gfile(fi->path);
    list = g_file_query_settable_attributes(gf, NULL, NULL);
    if (G_LIKELY(list))
    {
        if (g_file_attribute_info_list_lookup(list, G_FILE_ATTRIBUTE_STANDARD_ICON))
            fi->icon_is_changeable = TRUE;
        if (g_file_attribute_info_list_lookup(list, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN))
            fi->hidden_is_changeable = TRUE;
        g_file_attribute_info_list_unref(list);
    }
    if (G_UNLIKELY(_gf))
        g_object_unref(_gf);
}


/**
 * fm_file_info_new_from_gfileinfo:
 * @path:  FmPath of a file
 * @inf: a GFileInfo object
 *
 * Create a new FmFileInfo for file pointed by @path based on
 * information stored in the GFileInfo object.
 *
 * Returns: A new FmFileInfo struct which should be freed with
 * fm_file_info_unref() when no longer needed.
 *
 * Deprecated: 1.2.0: Use fm_file_info_new_from_g_file_data() instead.
 */
FmFileInfo* fm_file_info_new_from_gfileinfo(FmPath* path, GFileInfo* inf)
{
    GFile *gf = fm_path_to_gfile(path);
    FmFileInfo *fi;
    fi = fm_file_info_new_from_g_file_data(gf, inf, path);
    g_object_unref(gf);
    return fi;
}

/**
 * fm_file_info_new_from_g_file_data
 * @gf: a #GFile of a file
 * @inf: a #GFileInfo object
 * @path: (allow-none): a #FmPath of a file
 *
 * Creates a new #FmFileInfo for file pointed by @path and @gf based on
 * information stored in the @inf. Returned data should be freed with
 * fm_file_info_unref() when no longer needed.
 *
 * Returns: (transfer full): a new #FmFileInfo struct.
 *
 * Since: 1.2.0
 */
FmFileInfo* fm_file_info_new_from_g_file_data(GFile *gf, GFileInfo *inf, FmPath *path)
{
    FmFileInfo* fi = fm_file_info_new();
    if (path)
        fi->path = fm_path_ref(path);
    else
        fi->path = fm_path_new_for_gfile(gf);
    fm_file_info_set_from_g_file_data(fi, gf, inf);
    return fi;
}

/**
 * fm_file_info_set_from_menu_cache_item
 * @fi: a file info to update
 * @item: a menu cache item
 *
 * Deprecated: 1.2.0:
 */
void fm_file_info_set_from_menu_cache_item(FmFileInfo* fi, MenuCacheItem* item)
{
    const char* icon_name;
    icon_name = menu_cache_item_get_icon(item);
    _fm_path_set_display_name(fi->path, menu_cache_item_get_name(item));
    if(icon_name)
    {
        fi->icon = fm_icon_from_name(icon_name);
    }
    if(menu_cache_item_get_type(item) == MENU_CACHE_TYPE_DIR)
    {
        fi->mode = S_IFDIR;
        fi->mime_type = fm_mime_type_ref(_fm_mime_type_get_inode_directory());
#if MENU_CACHE_CHECK_VERSION(0, 5, 0)
        fi->hidden = !menu_cache_dir_is_visible(MENU_CACHE_DIR(item));
#endif
    }
    else if(menu_cache_item_get_type(item) == MENU_CACHE_TYPE_APP)
    {
        fi->target = menu_cache_item_get_file_path(item);
        fi->mime_type = fm_mime_type_ref(_fm_mime_type_get_application_x_desktop());
        fi->hidden = !menu_cache_app_get_is_visible(MENU_CACHE_APP(item), (guint32)-1);
        fi->hidden_is_changeable = TRUE;
        fi->shortcut = TRUE;
    }
    else /* nothing to set if separator */
        return;
    fi->accessible = TRUE;
    fi->name_is_changeable = TRUE;
    fi->icon_is_changeable = TRUE;
}

/**
 * fm_file_info_new_from_menu_cache_item
 * @path: a file path
 * @item: a menu cache item
 *
 * Creates a new #FmFileInfo for a file by @path and fills it with info
 * from a menu cache @item. Returned data should be freed with
 * fm_file_info_unref() when no longer needed.
 *
 * Returns: (transfer full): a new #FmFileInfo struct.
 *
 * Since: 0.1.1
 */
FmFileInfo* fm_file_info_new_from_menu_cache_item(FmPath* path, MenuCacheItem* item)
{
    FmFileInfo* fi = fm_file_info_new();
    fi->path = fm_path_ref(path);
    fm_file_info_set_from_menu_cache_item(fi, item);
    return fi;
}

static void fm_file_info_clear(FmFileInfo* fi)
{
    if(fi->collate_key)
    {
        if(fi->collate_key != COLLATE_USING_DISPLAY_NAME)
            g_free(fi->collate_key);
        fi->collate_key = NULL;
    }

    if(fi->collate_key_case)
    {
        if(fi->collate_key_case != COLLATE_USING_DISPLAY_NAME)
            g_free(fi->collate_key_case);
        fi->collate_key_case = NULL;
    }

    if(G_LIKELY(fi->path))
    {
        fm_path_unref(fi->path);
        fi->path = NULL;
    }

    if(G_LIKELY(fi->disp_size))
    {
        g_free(fi->disp_size);
        fi->disp_size = NULL;
    }

    if(G_UNLIKELY(fi->disp_mtime))
    {
        g_free(fi->disp_mtime);
        fi->disp_mtime = NULL;
    }

    g_free(fi->disp_owner);
    fi->disp_owner = NULL;
    g_free(fi->disp_group);
    fi->disp_group = NULL;

    if(G_UNLIKELY(fi->target))
    {
        g_free(fi->target);
        fi->target = NULL;
    }

    if(G_LIKELY(fi->mime_type))
    {
        fm_mime_type_unref(fi->mime_type);
        fi->mime_type = NULL;
    }
    if(G_LIKELY(fi->icon))
    {
        g_object_unref(fi->icon);
        fi->icon = NULL;
    }
}

/**
 * fm_file_info_ref:
 * @fi:  A FmFileInfo struct
 *
 * Increase reference count of the FmFileInfo struct.
 *
 * Returns: the FmFileInfo struct itself
 */
FmFileInfo* fm_file_info_ref(FmFileInfo* fi)
{
    g_return_val_if_fail(fi != NULL, NULL);
    g_atomic_int_inc(&fi->n_ref);
    return fi;
}

/**
 * fm_file_info_unref:
 * @fi:  A FmFileInfo struct
 *
 * Decrease reference count of the FmFileInfo struct.
 * When the last reference to the struct is released,
 * the FmFileInfo struct is freed.
 */
void fm_file_info_unref(FmFileInfo* fi)
{
    g_return_if_fail(fi != NULL);
    /* g_debug("unref file info: %d", fi->n_ref); */
    if (g_atomic_int_dec_and_test(&fi->n_ref))
    {
        fm_file_info_clear(fi);
        g_slice_free(FmFileInfo, fi);
    }
}

/**
 * fm_file_info_update:
 * @fi:  A FmFileInfo struct
 * @src: another FmFileInfo struct
 * 
 * This API is not thread-safe and should be used only in default context.
 *
 * Update the content of @fi by copying file info
 * stored in @src to @fi.
 */
void fm_file_info_update(FmFileInfo* fi, FmFileInfo* src)
{
    FmPath* tmp_path = fm_path_ref(src->path);
    FmMimeType* tmp_type = fm_mime_type_ref(src->mime_type);
    FmIcon* tmp_icon = g_object_ref(src->icon);
    /* NOTE: we need to ref source first. Otherwise,
     * if path, mime_type, and icon are identical in src
     * and fi, calling fm_file_info_clear() first on fi
     * might unref that. */
    fm_file_info_clear(fi);
    fi->path = tmp_path;
    fi->mime_type = tmp_type;
    fi->icon = tmp_icon;

    fi->mode = src->mode;
    if(fm_path_is_native(fi->path))
        fi->dev = src->dev;
    else
        fi->fs_id = src->fs_id;
    fi->uid = src->uid;
    fi->gid = src->gid;
    fi->size = src->size;
    fi->mtime = src->mtime;
    fi->atime = src->atime;
    fi->ctime = src->ctime;

    fi->blksize = src->blksize;
    fi->blocks = src->blocks;

    if(src->collate_key == COLLATE_USING_DISPLAY_NAME)
        fi->collate_key = COLLATE_USING_DISPLAY_NAME;
    else
        fi->collate_key = g_strdup(src->collate_key);
    if(src->collate_key_case == COLLATE_USING_DISPLAY_NAME)
        fi->collate_key_case = COLLATE_USING_DISPLAY_NAME;
    else
        fi->collate_key_case = g_strdup(src->collate_key_case);
    fi->disp_size = g_strdup(src->disp_size);
    fi->disp_mtime = g_strdup(src->disp_mtime);
    fi->disp_owner = g_strdup(src->disp_owner);
    fi->disp_group = g_strdup(src->disp_group);
    fi->target = g_strdup(src->target);
    fi->accessible = src->accessible;
    fi->hidden = src->hidden;
    fi->backup = src->backup;
    fi->name_is_changeable = src->name_is_changeable;
    fi->icon_is_changeable = src->icon_is_changeable;
    fi->hidden_is_changeable = src->hidden_is_changeable;
    fi->shortcut = src->shortcut;
    fi->fs_is_ro = src->fs_is_ro;
}

/**
 * fm_file_info_get_icon:
 * @fi:  A FmFileInfo struct
 *
 * Get the icon used to show the file in the file manager.
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: a FmIcon struct. The returned FmIcon struct is
 * owned by FmFileInfo and should not be freed.
 * If you need to keep it, use g_object_ref() to obtain a 
 * reference.
 */
FmIcon* fm_file_info_get_icon(FmFileInfo* fi)
{
    return fi->icon;
}

/**
 * fm_file_info_get_path:
 * @fi:  A FmFileInfo struct
 *
 * Get the path of the file
 * 
 * Returns: a FmPath struct. The returned FmPath struct is
 * owned by FmFileInfo and should not be freed.
 * If you need to keep it, use fm_path_ref() to obtain a 
 * reference.
 */
FmPath* fm_file_info_get_path(FmFileInfo* fi)
{
    return fi->path;
}

/**
 * fm_file_info_get_name:
 * @fi:  A FmFileInfo struct
 *
 * Get the base name of the file in filesystem encoding.
 *
 * Returns: a const string owned by FmFileInfo which should
 * not be freed.
 */
const char* fm_file_info_get_name(FmFileInfo* fi)
{
    return fm_path_get_basename(fi->path);
}

/**
 * fm_file_info_get_disp_name:
 * @fi:  A FmFileInfo struct
 *
 * Get the display name used to show the file in the file 
 * manager UI. The display name is guaranteed to be UTF-8
 * and may be different from the real file name on the 
 * filesystem.
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: a const strin owned by FmFileInfo which should
 * not be freed.
 */
const char* fm_file_info_get_disp_name(FmFileInfo* fi)
{
    const char *disp_name = _fm_path_get_display_name(fi->path);
    /* return basename if FmFileInfo is incomplete yet. it is a failure. */
    g_return_val_if_fail(disp_name != NULL, fm_path_get_basename(fi->path));
    return disp_name;
}

/**
 * fm_file_info_set_path:
 * @fi:  A FmFileInfo struct
 * @path: a FmPath struct
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Change the path of the FmFileInfo.
 */
void fm_file_info_set_path(FmFileInfo* fi, FmPath* path)
{
    if(fi->path)
        fm_path_unref(fi->path);

    if(path)
        fi->path = fm_path_ref(path);
    else
        fi->path = NULL;
}

/**
 * fm_file_info_set_disp_name:
 * @fi:  A FmFileInfo struct
 * @name: A UTF-8 display name. (can be NULL).
 *
 * Set the display name used to show the file in the 
 * file manager UI. If NULL is passed for @name,
 * the original display will be freed and the real base name
 * will be used for display.
 */
/* if disp name is set to NULL, we use the real filename for display. */
void fm_file_info_set_disp_name(FmFileInfo* fi, const char* name)
{
    _fm_path_set_display_name(fi->path, name);
    /* reset collate keys */
    if(fi->collate_key)
    {
        if(fi->collate_key != COLLATE_USING_DISPLAY_NAME)
            g_free(fi->collate_key);
        fi->collate_key = NULL;
    }
    if(fi->collate_key_case)
    {
        if(fi->collate_key_case != COLLATE_USING_DISPLAY_NAME)
            g_free(fi->collate_key_case);
        fi->collate_key_case = NULL;
    }
}

/**
 * fm_file_info_set_icon
 * @fi: a #FmFileInfo
 * @icon: an icon to update
 *
 * Updates the icon used to show the file in the file manager UI.
 *
 * Since: 1.2.0
 */
void fm_file_info_set_icon(FmFileInfo *fi, GIcon *icon)
{
    FmIcon *fm_icon = fm_icon_from_gicon(icon);

    if (fi->icon)
        g_object_unref(fi->icon);
    fi->icon = fm_icon;
}

/**
 * fm_file_info_get_size:
 * @fi:  A FmFileInfo struct
 *
 * Returns: the size of the file in bytes.
 */
goffset fm_file_info_get_size(FmFileInfo* fi)
{
    return fi->size;
}

/**
 * fm_file_info_get_disp_size:
 * @fi:  A FmFileInfo struct
 *
 * Get the size of the file as a human-readable string.
 * It's convinient for show the file size to the user.
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: a const string owned by FmFileInfo which should
 * not be freed. (non-NULL)
 */
const char* fm_file_info_get_disp_size(FmFileInfo* fi)
{
    if (G_UNLIKELY(!fi->disp_size))
    {
        if(S_ISREG(fi->mode))
        {
            char buf[ 64 ];
            fm_file_size_to_str2(buf, sizeof(buf), fi->size,
                        fm_config->list_view_size_units ? fm_config->list_view_size_units[0] : 0);
            fi->disp_size = g_strdup(buf);
        }
    }
    return fi->disp_size;
}

/**
 * fm_file_info_get_blocks
 * @fi:  A FmFileInfo struct
 *
 * Returns: how many filesystem blocks used by the file.
 */
goffset fm_file_info_get_blocks(FmFileInfo* fi)
{
    return fi->blocks;
}

/**
 * fm_file_info_get_mime_type:
 * @fi:  A FmFileInfo struct
 *
 * Get the mime-type of the file.
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: a FmMimeType struct owned by FmFileInfo which
 * should not be freed.
 * If you need to keep it, use fm_mime_type_ref() to obtain a 
 * reference.
 */
FmMimeType* fm_file_info_get_mime_type(FmFileInfo* fi)
{
    return fi->mime_type;
}

/**
 * fm_file_info_get_mode:
 * @fi:  A FmFileInfo struct
 *
 * Get the mode of the file. For detail about the meaning of
 * mode, see manpage of stat() and the st_mode struct field.
 *
 * Returns: mode_t value of the file as defined in POSIX struct stat.
 */
mode_t fm_file_info_get_mode(FmFileInfo* fi)
{
    return fi->mode;
}

/**
 * fm_file_info_get_is_native:
 * @fi:  A FmFileInfo struct
 *
 * Check if the file is a native UNIX file.
 * 
 * Returns: TRUE for native UNIX files, FALSE for
 * remote filesystems or other URIs, such as 
 * trahs:///, computer:///, ...etc.
 */
gboolean fm_file_info_is_native(FmFileInfo* fi)
{
    return fm_path_is_native(fi->path);
}

/**
 * fm_file_info_get_is_dir:
 * @fi:  A FmFileInfo struct
 *
 * Returns: TRUE if the file is a directory.
 */
gboolean fm_file_info_is_dir(FmFileInfo* fi)
{
    return (S_ISDIR(fi->mode) ||
            (fi->mime_type == _fm_mime_type_get_inode_directory()));
}

/**
 * fm_file_info_get_is_symlink:
 * @fi:  A FmFileInfo struct
 *
 * Check if the file is a symlink. Note that for symlinks,
 * all infos stored in FmFileInfo are actually the info of
 * their targets.
 * The only two places you can tell that is a symlink are:
 * 1. fm_file_info_get_is_symlink()
 * 2. fm_file_info_get_target() which returns the target
 * of the symlink.
 * 
 * Returns: TRUE if the file is a symlink
 */
gboolean fm_file_info_is_symlink(FmFileInfo* fi)
{
    return S_ISLNK(fi->mode) ? TRUE : FALSE;
}

/**
 * fm_file_info_get_is_shortcut:
 * @fi:  A FmFileInfo struct
 *
 * Returns: TRUE if the file is a shortcut.
 * For a shortcut, read the value of fm_file_info_get_target()
 * to get the destination the shortut points to.
 * An example of shortcut type FmFileInfo is file info of
 * files in menu://applications/
 */
gboolean fm_file_info_is_shortcut(FmFileInfo* fi)
{
    return fi->shortcut;
}

/**
 * fm_file_info_is_mountable
 * @fi: file info to inspect
 *
 * Checks if @fi is "inode/mount-point" type.
 *
 * Returns: %TRUE if @fi is mountable type.
 */
gboolean fm_file_info_is_mountable(FmFileInfo* fi)
{
    return fi->mime_type == _fm_mime_type_get_inode_mount_point();
}

/**
 * fm_file_info_get_is_image:
 * @fi:  A FmFileInfo struct
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: TRUE if the file is a image file (*.jpg, *.png, ...).
 */
gboolean fm_file_info_is_image(FmFileInfo* fi)
{
    /* FIXME: We had better use functions of xdg_mime to check this */
    if (!strncmp("image/", fm_mime_type_get_type(fi->mime_type), 6))
        return TRUE;
    return FALSE;
}

/**
 * fm_file_info_get_is_text:
 * @fi:  A FmFileInfo struct
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: TRUE if the file is a plain text file.
 */
gboolean fm_file_info_is_text(FmFileInfo* fi)
{
    if(g_content_type_is_a(fm_mime_type_get_type(fi->mime_type), "text/plain"))
        return TRUE;
    return FALSE;
}

/**
 * fm_file_info_get_is_desktop_entry:
 * @fi:  A FmFileInfo struct
 *
 * Returns: TRUE if the file is a desktop entry file.
 */
gboolean fm_file_info_is_desktop_entry(FmFileInfo* fi)
{
    return fi->mime_type == _fm_mime_type_get_application_x_desktop();
}

/**
 * fm_file_info_get_is_unknown_type:
 * @fi:  A FmFileInfo struct
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: TRUE if the mime type of the file cannot be
 * recognized.
 */
gboolean fm_file_info_is_unknown_type(FmFileInfo* fi)
{
    return g_content_type_is_unknown(fm_mime_type_get_type(fi->mime_type));
}

/**
 * fm_file_info_get_is_executable_type:
 * @fi:  A FmFileInfo struct
 *
 * Note that the function only check if the file seems
 * to be an executable file. It does not check if the
 * user really has the permission to execute the file or
 * if the executable bit of the file is set.
 * To check if a file is really executable by the current
 * user, you may need to call POSIX access() or euidaccess().
 * 
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: TRUE if the file is a kind of executable file,
 * such as shell script, python script, perl script, or 
 * binary executable file.
 */
/* full path of the file is required by this function */
gboolean fm_file_info_is_executable_type(FmFileInfo* fi)
{
    const char* type = fm_mime_type_get_type(fi->mime_type);
    if(strncmp(type, "text/", 5) == 0)
    { /* g_content_type_can_be_executable reports text files as executables too */
        /* We don't execute remote files nor files in trash */
        if(fm_path_is_native(fi->path) && (fi->mode & (S_IXOTH|S_IXGRP|S_IXUSR)))
        { /* it has executable bits so lets check shell-bang */
            char *path = fm_path_to_str(fi->path);
            int fd = open(path, O_RDONLY);
            g_free(path);
            if(fd >= 0)
            {
                char buf[2];
                ssize_t rdlen = read(fd, &buf, 2);
                close(fd);
                if(rdlen == 2 && buf[0] == '#' && buf[1] == '!')
                    return TRUE;
            }
        }
        return FALSE;
    }
    else if(strcmp(type, "application/x-desktop") == 0)
    { /* treat desktop entries as executables if
         they are native and have read permission */
        if(fm_path_is_native(fi->path) && (fi->mode & (S_IRUSR|S_IRGRP|S_IROTH)))
        {
            if (fi->shortcut && fi->target) {
                /* handle shortcuts from desktop to menu entries:
                   first check for entries in /usr/share/applications and such
                   which may be considered as a safe desktop entry path
                   then check if that is a shortcut to a native file
                   otherwise it is a link to a file under menu:// */
                if (!g_str_has_prefix(fi->target, "/usr/share/"))
                {
                    FmPath *target = fm_path_new_for_str(fi->target);
                    gboolean is_native = fm_path_is_native(target);
                    fm_path_unref(target);
                    if (is_native)
                        return TRUE;
                }
            }
            else
                return TRUE;
        }
        return FALSE;
    }
    return g_content_type_can_be_executable(fm_mime_type_get_type(fi->mime_type));
}

/**
 * fm_file_info_is_accessible
 * @fi: a file info descriptor
 *
 * Checks if the user has read access to file or directory @fi.
 *
 * Returns: %TRUE if @fi is accessible for user.
 *
 * Since: 1.0.1
 */
gboolean fm_file_info_is_accessible(FmFileInfo* fi)
{
    return fi->accessible;
}

/**
 * fm_file_info_get_is_hidden:
 * @fi:  A FmFileInfo struct
 *
 * Files treated as hidden files are filenames with dot prefix
 * or ~ suffix.
 * 
 * Returns: TRUE if the file is a hidden file.
 */
gboolean fm_file_info_is_hidden(FmFileInfo* fi)
{
    return (fi->hidden ||
            /* bug #3416724: backup and hidden files should be distinguishable */
            (fm_config->backup_as_hidden && fi->backup));
}

/**
 * fm_file_info_is_backup
 * @fi:  A FmFileInfo struct
 *
 * Checks if file is backup. Native files are considered backup if they
 * have ~ suffix.
 *
 * Returns: %TRUE if the file is a backup file.
 *
 * Since: 1.2.0
 */
gboolean fm_file_info_is_backup(FmFileInfo* fi)
{
    return fi->backup;
}

/**
 * fm_file_info_get_can_thumbnail:
 * @fi:  A FmFileInfo struct
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: TRUE if the the file manager can try to 
 * generate a thumbnail for the file.
 */
gboolean fm_file_info_can_thumbnail(FmFileInfo* fi)
{
    /* We cannot use S_ISREG here as this exclude all symlinks */
    if( fi->size == 0 || /* don't generate thumbnails for empty files */
        !(fi->mode & S_IFREG) ||
        fm_file_info_is_desktop_entry(fi) ||
        fm_file_info_is_unknown_type(fi))
        return FALSE;
    return TRUE;
}


/**
 * fm_file_info_get_collate_key:
 * @fi:  A FmFileInfo struct
 *
 * Get the collate key used for locale-dependent
 * filename sorting. The keys of different files 
 * can be compared with strcmp() directly.
 * 
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: a const string owned by FmFileInfo which should
 * not be freed.
 */
const char* fm_file_info_get_collate_key(FmFileInfo* fi)
{
    /* create a collate key on demand, if we don't have one */
    if(G_UNLIKELY(!fi->collate_key))
    {
        const char* disp_name = fm_file_info_get_disp_name(fi);
        char* casefold = g_utf8_casefold(disp_name, -1);
        char* collate = g_utf8_collate_key_for_filename(casefold, -1);
        g_free(casefold);
        if(strcmp(collate, disp_name))
            fi->collate_key = collate;
        else
        {
            /* if the collate key is the same as the display name,
             * then there is no need to save it.
             * Just use the display name directly. */
            fi->collate_key = COLLATE_USING_DISPLAY_NAME;
            g_free(collate);
        }
    }

    /* if the collate key is the same as the display name, 
     * just return the display name instead. */
    if(fi->collate_key == COLLATE_USING_DISPLAY_NAME)
        return fm_file_info_get_disp_name(fi);

    return fi->collate_key;
}

/**
 * fm_file_info_get_collate_key_nocasefold
 * @fi: a #FmFileInfo struct
 *
 * Get the collate key used for locale-dependent filename sorting but
 * in case-sensitive manner. The keys of different files can be compared
 * with strcmp() directly. Returned data are owned by FmFileInfo and
 * should be not freed by caller.
 *
 * This API is not thread-safe and should be used only in default context.
 *
 * See also: fm_file_info_get_collate_key().
 *
 * Returns: collate string.
 *
 * Since: 1.0.2
 */
const char* fm_file_info_get_collate_key_nocasefold(FmFileInfo* fi)
{
    /* create a collate key on demand, if we don't have one */
    if(G_UNLIKELY(!fi->collate_key_case))
    {
        const char* disp_name = fm_file_info_get_disp_name(fi);
        char* collate = g_utf8_collate_key_for_filename(disp_name, -1);
        if(strcmp(collate, disp_name))
            fi->collate_key_case = collate;
        else
        {
            /* if the collate key is the same as the display name,
             * then there is no need to save it.
             * Just use the display name directly. */
            fi->collate_key_case = COLLATE_USING_DISPLAY_NAME;
            g_free(collate);
        }
    }

    /* if the collate key is the same as the display name, 
     * just return the display name instead. */
    if(fi->collate_key_case == COLLATE_USING_DISPLAY_NAME)
        return fm_file_info_get_disp_name(fi);

    return fi->collate_key_case;
}

/**
 * fm_file_info_get_target:
 * @fi:  A FmFileInfo struct
 *
 * Get the target of a symlink or a shortcut.
 * 
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: a const string owned by FmFileInfo which should
 * not be freed. NULL if the file is not a symlink or
 * shortcut.
 */
const char* fm_file_info_get_target(FmFileInfo* fi)
{
    return fi->target;
}

/**
 * fm_file_info_get_desc:
 * @fi:  A FmFileInfo struct
 * 
 * Get a human-readable description for the file.
 * 
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: a const string owned by FmFileInfo which should
 * not be freed.
 */
const char* fm_file_info_get_desc(FmFileInfo* fi)
{
    /* FIXME: how to handle descriptions for virtual files without mime-tyoes? */
    return fi->mime_type ? fm_mime_type_get_desc(fi->mime_type) : NULL;
}

/**
 * fm_file_info_get_disp_mtime:
 * @fi:  A FmFileInfo struct
 * 
 * Get a human-readable string for showing file modification
 * time in the UI.
 * 
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: a const string owned by FmFileInfo which should
 * not be freed.
 */
const char* fm_file_info_get_disp_mtime(FmFileInfo* fi)
{
    /* FIXME: This can cause problems if the file really has mtime=0. */
    /*        We'd better hide mtime for virtual files only. */
    if(fi->mtime > 0)
    {
        if (!fi->disp_mtime)
        {
            char buf[ 128 ];
            strftime(buf, sizeof(buf),
                      "%x %R",
                      localtime(&fi->mtime));
            fi->disp_mtime = g_strdup(buf);
        }
    }
    return fi->disp_mtime;
}

/**
 * fm_file_info_get_mtime:
 * @fi:  A FmFileInfo struct
 * 
 * Returns: file modification time.
 */
time_t fm_file_info_get_mtime(FmFileInfo* fi)
{
    return fi->mtime;
}

/**
 * fm_file_info_get_atime
 * @fi:  A FmFileInfo struct
 * 
 * Returns: file access time.
 */
time_t fm_file_info_get_atime(FmFileInfo* fi)
{
    return fi->atime;
}

/**
 * fm_file_info_get_ctime
 * @fi: a file info to inspect
 *
 * Retrieves time when access right were changed last time for file @fi.
 *
 * Returns: file access change time.
 *
 * Since: 1.2.0
 */
time_t fm_file_info_get_ctime(FmFileInfo *fi)
{
    return fi->ctime;
}

/**
 * fm_file_info_get_uid:
 * @fi:  A FmFileInfo struct
 * 
 * Returns: user id (uid) of the file owner.
 */
uid_t fm_file_info_get_uid(FmFileInfo* fi)
{
    return fi->uid;
}

/**
 * fm_file_info_get_gid:
 * @fi:  A FmFileInfo struct
 * 
 * Returns: group id (gid) of the file owner.
 */
gid_t fm_file_info_get_gid(FmFileInfo* fi)
{
    return fi->gid;
}


/**
 * fm_file_info_get_fs_id:
 * @fi:  A FmFileInfo struct
 * 
 * Get the filesystem id string
 * This is only applicable when the file is on a remote
 * filesystem. e.g. fm_file_info_is_native() returns FALSE.
 * 
 * This API is not thread-safe and should be used only in default context.
 *
 * Returns: a const string owned by FmFileInfo which should
 * not be freed.
 */
const char* fm_file_info_get_fs_id(FmFileInfo* fi)
{
    return fi->fs_id;
}

/**
 * fm_file_info_get_dev:
 * @fi:  A FmFileInfo struct
 * 
 * Get the filesystem device id (POSIX dev_t)
 * This is only applicable when the file is native.
 * e.g. fm_file_info_is_native() returns TRUE.
 * 
 * Returns: device id (POSIX dev_t, st_dev member of 
 * struct stat).
 */
dev_t fm_file_info_get_dev(FmFileInfo* fi)
{
    return fi->dev;
}

/**
 * fm_file_info_can_set_name
 * @fi: a #FmFileInfo to inspect
 *
 * Checks if file system supports name change for @fi. Returned value
 * %TRUE is just a potential possibility, name still may be unable to
 * change due to access reasons for example.
 *
 * Returns: %TRUE if change is supported for @fi.
 *
 * Since: 1.2.0
 */
gboolean fm_file_info_can_set_name(FmFileInfo *fi)
{
    return (fi != NULL && fi->name_is_changeable);
}

/**
 * fm_file_info_can_set_icon
 * @fi: a #FmFileInfo to inspect
 *
 * Checks if file system supports icon change for @fi. Returned value
 * %TRUE is just a potential possibility, icon still may be unable to
 * change due to access reasons for example.
 *
 * Returns: %TRUE if change is supported for @fi.
 *
 * Since: 1.2.0
 */
gboolean fm_file_info_can_set_icon(FmFileInfo *fi)
{
    return (fi != NULL && fi->icon_is_changeable);
}

/**
 * fm_file_info_can_set_hidden
 * @fi: a #FmFileInfo to inspect
 *
 * Checks if file system supports "hidden" attribute change for @fi.
 * Returned value %TRUE is just a potential possibility, the attribute
 * still may be unable to change due to access reasons for example.
 *
 * Returns: %TRUE if change is supported for @fi.
 *
 * Since: 1.2.0
 */
gboolean fm_file_info_can_set_hidden(FmFileInfo *fi)
{
    return (fi != NULL && fi->hidden_is_changeable);
}

/**
 * fm_file_info_is_writable_directory
 * @fi: a #FmFileInfo to inspect
 *
 * Checks if directory @fi lies on writable file system. Returned value
 * %TRUE is just a potential possibility, it may still not allow write
 * due to access reasons for example.
 *
 * Returns: %TRUE if @fi may be writable.
 *
 * Since: 1.2.0
 */
gboolean fm_file_info_is_writable_directory(FmFileInfo* fi)
{
    return (!fi->fs_is_ro && fm_file_info_is_dir(fi));
}

/**
 * fm_file_info_get_disp_owner
 * @fi: file info to inspect
 *
 * Retrieves human-readable string value for owner of @fi. Returned value
 * is either owner login name or numeric string if owner has no entry in
 * /etc/passwd file. Returned value is owned by @fi and should be not
 * altered by caller.
 *
 * Returns: (transfer none): string value for owner.
 *
 * Since: 1.2.0
 */
const char *fm_file_info_get_disp_owner(FmFileInfo *fi)
{
    g_return_val_if_fail(fi, NULL);
    if (!fi->disp_owner)
    {
        struct passwd* pw = NULL;
        struct passwd pwb;
        char unamebuf[1024];

        getpwuid_r(fi->uid, &pwb, unamebuf, sizeof(unamebuf), &pw);
        if (pw)
            fi->disp_owner = g_strdup(pw->pw_name);
        else
            fi->disp_owner = g_strdup_printf("%u", (guint)fi->uid);
    }
    return fi->disp_owner;
}

/**
 * fm_file_info_get_disp_group
 * @fi: file info to inspect
 *
 * Retrieves human-readable string value for group of @fi. Returned value
 * is either group name or numeric string if grop has no entry in the
 * /etc/group file. Returned value is owned by @fi and should be not
 * altered by caller.
 *
 * Returns: (transfer none): string value for file group.
 *
 * Since: 1.2.0
 */
const char *fm_file_info_get_disp_group(FmFileInfo *fi)
{
    g_return_val_if_fail(fi, NULL);
    if (!fi->disp_group)
    {
        struct group* grp = NULL;
        struct group grpb;
        char unamebuf[1024];

        getgrgid_r(fi->gid, &grpb, unamebuf, sizeof(unamebuf), &grp);
        if (grp)
            fi->disp_group = g_strdup(grp->gr_name);
        else
            fi->disp_group = g_strdup_printf("%u", (guint)fi->gid);
    }
    return fi->disp_group;
}


static FmListFuncs fm_list_funcs =
{
    .item_ref = (gpointer (*)(gpointer))&fm_file_info_ref,
    .item_unref = (void (*)(gpointer))&fm_file_info_unref
};

/**
 * fm_file_info_list_new
 *
 * Creates a new #FmFileInfoList.
 *
 * Returns: new #FmFileInfoList object.
 */
FmFileInfoList* fm_file_info_list_new(void)
{
    return (FmFileInfoList*)fm_list_new(&fm_list_funcs);
}

/**
 * fm_file_info_list_is_same_type
 * @list: a #FmFileInfoList
 *
 * Checks if all files in the list are of the same type.
 *
 * Returns: %TRUE if all files in the list are of the same type
 */
gboolean fm_file_info_list_is_same_type(FmFileInfoList* list)
{
    /* FIXME: handle virtual files without mime-types */
    if(!fm_list_is_empty((FmList*)list))
    {
        GList* l = fm_list_peek_head_link((FmList*)list);
        FmFileInfo* fi = (FmFileInfo*)l->data;
        l = l->next;
        for(;l;l=l->next)
        {
            FmFileInfo* fi2 = (FmFileInfo*)l->data;
            if(fi->mime_type != fi2->mime_type)
                return FALSE;
        }
    }
    return TRUE;
}

/**
 * fm_file_info_list_is_same_fs
 * @list: a #FmFileInfoList
 *
 * Checks if all files in the list are on the same file system.
 *
 * Returns: %TRUE if all files in the list are on the same fs.
 */
gboolean fm_file_info_list_is_same_fs(FmFileInfoList* list)
{
    if(!fm_list_is_empty((FmList*)list))
    {
        GList* l = fm_list_peek_head_link((FmList*)list);
        FmFileInfo* fi = (FmFileInfo*)l->data;
        l = l->next;
        for(;l;l=l->next)
        {
            FmFileInfo* fi2 = (FmFileInfo*)l->data;
            gboolean is_native = fm_path_is_native(fi->path);
            if(is_native != fm_path_is_native(fi2->path))
                return FALSE;
            if(is_native)
            {
                if(fi->dev != fi2->dev)
                    return FALSE;
            }
            else
            {
                if(fi->fs_id != fi2->fs_id)
                    return FALSE;
            }
        }
    }
    return TRUE;
}
