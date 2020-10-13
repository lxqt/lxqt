/*
 *      fm-dnd-dest.c
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
 * SECTION:fm-dnd-dest
 * @short_description: Libfm support for drag&drop destination.
 * @title: FmDndDest
 *
 * @include: libfm/fm-gtk.h
 *
 * The #FmDndDest can be used by some widget to provide support for Drop
 * operations onto that widget.
 *
 * To use #FmDndDest the widget should create it - the simplest API for
 * this is fm_dnd_dest_new_with_handlers(). When #FmDndDest is created
 * some drop data types ("targets") are set for the widget. The widget
 * can extend the list by adding own targets to the list and connecting
 * own handlers to the #GtkWidget::drag-leave, #GtkWidget::drag-drop,
 * and #GtkWidget::drag-data-received signals.
 *
 * The #GtkWidget::drag-motion signal should be always handled by the
 * widget. The handler should check if drop can be performed. And if
 * #FmDndDest can accept the drop then widget should inform #FmDndDest
 * object about #FmFileInfo object the mouse pointer targets at that
 * moment by calling fm_dnd_dest_set_dest_file(). The #FmDndDest uses a
 * little different sequence for collecting dragged data - it queries
 * data in time of drag motion and uses when data are dropped therefore
 * widget should always call API fm_dnd_dest_get_default_action() from
 * handler of the #GtkWidget::drag-motion signal for any target which
 * #FmDndDest supports.
 * <example id="example-fmdnddest-usage">
 * <title>Sample Usage</title>
 * <programlisting>
 * {
 *    widget->dd = fm_dnd_dest_new_with_handlers(widget);
 *    g_signal_connect(widget, "drag-motion", G_CALLBACK(on_drag_motion), dd);
 *
 *    ...
 * }
 *
 * static void on_object_finalize(MyWidget *widget)
 * {
 *    ...
 *
 *    g_object_unref(G_OBJECT(widget->dd));
 * }
 *
 * static gboolean on_drag_motion(MyWidget *widget, GdkDragContext *drag_context,
 *                                gint x, gint y, guint time, FmDndDest *dd)
 * {
 *    GdkAtom target;
 *    GdkDragAction action = 0;
 *    FmFileInfo *file_info;
 *
 *    file_info = my_widget_find_file_at_coords(widget, x, y);
 *    fm_dnd_dest_set_dest_file(widget->dd, file_info);
 *    if (file_info == NULL)
 *       return FALSE; /&ast; not in drop zone &ast;/
 *    target = fm_dnd_dest_find_target(widget->dd, drag_context);
 *    if (target != GDK_NONE && fm_dnd_dest_is_target_supported(widget->dd, target))
 *       action = fm_dnd_dest_get_default_action(widget->dd, drag_context, target);
 *    if (action == 0)
 *       return FALSE; /&ast; cannot drop on that destination &ast;/
 *    gdk_drag_status(drag_context, action, time);
 *    return TRUE;
 * }
 * </programlisting>
 * </example>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "gtk-compat.h"

#include "fm-dnd-dest.h"
#include "fm-gtk-utils.h"
#include "fm-gtk-marshal.h"
#include "fm-file-info-job.h"
#include "fm-gtk-file-launcher.h"

#include "fm-config.h"

#include <glib/gi18n-lib.h>
#include <string.h>

struct _FmDndDest
{
    GObject parent;
    GtkWidget* widget;

    int info_type; /* type of src_files */
    FmPathList* src_files;
    GdkDragContext* context;
    guint32 src_dev; /* UNIX dev of source fs */
    const char* src_fs_id; /* filesystem id of source fs */
    FmFileInfo* dest_file;

    gboolean waiting_data;
    gboolean has_handlers;
    gboolean can_copy;
};

enum
{
//    QUERY_INFO,
    FILES_DROPPED,
    N_SIGNALS
};

GtkTargetEntry fm_default_dnd_dest_targets[] =
{
    {"application/x-fmlist-ptr", GTK_TARGET_SAME_APP, FM_DND_DEST_TARGET_FM_LIST},
    {"text/uri-list", 0, FM_DND_DEST_TARGET_URI_LIST}, /* text/uri-list */
    {"XdndDirectSave0", 0, FM_DND_DEST_TARGET_XDS} /* X direct save */
    /* TODO: add more targets to support: text types, _NETSCAPE_URL, property/bgimage ... */
};

static GdkAtom dest_target_atom[N_FM_DND_DEST_DEFAULT_TARGETS];

static void fm_dnd_dest_dispose              (GObject *object);
static gboolean fm_dnd_dest_files_dropped(FmDndDest* dd, int x, int y, guint action, guint info_type, FmPathList* files);

static void clear_src_cache(FmDndDest* dd);

static void on_drag_leave(GtkWidget *widget, GdkDragContext *drag_context,
                          guint time, FmDndDest* dd);
static gboolean on_drag_drop(GtkWidget *widget, GdkDragContext *drag_context,
                             gint x, gint y, guint time, FmDndDest* dd);
