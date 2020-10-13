/*
 *      fm-file-properties.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2012-2015 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

/**
 * SECTION:fm-file-properties
 * @short_description: Dialog window for changing properties of file.
 * @title: File properties dialog
 *
 * @include: libfm/fm-gtk.h
 *
 * The file properties dialog is a window with few tabs and buttons "OK"
 * and "Cancel". Most of content of those tabs is handled by the widget
 * itself but there is a possibility to change its content for some file
 * type. See fm_file_properties_add_for_mime_type() for details.
 *
 * Default content of tabs of file properties dialog follows (each tab has
 * a GtkAlignment element, tab ids below meant of those):
 *
 * Tab 1: contains GtkTable (id general_table) with items:
 * - GtkImage (id icon)         : file icon, eventbox: id icon_eventbox
 * - GtkLabel (id file)         : reserved (hidden), hidden label: id file_label
 * - GtkEntry (id name)         : label: "Name"
 * - GtkLabel (id dir)          : label: "Location"
 * - GtkLabel (id target)       : label: "Target", id target_label
 * - GtkLabel (id type)         : label: "File type"
 * - GtkComboBox (id open_with) : label: "Open with", id open_with_label
 * - GtkLabel (id total_files)   : (hidden) label: "Total count of files", id total_files_label
 * - GtkLabel (id total_size)   : label: "Total Size of Files", id total_size_label
 * - GtkLabel (id size_on_disk) : label: "Size on Disk", id size_on_disk_label
 * - GtkLabel (id mtime)        : label: "Last Modification", id mtime_label
 * - GtkLabel (id atime)        : label: "Last Access", id atime_label
 * - GtkLabel (id ctime)        : (hidden) label: "Last Permissions Change", id ctime_label
 *
 * Tab 2: id permissions_tab, contains items inside:
 * - GtkEntry (id owner)        : label: "Owner", id owner_label
 * - GtkEntry (id group)        : label: "Group", id group_label
 * - GtkComboBox (id read_perm) : label: "View content"
 * - GtkComboBox (id write_perm) : label: "Change content"
 * - GtkComboBox (id exec_perm) : label: "Execute", id exec_label
 * - GtkComboBox (id flags_set_file) : label: "Special bits", id flags_label
 * - GtkComboBox (id flags_set_dir) : share the place with flags_set_file
 * - GtkCheckButton (id hidden) "Hidden file"
 *
 * Tab 3: id extra_tab (hidden), empty, label: id extra_tab_label
 *
 * Since gtk_table_get_size() is available only for GTK 2.22 ... GTK 3.4
 * it is not generally recommended to change size of GtkTable but be also
 * aware that gtk_table_attach() is marked deprecated in GTK 3.4 though.
 *
 * Widget sets icon activatable if target file supports icon change. If
 * user changes icon then its name (either themed name or file path) will
 * be stored as GQuark fm_qdata_id in GtkImage "icon" (see above).
 *
 * Widget sets name entry editable if target file supports renaming. Both
 * icon name and file name will be checked after extensions are finished
 * therefore extensions have a chance to reset the data and widget will
 * not do own processing.
 */

#include <config.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <ctype.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>

#include "fm.h"

#include "fm-file-properties.h"

#include "fm-progress-dlg.h"
#include "fm-gtk-utils.h"

#include "fm-app-chooser-combo-box.h"

#include "gtk-compat.h"

#define     UI_FILE             PACKAGE_UI_DIR"/file-prop.ui"
#define     GET_WIDGET(transform,name) data->name = transform(gtk_builder_get_object(builder, #name))

/* for 'Read' combo box */
enum {
    NO_CHANGE = 0,
    READ_USER,
    READ_GROUP,
    READ_ALL
};

/* for 'Write' and 'Exec'/'Enter' combo box */
enum {
    /* NO_CHANGE, */
    ACCESS_NOBODY = 1,
    ACCESS_USER,
    ACCESS_GROUP,
    ACCESS_ALL
};

/* for files-only 'Special' combo box */
enum {
    /* NO_CHANGE, */
    FILE_COMMON = 1,
    FILE_SUID,
    FILE_SGID,
    FILE_SUID_SGID
};

/* for directories-only 'Special' combo box */
enum {
    /* NO_CHANGE, */
    DIR_COMMON = 1,
    DIR_STICKY,
    DIR_SGID,
    DIR_STICKY_SGID
};

typedef struct _FmFilePropExt FmFilePropExt;
struct _FmFilePropExt
{
    FmFilePropExt *next; /* built-in GSList */
    FmMimeType *type;
    FmFilePropertiesExtensionInit cb; /* callbacks */
};

static FmFilePropExt *extensions = NULL;

typedef struct _FmFilePropData FmFilePropData;
struct _FmFilePropData
{
    GtkDialog* dlg;

    /* General page */
    GtkTable* general_table;
    GtkImage* icon;
    GtkWidget* icon_eventbox;
    GtkEntry* name;
    GtkLabel* file;
    GtkLabel* file_label;
    GtkLabel* dir;
    GtkLabel* target;
    GtkWidget* target_label;
    GtkLabel* type;
    GtkWidget* open_with_label;
    GtkComboBox* open_with;
    GtkLabel* total_files;
    GtkWidget* total_files_label;
    GtkLabel* total_size;
    GtkLabel* size_on_disk;
    GtkLabel* mtime;
    GtkWidget* mtime_label;
    GtkLabel* atime;
    GtkWidget* atime_label;
    GtkLabel* ctime;
    GtkWidget* ctime_label;

    /* Permissions page */
    GtkWidget* permissions_tab;
    GtkEntry* owner;
    char* orig_owner;
    GtkEntry* group;
    char* orig_group;
    GtkComboBox* read_perm;
    int read_perm_sel;
    GtkComboBox* write_perm;
    int write_perm_sel;
    GtkLabel* exec_label;
    GtkComboBox* exec_perm;
    int exec_perm_sel;
    GtkLabel* flags_label;
    GtkComboBox* flags_set_file;
    GtkComboBox* flags_set_dir;
    int flags_set_sel;
    GtkWidget *hidden;

    FmFileInfoList* files;
    FmFileInfo* fi;
    gboolean single_type;
    gboolean single_file;
    gboolean all_native;
    gboolean has_dir;
    gboolean all_dirs;
    FmMimeType* mime_type;

    gint32 uid;
    gint32 gid;

    guint timeout;
    FmDeepCountJob* dc_job;

    GSList *ext; /* elements: FmFilePropExt */
    GSList *extdata;
};


/* ---- icon change handling ---- */
/* this handler is taken from lxshortcut and modified a bit */
static void on_update_preview(GtkFileChooser* chooser, GtkImage* img)
{
    char *file = gtk_file_chooser_get_preview_filename(chooser);
    if (file)
    {
        GdkPixbuf *pix = gdk_pixbuf_new_from_file_at_scale(file, 48, 48, TRUE, NULL);
        if (pix)
        {
            gtk_image_set_from_pixbuf(img, pix);
            g_object_unref(pix);
            return;
        }
    }
    gtk_image_clear(img);
}

