/*
 *      fm-path-entry.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2009 Jürgen Hötzel <juergen@archlinux.org>
 *      Copyright 2012-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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
 * SECTION:fm-path-entry
 * @short_description: An entry to enter path with completion.
 * @title: FmPathEntry
 *
 * @include: libfm/fm-gtk.h
 *
 * The #FmPathEntry represents a widget to enter folder path for changing
 * current directory. The path entry supports completion and can be used
 * for both UNIX path or file URI entering. The path is represented in
 * the entry unescaped therefore there is no way to enter escape sequence
 * (such as \%23) into entry.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>

#include "../gtk-compat.h"

#include "fm-path-entry.h"
/* for completion */
#include "fm-folder-model.h"
#include "fm-file.h"
#include "fm-utils.h"

#include <string.h>
#include <gio/gio.h>
#include <gdk/gdkkeysyms.h>

struct _FmPathEntry
{
  GtkEntry parent_instance;
};

struct _FmPathEntryClass
{
  GtkEntryClass parent_class;
};

/* properties */
enum
{
    PROP_0,
    PROP_HIGHLIGHT_COMPLETION_MATCH
};

typedef struct _FmPathEntryModel FmPathEntryModel;

#define FM_PATH_ENTRY_GET_PRIVATE(obj) ( G_TYPE_INSTANCE_GET_PRIVATE( (obj), FM_TYPE_PATH_ENTRY, FmPathEntryPrivate ) )

typedef struct _FmPathEntryPrivate FmPathEntryPrivate;

struct _FmPathEntryPrivate
{
    FmPath* path;
    /* model used for completion */
    FmPathEntryModel* model;

    /* name of parent dir */
    char* parent_dir;
    /* length of parent dir */
    gint parent_len;

    gboolean folder_loaded : 1;
    gboolean highlight_completion_match : 1;
    gboolean long_list : 1;
    //gboolean complete_on_load :1;
    GtkEntryCompletion* completion;
    guint id_changed;

    /* cancellable for dir listing */
    GCancellable* cancellable;

    /* length of basename typed by the user */
    gint typed_basename_len;
};

typedef struct
{
    FmPathEntry* entry;
    GFile* dir;
    GList* subdirs;
    GCancellable* cancellable;
}ListSubDirNames;

//static gboolean  fm_path_entry_grab_focus(GtkWidget *widget);
static gboolean  fm_path_entry_focus_in_event(GtkWidget *widget, GdkEventFocus *event);
static gboolean  fm_path_entry_focus_out_event(GtkWidget *widget, GdkEventFocus *event);
static void      fm_path_entry_changed(GtkEditable *editable, gpointer user_data);
static void      fm_path_entry_dispose(GObject *object);
static void      fm_path_entry_finalize(GObject *object);
static gboolean  fm_path_entry_match_func(GtkEntryCompletion   *completion,
                                          const gchar          *key,
                                          GtkTreeIter          *iter,
                                          gpointer user_data);
static void fm_path_entry_completion_render_func(GtkCellLayout *cell_layout,
                                                 GtkCellRenderer *cell,
                                                 GtkTreeModel *model,
                                                 GtkTreeIter *iter,
                                                 gpointer data);
static void fm_path_entry_set_property(GObject *object,
                                       guint prop_id,
                                       const GValue *value,
                                       GParamSpec *pspec);
static void fm_path_entry_get_property(GObject *object,
                                       guint prop_id,
                                       GValue *value,
                                       GParamSpec *pspec);

G_DEFINE_TYPE(FmPathEntry, fm_path_entry, GTK_TYPE_ENTRY)

/* customized model used for entry completion to save memory.
 * GtkEntryCompletion requires that we store full paths in the model
 * to work, but we only want to store basenames to save memory.
 * So we created a custom model to do this. */

enum {
    COL_BASENAME,
    COL_FULL_PATH,
    N_COLS
};

#define FM_TYPE_PATH_ENTRY_MODEL (fm_path_entry_model_get_type())

typedef struct _FmPathEntryModelClass FmPathEntryModelClass;

struct _FmPathEntryModel
{
    GtkListStore parent_instance;
    char* parent_dir;
};

struct _FmPathEntryModelClass
{
    GtkListStoreClass parent_class;
};

