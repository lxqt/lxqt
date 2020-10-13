/*
 *      fm-templates.c
 *
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
 * SECTION:fm-templates
 * @short_description: Templates for new files creation.
 * @title: FmTemplate
 *
 * @include: libfm/fm.h
 *
 * The #FmTemplate object represents description which files was set for
 * creation and how those files should be created - that includes custom
 * prompt, file name template, and template contents.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "fm-templates.h"
#include "fm-monitor.h"
#include "fm-dir-list-job.h"
#include "fm-config.h"
#include "fm-folder.h"

typedef struct _FmTemplateFile  FmTemplateFile;
typedef struct _FmTemplateDir   FmTemplateDir;

struct _FmTemplate
{
    GObject parent;
    FmTemplateFile *files; /* not referenced, it references instead */
    FmMimeType *mime_type;
    FmPath *template_file;
    FmIcon *icon;
    gchar *command;
    gchar *prompt;
    gchar *label;
};

struct _FmTemplateClass
{
    GObjectClass parent;
};


static void fm_template_finalize(GObject *object);

G_DEFINE_TYPE(FmTemplate, fm_template, G_TYPE_OBJECT);

static void fm_template_class_init(FmTemplateClass *klass)
{
    GObjectClass *g_object_class;

    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->finalize = fm_template_finalize;
}

static GList *templates = NULL; /* in appearance reversed order */
G_LOCK_DEFINE_STATIC(templates);

static void fm_template_finalize(GObject *object)
{
    FmTemplate *self;

    g_return_if_fail(FM_IS_TEMPLATE(object));
    self = (FmTemplate*)object;
    if(self->files)
        g_error("template reference failure");
    fm_mime_type_unref(self->mime_type);
    if(self->template_file)
        fm_path_unref(self->template_file);
    if(self->icon)
        g_object_unref(self->icon);
    g_free(self->command);
    g_free(self->prompt);
    g_free(self->label);

    G_OBJECT_CLASS(fm_template_parent_class)->finalize(object);
}

static void fm_template_init(FmTemplate *self)
{
}

static FmTemplate* fm_template_new(void)
{
    return (FmTemplate*)g_object_new(FM_TEMPLATE_TYPE, NULL);
}


struct _FmTemplateFile
{
    /* using 'built-in' GList/GSList for speed and less memory consuming */
    FmTemplateFile *next_in_dir;
    FmTemplateFile *prev_in_dir;
    FmTemplateDir *dir;
    FmTemplateFile *next_in_templ; /* in priority-less order */
    /* referenced */
    FmTemplate *templ;
    FmPath *path;
    gboolean is_desktop_entry : 1;
    gboolean inactive : 1;
};

struct _FmTemplateDir
{
    /* using 'built-in' GSList for speed and less memory consuming */
    FmTemplateDir *next;
    FmTemplateFile *files;
    /* referenced */
    FmPath *path;
    GFileMonitor *monitor;
    gboolean user_dir : 1;
};

/* allocated once, on init */
static FmTemplateDir *templates_dirs = NULL;


/* determine mime type for the template
   using just fm_mime_type_* for this isn't appropriate because
   we need completely another guessing for templates content */