static void on_toggle_theme(GtkToggleButton *btn, GtkNotebook *notebook)
{
    gtk_notebook_set_current_page(notebook, 0);
}

static void on_toggle_files(GtkToggleButton *btn, GtkNotebook *notebook)
{
    gtk_notebook_set_current_page(notebook, 1);
}

static GdkPixbuf *vfs_load_icon(GtkIconTheme *theme, const char *icon_name, int size)
{
    GdkPixbuf *icon = NULL;
    const char *file;
    GtkIconInfo *inf = gtk_icon_theme_lookup_icon(theme, icon_name, size,
                                                  GTK_ICON_LOOKUP_USE_BUILTIN);
    if (G_UNLIKELY(!inf))
        return NULL;

    file = gtk_icon_info_get_filename(inf);
    if (G_LIKELY(file))
        icon = gdk_pixbuf_new_from_file_at_scale(file, size, size, TRUE, NULL);
    else
    {
        icon = gtk_icon_info_get_builtin_pixbuf(inf);
        g_object_ref(icon);
    }
    gtk_icon_info_free(inf);

    if (G_LIKELY(icon))  /* scale down the icon if it's too big */
    {
        int width, height;
        height = gdk_pixbuf_get_height(icon);
        width = gdk_pixbuf_get_width(icon);

        if (G_UNLIKELY(height > size || width > size))
        {
            GdkPixbuf *scaled;
            if (height > width)
            {
                width = size * height / width;
                height = size;
            }
            else if (height < width)
            {
                height = size * width / height;
                width = size;
            }
            else
                height = width = size;
            scaled = gdk_pixbuf_scale_simple(icon, width, height, GDK_INTERP_BILINEAR);
            g_object_unref(icon);
            icon = scaled;
        }
    }
    return icon;
}

typedef struct {
    GtkIconView *view;
    GtkListStore *model;
    GAsyncQueue *queue;
} IconThreadData;

static gpointer load_themed_icon(GtkIconTheme *theme, IconThreadData *data)
{
    GdkPixbuf *pix;
    char *icon_name = g_async_queue_pop(data->queue);

    GDK_THREADS_ENTER();
    pix = vfs_load_icon(theme, icon_name, 48);
    if (pix)
    {
        GtkTreeIter it;
        gtk_list_store_append(data->model, &it);
        gtk_list_store_set(data->model, &it, 0, pix, 1, icon_name, -1);
        g_object_unref(pix);
    }
    if (g_async_queue_length(data->queue) == 0)
    {
        if (gtk_icon_view_get_model(data->view) == NULL)
        {
            gtk_icon_view_set_model(data->view, GTK_TREE_MODEL(data->model));
#if GTK_CHECK_VERSION(2, 20, 0)
            if (gtk_widget_get_realized(GTK_WIDGET(data->view)))
                gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(data->view)), NULL);
#else
            if (GTK_WIDGET_REALIZED(GTK_WIDGET(data->view)))
                gdk_window_set_cursor(GTK_WIDGET(data->view)->window, NULL);
#endif
        }
    }
    GDK_THREADS_LEAVE();
    g_thread_yield();
    /* g_debug("load: %s", icon_name); */
    g_free(icon_name);
    return NULL;
}

static void _change_icon(GtkWidget *dlg, FmFilePropData *data)
{
    GtkBuilder *builder;
    GtkFileChooser *chooser;
    GtkWidget *chooser_dlg, *preview, *notebook;
    GtkFileFilter *filter;
    GtkIconTheme *theme;
    GList *contexts, *l;
    GThreadPool *thread_pool;
    IconThreadData thread_data;

    builder = gtk_builder_new();
    gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);
    gtk_builder_add_from_file(builder, PACKAGE_UI_DIR "/choose-icon.ui", NULL);
    chooser_dlg = GTK_WIDGET(gtk_builder_get_object(builder, "dlg"));
    chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "chooser"));
    thread_data.view = GTK_ICON_VIEW(gtk_builder_get_object(builder, "icons"));
    notebook = GTK_WIDGET(gtk_builder_get_object(builder, "notebook"));
    g_signal_connect(gtk_builder_get_object(builder,"theme"), "toggled", G_CALLBACK(on_toggle_theme), notebook);
    g_signal_connect(gtk_builder_get_object(builder,"files"), "toggled", G_CALLBACK(on_toggle_files), notebook);

    gtk_window_set_default_size(GTK_WINDOW(chooser_dlg), 600, 440);
    gtk_window_set_transient_for(GTK_WINDOW(chooser_dlg), GTK_WINDOW(dlg));

    preview = gtk_image_new();
    gtk_widget_show(preview);
    gtk_file_chooser_set_preview_widget(chooser, preview);
    g_signal_connect(chooser, "update-preview", G_CALLBACK(on_update_preview),
                     GTK_IMAGE(preview));

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(GTK_FILE_FILTER(filter), _("Image files"));
    gtk_file_filter_add_pixbuf_formats(GTK_FILE_FILTER(filter));
    gtk_file_chooser_add_filter(chooser, filter);
    gtk_file_chooser_set_local_only(chooser, TRUE);
    gtk_file_chooser_set_select_multiple(chooser, FALSE);
    gtk_file_chooser_set_use_preview_label(chooser, FALSE);

    gtk_widget_show(chooser_dlg);
    while (gtk_events_pending())
        gtk_main_iteration();

    gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(thread_data.view)),
                          gdk_cursor_new(GDK_WATCH));

    /* load themed icons */
    thread_pool = g_thread_pool_new((GFunc)load_themed_icon, &thread_data, 1, TRUE, NULL);
    g_thread_pool_set_max_threads(thread_pool, 1, NULL);
    thread_data.queue = g_async_queue_new();

    thread_data.model = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
    theme = gtk_icon_theme_get_default();

    gtk_icon_view_set_pixbuf_column(thread_data.view, 0);
    gtk_icon_view_set_item_width(thread_data.view, 80);
    gtk_icon_view_set_text_column(thread_data.view, 1);

    /* GList* contexts = gtk_icon_theme_list_contexts(theme); */
    contexts = g_list_alloc();
    /* FIXME: we should enable more contexts */
    /* http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html#context */
    contexts->data = g_strdup("Applications");
    for (l = contexts; l; l = l->next)
    {
        /* g_debug(l->data); */
        GList *icon_names = gtk_icon_theme_list_icons(theme, (char*)l->data);
        GList *icon_name;
        for (icon_name = icon_names; icon_name; icon_name = icon_name->next)
        {
            g_async_queue_push(thread_data.queue, icon_name->data);
            g_thread_pool_push(thread_pool, theme, NULL);
        }
        g_list_free(icon_names);
        g_free(l->data);
    }
    g_list_free(contexts);

    if (gtk_dialog_run(GTK_DIALOG(chooser_dlg)) == GTK_RESPONSE_OK)
    {
        char* icon_name = NULL;
        if (gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)) == 0)
        {
            GList *sels = gtk_icon_view_get_selected_items(thread_data.view);
            GtkTreePath *tp = (GtkTreePath*)sels->data;
            GtkTreeIter it;
            if (gtk_tree_model_get_iter(GTK_TREE_MODEL(thread_data.model), &it, tp))
            {
                gtk_tree_model_get(GTK_TREE_MODEL(thread_data.model), &it, 1, &icon_name, -1);
            }
            g_list_foreach(sels, (GFunc)gtk_tree_path_free, NULL);
            g_list_free(sels);
            if (icon_name)
                gtk_image_set_from_icon_name(data->icon, icon_name, GTK_ICON_SIZE_DIALOG);
        }
        else
        {
            icon_name = gtk_file_chooser_get_filename(chooser);
            if (icon_name)
            {
                GdkPixbuf *pix = gdk_pixbuf_new_from_file_at_scale(icon_name,
                                                            48, 48, TRUE, NULL);
                if (pix)
                {
                    gtk_image_set_from_pixbuf(data->icon, pix);
                    g_object_unref(pix);
                }
            }
        }
        if (icon_name)
        {
            g_object_set_qdata_full(G_OBJECT(data->icon), fm_qdata_id, icon_name,
                                    g_free);
        }
    }
    g_thread_pool_free(thread_pool, TRUE, FALSE);
    gtk_widget_destroy(chooser_dlg);
}