static void on_drag_data_received(GtkWidget *w, GdkDragContext *drag_context,
                                  gint x, gint y, GtkSelectionData *data,
                                  guint info, guint time, FmDndDest* dd);

static guint signals[N_SIGNALS];


G_DEFINE_TYPE(FmDndDest, fm_dnd_dest, G_TYPE_OBJECT);


static void fm_dnd_dest_class_init(FmDndDestClass *klass)
{
    GObjectClass *g_object_class;
    FmDndDestClass *dnd_dest_class;
    guint i;

    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->dispose = fm_dnd_dest_dispose;

    dnd_dest_class = FM_DND_DEST_CLASS(klass);
    dnd_dest_class->files_dropped = fm_dnd_dest_files_dropped;

    /**
     * FmDndDest::files-dropped:
     * @dd: the object which emitted the signal
     * @x: horisontal position of drop
     * @y: vertical position of drop
     * @action: (#GdkDragAction) action requested on drop
     * @info_type: (#FmDndDestTargetType) type of data that are dropped
     * @files: (#FmPathList *) list of files that are dropped
     *
     * The #FmDndDest::files-dropped signal is emitted when @files are
     * dropped on the destination widget. If handler connected to this
     * signal returns %TRUE then further emission of the signal will be
     * stopped.
     *
     * Return value: %TRUE if action can be performed.
     *
     * Since: 0.1.0
     */
    signals[ FILES_DROPPED ] =
        g_signal_new("files-dropped",
                     G_TYPE_FROM_CLASS( klass ),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET ( FmDndDestClass, files_dropped ),
                     g_signal_accumulator_true_handled, NULL,
                     fm_marshal_BOOL__INT_INT_UINT_UINT_POINTER,
                     G_TYPE_BOOLEAN, 5, G_TYPE_INT, G_TYPE_INT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_POINTER);

    for(i = 0; i < N_FM_DND_DEST_DEFAULT_TARGETS; i++)
        dest_target_atom[i] = GDK_NONE;
    for(i = 0; i < G_N_ELEMENTS(fm_default_dnd_dest_targets); i++)
        dest_target_atom[fm_default_dnd_dest_targets[i].info] =
            gdk_atom_intern_static_string(fm_default_dnd_dest_targets[i].target);
}


static void fm_dnd_dest_dispose(GObject *object)
{
    FmDndDest *dd;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_DND_DEST(object));

    dd = (FmDndDest*)object;

    fm_dnd_dest_set_widget(dd, NULL);

    clear_src_cache(dd);

    G_OBJECT_CLASS(fm_dnd_dest_parent_class)->dispose(object);
}


static void fm_dnd_dest_init(FmDndDest *self)
{

}

/**
 * fm_dnd_dest_new
 * @w: a widget that probably is drop destination
 *
 * Creates new drag destination descriptor and sets a widget as a
 * potential drop destination. Caller should connect handlers for the
 * Gtk+ Drag and Drop signals to the widget: #GtkWidget::drag-leave,
 * #GtkWidget::drag-motion, #GtkWidget::drag-drop, and
 * #GtkWidget::drag-data-received.
 *
 * Before 1.0.1 this API didn't set drop destination on widget so caller
 * should set it itself. Access to fm_default_dnd_dest_targets outside
 * of this API considered unsecure so that behavior was changed.
 *
 * See also: fm_dnd_dest_new_with_handlers().
 *
 * Returns: (transfer full): a new #FmDndDest object.
 *
 * Since: 0.1.0
 */
FmDndDest *fm_dnd_dest_new(GtkWidget* w)
{
    FmDndDest* dd = (FmDndDest*)g_object_new(FM_TYPE_DND_DEST, NULL);
    dd->has_handlers = FALSE;
    fm_dnd_dest_set_widget(dd, w);
    return dd;
}

/**
 * fm_dnd_dest_new_with_handlers
 * @w: a widget that probably is drop destination
 *
 * Creates new drag destination descriptor, sets a widget as a potential
 * drop destination, and connects handlers for the Gtk+ Drag and Drop
 * signals: #GtkWidget::drag-leave, #GtkWidget::drag-drop, and
 * #GtkWidget::drag-data-received. Caller should connect own handler for
 * the #GtkWidget::drag-motion signal to the widget to complete the
 * support.
 *
 * See also: fm_dnd_dest_new().
 *
 * Returns: (transfer full): a new #FmDndDest object.
 *
 * Since: 1.0.1
 */
FmDndDest *fm_dnd_dest_new_with_handlers(GtkWidget* w)
{
    FmDndDest* dd = (FmDndDest*)g_object_new(FM_TYPE_DND_DEST, NULL);
    dd->has_handlers = TRUE;
    fm_dnd_dest_set_widget(dd, w);
    return dd;
}