static FmMimeType *_fm_template_guess_mime_type(FmPath *path, FmMimeType *mime_type,
                                                FmPath **tpath)
{
    const gchar *basename = fm_path_get_basename(path);
    FmPath *subpath = NULL;
    gchar *filename, *type, *url;
    GKeyFile *kf;

    /* SF bug #902: if file was deleted instantly we get NULL here */
    if (mime_type == NULL)
        return NULL;

    /* if file is desktop entry then find the real template file path */
    if(mime_type != _fm_mime_type_get_application_x_desktop())
    {
        if(tpath)
            *tpath = fm_path_ref(path);
        return fm_mime_type_ref(mime_type);
    }
    /* parse file to find mime type */
    kf = g_key_file_new();
    filename = fm_path_to_str(path);
    type = NULL;
    if(g_key_file_load_from_file(kf, filename, G_KEY_FILE_NONE, NULL))
    {
        type = g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                     G_KEY_FILE_DESKTOP_KEY_TYPE, NULL);
        if(type && strcmp(type, G_KEY_FILE_DESKTOP_TYPE_APPLICATION) == 0)
        {
            /* Type=Application, assume it's just file */
            g_key_file_free(kf);
            g_free(filename);
            g_free(type);
            if(tpath)
                *tpath = fm_path_ref(path);
            return fm_mime_type_ref(_fm_mime_type_get_application_x_desktop());
        }
        if(!type || strcmp(type, G_KEY_FILE_DESKTOP_TYPE_LINK) != 0)
        {
            /* desktop entry file invalid as template */
            g_key_file_free(kf);
            g_free(filename);
            g_free(type);
            return NULL;
        }
        g_free(type);
        /* valid template should have 'URL' key */
        url = g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                    G_KEY_FILE_DESKTOP_KEY_URL, NULL);
        if(url)
        {
            if(G_UNLIKELY(url[0] == '/')) /* absolute path */
                subpath = fm_path_new_for_path(url);
            else if(G_UNLIKELY(strchr(url, ':'))) /* URI (huh?) */
                subpath = fm_path_new_for_uri(url);
            else /* path relative to directory containing file */
                subpath = fm_path_new_relative(fm_path_get_parent(path), url);
            path = subpath; /* shift to new path */
            basename = fm_path_get_basename(path);
            g_free(url);
        }
        else
        {
            g_key_file_free(kf);
            g_free(filename);
            return NULL; /* invalid template file */
        }
        /* some templates may have 'MimeType' key */
        type = g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                     G_KEY_FILE_DESKTOP_KEY_MIME_TYPE, NULL);
        if(type)
        {
            if(tpath)
                *tpath = subpath;
            else
                fm_path_unref(subpath);
            g_key_file_free(kf);
            g_free(filename);
            mime_type = fm_mime_type_from_name(type);
            g_free(type);
            return mime_type;
        }
    }
    /* so we have real template file now, guess from file content first */
    mime_type = NULL;
    g_free(filename);
    filename = fm_path_to_str(path);
    /* type is NULL still */
    mime_type = fm_mime_type_from_native_file(filename, basename, NULL);
    if(mime_type == _fm_mime_type_get_application_x_desktop())
    {
        /* template file is an entry */
        kf = g_key_file_new();
        if(g_key_file_load_from_file(kf, filename, G_KEY_FILE_NONE, NULL))
        {
            type = g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                         G_KEY_FILE_DESKTOP_KEY_TYPE, NULL);
            if(type)
            {
                /* TODO: we support only 'Application' type for now */
                if(strcmp(type, G_KEY_FILE_DESKTOP_TYPE_APPLICATION) == 0)
                    ;
                else
                {
                    /* desktop entry file invalid as template */
                    g_key_file_free(kf);
                    g_free(filename);
                    if(subpath)
                        fm_path_unref(subpath);
                    g_free(type);
                    fm_mime_type_unref(mime_type);
                    return NULL;
                }
                g_free(type);
            }
        }
        g_key_file_free(kf);
    }
    /* we got a desktop entry link to not existing file, guess from file name */
    if(!mime_type)
    {
        gboolean uncertain;
        type = g_content_type_guess(basename, NULL, 0, &uncertain);
        if(type && !uncertain)
            mime_type = fm_mime_type_from_name(type);
        g_free(type);
        /* ignore unrecognizable files instead of using application/octet-stream */
    }
    g_free(filename);
    if(tpath)
        *tpath = subpath;
    else if(subpath)
        fm_path_unref(subpath);
    return mime_type;
}

/* find or create new FmTemplate */
/* requires lock held */
static FmTemplate *_fm_template_find_for_file(FmPath *path, FmMimeType *mime_type)
{
    GList *l;
    FmTemplate *templ;
    FmPath *tpath = NULL;
    gboolean template_type_once = fm_config->template_type_once;

    if(template_type_once)
        mime_type = _fm_template_guess_mime_type(path, mime_type, NULL);
    else
        mime_type = _fm_template_guess_mime_type(path, mime_type, &tpath);
    if(!mime_type)
    {
        /* g_debug("could not guess MIME type for template %s, ignoring it",
                fm_path_get_basename(path)); */
        if(tpath)
            fm_path_unref(tpath);
        return NULL;
    }
    if(!template_type_once && !tpath)
    {
        /* g_debug("no basename defined in template %s, ignoring it",
                fm_path_get_basename(path)); */
        fm_mime_type_unref(mime_type);
        return NULL;
    }
    for(l = templates; l; l = l->next)
    {
        templ = l->data;
        if(!template_type_once)
        {
            if(strcmp(fm_path_get_basename(templ->template_file),
                      fm_path_get_basename(tpath)) == 0)
            {
                g_object_ref(templ);
                fm_mime_type_unref(mime_type);
                fm_path_unref(tpath);
                return templ;
            }
        }
        else if(templ->mime_type == mime_type)
        {
            g_object_ref(templ);
            fm_mime_type_unref(mime_type);
            return templ;
        }
    }
    templ = fm_template_new();
    templ->mime_type = mime_type;
    templ->template_file = tpath;
    templates = g_list_prepend(templates, g_object_ref(templ));
    return templ;
}