static gboolean _icon_click_event(GtkWidget *widget, GdkEventButton *event,
                                  FmFilePropData *data)
{
    /* g_debug("icon click received (button=%d)", event->button); */
    if (event->button == 1 && gtk_widget_get_can_focus(data->icon_eventbox))
    {
         /* accept only left click */
        _change_icon(gtk_widget_get_toplevel(widget), data);
        return TRUE;
    }
    return FALSE;
}

static gboolean _icon_press_event(GtkWidget *widget, GdkEventKey *event,
                                  FmFilePropData *data)
{
    /* g_debug("icon key received (key=%u)", event->keyval); */
    if (event->keyval == GDK_KEY_space)
        _change_icon(gtk_widget_get_toplevel(widget), data);
    return FALSE;
}


/* ---- all other handlers ---- */
static gboolean _on_timeout(gpointer user_data)
{
    FmFilePropData* data = (FmFilePropData*)user_data;
    char size_str[128];
    FmDeepCountJob* dc;

    dc = data->dc_job;
    if(G_LIKELY(dc && !fm_job_is_cancelled(FM_JOB(dc))))
    {
        char* str;
        fm_file_size_to_str(size_str, sizeof(size_str), dc->total_size,
                            fm_config->si_unit);
        str = g_strdup_printf("%s (%'llu %s)", size_str,
                              (long long unsigned int)dc->total_size,
                              dngettext(GETTEXT_PACKAGE, "byte", "bytes",
                                       (gulong)dc->total_size));
        gtk_label_set_text(data->total_size, str);
        g_free(str);

        fm_file_size_to_str(size_str, sizeof(size_str), dc->total_ondisk_size,
                            fm_config->si_unit);
        str = g_strdup_printf("%s (%'llu %s)", size_str,
                              (long long unsigned int)dc->total_ondisk_size,
                              dngettext(GETTEXT_PACKAGE, "byte", "bytes",
                                       (gulong)dc->total_ondisk_size));
        gtk_label_set_text(data->size_on_disk, str);
        g_free(str);

        if (data->total_files)
        {
            str = g_strdup_printf(_("scanning... %d"), dc->count);
            gtk_label_set_text(data->total_files, str);
            g_free(str);
        }
    }
    return TRUE;
}

static gboolean on_timeout(gpointer user_data)
{
    if(g_source_is_destroyed(g_main_current_source()))
        return FALSE;
    return _on_timeout(user_data);
}

static void on_finished(FmDeepCountJob* job, FmFilePropData* data)
{
    GDK_THREADS_ENTER();
    _on_timeout(data); /* update display */
    if(data->timeout)
    {
        g_source_remove(data->timeout);
        data->timeout = 0;
    }
    if (data->total_files)
    {
        char *tt = g_strdup_printf("%d", job->count);
        gtk_label_set_text(data->total_files, tt);
        g_free(tt);
    }
    GDK_THREADS_LEAVE();
    g_object_unref(data->dc_job);
    data->dc_job = NULL;
}

static void fm_file_prop_data_free(FmFilePropData* data)
{
    g_free(data->orig_owner);
    g_free(data->orig_group);

    if(data->timeout)
        g_source_remove(data->timeout);
    if(data->dc_job) /* FIXME: check if it's running */
    {
        fm_job_cancel(FM_JOB(data->dc_job));
        g_signal_handlers_disconnect_by_func(data->dc_job, on_finished, data);
        g_object_unref(data->dc_job);
    }
    if(data->mime_type)
        fm_mime_type_unref(data->mime_type);
    fm_file_info_list_unref(data->files);
    g_slice_free(FmFilePropData, data);
}

static gboolean ensure_valid_owner(FmFilePropData* data)
{
    gboolean ret = TRUE;
    const char* tmp = gtk_entry_get_text(data->owner);

    data->uid = -1;
    if(tmp && *tmp)
    {
        if(data->all_native && !isdigit(tmp[0])) /* entering names instead of numbers is only allowed for local files. */
        {
            struct passwd* pw;
            if(!(pw = getpwnam(tmp)))
                ret = FALSE;
            else
                data->uid = pw->pw_uid;
        }
        else
            data->uid = atoi(tmp);
    }
    else
        ret = FALSE;

    if(!ret)
    {
        fm_show_error(GTK_WINDOW(data->dlg), NULL, _("Please enter a valid user name or numeric id."));
        gtk_widget_grab_focus(GTK_WIDGET(data->owner));
    }

    return ret;
}

static gboolean ensure_valid_group(FmFilePropData* data)
{
    gboolean ret = TRUE;
    const char* tmp = gtk_entry_get_text(data->group);

    if(tmp && *tmp)
    {
        if(data->all_native && !isdigit(tmp[0])) /* entering names instead of numbers is only allowed for local files. */
        {
            struct group* gr;
            if(!(gr = getgrnam(tmp)))
                ret = FALSE;
            else
                data->gid = gr->gr_gid;
        }
        else
            data->gid = atoi(tmp);
    }
    else
        ret = FALSE;

    if(!ret)
    {
        fm_show_error(GTK_WINDOW(data->dlg), NULL, _("Please enter a valid group name or numeric id."));
        gtk_widget_grab_focus(GTK_WIDGET(data->group));
    }

    return ret;
}