/**
 * fm_dnd_dest_set_widget
 * @dd: a drag destination descriptor
 * @w: a widget that probably is drop destination
 *
 * Updates link to widget that probably is drop destination and setups
 * widget with drop targets supported by FmDndDest.
 *
 * Before 1.0.1 this API didn't update drop destination on widget so caller
 * should set and unset it itself. Access to fm_default_dnd_dest_targets
 * outside of this API considered unsecure so that behavior was changed.
 *
 * See also: fm_dnd_dest_new(), fm_dnd_dest_new_with_handlers().
 *
 * Since: 0.1.0
 */
void fm_dnd_dest_set_widget(FmDndDest* dd, GtkWidget* w)
{
    if(w == dd->widget)
        return;
    if(dd->widget)
    {
        if(dd->has_handlers)
        {
            g_signal_handlers_disconnect_by_func(dd->widget, on_drag_drop, dd);
            g_signal_handlers_disconnect_by_func(dd->widget, on_drag_leave, dd);
            g_signal_handlers_disconnect_by_func(dd->widget, on_drag_data_received, dd);
        }
        gtk_drag_dest_unset(dd->widget);
        g_object_remove_weak_pointer(G_OBJECT(dd->widget), (gpointer*)&dd->widget);
    }
    dd->widget = w;
    if( w )
    {
        g_object_add_weak_pointer(G_OBJECT(w), (gpointer*)&dd->widget);
        gtk_drag_dest_set(w, 0, fm_default_dnd_dest_targets,
                          G_N_ELEMENTS(fm_default_dnd_dest_targets),
                          GDK_ACTION_COPY|GDK_ACTION_MOVE|GDK_ACTION_LINK|GDK_ACTION_ASK);
        if(dd->has_handlers)
        {
            g_signal_connect(w, "drag-drop", G_CALLBACK(on_drag_drop), dd);
            g_signal_connect(w, "drag-leave", G_CALLBACK(on_drag_leave), dd);
            g_signal_connect(w, "drag-data-received", G_CALLBACK(on_drag_data_received), dd);
        }
    }
}

/* returns -1 if we drop into the same dir, 0 of cannot dropm 1 if can */
static int fm_dnd_dest_can_receive_drop(FmFileInfo* dest_fi, FmPath* dest,
                                        FmPath* src)
{
    /* g_debug("fm_dnd_dest_can_receive_drop: src %p(%s) dest %p(%s)", src,
            fm_path_get_basename(src), dest, fm_path_get_basename(dest)); */
    if(!dest)
        return 0;

    /* we can drop only onto directory or desktop entry */
    if(fm_file_info_is_desktop_entry(dest_fi))
        return 1;
    if(!fm_file_info_is_dir(dest_fi) || !fm_file_info_is_accessible(dest_fi))
        return 0;
    /* check if target FS is R/O, we cannot drop anything into R/O place */
    if (!fm_file_info_is_writable_directory(dest_fi))
        return 0;

    /* check if we drop directory onto itself */
    if(!src || fm_path_equal(src, dest))
        return 0;
    /* check if source and destination directories are the same */
    if (fm_path_equal(fm_path_get_parent(src), dest))
        return -1;
    return 1;
}

/* Part of this code was taken from GTK sources for gtk_dialog_run() */
typedef struct
{
    GtkMenu *menu;
    GdkDragAction action;
    GMainLoop *loop;
    gboolean destroyed;
} RunInfo;

static void shutdown_loop(RunInfo *ri)
{
    if (g_main_loop_is_running(ri->loop))
        g_main_loop_quit(ri->loop);
}

static void run_unmap_handler(GtkWidget *menu, RunInfo *ri)
{
    shutdown_loop (ri);
}

static void run_destroy_handler(GtkWidget *menu, RunInfo *ri)
{
  /* shutdown_loop will be called by run_unmap_handler */

  ri->destroyed = TRUE;
}

static void on_cancel_sel(GtkAction *act, RunInfo *ri)
{
    /* nothing to select */
}

static void on_copy_sel(GtkAction *act, RunInfo *ri)
{
    ri->action = GDK_ACTION_COPY;
}

static void on_move_sel(GtkAction *act, RunInfo *ri)
{
    ri->action = GDK_ACTION_MOVE;
}

static void on_link_sel(GtkAction *act, RunInfo *ri)
{
    ri->action = GDK_ACTION_LINK;
}

static const char drop_menu_xml[]=
"<popup>"
  "<menuitem action='Copy'/>"
  "<menuitem action='Move'/>"
  "<menuitem action='Link'/>"
  "<menuitem action='Cancel'/>"
"</popup>";

static GtkActionEntry drop_menu_actions[]=
{
    {"Cancel", NULL, N_("_Cancel"), NULL, NULL, G_CALLBACK(on_cancel_sel)},
    {"Copy", NULL, N_("C_opy Here"), NULL, NULL, G_CALLBACK(on_copy_sel)},
    {"Move", NULL, N_("_Move Here"), NULL, NULL, G_CALLBACK(on_move_sel)},
    /* Note to translators: Link in not noun but verb here */
    {"Link", NULL, N_("_Link Here"), NULL, NULL, G_CALLBACK(on_link_sel)}
};