static GType fm_path_entry_model_get_type(void);
static void fm_path_entry_model_iface_init(GtkTreeModelIface *iface);
static FmPathEntryModel* fm_path_entry_model_new(const char *dir);
static void fm_path_entry_model_set_parent_dir(FmPathEntryModel *model, const char *dir);

G_DEFINE_TYPE_EXTENDED( FmPathEntryModel, fm_path_entry_model, GTK_TYPE_LIST_STORE,
                       0, G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_MODEL, fm_path_entry_model_iface_init) );

/* end declaration of the customized model. */

static GtkTreeModelIface *parent_tree_model_interface = NULL;

#if 0
static void fm_path_entry_dispatch_properties_changed(GObject *object,
                                                      guint n_pspecs,
                                                      GParamSpec **pspecs)
{
    FmPathEntry *entry = FM_PATH_ENTRY(object);
    //FmPathEntryPrivate *priv  = FM_PATH_ENTRY_GET_PRIVATE(entry);
    guint i;

    G_OBJECT_CLASS(fm_path_entry_parent_class)->dispatch_properties_changed(object, n_pspecs, pspecs);

    /* What we are after: The text in front of the cursor was modified.
     * Unfortunately, there's no other way to catch this. */

    for(i = 0; i < n_pspecs; i++)
    {
        if(pspecs[i]->name == g_intern_static_string("cursor-position") ||
           pspecs[i]->name == g_intern_static_string("selection-bound") ||
           pspecs[i]->name == g_intern_static_string("text"))
        {
            //priv->complete_on_load = FALSE;
            break;
        }
    }
}
#endif

static gboolean _path_entry_is_single_match(FmPathEntry *entry, FmPathEntryPrivate *priv)
{
    GtkTreeModel *model = GTK_TREE_MODEL(priv->model);
    char *model_basename;
    const char* typed_basename;
    GtkTreeIter it;
    gboolean partial = FALSE, match = FALSE;

    typed_basename = gtk_entry_get_text(GTK_ENTRY(entry)) + priv->parent_len;
    if (gtk_tree_model_get_iter_first(model, &it)) do
    {
        gtk_tree_model_get(model, &it, COL_BASENAME, &model_basename, -1);
        if (!match && strcmp(model_basename, typed_basename) == 0)
            match = TRUE; /* exact match */
        else if (g_str_has_prefix(model_basename, typed_basename))
            partial = TRUE;
        g_free(model_basename);
    }
    while (!partial && gtk_tree_model_iter_next(model, &it));

    return (match && !partial);
}