static void on_response(GtkDialog* dlg, int response, FmFilePropData* data)
{
    FmFileOpsJob* job = NULL;

    if( response == GTK_RESPONSE_OK )
    {
        /* call the extension if it was set */
        if(data->ext != NULL)
        {
            GSList *l, *l2;
            for (l = data->ext, l2 = data->extdata; l; l = l->next, l2 = l2->next)
                ((FmFilePropExt*)l->data)->cb.finish(l2->data, FALSE);
            g_slist_free(data->ext);
            g_slist_free(data->extdata);
            data->ext = NULL;
        }

      /* if permissions_tab is hidden then it has undefined data
         so don't ever try to check those data */
      if(gtk_widget_get_visible(data->permissions_tab))
      {
        int sel;
        const char* new_owner = gtk_entry_get_text(data->owner);
        const char* new_group = gtk_entry_get_text(data->group);
        mode_t new_mode = 0, new_mode_mask = 0;

        if(!ensure_valid_owner(data) || !ensure_valid_group(data))
        {
            g_signal_stop_emission_by_name(dlg, "response");
            return;
        }

        /* FIXME: if all files are native, it's possible to check
         * if the names are legal user and group names on the local
         * machine prior to chown. */
        if(new_owner && *new_owner && g_strcmp0(data->orig_owner, new_owner))
        {
            /* change owner */
            g_debug("change owner to: %d", data->uid);
        }
        else
            data->uid = -1;

        if(new_group && *new_group && g_strcmp0(data->orig_group, new_group))
        {
            /* change group */
            g_debug("change group to: %d", data->gid);
        }
        else
            data->gid = -1;

        /* check if chmod is needed here. */
        sel = gtk_combo_box_get_active(data->read_perm);
        if( sel > NO_CHANGE ) /* requested to change read permissions */
        {
            g_debug("got selection for read: %d", sel);
            if(data->read_perm_sel != sel) /* new value is different from original */
            {
                new_mode_mask = (S_IRUSR|S_IRGRP|S_IROTH);
                data->read_perm_sel = sel;
                switch(sel)
                {
                case READ_ALL:
                    new_mode = S_IROTH;
                case READ_GROUP:
                    new_mode |= S_IRGRP;
                case READ_USER:
                default:
                    new_mode |= S_IRUSR;
                }
            }
            else /* otherwise, no change */
                data->read_perm_sel = NO_CHANGE;
        }
        else
            data->read_perm_sel = NO_CHANGE;

        sel = gtk_combo_box_get_active(data->write_perm);
        if( sel > NO_CHANGE ) /* requested to change write permissions */
        {
            g_debug("got selection for write: %d", sel);
            if(data->write_perm_sel != sel) /* new value is different from original */
            {
                new_mode_mask |= (S_IWUSR|S_IWGRP|S_IWOTH);
                data->write_perm_sel = sel;
                switch(sel)
                {
                case ACCESS_ALL:
                    new_mode |= S_IWOTH;
                case ACCESS_GROUP:
                    new_mode |= S_IWGRP;
                case ACCESS_USER:
                    new_mode |= S_IWUSR;
                case ACCESS_NOBODY: default: ;
                }
            }
            else /* otherwise, no change */
                data->write_perm_sel = NO_CHANGE;
        }
        else
            data->write_perm_sel = NO_CHANGE;

        sel = gtk_combo_box_get_active(data->exec_perm);
        if( sel > NO_CHANGE ) /* requested to change exec permissions */
        {
            g_debug("got selection for exec: %d", sel);
            if(data->exec_perm_sel != sel) /* new value is different from original */
            {
                new_mode_mask |= (S_IXUSR|S_IXGRP|S_IXOTH);
                data->exec_perm_sel = sel;
                switch(sel)
                {
                case ACCESS_ALL:
                    new_mode |= S_IXOTH;
                case ACCESS_GROUP:
                    new_mode |= S_IXGRP;
                case ACCESS_USER:
                    new_mode |= S_IXUSR;
                case ACCESS_NOBODY: default: ;
                }
            }
            else /* otherwise, no change */
                data->exec_perm_sel = NO_CHANGE;
        }
        else
            data->exec_perm_sel = NO_CHANGE;

        if(data->all_dirs)
            sel = gtk_combo_box_get_active(data->flags_set_dir);
        else if(!data->has_dir)
            sel = gtk_combo_box_get_active(data->flags_set_file);
        else
            sel = NO_CHANGE;
        if( sel > NO_CHANGE ) /* requested to change special bits */
        {
            g_debug("got selection for flags: %d", sel);
            if(data->flags_set_sel != sel) /* new value is different from original */
            {
                new_mode_mask |= (S_ISUID|S_ISGID|S_ISVTX);
                data->flags_set_sel = sel;
                if(data->all_dirs)
                {
                    switch(sel)
                    {
                    case DIR_STICKY:
                        new_mode |= S_ISVTX;
                        break;
                    case DIR_STICKY_SGID:
                        new_mode |= S_ISVTX;
                    case DIR_SGID:
                        new_mode |= S_ISGID;
                    case DIR_COMMON: default: ;
                    }
                }
                else
                {
                    switch(sel)
                    {
                    case FILE_SUID:
                        new_mode |= S_ISUID;
                        break;
                    case FILE_SUID_SGID:
                        new_mode |= S_ISUID;
                    case FILE_SGID:
                        new_mode |= S_ISGID;
                    case FILE_COMMON: default: ;
                    }
                }
            }
            else /* otherwise, no change */
                data->flags_set_sel = NO_CHANGE;
        }
        else
            data->flags_set_sel = NO_CHANGE;

        if(new_mode_mask || data->uid != -1 || data->gid != -1)
        {
            FmPathList* paths = fm_path_list_new_from_file_info_list(data->files);

            job = fm_file_ops_job_new(FM_FILE_OP_CHANGE_ATTR, paths);
            fm_path_list_unref(paths);

            /* need to chown */
            if(data->uid != -1 || data->gid != -1)
                fm_file_ops_job_set_chown(job, data->uid, data->gid);

            /* need to do chmod */
            if(new_mode_mask) {
                g_debug("going to set mode bits %04o by mask %04o", new_mode, new_mode_mask);
                fm_file_ops_job_set_chmod(job, new_mode, new_mode_mask);
            }

            /* try recursion but don't recurse exec/sgid/sticky changes */
            if(data->has_dir && data->exec_perm_sel == NO_CHANGE &&
               data->flags_set_sel == NO_CHANGE)
            {
                gtk_combo_box_set_active(data->read_perm, data->read_perm_sel);
                gtk_combo_box_set_active(data->write_perm, data->write_perm_sel);
                gtk_combo_box_set_active(data->exec_perm, NO_CHANGE);
                gtk_combo_box_set_active(data->flags_set_dir, NO_CHANGE);
                /* FIXME: may special bits and exec flags still be messed up? */
                if(fm_yes_no(GTK_WINDOW(data->dlg), NULL, _( "Do you want to recursively apply these changes to all files and sub-folders?" ), TRUE))
                    fm_file_ops_job_set_recursive(job, TRUE);
            }
        }
        if(data->hidden != NULL && gtk_widget_get_visible(data->hidden))
        {
            gboolean new_hidden = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->hidden));
            if(new_hidden != fm_file_info_is_hidden(fm_file_info_list_peek_head(data->files)))
            {
                /* change hidden attribute of file */
                g_debug("hidden changed to %d",new_hidden);
                if (job == NULL)
                {
                    FmPathList* paths = fm_path_list_new_from_file_info_list(data->files);

                    job = fm_file_ops_job_new(FM_FILE_OP_CHANGE_ATTR, paths);
                    fm_path_list_unref(paths);
                }
                fm_file_ops_job_set_hidden(job, new_hidden);
            }
        }
      } /* end of permissions update */

        /* change default application for the mime-type if needed */
        if(data->mime_type && fm_mime_type_get_type(data->mime_type) && data->open_with)
        {
            GAppInfo* app;
            gboolean default_app_changed = FALSE;
            GError* err = NULL;
            app = fm_app_chooser_combo_box_dup_selected_app(data->open_with, &default_app_changed);
            if(app)
            {
                if(default_app_changed)
                {
                    g_app_info_set_as_default_for_type(app, fm_mime_type_get_type(data->mime_type), &err);
                    if(err)
                    {
                        fm_show_error(GTK_WINDOW(dlg), NULL, err->message);
                        g_error_free(err);
                    }
                }
                g_object_unref(app);
            }
        }

        if(data->single_file) /* when only one file is shown */
        {
            const char *text = gtk_entry_get_text(data->name);
            /* if the user has changed the name of the file */
            if(g_strcmp0(fm_file_info_get_disp_name(data->fi), text))
            {
                /* rename the file or set display name for it. */
                if (job == NULL)
                {
                    FmPathList* paths = fm_path_list_new_from_file_info_list(data->files);

                    job = fm_file_ops_job_new(FM_FILE_OP_CHANGE_ATTR, paths);
                    fm_path_list_unref(paths);
                }
                fm_file_ops_job_set_display_name(job, text);
            }
            text = g_object_get_qdata(G_OBJECT(data->icon), fm_qdata_id);
            /* if the user has changed icon */
            if(text)
            {
                GIcon *icon = g_icon_new_for_string(text, NULL);
                if (icon)
                {
                    /* change icon property of the file. */
                    if (job == NULL)
                    {
                        FmPathList* paths = fm_path_list_new_from_file_info_list(data->files);

                        job = fm_file_ops_job_new(FM_FILE_OP_CHANGE_ATTR, paths);
                        fm_path_list_unref(paths);
                    }
                    fm_file_ops_job_set_icon(job, icon);
                    g_object_unref(icon);
                }
                /* FIXME: handle errors */
            }
        }

        if (job)
            /* show progress dialog */
            fm_file_ops_job_run_with_progress(GTK_WINDOW(dlg), job);
                                                        /* it eats reference! */
    }
    /* call the extension if it was set */
    else if(data->ext != NULL)
    {
        GSList *l, *l2;
        for (l = data->ext, l2 = data->extdata; l; l = l->next, l2 = l2->next)
            ((FmFilePropExt*)l->data)->cb.finish(l2->data, TRUE);
        g_slist_free(data->ext);
        g_slist_free(data->extdata);
        data->ext = NULL;
    }
}