static GdkDragAction _ask_action_on_drop(GtkWidget *widget,
                                         GdkDragContext *drag_context,
                                         gboolean link_only)
{
    RunInfo ri = { NULL, GDK_ACTION_DEFAULT, NULL, FALSE };
    gulong unmap_handler;
    gulong destroy_handler;
    GtkUIManager* ui = gtk_ui_manager_new();
    GtkActionGroup* act_grp = gtk_action_group_new("Popup");
    GtkAction *act;
    GdkDragAction actions;

    gtk_action_group_set_translation_domain(act_grp, GETTEXT_PACKAGE);

    gtk_action_group_add_actions(act_grp, drop_menu_actions, G_N_ELEMENTS(drop_menu_actions), &ri);
    gtk_ui_manager_add_ui_from_string(ui, drop_menu_xml, -1, NULL);
    gtk_ui_manager_insert_action_group(ui, act_grp, 0);
    if (link_only)
    {
        /* we cannot move file but can link or copy */
        act = gtk_ui_manager_get_action(ui, "/popup/Move");
        gtk_action_set_visible(act, FALSE);
    }
    if (drag_context)
    {
        actions = gdk_drag_context_get_actions(drag_context);
        if ((actions & GDK_ACTION_COPY) == 0)
        {
            act = gtk_ui_manager_get_action(ui, "/popup/Copy");
            gtk_action_set_sensitive(act, FALSE);
        }
        if ((actions & GDK_ACTION_MOVE) == 0)
        {
            act = gtk_ui_manager_get_action(ui, "/popup/Move");
            gtk_action_set_sensitive(act, FALSE);
        }
        if ((actions & GDK_ACTION_LINK) == 0)
        {
            act = gtk_ui_manager_get_action(ui, "/popup/Link");
            gtk_action_set_sensitive(act, FALSE);
        }
    }
    ri.menu = g_object_ref(gtk_ui_manager_get_widget(ui, "/popup"));
    g_signal_connect(ri.menu, "selection-done", G_CALLBACK(gtk_widget_destroy), NULL);
    unmap_handler = g_signal_connect(ri.menu, "unmap",
                                     G_CALLBACK(run_unmap_handler), &ri);
    destroy_handler = g_signal_connect(ri.menu, "destroy",
                                       G_CALLBACK(run_destroy_handler), &ri);
    g_object_unref(act_grp);
    g_object_unref(ui);
    gtk_menu_attach_to_widget(ri.menu, widget, NULL);
    gtk_menu_popup(ri.menu, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());

    ri.loop = g_main_loop_new(NULL, FALSE);

    GDK_THREADS_LEAVE ();
    g_main_loop_run(ri.loop);
    GDK_THREADS_ENTER ();

    g_main_loop_unref(ri.loop);

    ri.loop = NULL;

    gtk_menu_detach(ri.menu);
    if (!ri.destroyed)
    {
        g_signal_handler_disconnect(ri.menu, unmap_handler);
        g_signal_handler_disconnect(ri.menu, destroy_handler);
    }
    g_object_unref(ri.menu);

    return ri.action;
}

/* own handler for "files-dropped" signal */
static gboolean fm_dnd_dest_files_dropped(FmDndDest* dd, int x, int y,
                                          guint action, guint info_type,
                                          FmPathList* files)
{
    FmPath* dest;
    GtkWidget* parent;
    int can_drop;

    dest = fm_dnd_dest_get_dest_path(dd);
    g_debug("%d files-dropped!, info_type: %d", fm_path_list_get_length(files), info_type);

    can_drop = fm_dnd_dest_can_receive_drop(dd->dest_file, dest,
                                            fm_path_list_peek_head(files));
    if (can_drop == 0)
        return FALSE;
    if (action != GDK_ACTION_ASK && can_drop < 0) /* drop into itself only if ask */
        return FALSE;

    if(fm_file_info_is_desktop_entry(dd->dest_file))
    {
        if(action != GDK_ACTION_COPY)
            return FALSE;
        parent = gtk_widget_get_toplevel(dd->widget);
        return fm_launch_desktop_entry_simple(GTK_WINDOW(parent), NULL,
                                              dd->dest_file, files);
    }

    parent = gtk_widget_get_toplevel(dd->widget);
    if (action == GDK_ACTION_ASK) /* special handling for ask */
        action = _ask_action_on_drop(dd->widget, dd->context, can_drop < 0);
    switch((GdkDragAction)action)
    {
    case GDK_ACTION_MOVE:
        if(fm_path_is_trash_root(dest))
            fm_trash_files(GTK_WINDOW(parent), files);
        else
            fm_move_files(GTK_WINDOW(parent), files, dest);
        break;
    case GDK_ACTION_COPY:
        fm_copy_files(GTK_WINDOW(parent), files, dest);
        break;
    case GDK_ACTION_LINK:
        fm_link_files(GTK_WINDOW(parent), files, dest);
        break;
    case GDK_ACTION_ASK:
    case GDK_ACTION_PRIVATE:
    default: /* invalid combination */
        return FALSE;
    }
    return TRUE;
}