/* requires lock held */
static void _fm_template_update_from_file(FmTemplate *templ, FmTemplateFile *file)
{
    if(file == NULL)
        return;
    /* update from less relevant file first */
    _fm_template_update_from_file(templ, file->next_in_templ);
    if(file->is_desktop_entry) /* desktop entry */
    {
        GKeyFile *kf = g_key_file_new();
        char *filename = fm_path_to_str(file->path);
        char *tmp;
        GError *error = NULL;
        gboolean hidden;

        if(g_key_file_load_from_file(kf, filename, G_KEY_FILE_NONE, &error))
        {
            hidden = g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                            G_KEY_FILE_DESKTOP_KEY_HIDDEN, NULL);
            file->inactive = hidden;
            /* g_debug("%s hidden: %d", filename, hidden); */
            /* FIXME: test for G_KEY_FILE_DESKTOP_KEY_ONLY_SHOW_IN ? */
            if(!hidden)
            {
                tmp = g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                            G_KEY_FILE_DESKTOP_KEY_TYPE, NULL);
                if(!tmp || strcmp(tmp, G_KEY_FILE_DESKTOP_TYPE_LINK) != 0)
                {
                    /* it seems it's just Application template */
                    g_key_file_free(kf);
                    g_free(filename);
                    g_free(tmp);
                    if(!templ->template_file)
                        templ->template_file = fm_path_ref(file->path);
                    return;
                }
                g_free(tmp);
                tmp = g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                            G_KEY_FILE_DESKTOP_KEY_URL, NULL);
                if(tmp)
                {
                    if(templ->template_file)
                        fm_path_unref(templ->template_file);
                    if(G_UNLIKELY(tmp[0] == '/')) /* absolute path */
                        templ->template_file = fm_path_new_for_path(tmp);
                    else if(G_UNLIKELY(strchr(tmp, ':'))) /* URI (huh?) */
                        templ->template_file = fm_path_new_for_uri(tmp);
                    else /* path relative to directory containing file */
                        templ->template_file = fm_path_new_relative(file->dir->path, tmp);
                    g_free(tmp);
                }
                tmp = g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                            G_KEY_FILE_DESKTOP_KEY_ICON, NULL);
                if(tmp)
                {
                    if(templ->icon)
                        g_object_unref(templ->icon);
                    templ->icon = fm_icon_from_name(tmp);
                    g_free(tmp);
                }
                tmp = g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                            G_KEY_FILE_DESKTOP_KEY_EXEC, NULL);
                if(tmp)
                {
                    g_free(templ->command);
                    templ->command = tmp;
                }
                tmp = g_key_file_get_locale_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                            G_KEY_FILE_DESKTOP_KEY_NAME, NULL, NULL);
                if(tmp)
                {
                    g_free(templ->label);
                    templ->label = tmp;
                }
                tmp = g_key_file_get_locale_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                            G_KEY_FILE_DESKTOP_KEY_COMMENT, NULL, NULL);
                if(tmp)
                {
                    g_free(templ->prompt);
                    templ->prompt = tmp;
                }
                /* FIXME: forge prompt from 'Name' if not set yet? */
            }
        }
        else
        {
            g_warning("problem loading template %s: %s", filename, error->message);
            g_error_free(error);
            file->inactive = TRUE;
        }
        g_key_file_free(kf);
        g_free(filename);
    }
    else /* plain file */
    {
        if(!templ->template_file)
            templ->template_file = fm_path_ref(file->path);
        file->inactive = FALSE;
    }
}