/* FIXME: this is too dirty. Need some refactor later. */
static void update_permissions(FmFilePropData* data)
{
    FmFileInfo* fi = fm_file_info_list_peek_head(data->files);
    GList *l;
    int sel;
    mode_t fi_mode = fm_file_info_get_mode(fi);
    mode_t read_perm = (fi_mode & (S_IRUSR|S_IRGRP|S_IROTH));
    mode_t write_perm = (fi_mode & (S_IWUSR|S_IWGRP|S_IWOTH));
    mode_t exec_perm = (fi_mode & (S_IXUSR|S_IXGRP|S_IXOTH));
    mode_t flags_set = (fi_mode & (S_ISUID|S_ISGID|S_ISVTX));
    gint32 uid = fm_file_info_get_uid(fi);
    gint32 gid = fm_file_info_get_gid(fi);
    gboolean mix_read = FALSE, mix_write = FALSE, mix_exec = FALSE;
    gboolean mix_flags = FALSE;

    data->all_native = fm_path_is_native(fm_file_info_get_path(fi));
    data->has_dir = (S_ISDIR(fi_mode) != FALSE);
    data->all_dirs = data->has_dir;

    if((fi_mode & ~S_IFDIR) == 0) /* no permissions accessible */
    {
        gtk_widget_hide(data->permissions_tab);
        return;
    }
    if(data->hidden == NULL)
        ;
    else if(data->single_file)
    {
        if(fm_file_info_can_set_hidden(fi))
        {
            gtk_widget_set_can_focus(data->hidden, TRUE);
            gtk_widget_set_sensitive(data->hidden, TRUE);
            gtk_widget_set_tooltip_text(data->hidden, _("Hide or unhide the file"));
        }
        /* FIXME: use markup for bold text? */
        else if(fm_file_info_is_hidden(fi))
            gtk_widget_set_tooltip_text(data->hidden,
                                        _("This file is hidden because its "
                                          "name starts with a dot ('.')."));
        else
            gtk_widget_set_tooltip_text(data->hidden,
                                        _("Files on this system are hidden "
                                          "if their name starts with a dot "
                                          "('.'). Hit <Ctrl+H> to toggle "
                                          "displaying hidden files."));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->hidden),
                                     fm_file_info_is_hidden(fi));
        gtk_widget_show(data->hidden);
        /* NOTE: for files in menu:// we have it not available still just
           because permissions tab is hidden for those files. That should
           stay this way because for all other .desktop files it should be
           'Hidden' key changed but for menu:// 'NoDisplay' is the key */
    }
    for(l=fm_file_info_list_peek_head_link(data->files)->next; l; l=l->next)
    {
        FmFileInfo* fi = FM_FILE_INFO(l->data);

        if(data->all_native && !fm_path_is_native(fm_file_info_get_path(fi)))
            data->all_native = FALSE;

        fi_mode = fm_file_info_get_mode(fi);
        if((fi_mode & ~S_IFDIR) == 0) /* no permissions accessible */
        {
            gtk_widget_hide(data->permissions_tab);
            return;
        }
        if(S_ISDIR(fi_mode))
            data->has_dir = TRUE;
        else
            data->all_dirs = FALSE;

        if( uid >= 0 && uid != (gint32)fm_file_info_get_uid(fi) )
            uid = -1;
        if( gid >= 0 && gid != (gint32)fm_file_info_get_gid(fi) )
            gid = -1;

        if(!mix_read && read_perm != (fi_mode & (S_IRUSR|S_IRGRP|S_IROTH)))
            mix_read = TRUE;
        if(!mix_write && write_perm != (fi_mode & (S_IWUSR|S_IWGRP|S_IWOTH)))
            mix_write = TRUE;
        if(!mix_exec && exec_perm != (fi_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))
            mix_exec = TRUE;
        if(!mix_flags && flags_set != (fi_mode & (S_ISUID|S_ISGID|S_ISVTX)))
            mix_flags = TRUE;
    }

    if( data->all_native )
    {
        if(uid >= 0)
            gtk_entry_set_text(data->owner, fm_file_info_get_disp_owner(fi));
        if(gid >= 0)
            gtk_entry_set_text(data->group, fm_file_info_get_disp_group(fi));
    }

    if (data->has_dir && data->total_files)
    {
        gtk_widget_show(data->total_files_label);
        gtk_widget_show(GTK_WIDGET(data->total_files));
        gtk_label_set_text(data->total_files, _("scanning..."));
    }
    else if (data->total_files)
    {
        gtk_widget_destroy(data->total_files_label);
        gtk_widget_destroy(GTK_WIDGET(data->total_files));
        gtk_table_set_row_spacing(data->general_table, 6, 0);
        data->total_files = NULL;
    }

    data->orig_owner = g_strdup(gtk_entry_get_text(data->owner));
    data->orig_group = g_strdup(gtk_entry_get_text(data->group));

    /* on local filesystems, only root can do chown. */
    if( data->all_native && geteuid() != 0 )
    {
        gtk_widget_set_sensitive(GTK_WIDGET(data->owner), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(data->group), FALSE);
    }

    /* read access chooser */
    sel = NO_CHANGE;
    if(!mix_read)
    {
        if(read_perm & S_IROTH)
            sel = READ_ALL;
        else if(read_perm & S_IRGRP)
            sel = READ_GROUP;
        else
            sel = READ_USER;
    }
    gtk_combo_box_set_active(data->read_perm, sel);
    data->read_perm_sel = sel;

    /* write access chooser */
    sel = NO_CHANGE;
    if(!mix_write)
    {
        if(write_perm & S_IWOTH)
            sel = ACCESS_ALL;
        else if(write_perm & S_IWGRP)
            sel = ACCESS_GROUP;
        else if(write_perm & S_IWUSR)
            sel = ACCESS_USER;
        else
            sel = ACCESS_NOBODY;
    }
    gtk_combo_box_set_active(data->write_perm, sel);
    data->write_perm_sel = sel;

    /* disable exec and special bits for mixed selection and return */
    if(data->has_dir && !data->all_dirs) {
        gtk_widget_hide(GTK_WIDGET(data->exec_label));
        gtk_widget_hide(GTK_WIDGET(data->exec_perm));
        data->exec_perm_sel = NO_CHANGE;
        gtk_widget_hide(GTK_WIDGET(data->flags_label));
        gtk_widget_hide(GTK_WIDGET(data->flags_set_file));
        gtk_widget_hide(GTK_WIDGET(data->flags_set_dir));
        data->flags_set_sel = NO_CHANGE;
        return;
    }
    if(data->has_dir)
        gtk_label_set_label(data->exec_label, _("<b>_Access content:</b>"));
    if(!fm_config->advanced_mode)
    {
        gtk_widget_hide(GTK_WIDGET(data->flags_label));
        gtk_widget_hide(GTK_WIDGET(data->flags_set_file));
        gtk_widget_hide(GTK_WIDGET(data->flags_set_dir));
        data->flags_set_sel = NO_CHANGE;
    }
    else if(data->has_dir)
        gtk_widget_hide(GTK_WIDGET(data->flags_set_file));
    else
        gtk_widget_hide(GTK_WIDGET(data->flags_set_dir));

    /* exec access chooser */
    sel = NO_CHANGE;
    if(!mix_exec)
    {
        if(exec_perm & S_IXOTH)
            sel = ACCESS_ALL;
        else if(exec_perm & S_IXGRP)
            sel = ACCESS_GROUP;
        else if(exec_perm & S_IXUSR)
            sel = ACCESS_USER;
        else
            sel = ACCESS_NOBODY;
    }
    gtk_combo_box_set_active(data->exec_perm, sel);
    data->exec_perm_sel = sel;

    /* special bits chooser */
    sel = NO_CHANGE;
    if(data->has_dir)
    {
        if(!mix_flags)
        {
            if((flags_set & (S_ISGID|S_ISVTX)) == (S_ISGID|S_ISVTX))
                sel = DIR_STICKY_SGID;
            else if(flags_set & S_ISGID)
                sel = DIR_SGID;
            else if(flags_set & S_ISVTX)
                sel = DIR_STICKY;
            else
                sel = DIR_COMMON;
        }
        gtk_combo_box_set_active(data->flags_set_dir, sel);
    }
    else
    {
        if(!mix_flags)
        {
            if((flags_set & (S_ISUID|S_ISGID)) == (S_ISUID|S_ISGID))
                sel = FILE_SUID_SGID;
            else if(flags_set & S_ISUID)
                sel = FILE_SUID;
            else if(flags_set & S_ISGID)
                sel = FILE_SGID;
            else
                sel = FILE_COMMON;
        }
        gtk_combo_box_set_active(data->flags_set_file, sel);
    }
    data->flags_set_sel = sel;
}