static void clear_src_cache(FmDndDest* dd)
{
    /* free cached source files */
    if(dd->context)
    {
        g_object_unref(dd->context);
        dd->context = NULL;
    }
    if(dd->src_files)
    {
        fm_path_list_unref(dd->src_files);
        dd->src_files = NULL;
    }
    if(dd->dest_file)
    {
        fm_file_info_unref(dd->dest_file);
        dd->dest_file = NULL;
    }
    dd->src_dev = 0;
    dd->src_fs_id = NULL;

    dd->info_type = 0;
    dd->waiting_data = FALSE;
}

/**
 * fm_dnd_dest_get_dest_file
 * @dd: a drag destination descriptor
 *
 * Retrieves file info of drag destination. Returned data are owned by @dd and
 * should not be freed by caller.
 *
 * Returns: (transfer none): file info of drag destination.
 *
 * Since: 0.1.0
 */
FmFileInfo* fm_dnd_dest_get_dest_file(FmDndDest* dd)
{
    return dd->dest_file;
}

/**
 * fm_dnd_dest_get_dest_path
 * @dd: a drag destination descriptor
 *
 * Retrieves file path of drag destination. Returned data are owned by @dd and
 * should not be freed by caller.
 *
 * Returns: (transfer none): file path of drag destination.
 *
 * Since: 0.1.0
 */
FmPath* fm_dnd_dest_get_dest_path(FmDndDest* dd)
{
    return dd->dest_file ? fm_file_info_get_path(dd->dest_file) : NULL;
}

/**
 * fm_dnd_dest_set_dest_file
 * @dd: a drag destination descriptor
 * @dest_file: file info of drag destination
 *
 * Sets drag destination for @dd.
 *
 * Since: 0.1.0
 */
void fm_dnd_dest_set_dest_file(FmDndDest* dd, FmFileInfo* dest_file)
{
    if(dd->dest_file == dest_file)
        return;
    if(dd->dest_file)
        fm_file_info_unref(dd->dest_file);
    dd->dest_file = dest_file ? fm_file_info_ref(dest_file) : NULL;
}

/**
 * fm_dnd_dest_drag_data_received
 * @dd: a drag destination descriptor
 * @drag_context: the drag context
 * @x: horisontal position of drop
 * @y: vertical position of drop
 * @sel_data: selection data that are dragged
 * @info: (#FmDndDestTargetType) type of data that are dragged
 * @time: timestamp of operation
 *
 * A common handler for signals that emitted when information about
 * dragged data is received, such as "drag-data-received".
 *
 * If the @dd was created with fm_dnd_dest_new_with_handlers() then this
 * API should be never used by the widget.
 *
 * Returns: %TRUE if dropping data is accepted for processing.
 *
 * Since: 0.1.17
 */