/* recreate data in FmTemplate from entries */
/* requires lock held */
static FmTemplate *_fm_template_update(FmTemplate *templ)
{
    GList *l;
    FmTemplate *new_templ;
    FmTemplateFile *file;

    if(templ->files == NULL) /* template is empty now */
    {
        templates = g_list_remove(templates, templ);
        g_object_unref(templ); /* we removed it from list so drop reference */
        return NULL;
    }
    l = g_list_find(templates, templ);
    if(l == NULL)
        g_error("FmTemplate not found in list");
    /* isolate and unref old template */
    l->data = new_templ = fm_template_new(); /* reference is bound to list */
    new_templ->mime_type = fm_mime_type_ref(templ->mime_type);
    new_templ->files = templ->files;
    templ->files = NULL;
    for(file = new_templ->files; file; file = file->next_in_templ)
    {
        file->templ = g_object_ref(new_templ);
        g_object_unref(templ);
    }
    /* update from file list */
    _fm_template_update_from_file(new_templ, new_templ->files);
    /* set template if it's not set, sorting by name requires it */
    if(!new_templ->template_file && templ->template_file && !fm_config->template_type_once)
        new_templ->template_file = fm_path_ref(templ->template_file);
    g_object_unref(templ); /* we removed it from list so drop reference */
    return new_templ;
}

/* add file into FmTemplate */
/* requires lock held */
static void _fm_template_insert_sorted(FmTemplate *templ, FmTemplateFile *file)
{
    FmTemplateDir *last_dir = templates_dirs;
    FmTemplateFile *last = NULL, *next;

    for(next = templ->files; next; next = next->next_in_templ)
    {
        while(last_dir && last_dir != file->dir && last_dir != next->dir)
            last_dir = last_dir->next;
        g_assert(last_dir != NULL); /* it must be corruption otherwise */
        if(last_dir == file->dir)
        {
            if(next->dir == last_dir && !file->is_desktop_entry)
            {
                if(!next->is_desktop_entry)
                    break;
                /* sort files after desktop items */
            }
            else
                break;
        }
        last = next;
    }
    file->next_in_templ = next;
    if(last)
        last->next_in_templ = file;
    else
        templ->files = file;
}

/* delete file from FmTemplate and free it */
/* requires lock held */
static void _fm_template_file_free(FmTemplate *templ, FmTemplateFile *file,
                                   gboolean do_update)
{
    FmTemplateFile *file2 = templ->files;

    if(file2 == file)
        templ->files = file->next_in_templ;
    else while(file2)
    {
        if(file2->next_in_templ == file)
        {
            file2->next_in_templ = file->next_in_templ;
            break;
        }
        file2 = file2->next_in_templ;
    }
    if(!file2)
        g_critical("FmTemplate: file being freed is missed in template");
    g_object_unref(templ);
    fm_path_unref(file->path);
    g_slice_free(FmTemplateFile, file);
    if(do_update)
        _fm_template_update(templ);
}

static void on_job_finished(FmJob *job, FmTemplateDir *dir)
{
    GList *file_infos, *l;
    FmFileInfo *fi;
    FmPath *path;
    FmTemplateFile *file;
    FmTemplate *templ;

    g_signal_handlers_disconnect_by_func(job, on_job_finished, dir);
    file_infos = fm_file_info_list_peek_head_link(fm_dir_list_job_get_files(FM_DIR_LIST_JOB(job)));
    for(l = file_infos; l; l = l->next)
    {
        fi = l->data;
        if(fm_file_info_is_hidden(fi) || fm_file_info_is_backup(fi))
            continue;
        path = fm_file_info_get_path(fi);
        G_LOCK(templates);
        for(file = dir->files; file; file = file->next_in_dir)
            if(fm_path_equal(path, file->path))
                break;
        G_UNLOCK(templates);
        if(file) /* it's duplicate */
            continue;
        /* ensure the path is based on dir->path */
        path = fm_path_new_child(dir->path, fm_path_get_basename(path));
        G_LOCK(templates);
        templ = _fm_template_find_for_file(path, fm_file_info_get_mime_type(fi));
        G_UNLOCK(templates);
        if(!templ) /* mime type guessing error */
        {
            fm_path_unref(path);
            continue;
        }
        file = g_slice_new(FmTemplateFile);
        file->templ = templ;
        file->path = path;
        file->is_desktop_entry = fm_file_info_is_desktop_entry(fi);
        file->dir = dir;
        G_LOCK(templates);
        file->next_in_dir = dir->files;
        file->prev_in_dir = NULL;
        if(dir->files)
            dir->files->prev_in_dir = file;
        dir->files = file;
        _fm_template_insert_sorted(templ, file);
        _fm_template_update(templ);
        G_UNLOCK(templates);
    }
}