static gboolean on_icon_enter_notify(GtkWidget *widget, GdkEvent *event,
                                     FmFilePropData *data)
{
    GdkWindow *window = gtk_widget_get_window(data->icon_eventbox);

    if (window && gdk_window_get_cursor(window) == NULL)
        gdk_window_set_cursor(window, gdk_cursor_new(GDK_HAND1));
    return FALSE;
}

static void update_ui(FmFilePropData* data)
{
    GtkImage* img = data->icon;
    const char *text;

    if( data->single_type ) /* all files are of the same mime-type */
    {
        GIcon* icon = NULL;
        FmFilePropExt* ext;

        if( data->single_file ) /* only one file is selected. */
        {
            FmFileInfo* fi = fm_file_info_list_peek_head(data->files);
            FmIcon* fi_icon = fm_file_info_get_icon(fi);
            if(fi_icon)
                icon = G_ICON(fi_icon);
            if(fm_file_info_can_set_icon(fi))
            {
                /* enable icon change if file allows that */
                gtk_widget_set_can_focus(data->icon_eventbox, TRUE);
            }
        }

        if(data->mime_type)
        {
            if(!icon)
            {
                FmIcon* ficon = fm_mime_type_get_icon(data->mime_type);
                if(ficon)
                    icon = G_ICON(ficon);
            }
            text = fm_mime_type_get_desc(data->mime_type);
            gtk_label_set_text(data->type, text);
            if(strlen(text) > 16)
                gtk_widget_set_tooltip_text(GTK_WIDGET(data->type), text);
        }

        if(icon)
            gtk_image_set_from_gicon(img, icon, GTK_ICON_SIZE_DIALOG);

        if(data->single_file &&
           (fm_file_info_is_symlink(data->fi) || fm_file_info_is_shortcut(data->fi)))
        {
            text = fm_file_info_get_target(data->fi);
            gtk_widget_show(data->target_label);
            gtk_widget_show(GTK_WIDGET(data->target));
            gtk_label_set_text(data->target, text);
            if(strlen(text) > 16)
                gtk_widget_set_tooltip_text(GTK_WIDGET(data->target), text);
        }
        else
        {
            gtk_widget_destroy(data->target_label);
            gtk_widget_destroy(GTK_WIDGET(data->target));
            gtk_table_set_row_spacing(data->general_table, 3, 0);
        }
        CHECK_MODULES();
        for(ext = extensions; ext; ext = ext->next)
            if(ext->type == data->mime_type)
            {
                data->ext = g_slist_append(data->ext, ext);
                data->extdata = g_slist_append(data->extdata, NULL);
            }
        if(!data->ext)
            for(ext = extensions; ext; ext = ext->next)
                if(ext->type == NULL) /* fallback handler */
                {
                    data->ext = g_slist_append(data->ext, ext);
                    data->extdata = g_slist_append(data->extdata, NULL);
                    break;
                }
    }
    else
    {
        gtk_image_set_from_stock(img, GTK_STOCK_DND_MULTIPLE, GTK_ICON_SIZE_DIALOG);
        gtk_widget_set_sensitive(GTK_WIDGET(data->name), FALSE);

        gtk_label_set_text(data->type, _("Files of different types"));

        gtk_widget_destroy(data->target_label);
        gtk_widget_destroy(GTK_WIDGET(data->target));

        gtk_widget_destroy(data->open_with_label);
        gtk_widget_destroy(GTK_WIDGET(data->open_with));
        gtk_table_set_row_spacing(data->general_table, 5, 0);
        data->open_with = NULL;
        data->open_with_label = NULL;
    }

    /* FIXME: check if all files has the same parent dir, mtime, or atime */
    if( data->single_file )
    {
        char buf[128];
        FmPath* path = fm_file_info_get_path(data->fi);
        GFile* gfile = fm_path_to_gfile(path);
        char* parent_str;
        time_t atime;
        struct tm tm;

        /* FIXME: use fm_file_info_get_edit_name */
        text = fm_file_info_get_disp_name(data->fi);
        gtk_entry_set_text(data->name, text);
        if(strlen(text) > 16)
            gtk_widget_set_tooltip_text(GTK_WIDGET(data->name), text);
        if (g_strcmp0(text, fm_path_get_basename(path)) != 0)
        {
            char *basename = NULL;
            if (fm_path_is_native(path))
            {
                parent_str = g_file_get_path(gfile);
                if (parent_str)
                {
                    basename = g_filename_display_basename(parent_str);
                    g_free(parent_str);
                }
            }
            else
                basename = g_uri_unescape_string(fm_path_get_basename(path), NULL);
            if (basename && strcmp(text, basename) != 0)
            {
                gtk_label_set_text(data->file, basename);
                gtk_label_set_markup(data->file_label, _("<b>File:</b>"));
                gtk_widget_show(GTK_WIDGET(data->file));
                gtk_widget_show(GTK_WIDGET(data->file_label));
            }
            g_free(basename);
        }
        parent_str = NULL;
        if (fm_path_get_parent(path))
        {
            GFile *parent_gf = g_file_get_parent(gfile);
            if (parent_gf) /* FmPath may think wrong query URI has parent */
            {
                parent_str = g_file_get_parse_name(parent_gf);
                g_object_unref(parent_gf);
            }
        }
        g_object_unref(gfile);
        if(parent_str)
        {
            gtk_label_set_text(data->dir, parent_str);
            if(strlen(parent_str) > 16)
                gtk_widget_set_tooltip_text(GTK_WIDGET(data->dir), parent_str);
            g_free(parent_str);
        }
        else
            gtk_label_set_text(data->dir, "");
        if(fm_file_info_get_mtime(data->fi) > 0)
            gtk_label_set_text(data->mtime, fm_file_info_get_disp_mtime(data->fi));
        else
        {
            gtk_widget_destroy(data->mtime_label);
            gtk_widget_destroy(GTK_WIDGET(data->mtime));
        }

        /* FIXME: need to encapsulate this in an libfm API. */
        atime = fm_file_info_get_atime(data->fi);
        if(atime > 0)
        {
            localtime_r(&atime, &tm);
            strftime(buf, sizeof(buf), "%x %R", &tm);
            gtk_label_set_text(data->atime, buf);
        }
        else
        {
            gtk_widget_destroy(data->atime_label);
            gtk_widget_destroy(GTK_WIDGET(data->atime));
        }
        atime = fm_file_info_get_ctime(data->fi);
        if(atime > 0 && data->ctime)
        {
            localtime_r(&atime, &tm);
            strftime(buf, sizeof(buf), "%x %R", &tm);
            gtk_label_set_text(data->ctime, buf);
            gtk_widget_show(data->ctime_label);
            gtk_widget_show(GTK_WIDGET(data->ctime));
        }
        if (!fm_file_info_can_set_name(data->fi) ||
            fm_file_info_is_shortcut(data->fi))
        {
            /* changing file name isn't available, disable entry */
            gtk_widget_set_can_focus(GTK_WIDGET(data->name), FALSE);
            gtk_editable_set_editable(GTK_EDITABLE(data->name), FALSE);
        }
    }
    else
    {
        gtk_entry_set_text(data->name, _("Multiple files"));
        gtk_widget_set_sensitive(GTK_WIDGET(data->name), FALSE);
    }

    update_permissions(data);

    _on_timeout(data);
}