static inline
gboolean _on_drag_data_received(FmDndDest* dd, GdkDragContext *drag_context,
             gint x, gint y, GtkSelectionData *sel_data, guint info, guint time)
{
    FmPathList* files = NULL;
    gint length, format;
    const gchar* data;

    data = (const gchar*)gtk_selection_data_get_data_with_length(sel_data, &length);
    format = gtk_selection_data_get_format(sel_data);

    dd->can_copy = FALSE;
    if(info == FM_DND_DEST_TARGET_FM_LIST)
    {
        if((length == sizeof(gpointer)) && (format==8))
        {
            /* get the pointer */
            FmFileInfoList* file_infos = *(FmFileInfoList**)data;
            if(file_infos)
            {
                FmFileInfo* fi = fm_file_info_list_peek_head(fm_file_info_list_ref(file_infos));
                /* FIXME: how can it be? it should be checked beforehand */
                if(fi == NULL) ;
                /* get the device of the first dragged source file */
                else if(fm_path_is_native(fm_file_info_get_path(fi)))
                {
                    if (fm_path_get_parent(fm_file_info_get_path(fi)) != fm_path_get_home())
                        dd->can_copy = TRUE;
                    dd->src_dev = fm_file_info_get_dev(fi);
                }
                else
                    dd->src_fs_id = fm_file_info_get_fs_id(fi);
                files = fm_path_list_new_from_file_info_list(file_infos);
                fm_file_info_list_unref(file_infos);
            }
        }
    }
    else if(info == FM_DND_DEST_TARGET_URI_LIST)
    {
        if((length >= 0) && (format==8))
        {
            gchar **uris;
            uris = gtk_selection_data_get_uris( sel_data );
            files = fm_path_list_new_from_uris(uris);
            g_free(uris);
            if(files && !fm_path_list_is_empty(files))
            {
                GFileInfo* inf;
                FmPath* path = fm_path_list_peek_head(files);
                GFile* gf = fm_path_to_gfile(path);
                const char* attr = fm_path_is_native(path) ? G_FILE_ATTRIBUTE_UNIX_DEVICE : G_FILE_ATTRIBUTE_ID_FILESYSTEM;
                inf = g_file_query_info(gf, attr, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
                g_object_unref(gf);

                if(fm_path_is_native(path))
                {
                    if (fm_path_get_parent(path) != fm_path_get_home())
                        dd->can_copy = TRUE;
                    if (inf)
                        dd->src_dev = g_file_info_get_attribute_uint32(inf, G_FILE_ATTRIBUTE_UNIX_DEVICE);
                    else
                        dd->src_dev = 0;
                }
                else if (inf)
                    dd->src_fs_id = g_intern_string(g_file_info_get_attribute_string(inf, G_FILE_ATTRIBUTE_ID_FILESYSTEM));
                else
                    dd->src_fs_id = NULL;
                if (inf)
                    g_object_unref(inf);
            }
        }
    }
    else if(info == FM_DND_DEST_TARGET_XDS) /* X direct save */
    {
        if(format == 8 && length == 1 && data[0] == 'F')
        {
            GdkWindow *source_window;
            source_window = gdk_drag_context_get_source_window(drag_context);
            gdk_property_change(source_window, dest_target_atom[FM_DND_DEST_TARGET_XDS],
                               gdk_atom_intern_static_string("text/plain"), 8,
                               GDK_PROP_MODE_REPLACE, (const guchar *)"", 0);
        }
        else if(format == 8 && length == 1 && data[0] == 'S')
        {
            /* XDS succeeds */
        }
        gtk_drag_finish(drag_context, TRUE, FALSE, time);
        return TRUE;
    }

    /* remove previously cached source files. */
    if(G_UNLIKELY(dd->src_files))
        fm_path_list_unref(dd->src_files);
    if(files && fm_path_list_is_empty(files))
    {
        g_warning("drag-data-received with empty list");
        fm_path_list_unref(files);
        files = NULL;
    }
    dd->src_files = files;
    dd->waiting_data = FALSE;
    dd->info_type = info;
    /* keep context to verify if it's changed */
    if(G_UNLIKELY(dd->context))
        g_object_unref(dd->context);
    dd->context = g_object_ref(drag_context);
    return (files != NULL);
}

gboolean fm_dnd_dest_drag_data_received(FmDndDest* dd, GdkDragContext *drag_context,
             gint x, gint y, GtkSelectionData *sel_data, guint info, guint time)
{
    return _on_drag_data_received(dd, drag_context, x, y, sel_data, info, time);
}

static void on_drag_data_received(GtkWidget *w, GdkDragContext *drag_context,
                                  gint x, gint y, GtkSelectionData *data,
                                  guint info, guint time, FmDndDest* dd)
{
    _on_drag_data_received(dd, drag_context, x, y, data, info, time);
}

/**
 * fm_dnd_dest_find_target
 * @dd: a drag destination descriptor
 * @drag_context: the drag context
 *
 * Finds target type that is supported for @drag_context.
 *
 * Returns: supported target type or %GDK_NONE if none found.
 *
 * Since: 0.1.17
 */
GdkAtom fm_dnd_dest_find_target(FmDndDest* dd, GdkDragContext *drag_context)
{
    guint i;
    for(i = 1; i < N_FM_DND_DEST_DEFAULT_TARGETS; i++)
    {
        GdkAtom target = dest_target_atom[i];
        if(G_LIKELY(target != GDK_NONE)
           && fm_drag_context_has_target(drag_context, target)
           /* accept FM_DND_DEST_TARGET_FM_LIST only from the same application */
           && (i != FM_DND_DEST_TARGET_FM_LIST ||
               gtk_drag_get_source_widget(drag_context)))
            return target;
    }
    return GDK_NONE;
}

/**
 * fm_dnd_dest_is_target_supported
 * @dd: a drag destination descriptor
 * @target: target type
 *
 * Checks if @target is supported by libfm.
 *
 * Returns: %TRUE if drop to @target is supported by libfm.
 *
 * Since: 0.1.17
 */
gboolean fm_dnd_dest_is_target_supported(FmDndDest* dd, GdkAtom target)
{
    guint i;

    if(G_LIKELY(target != GDK_NONE))
        for(i = 1; i < N_FM_DND_DEST_DEFAULT_TARGETS; i++)
            if(dest_target_atom[i] == target)
                return TRUE;
    return FALSE;
}

/**
 * fm_dnd_dest_drag_drop
 * @dd: a drag destination descriptor
 * @drag_context: the drag context
 * @target: target type
 * @x: horisontal position of drop
 * @y: vertical position of drop
 * @time: timestamp of operation
 *
 * A common handler for signals that emitted when dragged data are
 * dropped onto destination, "drag-drop". Prepares data and emits the
 * #FmDndDest::files-dropped signal if drop is supported.
 *
 * If the @dd was created with fm_dnd_dest_new_with_handlers() then this
 * API should be never used by the widget.
 *
 * Returns: %TRUE if drop to @target is supported by libfm.
 *
 * Since: 0.1.17
 */
static inline
gboolean _on_drag_drop(FmDndDest* dd, GdkDragContext *drag_context,
                       GdkAtom target, int x, int y, guint time)
{
    gboolean ret = FALSE;
    GtkWidget* dest_widget = dd->widget;
    guint i;
    if(G_LIKELY(target != GDK_NONE))
        for(i = 1; i < N_FM_DND_DEST_DEFAULT_TARGETS; i++)
            if(dest_target_atom[i] == target)
            {
                ret = TRUE;
                break;
            }
    if(ret) /* we support this kind of target */
    {
        if(i == FM_DND_DEST_TARGET_XDS) /* if this is XDS */
        {
            guchar *data = NULL;
            GdkWindow *source_window;
            gint len = 0;
            GdkAtom text_atom = gdk_atom_intern_static_string("text/plain");
            /* get filename from the source window */
            source_window = gdk_drag_context_get_source_window(drag_context);
            if(gdk_property_get(source_window, target, text_atom,
                                0, 1024, FALSE, NULL, NULL,
                                &len, &data) && data)
            {
                FmFileInfo* dest = fm_dnd_dest_get_dest_file(dd);
                if( dest && fm_file_info_is_dir(dest) )
                {
                    FmPath* path = fm_path_new_child(fm_file_info_get_path(dest), (gchar*)data);
                    char* uri = fm_path_to_uri(path);
                    /* setup the property */
                    gdk_property_change(source_window, target,
                                       text_atom, 8, GDK_PROP_MODE_REPLACE, (const guchar *)uri,
                                       strlen(uri) + 1);
                    fm_path_unref(path);
                    g_free(uri);
                }
            }
            else
            {
                fm_show_error(GTK_WINDOW(gtk_widget_get_toplevel(dest_widget)), NULL,
                              _("XDirectSave failed."));
                gdk_property_change(source_window, target,
                                   text_atom, 8, GDK_PROP_MODE_REPLACE, (const guchar *)"", 0);
            }
            g_free(data);
            gtk_drag_get_data(dest_widget, drag_context, target, time);
            /* we should call gtk_drag_finish later in data-received callback. */
            return TRUE;
        }

        /* see if the dragged files are cached by "drag-motion" handler */
        if(dd->src_files && drag_context == dd->context)
        {
            GdkDragAction action = gdk_drag_context_get_selected_action(drag_context);
            /* emit files-dropped signal */
            g_signal_emit(dd, signals[FILES_DROPPED], 0, x, y, action, dd->info_type, dd->src_files, &ret);
        }
        else /* we don't have the data */
        {
            if(dd->waiting_data) /* if we're still waiting for the data */
            {
                /* FIXME: how to handle this? */
                ret = FALSE;
            }
            else
                ret = FALSE;
        }
        gtk_drag_finish(drag_context, ret, FALSE, time);
    }
    return ret;
}

gboolean fm_dnd_dest_drag_drop(FmDndDest* dd, GdkDragContext *drag_context,
                               GdkAtom target, int x, int y, guint time)
{
    return _on_drag_drop(dd, drag_context, target, x, y, time);
}

static gboolean on_drag_drop(GtkWidget *widget, GdkDragContext *drag_context,
                             gint x, gint y, guint time, FmDndDest* dd)
{
    GdkAtom target = fm_dnd_dest_find_target(dd, drag_context);
    if(G_UNLIKELY(target == GDK_NONE))
        return FALSE;
    return _on_drag_drop(dd, drag_context, target, x, y, time);
}

/**
 * fm_dnd_dest_get_default_action
 * @dd: object which will receive data
 * @drag_context: the drag context
 * @target: #GdkAtom of the target data type
 *
 * Returns: the default action to take for the dragged files.
 *
 * Since: 0.1.17
 */
GdkDragAction fm_dnd_dest_get_default_action(FmDndDest* dd,
                                             GdkDragContext* drag_context,
                                             GdkAtom target)
{
    GdkDragAction action;
    FmFileInfo* dest = dd->dest_file;
    FmPath* dest_path;
    int can_drop;

    if(!dest || !(dest_path = fm_file_info_get_path(dest)))
        /* query drag sources in any case */
        goto query_sources;

    /* special support for dropping onto desktop entry */
    if(fm_file_info_is_desktop_entry(dest))
    {
        GdkModifierType mask = 0;
        gdk_window_get_device_position (gtk_widget_get_window(dd->widget),
                                        gdk_drag_context_get_device(drag_context),
                                        NULL, NULL, &mask);
        mask &= gtk_accelerator_get_default_mod_mask();
        if ((mask & ~GDK_CONTROL_MASK) != 0) /* only "copy" action is allowed */
            return 0;
        if(!dd->src_files || dd->context != drag_context)
        {
            /* we have no valid data, query it now */
            clear_src_cache(dd);
            if(!dd->waiting_data) /* we're still waiting for "drag-data-received" signal */
            {
                /* retrieve the source files */
                gtk_drag_get_data(dd->widget, drag_context, target, time(NULL));
                dd->waiting_data = TRUE;
            }
            return 0;
        }
        return GDK_ACTION_COPY;
    }

    /* this is XDirectSave */
    if(target == dest_target_atom[FM_DND_DEST_TARGET_XDS])
        return GDK_ACTION_COPY;

    /* we have no valid data, query it now */
    if(!dd->src_files || dd->context != drag_context)
    {
query_sources:
        if (dd->context != drag_context)
            clear_src_cache(dd);
        action = 0;
        if(!dd->waiting_data) /* we're still waiting for "drag-data-received" signal */
        {
            /* retrieve the source files */
            gtk_drag_get_data(dd->widget, drag_context, target, time(NULL));
            dd->waiting_data = TRUE;
        }
    }
    else /* we have got drag source files */
    {
        FmPath *src_path = fm_path_list_peek_head(dd->src_files);

        /* dest is an ordinary path, check if drop on it is supported */
        can_drop = fm_dnd_dest_can_receive_drop(dest, dest_path, src_path);
        if (can_drop < 0 && fm_config->drop_default_action == FM_DND_DEST_DROP_ASK)
            return GDK_ACTION_ASK; /* we are dropping into the same place */
        else if (can_drop <= 0)
            action = 0;
        else
        {
            /* determine if the dragged files are on the same device as destination file */
            /* Here we only check the first dragged file since checking all of them can
             * make the operation very slow. */
            gboolean same_fs;
            GdkModifierType mask = 0;
            gdk_window_get_device_position (gtk_widget_get_window(dd->widget),
                                            gdk_drag_context_get_device(drag_context),
                                            NULL, NULL, &mask);
            mask &= gtk_accelerator_get_default_mod_mask();
            if(fm_path_is_trash(dest_path))
            {
                if((mask & ~GDK_SHIFT_MASK) == 0 && /* no modifiers or shift */
                   fm_path_is_trash_root(dest_path))
                    /* we can only move files to trash can */
                    action = GDK_ACTION_MOVE;
                else /* files inside trash are read only */
                    action = 0;
            }
            else if (fm_path_is_trash(src_path))
            {
                if ((mask & ~GDK_SHIFT_MASK) == 0) /* no modifiers or shift */
                    /* bug #3615258: only do move files from trash can */
                    action = GDK_ACTION_MOVE;
                else /* we should not do anything else */
                    action = 0;
            }
            /* use Shift for Move, Ctrl for Copy, Ctrl+Shift for Link, Alt for Ask */
            else if(mask == (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
                action = GDK_ACTION_LINK;
            else if(mask == GDK_SHIFT_MASK)
                action = GDK_ACTION_MOVE;
            else if(mask == GDK_CONTROL_MASK)
                action = GDK_ACTION_COPY;
            else if(mask == GDK_MOD1_MASK)
                action = GDK_ACTION_ASK;
            else if(mask != 0) /* another modifier key was pressed */
                action = 0;
            /* make decision based on config: Auto / Copy / Move / Ask */
            else switch((FmDndDestDropAction)fm_config->drop_default_action)
            {
            case FM_DND_DEST_DROP_MOVE:
                action = GDK_ACTION_MOVE;
                break;
            case FM_DND_DEST_DROP_COPY:
                action = GDK_ACTION_COPY;
                break;
            case FM_DND_DEST_DROP_ASK:
                return GDK_ACTION_ASK;
            default: /* FM_DND_DEST_DROP_AUTO or invalid values */
                if(!dd->src_dev && !dd->src_fs_id)
                    /* we don't know on which device the dragged source files are. */
                    same_fs = FALSE; /* fallback to copy then */
                /* compare the device/filesystem id against that of destination file */
                else if(fm_path_is_native(dest_path))
                    same_fs = dd->src_dev && (dd->src_dev == fm_file_info_get_dev(dest));
                else /* fs_id is interned string */
                    same_fs = dd->src_fs_id && (dd->src_fs_id == fm_file_info_get_fs_id(dest));
                /* prefer to make shortcuts on Desktop instead of copy/move files */
                if (fm_config->smart_desktop_autodrop && !dd->can_copy &&
                    fm_path_equal(dest_path, fm_path_get_desktop()))
                    /* it is either some directory under $HOME
                       or some remote URI (http:// for example) */
                    action = GDK_ACTION_LINK;
                else
                    action = same_fs ? GDK_ACTION_MOVE : GDK_ACTION_COPY;
            }
        }
    }

    if( action && 0 == (gdk_drag_context_get_actions(drag_context) & action) )
        action = gdk_drag_context_get_suggested_action(drag_context);

    return action;
}

/**
 * fm_dnd_dest_drag_leave
 * @dd: a drag destination descriptor
 * @drag_context: the drag context
 * @time: timestamp of operation
 *
 * A common handler for signals that emitted when drag leaves the destination
 * widget, such as "drag-leave".
 *
 * If the @dd was created with fm_dnd_dest_new_with_handlers() then this
 * API should be never used by the widget.
 *
 * Since: 0.1.17
 */
void fm_dnd_dest_drag_leave(FmDndDest* dd, GdkDragContext* drag_context, guint time)
{
}

static void on_drag_leave(GtkWidget *widget, GdkDragContext *drag_context,
                          guint time, FmDndDest* dd)
{
}