static void on_dir_changed(GFileMonitor *mon, GFile *gf, GFile *other,
                           GFileMonitorEvent evt, FmTemplateDir *dir)
{
    GFile *gfile = fm_path_to_gfile(dir->path);
    char *basename, *pathname;
    FmTemplateFile *file;
    FmPath *path;
    FmTemplate *templ;
    FmMimeType *mime_type;

    if(g_file_equal(gf, gfile))
    {
        /* it's event on folder itself, ignoring */
        g_object_unref(gfile);
        return;
    }
    g_object_unref(gfile);
    switch(evt)
    {
    case G_FILE_MONITOR_EVENT_CHANGED:
        basename = g_file_get_basename(gf);
        if (basename == NULL)
            break;
        G_LOCK(templates);
        for(file = dir->files; file; file = file->next_in_dir)
            if(strcmp(fm_path_get_basename(file->path), basename) == 0)
                break;
        g_free(basename);
        if(file)
        {
            if(file->is_desktop_entry) /* we aware only of entries content */
                _fm_template_update(file->templ);
        }
        else
            g_warning("templates monitor: change for unknown file");
        G_UNLOCK(templates);
        break;
    case G_FILE_MONITOR_EVENT_DELETED:
        basename = g_file_get_basename(gf);
        if (basename == NULL)
            break;
        G_LOCK(templates);
        for(file = dir->files; file; file = file->next_in_dir)
            if(strcmp(fm_path_get_basename(file->path), basename) == 0)
                break;
        g_free(basename);
        if(file)
        {
            if(file == dir->files)
                dir->files = file->next_in_dir;
            else if(file->prev_in_dir)
                file->prev_in_dir->next_in_dir = file->next_in_dir;
            if(file->next_in_dir)
                file->next_in_dir->prev_in_dir = file->prev_in_dir;
            _fm_template_file_free(file->templ, file, TRUE);
            G_UNLOCK(templates);
        }
        else
            G_UNLOCK(templates);
            /* else it is already deleted */
        break;
    case G_FILE_MONITOR_EVENT_CREATED:
        basename = g_file_get_basename(gf);
        G_LOCK(templates);
        for(file = dir->files; file; file = file->next_in_dir)
            if(strcmp(fm_path_get_basename(file->path), basename) == 0)
                break;
        G_UNLOCK(templates);
        /* NOTE: to query file info is too heavy so do own assumptions */
        if(!file && basename[0] != '.' && !g_str_has_suffix(basename, "~"))
        {
            path = fm_path_new_child(dir->path, basename);
            pathname = fm_path_to_str(path);
            mime_type = fm_mime_type_from_native_file(pathname, basename, NULL);
            g_free(pathname);
            G_LOCK(templates);
            templ = _fm_template_find_for_file(path, mime_type);
            if(templ)
            {
                file = g_slice_new(FmTemplateFile);
                file->templ = templ;
                file->path = path;
                file->is_desktop_entry = (mime_type == _fm_mime_type_get_application_x_desktop());
                file->dir = dir;
                file->next_in_dir = dir->files;
                file->prev_in_dir = NULL;
                if (dir->files)
                    dir->files->prev_in_dir = file;
                dir->files = file;
                _fm_template_insert_sorted(templ, file);
                _fm_template_update(templ);
                G_UNLOCK(templates);
            }
            else
            {
                G_UNLOCK(templates);
                fm_path_unref(path);
                g_warning("could not guess type of template %s, ignoring it",
                          basename);
            }
            if (G_LIKELY(mime_type))
                fm_mime_type_unref(mime_type);
        }
        else
            g_debug("templates monitor: duplicate file %s", basename);
        g_free(basename);
        break;
#if GLIB_CHECK_VERSION(2, 24, 0)
    case G_FILE_MONITOR_EVENT_MOVED:
#endif
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
    case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
    case G_FILE_MONITOR_EVENT_UNMOUNTED:
        /* ignore those */
        break;
    }
}