static void init_application_list(FmFilePropData* data)
{
    if(data->single_type && data->mime_type)
    {
        if(!fm_file_info_is_dir(data->fi))
            fm_app_chooser_combo_box_setup_for_mime_type(data->open_with, data->mime_type);
        else /* shouldn't allow set file association for folders. */
        {
            gtk_widget_destroy(data->open_with_label);
            gtk_widget_destroy(GTK_WIDGET(data->open_with));
            gtk_table_set_row_spacing(data->general_table, 5, 0);
            data->open_with = NULL;
            data->open_with_label = NULL;
        }
    }
}

/**
 * fm_file_properties_widget_new
 * @files: list of files
 * @toplevel: choose appearance of dialog
 *
 * Creates new dialog widget for change properties of @files.
 *
 * Returns: (transfer full): a new widget.
 *
 * Since: 0.1.0
 */
GtkDialog* fm_file_properties_widget_new(FmFileInfoList* files, gboolean toplevel)
{
    GtkBuilder* builder=gtk_builder_new();
    GtkDialog* dlg;
    FmFilePropData* data;
    FmPathList* paths;

    gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);
    data = g_slice_new0(FmFilePropData);

    data->files = fm_file_info_list_ref(files);
    data->single_type = fm_file_info_list_is_same_type(files);
    data->single_file = (fm_file_info_list_get_length(files) == 1);
    data->fi = fm_file_info_list_peek_head(files);
    if(data->single_type)
        data->mime_type = fm_mime_type_ref(fm_file_info_get_mime_type(data->fi));
    paths = fm_path_list_new_from_file_info_list(files);
    data->dc_job = fm_deep_count_job_new(paths, FM_DC_JOB_DEFAULT);
    fm_path_list_unref(paths);
    data->ext = NULL; /* no extension by default */
    data->extdata = NULL;

    if(toplevel)
    {
        gtk_builder_add_from_file(builder, UI_FILE, NULL);
        GET_WIDGET(GTK_DIALOG,dlg);
        gtk_dialog_set_alternative_button_order(GTK_DIALOG(data->dlg), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
    }
    else
    {
        /* FIXME: is this really useful? */
        char* names[]={"notebook", NULL};
        gtk_builder_add_objects_from_file(builder, UI_FILE, names, NULL);
        data->dlg = GTK_DIALOG(gtk_builder_get_object(builder, "notebook"));
    }

    dlg = data->dlg;

    GET_WIDGET(GTK_TABLE,general_table);
    GET_WIDGET(GTK_IMAGE,icon);
    GET_WIDGET(GTK_WIDGET,icon_eventbox);
    GET_WIDGET(GTK_ENTRY,name);
    GET_WIDGET(GTK_LABEL,file);
    GET_WIDGET(GTK_LABEL,file_label);
    GET_WIDGET(GTK_LABEL,dir);
    GET_WIDGET(GTK_LABEL,target);
    GET_WIDGET(GTK_WIDGET,target_label);
    GET_WIDGET(GTK_LABEL,type);
    GET_WIDGET(GTK_WIDGET,open_with_label);
    GET_WIDGET(GTK_COMBO_BOX,open_with);
    GET_WIDGET(GTK_LABEL,total_files);
    GET_WIDGET(GTK_WIDGET,total_files_label);
    GET_WIDGET(GTK_LABEL,total_size);
    GET_WIDGET(GTK_LABEL,size_on_disk);
    GET_WIDGET(GTK_LABEL,mtime);
    GET_WIDGET(GTK_WIDGET,mtime_label);
    GET_WIDGET(GTK_LABEL,atime);
    GET_WIDGET(GTK_WIDGET,atime_label);
    GET_WIDGET(GTK_LABEL,ctime);
    GET_WIDGET(GTK_WIDGET,ctime_label);

    GET_WIDGET(GTK_WIDGET,permissions_tab);
    GET_WIDGET(GTK_ENTRY,owner);
    GET_WIDGET(GTK_ENTRY,group);

    GET_WIDGET(GTK_COMBO_BOX,read_perm);
    GET_WIDGET(GTK_COMBO_BOX,write_perm);
    GET_WIDGET(GTK_LABEL,exec_label);
    GET_WIDGET(GTK_COMBO_BOX,exec_perm);
    GET_WIDGET(GTK_LABEL,flags_label);
    GET_WIDGET(GTK_COMBO_BOX,flags_set_file);
    GET_WIDGET(GTK_COMBO_BOX,flags_set_dir);
    GET_WIDGET(GTK_WIDGET,hidden);

    init_application_list(data);

    data->timeout = gdk_threads_add_timeout(600, on_timeout, data);
    g_signal_connect(dlg, "response", G_CALLBACK(on_response), data);
    g_signal_connect_swapped(dlg, "destroy", G_CALLBACK(fm_file_prop_data_free), data);
    g_signal_connect(data->dc_job, "finished", G_CALLBACK(on_finished), data);

    g_signal_connect(data->icon_eventbox, "button-press-event",
                     G_CALLBACK(_icon_click_event), data);
    g_signal_connect(data->icon_eventbox, "key-press-event",
                     G_CALLBACK(_icon_press_event), data);

    if (!fm_job_run_async(FM_JOB(data->dc_job)))
    {
        g_object_unref(data->dc_job);
        data->dc_job = NULL;
        g_critical("failed to run scanning job for file properties dialog");
    }

    update_ui(data);

    /* if we got some extension then activate it updating dialog window */
    if(data->ext != NULL)
    {
        GSList *l, *l2;
        for (l = data->ext, l2 = data->extdata; l; l = l->next, l2 = l2->next)
            l2->data = ((FmFilePropExt*)l->data)->cb.init(builder, data, data->files);
    }
    /* add this after all updates from extensions was made */
    if (gtk_widget_get_can_focus(data->icon_eventbox))
    {
        /* the dialog isn't realized yet so set cursor in callback */
        g_signal_connect(data->icon_eventbox, "enter-notify-event",
                         G_CALLBACK(on_icon_enter_notify), data);
    }

    g_object_unref(builder);

    return dlg;
}