static gboolean fm_path_entry_key_press(GtkWidget   *widget, GdkEventKey *event, gpointer user_data)
{
    FmPathEntry *entry = FM_PATH_ENTRY(widget);
    FmPathEntryPrivate *priv  = FM_PATH_ENTRY_GET_PRIVATE(entry);
    GdkModifierType state;

    if(gtk_get_current_event_state(&state) &&
       (state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
        return FALSE;

    switch( event->keyval )
    {
    case GDK_KEY_Tab:
        {
#if 0
            gtk_editable_get_selection_bounds(editable, &start, &end);
            if(start != end)
                gtk_editable_set_position(editable, MAX(start, end));
            else
                start_explicit_completion(entry);
#endif
            gtk_entry_completion_insert_prefix(priv->completion);
            gtk_editable_set_position(GTK_EDITABLE(entry), -1);
            if (_path_entry_is_single_match(entry, priv))
            {
                int pos = gtk_editable_get_position(GTK_EDITABLE(entry));
                gtk_editable_insert_text(GTK_EDITABLE(entry), "/", 1, &pos);
                gtk_editable_set_position(GTK_EDITABLE(entry), pos);
            }
            return TRUE;
        }
    }
    return FALSE;
}

static void _set_entry_text_from_path(GtkEntry *entry, FmPathEntryPrivate *priv)
{
    char *disp_name;
    FmPath *path = priv->path;

    disp_name = fm_path_display_name(path, FALSE);
    /* block our handler for "changed" signal, we'll update it below */
    if(priv->id_changed > 0)
        g_signal_handler_block(entry, priv->id_changed);
    gtk_entry_set_text(entry, disp_name);
    if(priv->id_changed > 0)
        g_signal_handler_unblock(entry, priv->id_changed);
    /* update list of items now */
    fm_path_entry_changed(GTK_EDITABLE(entry), NULL);
    g_free(disp_name);
}

static void fm_path_entry_activate(GtkEntry *entry, gpointer user_data)
{
    FmPathEntryPrivate *priv  = FM_PATH_ENTRY_GET_PRIVATE(entry);
    const char* full_path;
    /* convert current path string to FmPath here */

    full_path = gtk_entry_get_text(entry);
    if(priv->path)
        fm_path_unref(priv->path);

    /* special handling for home dir */
    if(full_path[0] == '~' && full_path[1] == G_DIR_SEPARATOR)
        priv->path = fm_path_new_relative(fm_path_get_home(), full_path + 2);
    else if(full_path[0] == '~' && full_path[1] == 0)
        priv->path = fm_path_ref(fm_path_get_home());
    else
        priv->path = fm_path_new_for_display_name(full_path);

#if G_ENABLE_DEBUG
    {
        char *real_path = fm_path_to_str(priv->path);
        g_debug("FmPathEntry activated: '%s' => '%s'", full_path, real_path);
        g_free(real_path);
    }
#endif

    _set_entry_text_from_path(entry, priv);

    gtk_editable_set_position(GTK_EDITABLE(entry), -1);
}

static void fm_path_entry_class_init(FmPathEntryClass *klass)
{
    GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);

    object_class->get_property = fm_path_entry_get_property;
    object_class->set_property = fm_path_entry_set_property;
    /**
     * FmPathEntry:highlight-completion-match:
     *
     * The #FmPathEntry:highlight-completion-match property is the flag
     * whether the completion match should be highlighted or not.
     *
     * Since: 0.1.0
     */
    g_object_class_install_property( object_class,
                                    PROP_HIGHLIGHT_COMPLETION_MATCH,
                                    g_param_spec_boolean("highlight-completion-match",
                                                         "Highlight completion match",
                                                         "Whether to highlight the completion match",
                                                         TRUE, G_PARAM_READWRITE) );
    object_class->dispose = fm_path_entry_dispose;
    object_class->finalize = fm_path_entry_finalize;
    /* object_class->dispatch_properties_changed = fm_path_entry_dispatch_properties_changed; */

    widget_class->focus_in_event = fm_path_entry_focus_in_event;
    /* widget_class->grab_focus = fm_path_entry_grab_focus; */
    widget_class->focus_out_event = fm_path_entry_focus_out_event;

    g_type_class_add_private( klass, sizeof (FmPathEntryPrivate) );
}

static inline void update_inline_completion(FmPathEntryPrivate* priv)
{
    gtk_entry_completion_set_inline_completion(priv->completion,
                                               priv->folder_loaded);
}

static void clear_completion(FmPathEntryPrivate* priv)
{
    if(priv->model)
    {
        priv->parent_len = 0;
        fm_path_entry_model_set_parent_dir(priv->model, NULL);
        g_free(priv->parent_dir);
        priv->parent_dir = NULL;
        /* cancel running dir-listing jobs */
        if(priv->cancellable)
        {
            g_cancellable_cancel(priv->cancellable);
            g_object_unref(priv->cancellable);
            priv->cancellable = NULL;
        }
        /* clear current model */
        gtk_list_store_clear(GTK_LIST_STORE(priv->model));
        update_inline_completion(priv);
    }
    priv->typed_basename_len = 0;
}

static gboolean  fm_path_entry_focus_in_event(GtkWidget *widget, GdkEventFocus *event)
{
    FmPathEntry *entry = FM_PATH_ENTRY(widget);
    FmPathEntryPrivate *priv  = FM_PATH_ENTRY_GET_PRIVATE(entry);

    /* listen to 'changed' signal for auto-completion */
    priv->id_changed = g_signal_connect(entry, "changed",
                                        G_CALLBACK(fm_path_entry_changed), NULL);

    return GTK_WIDGET_CLASS(fm_path_entry_parent_class)->focus_in_event(widget, event);
}

#if 0
static void gtk_file_chooser_entry_grab_focus(GtkWidget *widget)
{
    GTK_WIDGET_CLASS(fm_path_entry_parent_class)->grab_focus(widget);
    gtk_editable_select_region(GTK_EDITABLE(widget), 0, (gint)-1);
}
#endif

static gboolean fm_path_entry_focus_out_event(GtkWidget *widget, GdkEventFocus *event)
{
    FmPathEntry *entry = FM_PATH_ENTRY(widget);
    FmPathEntryPrivate *priv  = FM_PATH_ENTRY_GET_PRIVATE(entry);

    /* disconnect from 'changed' signal since we don't do auto-completion
     * when we have no keyboard focus. */
    priv->id_changed = 0;
    g_signal_handlers_disconnect_by_func(entry, fm_path_entry_changed, NULL);

    return GTK_WIDGET_CLASS(fm_path_entry_parent_class)->focus_out_event(widget, event);
}

#if GLIB_CHECK_VERSION(2, 36, 0)
static void on_dir_list_finished(GObject *source_object, GAsyncResult *res,
                                 gpointer user_data)
#else
static gboolean on_dir_list_finished(gpointer user_data)
#endif
{
    ListSubDirNames* data = (ListSubDirNames*)user_data;
    FmPathEntry* entry = data->entry;
    FmPathEntryPrivate *priv  = FM_PATH_ENTRY_GET_PRIVATE(entry);
    GList* l;
    FmPathEntryModel* new_model;

    /* final chance to check cancellable */
    if(g_cancellable_is_cancelled(data->cancellable))
#if GLIB_CHECK_VERSION(2, 36, 0)
        return;
#else
        return TRUE;
#endif
    /* FIXME: check errors! */

    new_model = fm_path_entry_model_new(priv->parent_dir);
    /* g_debug("dir list is finished!"); */

    /* update the model */
    for(l = data->subdirs; l; l=l->next)
    {
        char* name = l->data;
        gtk_list_store_insert_with_values((GtkListStore*)new_model, NULL, -1, COL_BASENAME, name, -1);
    }
    priv->folder_loaded = TRUE;
    priv->long_list = (g_list_length(data->subdirs) > 40);

    gtk_entry_completion_set_model(priv->completion, GTK_TREE_MODEL(new_model));
    if(priv->model)
        g_object_unref(priv->model);
    priv->model = new_model;
    //if(entry->complete_on_load)
        //explicitly_complete(entry);
    update_inline_completion(priv);
    gtk_entry_completion_insert_prefix(priv->completion);
    gtk_entry_completion_complete(priv->completion);

    /* NOTE: after the content of entry gets changed, by default gtk+ installs
     * an timeout handler with timeout 300 ms to popup completion list.
     * If the dir listing takes more than 300 ms and finished after the
     * timeout callback is called, then the completion list is empty at
     * that time. So completion doesn't work. So, we trigger a 'changed'
     * signal here to let GtkEntry do the completion with the new model again. */

    /* trigger completion popup. FIXME: this is a little bit dirty.
     * A even more dirty thing to do is to check if we finished after
     * 300 ms timeout happens. */
    g_signal_emit_by_name(entry, "changed", 0);
#if !GLIB_CHECK_VERSION(2, 36, 0)
    return TRUE;
#endif
}

#if GLIB_CHECK_VERSION(2, 36, 0)
static void list_sub_dirs(GTask *task, gpointer source_object, gpointer user_data,
                          GCancellable *cancellable)
#else
static gboolean list_sub_dirs(GIOSchedulerJob *job, GCancellable *cancellable, gpointer user_data)
#endif
{
    ListSubDirNames* data = (ListSubDirNames*)user_data;
    GError *err = NULL;
    /* g_debug("new dir listing job!"); */
    GFileEnumerator* enu = g_file_enumerate_children(data->dir,
                                    G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME","
                                    G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME","
                                    G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                    G_FILE_QUERY_INFO_NONE, cancellable,
                                    NULL);
    if(enu)
    {
        while(!g_cancellable_is_cancelled(cancellable))
        {
            GFileInfo* inf = g_file_enumerator_next_file(enu, cancellable, &err);
            if(inf)
            {
                GFileType type = g_file_info_get_file_type(inf);
                if(type == G_FILE_TYPE_DIRECTORY)
                {
                    const char* name = g_file_info_get_edit_name(inf);
                    if (!name)
                        name = g_file_info_get_display_name(inf);
                    data->subdirs = g_list_prepend(data->subdirs, g_strdup(name));
                }
                g_object_unref(inf);
            }
            else
            {
                if(err) /* error happens */
                    g_clear_error(&err);
                else /* EOF */
                    break;
            }
        }
        g_object_unref(enu);
    }

    if(!g_cancellable_is_cancelled(cancellable))
#if GLIB_CHECK_VERSION(2, 36, 0)
        g_task_return_pointer(task, NULL, NULL);
#else
    {
        /* finished! */
        g_io_scheduler_job_send_to_mainloop(job, on_dir_list_finished, data, NULL);
    }
    return FALSE;
#endif
}

static void list_sub_dir_names_free(gpointer user_data)
{
    ListSubDirNames* data = (ListSubDirNames*)user_data;
    g_object_unref(data->dir);
    g_object_unref(data->cancellable);
    g_list_foreach(data->subdirs, (GFunc)g_free, NULL);
    g_list_free(data->subdirs);
    g_slice_free(ListSubDirNames, data);
}

static void fm_path_entry_changed(GtkEditable *editable, gpointer user_data)
{
    FmPathEntry *entry = FM_PATH_ENTRY(editable);
    FmPathEntryPrivate *priv  = FM_PATH_ENTRY_GET_PRIVATE(entry);
    const gchar *path_str, *sep;
#if GLIB_CHECK_VERSION(2, 36, 0)
    GTask *task;
#endif

    if(priv->model == NULL)
        return;
    /* find parent dir of current path */
    path_str = gtk_entry_get_text( GTK_ENTRY(entry) );
    sep = g_utf8_strrchr(path_str, -1, G_DIR_SEPARATOR);
    if(sep) /* we found a parent dir */
    {
        int parent_len = (sep - path_str) + 1; /* includes the dir separator / */
        if(!priv->parent_dir
           || priv->parent_len != parent_len
           || strncmp(priv->parent_dir, path_str, parent_len ))
        {
            /* parent dir has been changed, reload dir list */
            ListSubDirNames* data = g_slice_new0(ListSubDirNames);
            priv->folder_loaded = FALSE;
            clear_completion(priv);
            priv->parent_dir = g_strndup(path_str, parent_len);
            priv->parent_len = parent_len;
            fm_path_entry_model_set_parent_dir(priv->model, priv->parent_dir);
            /* g_debug("parent dir is changed to %s", priv->parent_dir); */

            /* FIXME: convert utf-8 encoded path to on-disk encoding. */
            data->entry = entry;
            if(priv->parent_dir[0] == '~') /* special case for home dir */
            {
                char* expand = g_strconcat(fm_get_home_dir(), priv->parent_dir + 1, NULL);
                data->dir = fm_file_new_for_commandline_arg(expand);
                g_free(expand);
            }
            else
            {
                FmPath *p = fm_path_new_for_display_name(priv->parent_dir);
                data->dir = fm_path_to_gfile(p);
                fm_path_unref(p);
            }

            /* launch a new job to do dir listing */
            if (G_LIKELY(priv->cancellable == NULL))
                priv->cancellable = g_cancellable_new();
            data->cancellable = (GCancellable*)g_object_ref(priv->cancellable);
#if GLIB_CHECK_VERSION(2, 36, 0)
            task = g_task_new(editable, data->cancellable, on_dir_list_finished, data);
            g_task_set_task_data(task, data, list_sub_dir_names_free);
            g_task_set_priority(task, G_PRIORITY_LOW);
            g_task_run_in_thread(task, list_sub_dirs);
            g_object_unref(task);
#else
            g_io_scheduler_push_job(list_sub_dirs,
                                    data, list_sub_dir_names_free,
                                    G_PRIORITY_LOW, data->cancellable);
#endif
        }
        /* calculate the length of remaining part after / */
        priv->typed_basename_len = strlen(sep + 1);
    }
    else /* clear all autocompletion thing. */
        clear_completion(priv);
}

static void fm_path_entry_set_property(GObject *object,
                                       guint prop_id,
                                       const GValue *value,
                                       GParamSpec *pspec)
{
    FmPathEntry *entry = FM_PATH_ENTRY(object);
    FmPathEntryPrivate *priv  = FM_PATH_ENTRY_GET_PRIVATE(entry);

    switch( prop_id )
    {
    case PROP_HIGHLIGHT_COMPLETION_MATCH:
        priv->highlight_completion_match = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void fm_path_entry_get_property(GObject *object,
                                       guint prop_id,
                                       GValue *value,
                                       GParamSpec *pspec)
{
    FmPathEntry *entry = FM_PATH_ENTRY(object);
    FmPathEntryPrivate *priv  = FM_PATH_ENTRY_GET_PRIVATE(entry);

    switch( prop_id ) {
    case PROP_HIGHLIGHT_COMPLETION_MATCH:
        g_value_set_boolean(value, priv->highlight_completion_match);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void fm_path_entry_paste_and_go(GtkMenuItem *menuitem, GtkEntry *entry)
{
    GtkClipboard* clipboard = gtk_clipboard_get_for_display(
        gtk_widget_get_display(GTK_WIDGET(menuitem)), GDK_SELECTION_CLIPBOARD);

    gchar* full_path = gtk_clipboard_wait_for_text(clipboard);

    if (full_path)
    {
        FmPathEntryPrivate *priv  = FM_PATH_ENTRY_GET_PRIVATE(entry);

        if(priv->path)
            fm_path_unref(priv->path);

        /* special handling for home dir */
        if(full_path[0] == '~' && full_path[1] == G_DIR_SEPARATOR)
            priv->path = fm_path_new_relative(fm_path_get_home(), full_path + 2);
        else if(full_path[0] == '~' && full_path[1] == 0)
            priv->path = fm_path_ref(fm_path_get_home());
        else
            /* FIXME: use fm_path_new_for_display_name ? */
            priv->path = fm_path_new_for_str(full_path);

        gchar * disp_name = fm_path_display_name(priv->path, FALSE);
        gtk_entry_set_text(entry, disp_name);
        g_free(disp_name);

        gtk_editable_set_position(GTK_EDITABLE(entry), -1);

        g_free(full_path);

        g_signal_emit_by_name(entry, "activate", 0);
    }
}

static void fm_path_entry_populate_popup(GtkEntry *entry, GtkMenu *menu, gpointer user_data)
{
    GtkWidget* menuitem;

    GtkClipboard* clipboard = gtk_clipboard_get_for_display(
        gtk_widget_get_display(GTK_WIDGET(entry)), GDK_SELECTION_CLIPBOARD);

    menuitem = gtk_menu_item_new_with_mnemonic(_("Pa_ste and Go"));
    gtk_widget_show(menuitem);

    /* Insert menu item after default Paste menu item */
    gtk_menu_shell_insert(GTK_MENU_SHELL(menu), menuitem, 3);

    g_signal_connect(menuitem, "activate",
                     G_CALLBACK(fm_path_entry_paste_and_go), entry);

    if (!gtk_clipboard_wait_is_text_available(clipboard))
        gtk_widget_set_sensitive(menuitem, FALSE);
}

static void
fm_path_entry_init(FmPathEntry *entry)
{
    FmPathEntryPrivate *priv = FM_PATH_ENTRY_GET_PRIVATE(entry);
    GtkEntryCompletion* completion = gtk_entry_completion_new();
    GtkCellRenderer* render;
    AtkObject *obj;

    priv->model = fm_path_entry_model_new(NULL);
    priv->completion = completion;
    priv->cancellable = g_cancellable_new();
    priv->highlight_completion_match = TRUE;
    gtk_entry_completion_set_minimum_key_length(completion, 1);

    gtk_entry_completion_set_match_func(completion, fm_path_entry_match_func, NULL, NULL);
    g_object_set(completion, "text_column", COL_FULL_PATH, NULL);
    gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(priv->model));
    gtk_entry_set_completion(GTK_ENTRY(entry), completion);

    render = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(completion), render, TRUE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(completion), render, "text", COL_BASENAME);
    gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(completion), render, fm_path_entry_completion_render_func, entry, NULL);

    /* NOTE: this is to avoid a bug of gtk+.
     * The inline selection provided by GtkEntry is buggy.
     * If we change the content of the entry, it still stores
     * the old prefix sometimes so things don't work as expected.
     * So, unfortunately, we're not able to use this nice feature.
     *
     * Please see gtk_entry_completion_key_press() of gtk/gtkentry.c
     * and look for completion->priv->completion_prefix.
     */
    /* gtk_entry_completion_set_inline_selection(completion, FALSE); */

    /* gtk_entry_completion_set_inline_completion(completion, TRUE); */
    gtk_entry_completion_set_popup_set_width(completion, TRUE);
    gtk_entry_completion_set_popup_single_match(completion, FALSE);

    /* connect to these signals rather than overriding default handlers since
     * we want to invoke our handlers before the default ones provided by Gtk. */
    g_signal_connect(entry, "key-press-event", G_CALLBACK(fm_path_entry_key_press), NULL);
    g_signal_connect(entry, "activate", G_CALLBACK(fm_path_entry_activate), NULL);
    g_signal_connect(entry, "populate-popup", G_CALLBACK(fm_path_entry_populate_popup), NULL);

    obj = gtk_widget_get_accessible(GTK_WIDGET(entry));
    atk_object_set_description(obj, _("Folder location bar"));
}

static void fm_path_entry_completion_render_func(GtkCellLayout *cell_layout,
                                                 GtkCellRenderer *cell,
                                                 GtkTreeModel *model,
                                                 GtkTreeIter *iter,
                                                 gpointer data)
{
    gchar *model_file_name;
    int model_file_name_len;
    FmPathEntryPrivate *priv = FM_PATH_ENTRY_GET_PRIVATE( FM_PATH_ENTRY(data) );
    gtk_tree_model_get(model, iter, COL_BASENAME, &model_file_name, -1);
    model_file_name_len = strlen(model_file_name);

    if( priv->highlight_completion_match && (model_file_name_len >= priv->typed_basename_len) )
    {
        int buf_len = model_file_name_len + 14 + 1;
        gchar* markup = g_malloc(buf_len);
        gchar *trail = g_stpcpy(markup, "<b><u>");
        strncpy(trail, model_file_name, priv->typed_basename_len);
        trail += priv->typed_basename_len;
        trail = g_stpcpy(trail, "</u></b>");
        trail = g_stpcpy(trail, model_file_name + priv->typed_basename_len);
        g_object_set(cell, "markup", markup, NULL);
        g_free(markup);
    }
    /* FIXME: We don't need a custom render func if we don't hightlight */
    else
        g_object_set(cell, "text", model_file_name, NULL);
    g_free(model_file_name);
}

static void fm_path_entry_dispose(GObject *object)
{
    FmPathEntryPrivate* priv = FM_PATH_ENTRY_GET_PRIVATE(object);

    g_signal_handlers_disconnect_by_func(object, fm_path_entry_key_press, NULL);
    g_signal_handlers_disconnect_by_func(object, fm_path_entry_activate, NULL);

    gtk_entry_set_completion(GTK_ENTRY(object), NULL);
    clear_completion(priv);

    if(priv->completion)
    {
        gtk_entry_completion_set_model(priv->completion, NULL);
        g_object_unref(priv->completion);
        priv->completion = NULL;
    }

    if(priv->path)
    {
        fm_path_unref(priv->path);
        priv->path = NULL;
    }

    if(priv->model)
    {
        g_object_unref(priv->model);
        priv->model = NULL;
    }

    if(priv->cancellable)
    {
        g_cancellable_cancel(priv->cancellable);
        g_object_unref(priv->cancellable);
        priv->cancellable = NULL;
    }

    G_OBJECT_CLASS(fm_path_entry_parent_class)->dispose(object);
}

static void
fm_path_entry_finalize(GObject *object)
{
    FmPathEntryPrivate* priv = FM_PATH_ENTRY_GET_PRIVATE(object);

    g_free(priv->parent_dir);

    (*G_OBJECT_CLASS(fm_path_entry_parent_class)->finalize)(object);
}

/**
 * fm_path_entry_new
 *
 * Creates new path entry widget.
 *
 * Returns: (transfer full): a new #FmPathEntry object.
 *
 * Since: 0.1.0
 */
FmPathEntry* fm_path_entry_new(void)
{
    return g_object_new(FM_TYPE_PATH_ENTRY, NULL);
}

/**
 * fm_path_entry_set_path
 * @entry: a widget to apply
 * @path: new path to set
 *
 * Sets new path into enter field.
 *
 * Since: 0.1.10
 */
void fm_path_entry_set_path(FmPathEntry *entry, FmPath* path)
{
    FmPathEntryPrivate *priv = FM_PATH_ENTRY_GET_PRIVATE(entry);

    if(priv->path)
        fm_path_unref(priv->path);

    if(path)
    {
        priv->path = fm_path_ref(path);
        _set_entry_text_from_path(GTK_ENTRY(entry), priv);
    }
    else
    {
        priv->path = NULL;
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
}

static gboolean fm_path_entry_match_func(GtkEntryCompletion   *completion,
                                         const gchar          *key,
                                         GtkTreeIter          *iter,
                                         gpointer user_data)
{
    gboolean ret;
    GtkTreeModel *model = gtk_entry_completion_get_model(completion);
    FmPathEntry *entry = FM_PATH_ENTRY( gtk_entry_completion_get_entry(completion) );
    FmPathEntryPrivate *priv = FM_PATH_ENTRY_GET_PRIVATE(entry);
    char *model_basename;
    const char* typed_basename;
    /* we don't use the case-insensitive key provided by entry completion here */
    typed_basename = gtk_entry_get_text(GTK_ENTRY(entry)) + priv->parent_len;
    gtk_tree_model_get(model, iter, COL_BASENAME, &model_basename, -1);

    if (G_UNLIKELY(model_basename == NULL))
        ret = FALSE; /* it is invalid if file name is empty but it's possible! */
    else if(model_basename[0] == '.' && typed_basename[0] != '.')
        ret = FALSE; /* ignore hidden files when not requested. */
    else if(priv->long_list && /* don't create too long lists */
            (typed_basename[0] == '\0' /* "/xxx/" - no names here yet */
             || (typed_basename[0] == '.' && typed_basename[1] == '\0'))) /* "/xxx/." */
        ret = FALSE;
    else
        ret = g_str_has_prefix(model_basename, typed_basename); /* FIXME: should we be case insensitive here? */
    g_free(model_basename);
    return ret;
}


/**
 * fm_path_entry_get_path
 * @entry: the widget to inspect
 *
 * Retrieves the current path in the @entry. Returned data are owned by
 * @entry and should be not freed by caller.
 *
 * Returns: (transfer none): the current path.
 *
 * Since: 0.1.10
 */
FmPath* fm_path_entry_get_path(FmPathEntry *entry)
{
    FmPathEntryPrivate *priv = FM_PATH_ENTRY_GET_PRIVATE(entry);
    return priv->path;
}


/* ------------------------------------------------------------------------
 * custom tree model implementation. */

static void fm_path_entry_model_init(FmPathEntryModel *model)
{
    GType cols[] = {G_TYPE_STRING, G_TYPE_STRING};
    gtk_list_store_set_column_types(GTK_LIST_STORE(model), G_N_ELEMENTS(cols), cols);
}

static void fm_path_entry_model_finalize(GObject *object)
{
    g_free(((FmPathEntryModel*)object)->parent_dir);

    (*G_OBJECT_CLASS(fm_path_entry_model_parent_class)->finalize)(object);
}

static void fm_path_entry_model_class_init(FmPathEntryModelClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = fm_path_entry_model_finalize;
}

static void fm_path_entry_model_get_value(GtkTreeModel *tree_model,
                                          GtkTreeIter  *iter,
                                          gint          column,
                                          GValue       *value)
{
    FmPathEntryModel *model = (FmPathEntryModel*)tree_model;
    if(column == COL_FULL_PATH)
    {
        char* full_path;
        parent_tree_model_interface->get_value(tree_model, iter, COL_BASENAME, value);
        full_path = g_strconcat(model->parent_dir, g_value_get_string(value), NULL);
        g_value_take_string(value, full_path);
    }
    else
        parent_tree_model_interface->get_value(tree_model, iter, column, value);
}

static void fm_path_entry_model_iface_init(GtkTreeModelIface *iface)
{
    parent_tree_model_interface = g_type_interface_peek_parent(iface);
    iface->get_value = fm_path_entry_model_get_value;
}

static void fm_path_entry_model_set_parent_dir(FmPathEntryModel* model, const char *dir)
{
    g_free(model->parent_dir);
    model->parent_dir = dir ? g_strdup(dir) : NULL;
}

static FmPathEntryModel* fm_path_entry_model_new(const char *parent_dir)
{
    FmPathEntryModel* model = g_object_new(FM_TYPE_PATH_ENTRY_MODEL, NULL);
    fm_path_entry_model_set_parent_dir(model, parent_dir);
    return model;
}