static void _template_dir_init(FmTemplateDir *dir, GFile *gf)
{
    FmDirListJob *job = fm_dir_list_job_new2(dir->path, FM_DIR_LIST_JOB_FAST);
    GError *error = NULL;

    dir->files = NULL;
    g_signal_connect(job, "finished", G_CALLBACK(on_job_finished), dir);
    if(!fm_job_run_async(FM_JOB(job)))
        g_signal_handlers_disconnect_by_func(job, on_job_finished, dir);
    dir->monitor = fm_monitor_directory(gf, &error);
    if(dir->monitor)
        g_signal_connect(dir->monitor, "changed", G_CALLBACK(on_dir_changed), dir);
    else
    {
        g_debug("file monitor cannot be created: %s", error->message);
        g_error_free(error);
    }
    g_object_unref(job);
}

static void on_once_type_changed(FmConfig *cfg, gpointer unused)
{
    GList *l, *old_list;
    FmTemplateFile *file;
    FmTemplate *templ;

    G_LOCK(templates);
    /* rebuild templates list from known files */
    old_list = templates;
    templates = NULL;
    for(l = old_list; l; l = l->next)
    {
        templ = l->data;
        while((file = templ->files))
        {
            templ->files = file->next_in_templ;
            file->templ = _fm_template_find_for_file(file->path, templ->mime_type);
            _fm_template_insert_sorted(file->templ, file);
            g_object_unref(templ); /* for a removed file */
        }
        g_object_unref(templ); /* for removing from list */
    }
    g_list_free(old_list);
    /* update all templates now */
    g_list_foreach(templates, (GFunc)_fm_template_update, NULL);
    G_UNLOCK(templates);
}

void _fm_templates_init(void)
{
    const gchar * const *data_dirs = g_get_system_data_dirs();
    const gchar * const *data_dir;
    const gchar *dir_name;
    FmTemplateDir *dir = NULL;
    GFile *parent, *gfile;

    if(templates_dirs)
        return; /* someone called us again? */
    /* prepare list of system template directories */
    for(data_dir = data_dirs; *data_dir; ++data_dir)
    {
        parent = g_file_new_for_path(*data_dir);
        gfile = g_file_get_child(parent, "templates");
        g_object_unref(parent);
        if(g_file_query_exists(gfile, NULL))
        {
            if(G_LIKELY(dir))
            {
                dir->next = g_slice_new(FmTemplateDir);
                dir = dir->next;
            }
            else
                templates_dirs = dir = g_slice_new(FmTemplateDir);
            dir->path = fm_path_new_for_gfile(gfile);
            dir->user_dir = FALSE;
            _template_dir_init(dir, gfile);
        }
        g_object_unref(gfile);
    }
    if(G_LIKELY(dir))
        dir->next = NULL;
    /* add templates dir in user data */
    dir = g_slice_new(FmTemplateDir);
    dir->next = templates_dirs;
    templates_dirs = dir;
    parent = g_file_new_for_path(g_get_user_data_dir());
    gfile = g_file_get_child(parent, "templates");
    g_object_unref(parent);
    dir->path = fm_path_new_for_gfile(gfile);
    dir->user_dir = TRUE;
    /* FIXME: create it if it doesn't exist? */
    if(g_file_query_exists(gfile, NULL))
        _template_dir_init(dir, gfile);
    else
    {
        dir->files = NULL;
        dir->monitor = NULL;
    }
    g_object_unref(gfile);
    /* add XDG_TEMPLATES_DIR at last */
    dir = g_slice_new(FmTemplateDir);
    dir->next = templates_dirs;
    templates_dirs = dir;
    dir_name = g_get_user_special_dir(G_USER_DIRECTORY_TEMPLATES);
    if(dir_name)
        dir->path = fm_path_new_for_path(dir_name);
    else
        dir->path = fm_path_new_child(fm_path_get_home(), "Templates");
    dir->user_dir = TRUE;
    gfile = fm_path_to_gfile(dir->path);
    if(!g_file_query_exists(gfile, NULL))
    {
        g_warning("The directory '%s' doesn't exist, ignoring it",
                  dir_name ? dir_name : "~/Templates");
        goto _skip_templates_dir;
    }
    if (dir->path == fm_path_get_home() || dir->path == fm_path_get_root())
    {
        /* $HOME or / are invalid templates paths so just ignore */
        g_warning("XDG_TEMPLATES_DIR is set to invalid path, ignoring it");
_skip_templates_dir:
        dir->files = NULL;
        dir->monitor = NULL;
    }
    else
        _template_dir_init(dir, gfile);
    g_object_unref(gfile);
    /* jobs will fill list of files async */
    g_signal_connect(fm_config, "changed::template_type_once",
                     G_CALLBACK(on_once_type_changed), NULL);
}