/**
 * fm_show_file_properties
 * @parent: a window to put dialog over it
 * @files:list of files
 *
 * Creates and shows file properties dialog for @files.
 *
 * Returns: %TRUE.
 *
 * Since: 0.1.0
 */
gboolean fm_show_file_properties(GtkWindow* parent, FmFileInfoList* files)
{
    GtkDialog* dlg = fm_file_properties_widget_new(files, TRUE);
    if(parent)
        gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
    gtk_widget_show(GTK_WIDGET(dlg));
    g_signal_connect_after(dlg, "response", G_CALLBACK(gtk_widget_destroy), NULL);
    return TRUE;
}

/**
 * fm_file_properties_add_for_mime_type
 * @mime_type: mime type to add handler for
 * @callbacks: table of handler callbacks
 *
 * Adds a handler for some mime type into file properties dialog. The
 * handler will be used if file properties dialog is opened for single
 * file or for few files of the same type to extend its functionality.
 * The value "*" of @mime_type has special meaning - the handler will
 * be used for file types where no other extension is applied. No
 * wildcards are allowed otherwise.
 *
 * Returns: %TRUE if handler was added successfully.
 *
 * Since: 1.2.0
 */
gboolean fm_file_properties_add_for_mime_type(const char *mime_type,
                                              FmFilePropertiesExtensionInit *callbacks)
{
    FmMimeType *type;
    FmFilePropExt *ext;

    /* validate input */
    if(!mime_type || !callbacks || !callbacks->init || !callbacks->finish)
        return FALSE;
    if(strcmp(mime_type, "*") == 0)
        type = NULL;
    else
        type = fm_mime_type_from_name(mime_type);
    ext = g_slice_new(FmFilePropExt);
    ext->type = type;
    ext->next = extensions;
    ext->cb = *callbacks;
    extensions = ext;
    return TRUE;
}

/* modules support here */
FM_MODULE_DEFINE_TYPE(gtk_file_prop, FmFilePropertiesExtensionInit, 1)

static gboolean fm_module_callback_gtk_file_prop(const char *name, gpointer init, int ver)
{
    /* we don't test version yet since there is only version 1 */
    return fm_file_properties_add_for_mime_type(name, init);
}

void _fm_file_properties_init(void)
{
    fm_module_register_gtk_file_prop();
}

void _fm_file_properties_finalize(void)
{
    FmFilePropExt *ext;

    fm_module_unregister_type("gtk_file_prop");
    /* free all extensions */
    while ((ext = extensions))
    {
        extensions = ext->next;
        if (ext->type)
            fm_mime_type_unref(ext->type);
        g_slice_free(FmFilePropExt, ext);
    }
}