void _fm_templates_finalize(void)
{
    FmTemplateDir *dir;
    FmTemplateFile *file;

    g_signal_handlers_disconnect_by_func(fm_config, on_once_type_changed, NULL);
    while(templates_dirs)
    {
        dir = templates_dirs;
        templates_dirs = dir->next;
        fm_path_unref(dir->path);
        if(dir->monitor)
        {
            g_signal_handlers_disconnect_by_func(dir->monitor, on_dir_changed, dir);
            g_object_unref(dir->monitor);
        }
        while(dir->files)
        {
            file = dir->files;
            dir->files = file->next_in_dir;
            if(dir->files)
                dir->files->prev_in_dir = NULL;
            _fm_template_file_free(file->templ, file, FALSE);
        }
        g_slice_free(FmTemplateDir, dir);
    }
    g_list_foreach(templates, (GFunc)g_object_unref, NULL);
    g_list_free(templates);
    templates = NULL;
}

/**
 * fm_template_list_all
 * @user_only: %TRUE to ignore system templates
 *
 * Retrieves list of all templates. Returned data should be freed after
 * usage with g_list_free_full(list, g_object_unref).
 *
 * Returns: (transfer full) (element-type FmTemplate): list of all known templates.
 *
 * Since: 1.2.0
 */
GList *fm_template_list_all(gboolean user_only)
{
    GList *list = NULL, *l;

    G_LOCK(templates);
    for(l = templates; l; l = l->next)
        if(!((FmTemplate*)l->data)->files->inactive &&
           (!user_only || ((FmTemplate*)l->data)->files->dir->user_dir))
            list = g_list_prepend(list, g_object_ref(l->data));
    G_UNLOCK(templates);
    return list;
}

/**
 * fm_template_get_name
 * @templ: a template descriptor
 * @nlen: (allow-none): location to get template name length
 *
 * Retrieves file name template for @templ. If @nlen isn't %NULL then it
 * will receive length of file name template without suffix (in characters).
 * Returned data are owned by @templ and should be not freed by caller.
 *
 * Returns: (transfer none): file name template.
 *
 * Since: 1.2.0
 */
const gchar *fm_template_get_name(FmTemplate *templ, gint *nlen)
{
    const gchar *name;

    name = templ->template_file ? fm_path_get_basename(templ->template_file) : NULL;
    if(nlen)
    {
        char *point;
        if(!name)
            *nlen = 0;
        else if((point = strrchr(name, '.')))
            *nlen = g_utf8_strlen(name, point - name);
        else
            *nlen = g_utf8_strlen(name, -1);
    }
    return name;
}

/**
 * fm_template_get_mime_type
 * @templ: a template descriptor
 *
 * Retrieves MIME type descriptor for @templ. Returned data are owned by
 * @templ and should be not freed by caller.
 *
 * Returns: (transfer none): mime type descriptor.
 *
 * Since: 1.2.0
 */
FmMimeType *fm_template_get_mime_type(FmTemplate *templ)
{
    return templ->mime_type;
}

/**
 * fm_template_get_icon
 * @templ: a template descriptor
 *
 * Retrieves icon defined for @templ. Returned data are owned by @templ
 * and should be not freed by caller.
 *
 * Returns: (transfer none): icon for template.
 *
 * Since: 1.2.0
 */
FmIcon *fm_template_get_icon(FmTemplate *templ)
{
    if(templ->icon)
        return templ->icon;
    return fm_mime_type_get_icon(templ->mime_type);
}

/**
 * fm_template_get_prompt
 * @templ: a template descriptor
 *
 * Retrieves prompt for @templ. It can be used as label in entry for the
 * desired name. If no prompt is defined then returns %NULL. Returned
 * data are owned by @templ and should be not freed by caller.
 *
 * Returns: (transfer none): file prompt.
 *
 * Since: 1.2.0
 */
const gchar *fm_template_get_prompt(FmTemplate *templ)
{
    return templ->prompt;
}

/**
 * fm_template_get_label
 * @templ: a template descriptor
 *
 * Retrieves label for @templ. It can be used as label in menu. Returned
 * data are owned by @templ and should be not freed by caller.
 *
 * Returns: (transfer none): template label.
 *
 * Since: 1.2.0
 */
const gchar *fm_template_get_label(FmTemplate *templ)
{
    if(!templ->label && !fm_config->template_type_once && templ->template_file)
    {
        /* set label to filename if no MIME types are used */
        const char *basename = fm_path_get_basename(templ->template_file);
        const char *tmp = strrchr(basename, '.'); /* strip last suffix */
        if(tmp)
            templ->label = g_strndup(basename, tmp - basename);
        else
            templ->label = g_strdup(basename);
    }
    return templ->label;
}

/**
 * fm_template_is_directory
 * @templ: a template descriptor
 *
 * Checks if @templ is directory template.
 *
 * Returns: %TRUE if @templ is directory template.
 *
 * Since: 1.2.0
 */
gboolean fm_template_is_directory(FmTemplate *templ)
{
    return (templ->mime_type == _fm_mime_type_get_inode_directory());
}

/**
 * fm_template_create_file
 * @templ: (allow-none): a template descriptor
 * @path: path to file to create
 * @error: (allow-none): location to retrieve error
 * @run_default: %TRUE to run default application on new file
 *
 * Tries to create file at @path using rules of creating from @templ.
 *
 * Returns: %TRUE if file created successfully.
 *
 * Since: 1.2.0
 */
gboolean fm_template_create_file(FmTemplate *templ, GFile *path, GError **error,
                                 gboolean run_default)
{
    char *command;
    GAppInfo *app;
    GFile *tfile;
    GList *list;
    GFileOutputStream *f;
    FmPath *fm_path;
    FmFolder *fm_folder;
    gboolean ret;

    if((templ && !FM_IS_TEMPLATE(templ)) || !G_IS_FILE(path))
    {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("fm_template_create_file: invalid argument"));
        return FALSE;
    }
    tfile = NULL;
    if(!templ)
        goto _create_empty_file;
    if(templ->template_file)
    {
        command = fm_path_to_str(templ->template_file);
        tfile = g_file_new_for_path(command);
        g_free(command);
    }
    /* FIXME: it may block */
    if(templ->mime_type == _fm_mime_type_get_inode_directory())
    {
        if(!g_file_make_directory(path, NULL, error))
            return FALSE;
    }
    else if(!g_file_copy(tfile, path, G_FILE_COPY_TARGET_DEFAULT_PERMS, NULL,
                         NULL, NULL, error))
    {
        if((*error)->domain != G_IO_ERROR || (*error)->code != G_IO_ERROR_NOT_FOUND)
        {
            /* we ran into problems, application will run into them too
               the most probably, so don't try to launch it then */
            g_object_unref(tfile);
            return FALSE;
        }
        /* template file not found, it's normal */
        g_clear_error(error);
        /* create empty file instead */
_create_empty_file:
        f = g_file_create(path, G_FILE_CREATE_NONE, NULL, error);
        if(!f)
        {
            if(tfile)
                g_object_unref(tfile);
            return FALSE;
        }
        g_object_unref(f);
    }
    if(tfile)
        g_object_unref(tfile);
    fm_path = fm_path_new_for_gfile(path);
    fm_folder = fm_folder_find_by_path(fm_path_get_parent(fm_path));
    if (!fm_folder || !_fm_folder_event_file_added(fm_folder, fm_path))
        fm_path_unref(fm_path);
    if (fm_folder)
        g_object_unref(fm_folder);
    if(!run_default || !templ)
        return TRUE;
    if(templ->command)
    {
        app = g_app_info_create_from_commandline(templ->command, NULL,
                                                 G_APP_INFO_CREATE_NONE, error);
    }
    else
    {
        app = g_app_info_get_default_for_type(fm_mime_type_get_type(templ->mime_type), FALSE);
        if(!app && error)
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                        _("No default application is set for MIME type %s"),
                        fm_mime_type_get_type(templ->mime_type));
    }
    if(!app)
        return FALSE;
    list = g_list_prepend(NULL, path);
    ret = g_app_info_launch(app, list, NULL, error);
    g_list_free(list);
    g_object_unref(app);
    return ret;
}
