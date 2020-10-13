/*
 *      fm-places-view.c
 *
 *      Copyright 2009 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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
 * SECTION:fm-places-view
 * @short_description: A widget for side panel with places list.
 * @title: FmPlacesView
 *
 * @include: libfm/fm-gtk.h
 *
 * The #FmPlacesView displays list of pseudo-folders which contains
 * such items as Home directory, Trash bin, mounted removable drives,
 * bookmarks, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define FM_DISABLE_SEAL

#include <glib/gi18n-lib.h>
#include "fm-places-view.h"
#include "fm-config.h"
#include "fm-utils.h"
#include "fm-gtk-utils.h"
#include "fm-bookmarks.h"
#include "fm-file-menu.h"
#include "fm-cell-renderer-pixbuf.h"
#include "fm-dnd-auto-scroll.h"
#include "fm-places-model.h"
#include "fm-gtk-file-launcher.h"
#include "fm-gtk-marshal.h"

#include <gdk/gdkkeysyms.h>
#include "gtk-compat.h"

enum
{
    PROP_0,
    PROP_HOME_DIR
};

enum
{
    CHDIR,
    ITEM_POPUP,
    N_SIGNALS
};

static void activate_row(FmPlacesView* view, guint button, GtkTreePath* tree_path);
static void on_row_activated( GtkTreeView* view, GtkTreePath* tree_path, GtkTreeViewColumn *col);
static gboolean on_button_press(GtkWidget* view, GdkEventButton* evt);
static gboolean on_button_release(GtkWidget* view, GdkEventButton* evt);

static void on_mount(GtkAction* act, gpointer user_data);
static void on_umount(GtkAction* act, gpointer user_data);
static void on_eject(GtkAction* act, gpointer user_data);
static void on_format(GtkAction* act, gpointer user_data);

static void on_remove_bm(GtkAction* act, gpointer user_data);
static void on_rename_bm(GtkAction* act, gpointer user_data);
static void on_move_bm_up(GtkAction* act, gpointer user_data);
static void on_move_bm_down(GtkAction* act, gpointer user_data);
static void on_empty_trash(GtkAction* act, gpointer user_data);

static gboolean on_dnd_dest_files_dropped(FmDndDest* dd, int x, int y, GdkDragAction action,
                                          FmDndDestTargetType info_type,
                                          FmPathList* files, FmPlacesView* view);

//static void on_trash_changed(GFileMonitor *monitor, GFile *gf, GFile *other, GFileMonitorEvent evt, gpointer user_data);
//static void on_use_trash_changed(FmConfig* cfg, gpointer unused);
//static void on_pane_icon_size_changed(FmConfig* cfg, gpointer unused);

G_DEFINE_TYPE(FmPlacesView, fm_places_view, GTK_TYPE_TREE_VIEW);

/** One common FmPlacesModel for all FmPlacesView instances */
static FmPlacesModel* model = NULL;

static guint signals[N_SIGNALS];

#define PLACES_MENU_XML \
"<popup>" \
  "<placeholder name='ph1'/>" \
  "<separator/>" \
  "<placeholder name='ph2'/>" \
  "<separator/>" \
  "<placeholder name='ph3'/>" \
"</popup>"

static const char vol_menu_xml[]=
PLACES_MENU_XML
"<popup>"
  "<placeholder name='ph3'>"
  "<menuitem action='Mount'/>"
  "<menuitem action='Unmount'/>"
  "<menuitem action='Eject'/>"
  "<menuitem action='Format'/>"
  "</placeholder>"
"</popup>";

static const char mount_menu_xml[]=
PLACES_MENU_XML
"<popup>"
  "<placeholder name='ph3'>"
  "<menuitem action='Unmount'/>"
  "</placeholder>"
"</popup>";

static GtkActionEntry vol_menu_actions[]=
{
    {"Mount", NULL, N_("_Mount Volume"), NULL, NULL, G_CALLBACK(on_mount)},
    {"Unmount", NULL, N_("_Unmount Volume"), NULL, NULL, G_CALLBACK(on_umount)},
    {"Eject", NULL, N_("_Eject Removable Media"), NULL, NULL, G_CALLBACK(on_eject)},
    {"Format", NULL, N_("_Format Volume"), NULL, NULL, G_CALLBACK(on_format)}
};

static const char bookmark_menu_xml[]=
PLACES_MENU_XML
"<popup>"
  "<placeholder name='ph3'>"
  "<menuitem action='RenameBm'/>"
  "<menuitem action='RemoveBm'/>"
  "<menuitem action='MoveBmUp'/>"
  "<menuitem action='MoveBmDown'/>"
  "</placeholder>"
"</popup>";

static GtkActionEntry bm_menu_actions[]=
{
    {"RenameBm", GTK_STOCK_EDIT, N_("_Rename Bookmark Item"), NULL, NULL, G_CALLBACK(on_rename_bm)},
    {"RemoveBm", GTK_STOCK_REMOVE, N_("Re_move from Bookmarks"), NULL, NULL, G_CALLBACK(on_remove_bm)},
    {"MoveBmUp", GTK_STOCK_GO_UP, N_("Move Bookmark _Up"), NULL, NULL, G_CALLBACK(on_move_bm_up)},
    {"MoveBmDown", GTK_STOCK_GO_DOWN, N_("Move Bookmark _Down"), NULL, NULL, G_CALLBACK(on_move_bm_down)}
};

static const char trash_menu_xml[]=
PLACES_MENU_XML
"<popup>"
  "<placeholder name='ph3'>"
  "<menuitem action='EmptyTrash'/>"
  "</placeholder>"
"</popup>";

static GtkActionEntry trash_menu_actions[]=
{
    {"EmptyTrash", NULL, N_("_Empty Trash Can"), NULL, NULL, G_CALLBACK(on_empty_trash)}
};

/* targets are added to FmDndDest, also only targets for GtkTreeDragSource */
enum {
    FM_DND_TARGET_BOOOKMARK = N_FM_DND_DEST_DEFAULT_TARGETS
};

/* Target types for dragging items of list */
static const GtkTargetEntry dnd_targets[] = {
    { "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, FM_DND_TARGET_BOOOKMARK }
};

static GdkAtom tree_model_row_atom;

static gboolean sep_func( GtkTreeModel* model, GtkTreeIter* it, gpointer data )
{
    return fm_places_model_iter_is_separator(FM_PLACES_MODEL(model), it);
}

static void on_renderer_icon_size_changed(FmConfig* cfg, gpointer user_data)
{
    FmCellRendererPixbuf* render = FM_CELL_RENDERER_PIXBUF(user_data);
    fm_cell_renderer_pixbuf_set_fixed_size(render, fm_config->pane_icon_size, fm_config->pane_icon_size);
}

static void on_cell_renderer_pixbuf_destroy(gpointer user_data, GObject* render)
{
    g_signal_handler_disconnect(fm_config, GPOINTER_TO_UINT(user_data));
}

/*----------------------------------------------------------------------
   Drag source is handled by model which implements GtkTreeDragSource */

/*----------------------------------------------------------------------
   Drop destination is handled by FmDndDest. We add own target there. */

/* Given a drop path retrieved by gtk_tree_view_get_dest_row_at_pos, this function
 * determines whether dropping a bookmark item at the specified path is allow.
 * If dropping is not allowed, this function tries to choose an alternative position
 * for the bookmark item and modified the tree path @tp passed into this function. */
static gboolean get_bookmark_drag_dest(FmPlacesView* view, GtkTreePath** tp, GtkTreeViewDropPosition* pos)
{
    gboolean ret = TRUE;
    if(*tp)
    {
        /* if the drop site is below the separator (in the bookmark area) */
        if(fm_places_model_path_is_bookmark(model, *tp))
        {
            /* we cannot drop into a item */
            if(*pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE ||
               *pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
                ret = FALSE;
            else
                ret = TRUE;
        }
        else /* the drop site is above the separator (in the places area containing volumes) */
        {
            GtkTreePath* sep = fm_places_model_get_separator_path(model);
            /* set drop site at the first bookmark item */
            gtk_tree_path_get_indices(*tp)[0] = gtk_tree_path_get_indices(sep)[0] + 1;
            gtk_tree_path_free(sep);
            *pos = GTK_TREE_VIEW_DROP_BEFORE;
            ret = TRUE;
        }
    }
    else
    {
        /* drop at end of the bookmarks list instead */
        *tp = gtk_tree_path_new_from_indices(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL) - 1, -1);
        *pos = GTK_TREE_VIEW_DROP_AFTER;
        ret = TRUE;
    }
    /* g_debug("path: %s, pos: %d, ret: %d", gtk_tree_path_to_string(*tp), *pos, ret); */
    return ret;
}

static gboolean on_drag_motion (GtkWidget *dest_widget,
                    GdkDragContext *drag_context, gint x, gint y, guint time)
{
    FmPlacesView* view = FM_PLACES_VIEW(dest_widget);
    /* fm_drag_context_has_target_name(drag_context, "GTK_TREE_MODEL_ROW"); */
    GdkAtom target;
    GtkTreeViewDropPosition pos;
    GtkTreePath* tp = NULL;
    gboolean ret = FALSE;
    GdkDragAction action = 0;

    target = gtk_drag_dest_find_target(dest_widget, drag_context, NULL);
    if(target == GDK_NONE)
        return FALSE;

    gtk_tree_view_get_dest_row_at_pos(&view->parent, x, y, &tp, &pos);

    /* handle reordering bookmark items first */
    if(target == tree_model_row_atom)
    {
        /* bookmark item is being dragged */
        ret = get_bookmark_drag_dest(view, &tp, &pos);
        action = ret ? GDK_ACTION_MOVE : 0; /* bookmark items can only be moved */
    }
    /* try FmDndDest */
    else if(fm_dnd_dest_is_target_supported(view->dnd_dest, target))
    {
        /* the user is dragging files. get FmFileInfo of drop site. */
        if(pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) /* drag into items */
        {
            FmPlacesItem* item = NULL;
            GtkTreeIter it;
            FmFileInfo* fi;
            if(tp && gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &it, tp))
                gtk_tree_model_get(GTK_TREE_MODEL(model), &it, FM_PLACES_MODEL_COL_INFO, &item, -1);

            fi = item ? fm_places_item_get_info(item) : NULL;
            fm_dnd_dest_set_dest_file(view->dnd_dest, fi);
            /* query default action (this may trigger drag-data-received signal)
             * FIXME: this is a dirty and bad API design definitely requires refactor. */
            action = fm_dnd_dest_get_default_action(view->dnd_dest, drag_context, target);
        }
        else /* drop between items, create bookmark items for dragged files */
        {
            fm_dnd_dest_set_dest_file(view->dnd_dest, NULL);
            /* FmDndDest requires this call */
            fm_dnd_dest_get_default_action(view->dnd_dest, drag_context, target);
            if( (!tp || fm_places_model_path_is_bookmark(model, tp))
               && get_bookmark_drag_dest(view, &tp, &pos)) /* tp is after separator */
                action = GDK_ACTION_LINK;
            else
                action = 0;
        }
        ret = (action != 0);
    }
    gdk_drag_status(drag_context, action, time);

    if(ret)
        gtk_tree_view_set_drag_dest_row(&view->parent, tp, pos);
    else
        gtk_tree_view_set_drag_dest_row(&view->parent, NULL, 0);

    if(tp)
        gtk_tree_path_free(tp);

    return ret;
}

static void on_drag_leave ( GtkWidget *dest_widget,
                    GdkDragContext *drag_context, guint time)
{
    gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(dest_widget), NULL, 0);
    /* g_debug("drag_leave"); */
}

static gboolean on_drag_drop ( GtkWidget *dest_widget,
                    GdkDragContext *drag_context, gint x, gint y, guint time)
{
    /* this is to reorder bookmark */
    if(gtk_drag_dest_find_target(dest_widget, drag_context, NULL)
       == tree_model_row_atom)
    {
        gtk_drag_get_data(dest_widget, drag_context, tree_model_row_atom, time);
        return TRUE;
    }
    return FALSE;
}

static void on_drag_data_received ( GtkWidget *dest_widget,
                GdkDragContext *drag_context, gint x, gint y,
                GtkSelectionData *sel_data, guint info, guint time)
{
    FmPlacesView* view = FM_PLACES_VIEW(dest_widget);
    GtkTreePath* dest_tp = NULL;
    GtkTreeViewDropPosition pos;
    gboolean ret = FALSE;

    switch(info)
    {
    case FM_DND_TARGET_BOOOKMARK:
        gtk_tree_view_get_dest_row_at_pos(&view->parent, x, y, &dest_tp, &pos);
        if(get_bookmark_drag_dest(view, &dest_tp, &pos)) /* got the drop position */
        {
            GtkTreePath* src_tp;
            /* get the source row; the GtkTreeDragSource ensured it's bookmark */
            ret = gtk_tree_get_row_drag_data(sel_data, NULL, &src_tp);
            if(ret)
            {
                /* don't do anything if source and dest are the same row */
                if(G_UNLIKELY(gtk_tree_path_compare(src_tp, dest_tp) == 0))
                    ret = FALSE;
                else
                {
                    GtkTreeIter src_it, dest_it;
                    FmPlacesItem* item = NULL;
                    ret = FALSE;
                    /* get the source bookmark item */
                    if(gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &src_it, src_tp))
                        gtk_tree_model_get(GTK_TREE_MODEL(model), &src_it, FM_PLACES_MODEL_COL_INFO, &item, -1);
                    if(item)
                    {
                        /* move it to destination position */
                        if(gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &dest_it, dest_tp))
                        {
                            int new_pos, sep_pos;
                            /* get index of the separator */
                            GtkTreePath* sep_tp = fm_places_model_get_separator_path(model);
                            sep_pos = gtk_tree_path_get_indices(sep_tp)[0];

                            if(pos == GTK_TREE_VIEW_DROP_BEFORE)
                                gtk_list_store_move_before(GTK_LIST_STORE(model), &src_it, &dest_it);
                            else
                                gtk_list_store_move_after(GTK_LIST_STORE(model), &src_it, &dest_it);
                            new_pos = gtk_tree_path_get_indices(dest_tp)[0] - sep_pos - 1;
                            /* reorder the bookmark item */
                            fm_bookmarks_reorder(fm_places_model_get_bookmarks(model), fm_places_item_get_bookmark_item(item), new_pos);
                            gtk_tree_path_free(sep_tp);
                            ret = TRUE;
                        }
                    }
                    /* else it might be additional separator */
                }
                gtk_tree_path_free(src_tp);
            }
        }
        gtk_drag_finish(drag_context, ret, FALSE, time);
        break;
    }
    if(dest_tp)
        gtk_tree_path_free(dest_tp);
}

static gboolean on_dnd_dest_files_dropped(FmDndDest* dd, int x, int y,
                                          GdkDragAction action,
                                          FmDndDestTargetType info_type,
                                          FmPathList* files, FmPlacesView* view)
{
    FmPath* dest;
    GList* l;
    gboolean ret = FALSE;

    dest = fm_dnd_dest_get_dest_path(dd);
    /* g_debug("action= %d, %d files-dropped!, dest=%p info_type: %d", action, fm_path_list_get_length(files), dest, info_type); */

    if(!dest && action == GDK_ACTION_LINK) /* add bookmarks */
    {
        GtkTreePath* tp;
        GtkTreeViewDropPosition pos;
        gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(view), x, y, &tp, &pos);

        if(get_bookmark_drag_dest(view, &tp, &pos))
        {
            GtkTreePath* sep = fm_places_model_get_separator_path(model);
            int idx = gtk_tree_path_get_indices(tp)[0] - gtk_tree_path_get_indices(sep)[0];
            if(pos == GTK_TREE_VIEW_DROP_BEFORE)
                --idx;
            for( l=fm_path_list_peek_head_link(files); l; l=l->next, ++idx )
            {
                FmPath* path = FM_PATH(l->data);
                GFile* gf = fm_path_to_gfile(path);
                if(g_file_query_file_type(gf, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                          NULL) == G_FILE_TYPE_DIRECTORY)
                {
                    char* disp_name = fm_path_display_basename(path);
                    fm_bookmarks_insert(fm_places_model_get_bookmarks(model), path, disp_name, idx);
                    g_free(disp_name);
                }
                g_object_unref(gf);
                /* we don't need to add item to places view. Later the bookmarks will be reloaded. */
            }
            gtk_tree_path_free(sep);
        }
        if(tp)
            gtk_tree_path_free(tp);
        ret = TRUE;
    }

    return ret;
}

/*----------------------------------------------------------------------
   Widget initialization and finalization */

static void fm_places_view_dispose(GObject *object)
{
    FmPlacesView* self;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_PLACES_VIEW(object));
    self = (FmPlacesView*)object;

    if(self->dnd_dest)
    {
        g_signal_handlers_disconnect_by_func(self->dnd_dest, on_dnd_dest_files_dropped, self);
        g_object_unref(self->dnd_dest);
        self->dnd_dest = NULL;
    }

    G_OBJECT_CLASS(fm_places_view_parent_class)->dispose(object);
}

static void fm_places_view_finalize(GObject *object)
{
    FmPlacesView* self;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_PLACES_VIEW(object));
    self = (FmPlacesView*)object;

    if(self->clicked_row)
        gtk_tree_path_free(self->clicked_row);

    G_OBJECT_CLASS(fm_places_view_parent_class)->finalize(object);
}

static void fm_places_view_init(FmPlacesView *self)
{
    GtkTreeViewColumn* col;
    GtkCellRenderer* renderer;
    AtkObject *obj;
    guint handler;

    if(G_UNLIKELY(!model))
    {
        model = fm_places_model_new();
        g_object_add_weak_pointer(G_OBJECT(model), (gpointer*)&model);
    }
    else
        g_object_ref(model);

    gtk_tree_view_set_model(GTK_TREE_VIEW(self), GTK_TREE_MODEL(model));
    g_object_unref(model);

    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(self), FALSE);
    gtk_tree_view_set_row_separator_func(GTK_TREE_VIEW(self), sep_func, NULL, NULL);

    col = gtk_tree_view_column_new();
    renderer = (GtkCellRenderer*)fm_cell_renderer_pixbuf_new();
    handler = g_signal_connect(fm_config, "changed::pane_icon_size", G_CALLBACK(on_renderer_icon_size_changed), renderer);
    g_object_weak_ref(G_OBJECT(renderer), on_cell_renderer_pixbuf_destroy, GUINT_TO_POINTER(handler));
    fm_cell_renderer_pixbuf_set_fixed_size((FmCellRendererPixbuf*)renderer, fm_config->pane_icon_size, fm_config->pane_icon_size);

    gtk_tree_view_column_pack_start( col, renderer, FALSE );
    gtk_tree_view_column_set_attributes( col, renderer,
                                         "pixbuf", FM_PLACES_MODEL_COL_ICON, NULL );

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start( col, renderer, TRUE );
    g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    gtk_tree_view_column_set_attributes( col, renderer,
                                         "text", FM_PLACES_MODEL_COL_LABEL, NULL );

    renderer = gtk_cell_renderer_pixbuf_new();
    self->mount_indicator_renderer = GTK_CELL_RENDERER_PIXBUF(renderer);
    gtk_tree_view_column_pack_start( col, renderer, FALSE );
    gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(col), renderer,
                                       fm_places_model_mount_indicator_cell_data_func,
                                       NULL, NULL);

    gtk_tree_view_append_column ( GTK_TREE_VIEW(self), col );

    gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(self), GDK_BUTTON1_MASK,
                      dnd_targets, G_N_ELEMENTS(dnd_targets), GDK_ACTION_MOVE);

    self->dnd_dest = fm_dnd_dest_new_with_handlers(GTK_WIDGET(self));
    /* add our own targets */
    fm_dnd_dest_add_targets(GTK_WIDGET(self), dnd_targets, G_N_ELEMENTS(dnd_targets));

    g_signal_connect(self->dnd_dest, "files-dropped", G_CALLBACK(on_dnd_dest_files_dropped), self);
    obj = gtk_widget_get_accessible(GTK_WIDGET(self));
    atk_object_set_description(obj, _("Shows list of common places, devices, and bookmarks in sidebar"));
}

/*----------------------------------------------------------------------
   Widget interface */

/**
 * fm_places_view_new
 *
 * Creates new #FmPlacesView widget.
 *
 * Returns: (transfer full): a new #FmPlacesView object.
 *
 * Since: 0.1.0
 */
FmPlacesView *fm_places_view_new(void)
{
    return g_object_new(FM_PLACES_VIEW_TYPE, NULL);
}

static void activate_row(FmPlacesView* view, guint button, GtkTreePath* tree_path)
{
    GtkTreeIter it;
    if(gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &it, tree_path))
    {
        FmPlacesItem* item;
        FmPath* path;
        gtk_tree_model_get(GTK_TREE_MODEL(model), &it, FM_PLACES_MODEL_COL_INFO, &item, -1);
        if(!item)
            return;
        switch(fm_places_item_get_type(item))
        {
        case FM_PLACES_ITEM_PATH:
        case FM_PLACES_ITEM_MOUNT:
            path = fm_places_item_get_path(item);
            if (path == fm_path_get_home() && view->home_dir)
                path = fm_path_new_for_str(view->home_dir);
            else
                fm_path_ref(path);
            break;
        case FM_PLACES_ITEM_VOLUME:
        {
            GFile* gf;
            GMount* mnt = g_volume_get_mount(fm_places_item_get_volume(item));
            if(!mnt)
            {
                GtkWindow* parent = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(view)));
                if(!fm_mount_volume(parent, fm_places_item_get_volume(item), TRUE))
                    return;
                mnt = g_volume_get_mount(fm_places_item_get_volume(item));
                if(!mnt)
                {
                    g_debug("GMount is invalid after successful g_volume_mount().\nThis is quite possibly a gvfs bug.\nSee https://bugzilla.gnome.org/show_bug.cgi?id=552168");
                    return;
                }
            }
            gf = g_mount_get_root(mnt);
            g_object_unref(mnt);
            if(gf)
            {
                path = fm_path_new_for_gfile(gf);
                g_object_unref(gf);
            }
            else
                path = NULL;
            break;
        }
        default:
            return;
        }

        if(path)
        {
            g_signal_emit(view, signals[CHDIR], 0, button, path);
            fm_path_unref(path);
        }
    }
}

static void on_row_activated(GtkTreeView* view, GtkTreePath* tree_path, GtkTreeViewColumn *col)
{
    activate_row(FM_PLACES_VIEW(view), 1, tree_path);
}

/**
 * fm_places_view_chdir
 * @pv: a widget to apply
 * @path: the new path
 *
 * Changes active path and eventually sends the #FmPlacesView::chdir signal.
 *
 * Before 1.0.0 this call had name fm_places_chdir.
 * Before 0.1.12 this call had name fm_places_select.
 *
 * Since: 0.1.0
 */
void fm_places_view_chdir(FmPlacesView* pv, FmPath* path)
{
    GtkTreeIter it;
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(pv));
    GtkTreeSelection* ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(pv));
    if(fm_places_model_get_iter_by_fm_path(FM_PLACES_MODEL(model), &it, path))
    {
        gtk_tree_selection_select_iter(ts, &it);
    }
    else
        gtk_tree_selection_unselect_all(ts);
}

static gboolean on_button_release(GtkWidget* widget, GdkEventButton* evt)
{
    FmPlacesView* view = FM_PLACES_VIEW(widget);
    gboolean ret = GTK_WIDGET_CLASS(fm_places_view_parent_class)->button_release_event(widget, evt);

    /* we should finish the event before we do our work, otherwise gtk may
       activate already removed row in default handler */
    if(view->clicked_row)
    {
        if(evt->button == 1)
        {
            GtkTreePath* tp;
            GtkTreeViewColumn* col;
            int cell_x;
            if(gtk_tree_view_get_path_at_pos(&view->parent, evt->x, evt->y, &tp, &col, &cell_x, NULL))
            {
                /* check if we release the button on the row we previously clicked. */
                if(gtk_tree_path_compare(tp, view->clicked_row) == 0)
                {
                    /* check if we click on the "eject" icon. */
                    int start, cell_w;
                    gtk_tree_view_column_cell_get_position(col, GTK_CELL_RENDERER(view->mount_indicator_renderer),
                                                           &start, &cell_w);
                    if(cell_x > start && cell_x < (start + cell_w)) /* click on eject icon */
                    {
                        GtkTreeIter it;
                        /* do eject if needed */
                        if(gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &it, tp))
                        {
                            FmPlacesItem* item;
                            gtk_tree_model_get(GTK_TREE_MODEL(model), &it, FM_PLACES_MODEL_COL_INFO, &item, -1);
                            if(item && fm_places_item_is_mounted(item))
                            {
                                GtkWindow* toplevel = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(view)));
                                GMount* mount;
                                GVolume* volume;

                                gtk_tree_path_free(view->clicked_row);
                                view->clicked_row = NULL;
                                gtk_tree_path_free(tp);
                                switch(fm_places_item_get_type(item))
                                {
                                case FM_PLACES_ITEM_VOLUME:
                                    volume = fm_places_item_get_volume(item);
                                    /* eject the volume */
                                    if(g_volume_can_eject(volume))
                                        fm_eject_volume(toplevel, volume, TRUE);
                                    else /* not ejectable, do unmount */
                                    {
                                        mount = g_volume_get_mount(volume);
                                        if(mount)
                                        {
                                            fm_unmount_mount(toplevel, mount, TRUE);
                                            g_object_unref(mount);
                                        }
                                    }
                                    break;
                                case FM_PLACES_ITEM_MOUNT:
                                    mount = fm_places_item_get_mount(item);
                                    if(g_mount_can_unmount(mount))
                                        fm_unmount_mount(toplevel, mount, TRUE);
                                    break;
                                default:
                                    break;
                                }
                                /* bug #3614500: if we unmount volume, the main
                                   window handlers will destroy the FmPlacesView
                                   therefore we cannot touch it at this point */
                                goto _out;
                            }
                        }
                    }
                    /* activate the clicked row. */
                    gtk_tree_view_row_activated(&view->parent, view->clicked_row, col);
                }
                gtk_tree_path_free(tp);
            }
        }

        gtk_tree_path_free(view->clicked_row);
        view->clicked_row = NULL;
    }
_out:
    return ret;
}

static void on_selection_done(GtkMenu *menu, gpointer unused)
{
    GtkWidget *widget = gtk_menu_get_attach_widget(menu);

    /* g_debug("FmPlacesView:on_selection_done(): attached widget %p", widget); */
    if (widget) /* it may be destroyed and detached already */
        g_object_weak_unref(G_OBJECT(widget), (GWeakNotify)gtk_menu_detach, menu);
    gtk_widget_destroy(GTK_WIDGET(menu));
}

static void place_item_menu_unref(gpointer ui, GObject *menu)
{
    g_object_unref(ui);
}

static GtkWidget* place_item_get_menu(FmPlacesItem* item, GtkWidget *widget)
{
    GtkWidget* menu = NULL;
    GtkUIManager* ui = gtk_ui_manager_new();
    GtkActionGroup* act_grp = gtk_action_group_new("Popup");
    int sep_pos;
    GtkTreeIter src_it;
    GtkTreePath *tp, *sep_tp;
    GtkAction* act;

    gtk_action_group_set_translation_domain(act_grp, GETTEXT_PACKAGE);

    /* FIXME: merge with FmFileMenu when possible */
    if(fm_places_item_get_type(item) == FM_PLACES_ITEM_PATH)
    {
        if(fm_places_item_get_bookmark_item(item))
        {
            gtk_action_group_add_actions(act_grp, bm_menu_actions, G_N_ELEMENTS(bm_menu_actions), item);
            gtk_ui_manager_add_ui_from_string(ui, bookmark_menu_xml, -1, NULL);
            /* check and disable MoveBmUp and MoveBmDown */
            if(fm_places_model_get_iter_by_fm_path(model, &src_it,
                                                   fm_places_item_get_path(item)))
            {
                /* get index of the separator */
                sep_tp = fm_places_model_get_separator_path(model);
                sep_pos = gtk_tree_path_get_indices(sep_tp)[0];
                tp = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &src_it);
                if(!gtk_tree_path_prev(tp) ||
                   gtk_tree_path_get_indices(tp)[0] - sep_pos - 1 < 0)
                {
                    act = gtk_action_group_get_action(act_grp, "MoveBmUp");
                    gtk_action_set_sensitive(act, FALSE);
                }
                if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &src_it))
                {
                    act = gtk_action_group_get_action(act_grp, "MoveBmDown");
                    gtk_action_set_sensitive(act, FALSE);
                }
                gtk_tree_path_free(sep_tp);
                gtk_tree_path_free(tp);
            }
        }
        else if(fm_path_is_trash_root(fm_places_item_get_path(item)))
        {
            gtk_action_group_add_actions(act_grp, trash_menu_actions, G_N_ELEMENTS(trash_menu_actions), item);
            gtk_ui_manager_add_ui_from_string(ui, trash_menu_xml, -1, NULL);
        }
    }
    else if(fm_places_item_get_type(item) == FM_PLACES_ITEM_VOLUME)
    {
        GVolume *vol = fm_places_item_get_volume(item);
        GMount* mnt;
        char *unix_path = NULL;
        gtk_action_group_add_actions(act_grp, vol_menu_actions, G_N_ELEMENTS(vol_menu_actions), item);
        gtk_ui_manager_add_ui_from_string(ui, vol_menu_xml, -1, NULL);

        mnt = g_volume_get_mount(vol);
        if(mnt) /* mounted */
        {
            g_object_unref(mnt);
            act = gtk_action_group_get_action(act_grp, "Mount");
            gtk_action_set_visible(act, FALSE);
            act = gtk_action_group_get_action(act_grp, "Unmount");
            gtk_action_set_sensitive(act, g_mount_can_unmount(mnt));
        }
        else /* not mounted */
        {
            if (fm_config->format_cmd && fm_config->format_cmd[0])
                unix_path = g_volume_get_identifier(vol,
                                                    G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
            if (unix_path && unix_path[0] != '/') /* we can format only local */
            {
                g_free(unix_path);
                unix_path = NULL;
            }
            g_free(unix_path); /* use it to mark only */
            act = gtk_action_group_get_action(act_grp, "Unmount");
            gtk_action_set_visible(act, FALSE);
            act = gtk_action_group_get_action(act_grp, "Mount");
            gtk_action_set_sensitive(act, g_volume_can_mount(vol));
        }
        if (unix_path == NULL)
        {
            act = gtk_action_group_get_action(act_grp, "Format");
            if (act)
                 gtk_action_set_visible(act, FALSE);
        }

        if(!g_volume_can_eject(fm_places_item_get_volume(item)))
        {
            act = gtk_action_group_get_action(act_grp, "Eject");
            gtk_action_set_visible(act, FALSE);
        }
    }
    else if(fm_places_item_get_type(item) == FM_PLACES_ITEM_MOUNT)
    {
        GtkAction* act;
        GMount* mnt;
        gtk_action_group_add_actions(act_grp, vol_menu_actions, G_N_ELEMENTS(vol_menu_actions), item);
        gtk_ui_manager_add_ui_from_string(ui, mount_menu_xml, -1, NULL);

        mnt = fm_places_item_get_mount(item);
        if(mnt) /* mounted */
        {
            act = gtk_action_group_get_action(act_grp, "Mount");
            gtk_action_set_sensitive(act, FALSE);
            act = gtk_action_group_get_action(act_grp, "Unmount");
            gtk_action_set_sensitive(act, g_mount_can_unmount(mnt));
        }
        else /* not mounted */
        {
            act = gtk_action_group_get_action(act_grp, "Unmount");
            gtk_action_set_sensitive(act, FALSE);
        }
        act = gtk_action_group_get_action(act_grp, "Format");
        if (act)
             gtk_action_set_visible(act, FALSE);
        act = gtk_action_group_get_action(act_grp, "Eject");
        gtk_action_set_visible(act, FALSE);
    }
    else
        goto _out;
    gtk_ui_manager_insert_action_group(ui, act_grp, 0);

    /* send the signal so popup can be altered by application */
    g_signal_emit(widget, signals[ITEM_POPUP], 0, ui, act_grp, fm_places_item_get_info(item));

    menu = gtk_ui_manager_get_widget(ui, "/popup");
    if(menu)
    {
        g_signal_connect(menu, "selection-done", G_CALLBACK(on_selection_done), NULL);
        g_object_weak_ref(G_OBJECT(menu), place_item_menu_unref, g_object_ref(ui));
        gtk_menu_attach_to_widget(GTK_MENU(menu), widget, NULL);
        /* bug #3614500: widget may be destroyed in selections such as
           Unmount therefore we should detach menu to avoid crash
           note that we should remove this ref when we destroy menu */
        g_object_weak_ref(G_OBJECT(widget), (GWeakNotify)gtk_menu_detach, menu);
        gtk_ui_manager_ensure_update(ui);
    }

_out:
    g_object_unref(act_grp);
    g_object_unref(ui);
    return menu;
}

static void popup_position_func(GtkMenu *menu, gint *x, gint *y,
                                gboolean *push_in, gpointer user_data)
{
    GtkWidget *widget = gtk_menu_get_attach_widget(menu);
    GtkTreeView *view = GTK_TREE_VIEW(widget);
    GtkTreePath *path;
    GdkScreen *screen;
    gint index = GPOINTER_TO_INT(user_data);
    GdkRectangle cell;
    GtkAllocation a, ma;
    gint x2, y2, mon;
    gboolean rtl = (gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL);

    /* realize menu so we get actual size of it */
    gtk_widget_realize(GTK_WIDGET(menu));
    /* get all the relative coordinates */
    gtk_widget_get_allocation(widget, &a);
    screen = gtk_widget_get_screen(widget);
    gdk_window_get_device_position(gtk_widget_get_window(widget),
                                   gdk_device_manager_get_client_pointer(
                                        gdk_display_get_device_manager(
                                            gdk_screen_get_display(screen))),
                                   &x2, &y2, NULL);
    gtk_widget_get_allocation(GTK_WIDGET(menu), &ma);
    path = gtk_tree_path_new_from_indices(index, -1);
    gtk_tree_view_get_cell_area(view, path, gtk_tree_view_get_column(view, 0), &cell);
    gtk_tree_path_free(path);
    /* position menu inside the cell if pointer isn't already in it */
    if(x2 < cell.x || x2 > cell.x + cell.width)
        x2 = cell.x + cell.width/2;
    if(y2 < cell.y || y2 > cell.y + cell.height)
        y2 = cell.y + cell.height - cell.height/8;
    /* get absolute coordinate of parent window - we got coords relative to it */
    gdk_window_get_origin(gtk_widget_get_parent_window(widget), x, y);
    /* calculate desired position for menu */
    *x += a.x + x2;
    *y += a.y + y2;
    /* limit coordinates so menu will be not positioned outside of screen */
    mon = gdk_screen_get_monitor_at_point(screen, *x, *y);
    /* get monitor geometry into the rectangle */
    gdk_screen_get_monitor_geometry(screen, mon, &cell);
    if(rtl) /* RTL */
    {
        x2 = cell.x + cell.width;
        if (*x < cell.x + ma.width) /* out of monitor */
            *x = MIN(*x + ma.width, x2); /* place menu right to cursor */
        else
            *x = MIN(*x, x2);
    }
    else /* LTR */
    {
        if (*x + ma.width > cell.x + cell.width) /* out of monitor */
            *x = MAX(cell.x, *x - ma.width); /* place menu left to cursor */
        else
            *x = MAX(cell.x, *x); /* place menu right to cursor */
    }
    if (*y + ma.height > cell.y + cell.height) /* out of monitor */
        *y = MAX(cell.y, *y - ma.height); /* place menu above cursor */
    else
        *y = MAX(cell.y, *y); /* place menu below cursor */
}

static void fm_places_item_popup(GtkWidget *widget, GtkTreeIter *it, guint32 time)
{
    if(!fm_places_model_iter_is_separator(model, it))
    {
        FmPlacesItem* item;
        GtkMenu* menu;
        GtkTreePath* path;
        gint* indices;
        gtk_tree_model_get(GTK_TREE_MODEL(model), it, FM_PLACES_MODEL_COL_INFO, &item, -1);
        menu = GTK_MENU(place_item_get_menu(item, widget));
        if(menu)
        {
            path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), it);
            indices = gtk_tree_path_get_indices(path);
            gtk_menu_popup(menu, NULL, NULL, popup_position_func,
                           GINT_TO_POINTER(indices[0]), 3, time);
            gtk_tree_path_free(path);
        }
    }
}

static gboolean on_button_press(GtkWidget* widget, GdkEventButton* evt)
{
    FmPlacesView* view = FM_PLACES_VIEW(widget);
    GtkTreePath* tp;
    GtkTreeViewColumn* col;
    GtkTreeIter it;
    gboolean ret = GTK_WIDGET_CLASS(fm_places_view_parent_class)->button_press_event(widget, evt);

    gtk_tree_view_get_path_at_pos(&view->parent, evt->x, evt->y, &tp, &col, NULL, NULL);
    if(view->clicked_row) /* what? more than one botton clicked? */
        gtk_tree_path_free(view->clicked_row);
    view->clicked_row = tp;
    if(tp)
    {
        switch(evt->button) /* middle click */
        {
        case 1: /* left click */
            break;
        case 2: /* middle click */
            activate_row(view, 2, tp);
            break;
        case 3: /* right click */
            if(gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &it, tp))
                fm_places_item_popup(widget, &it, evt->time);
            break;
        }
    }
    return ret;
}

/* handle 'Menu' and 'Shift+F10' here */
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *evt)
{
    GtkTreeModel *model;
    GtkTreeSelection *sel;
    GtkTreeIter it;
    int modifier = (evt->state & gtk_accelerator_get_default_mod_mask());

    if((evt->keyval == GDK_KEY_Menu && !modifier) ||
       (evt->keyval == GDK_KEY_F10 && modifier == GDK_SHIFT_MASK))
    {
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
        if(gtk_tree_selection_get_selected(sel, &model, &it))
        {
            fm_places_item_popup(widget, &it, evt->time);
            return TRUE;
        }
    }
    /* let others do the job */
    return GTK_WIDGET_CLASS(fm_places_view_parent_class)->key_press_event(widget, evt);
}

static void on_mount(GtkAction* act, gpointer user_data)
{
    FmPlacesItem* item = (FmPlacesItem*)user_data;
    if(fm_places_item_get_type(item) == FM_PLACES_ITEM_VOLUME)
    {
        GMount* mnt = g_volume_get_mount(fm_places_item_get_volume(item));
        if(!mnt)
        {
            if(!fm_mount_volume(NULL, fm_places_item_get_volume(item), TRUE))
                return;
        }
        else
            g_object_unref(mnt);
    }
}

static void on_umount(GtkAction* act, gpointer user_data)
{
    FmPlacesItem* item = (FmPlacesItem*)user_data;
    GMount* mnt;
    switch(fm_places_item_get_type(item))
    {
    case FM_PLACES_ITEM_VOLUME:
        mnt = g_volume_get_mount(fm_places_item_get_volume(item));
        break;
    case FM_PLACES_ITEM_MOUNT:
        /* FIXME: this seems to be broken. */
        mnt = g_object_ref(fm_places_item_get_mount(item));
        break;
    default:
        mnt = NULL;
    }

    if(mnt)
    {
        fm_unmount_mount(NULL, mnt, TRUE);
        /* NOTE: the most probably FmPlacesView is destroyed at this point */
        g_object_unref(mnt);
    }
}

static GtkWindow *_get_gtk_window_from_action(GtkAction* act)
{
    /* FIXME: This is very dirty, but it's inevitable. :-( */
    GSList* proxies = gtk_action_get_proxies(act);
    GtkWidget *menu, *view;
    menu = gtk_widget_get_parent(proxies->data);
    if (menu == NULL || !GTK_IS_MENU(menu))
        return NULL;
    view = gtk_menu_get_attach_widget((GtkMenu*)menu);
    return view ? GTK_WINDOW(gtk_widget_get_toplevel(view)) : NULL;
}

static void on_eject(GtkAction* act, gpointer user_data)
{
    FmPlacesItem* item = (FmPlacesItem*)user_data;
    if(fm_places_item_get_type(item) == FM_PLACES_ITEM_VOLUME)
    {
        fm_eject_volume(_get_gtk_window_from_action(act),
                        fm_places_item_get_volume(item), TRUE);
        /* NOTE: the most probably FmPlacesView is destroyed at this point */
    }
}

static void on_format(GtkAction* act, gpointer user_data)
{
    FmPlacesItem* item = (FmPlacesItem*)user_data;
    char *unix_path;

    if (fm_config->format_cmd == NULL || fm_config->format_cmd[0] == 0)
        return;
    unix_path = g_volume_get_identifier(fm_places_item_get_volume(item),
                                        G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    if (unix_path)
    {
        /* call fm_config->format_cmd for device */
        g_debug("formatting %s ...", unix_path);
        FmPath *path = fm_path_new_for_path(unix_path);
        g_free(unix_path);
        FmPathList *paths = fm_path_list_new();
        fm_path_list_push_tail(paths, path);
        fm_path_unref(path);
        fm_launch_command_simple(_get_gtk_window_from_action(act), NULL, 0,
                                 fm_config->format_cmd, paths);
        fm_path_list_unref(paths);
    }
}

static void on_remove_bm(GtkAction* act, gpointer user_data)
{
    FmPlacesItem* item = (FmPlacesItem*)user_data;
    fm_bookmarks_remove(fm_places_model_get_bookmarks(model), fm_places_item_get_bookmark_item(item));
    /* FIXME: remove item from FmPlacesModel or invalidate it right now so
       make duplicate deletions impossible */
}

static void on_rename_bm(GtkAction* act, gpointer user_data)
{
    FmPlacesItem* item = (FmPlacesItem*)user_data;
    char* new_name = fm_get_user_input(_get_gtk_window_from_action(act),
                                       _("Rename Bookmark Item"),
                                       _("Enter a new name:"),
                                       fm_places_item_get_bookmark_item(item)->name);
    if(new_name)
    {
        if(strcmp(new_name, fm_places_item_get_bookmark_item(item)->name))
        {
            fm_bookmarks_rename(fm_places_model_get_bookmarks(model), fm_places_item_get_bookmark_item(item), new_name);
        }
        g_free(new_name);
    }
}

static void on_move_bm_up(GtkAction* act, gpointer user_data)
{
    FmPlacesItem* item = (FmPlacesItem*)user_data;
    int new_pos, sep_pos;
    GtkTreeIter src_it, dest_it;
    GtkTreePath *tp, *sep_tp;

    if(!fm_places_model_get_iter_by_fm_path(model, &src_it,
                                            fm_places_item_get_path(item)))
        return; /* FIXME: print error message */
    /* get index of the separator */
    sep_tp = fm_places_model_get_separator_path(model);
    sep_pos = gtk_tree_path_get_indices(sep_tp)[0];
    tp = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &src_it);
    if(!gtk_tree_path_prev(tp) ||
       (new_pos = gtk_tree_path_get_indices(tp)[0] - sep_pos - 1) < 0 ||
       !gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &dest_it, tp))
        goto _end; /* cannot move it up */
    gtk_list_store_move_before(GTK_LIST_STORE(model), &src_it, &dest_it);
    /* reorder the bookmark item */
    fm_bookmarks_reorder(fm_places_model_get_bookmarks(model), fm_places_item_get_bookmark_item(item), new_pos);
_end:
    gtk_tree_path_free(sep_tp);
    gtk_tree_path_free(tp);
}

static void on_move_bm_down(GtkAction* act, gpointer user_data)
{
    FmPlacesItem* item = (FmPlacesItem*)user_data;
    int new_pos, sep_pos;
    GtkTreeIter src_it, dest_it;
    GtkTreePath *tp, *sep_tp;

    if(!fm_places_model_get_iter_by_fm_path(model, &src_it,
                                            fm_places_item_get_path(item)))
        return; /* FIXME: print error message */
    /* get index of the separator */
    sep_tp = fm_places_model_get_separator_path(model);
    sep_pos = gtk_tree_path_get_indices(sep_tp)[0];
    dest_it = src_it;
    if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &dest_it))
        goto _end; /* cannot move it down */
    gtk_list_store_move_after(GTK_LIST_STORE(model), &src_it, &dest_it);
    /* reorder the bookmark item */
    tp = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &src_it);
    new_pos = gtk_tree_path_get_indices(tp)[0] - sep_pos - 1;
    fm_bookmarks_reorder(fm_places_model_get_bookmarks(model), fm_places_item_get_bookmark_item(item), new_pos);
    gtk_tree_path_free(tp);
_end:
    gtk_tree_path_free(sep_tp);
}

static void on_empty_trash(GtkAction* act, gpointer user_data)
{
    fm_empty_trash(_get_gtk_window_from_action(act));
}

#if !GTK_CHECK_VERSION(3, 0, 0)
static void on_set_scroll_adjustments(GtkTreeView* view, GtkAdjustment* hadj, GtkAdjustment* vadj)
{
    /* we don't want scroll horizontally, so we pass NULL instead of hadj. */
    fm_dnd_set_dest_auto_scroll(GTK_WIDGET(view), NULL, vadj);
    GTK_TREE_VIEW_CLASS(fm_places_view_parent_class)->set_scroll_adjustments(view, hadj, vadj);
}
#endif

static void fm_places_view_set_property(GObject *object,
                                        guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
    FmPlacesView *view = FM_PLACES_VIEW(object);
    const char *home_dir;

    switch( prop_id )
    {
    case PROP_HOME_DIR:
        home_dir = g_value_get_string(value);
        if (home_dir && (!*home_dir || strcmp(home_dir, fm_get_home_dir()) == 0))
            home_dir = NULL;
        g_free(view->home_dir);
        view->home_dir = g_strdup(home_dir);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void fm_places_view_get_property(GObject *object,
                                        guint prop_id,
                                        GValue *value,
                                        GParamSpec *pspec)
{
    FmPlacesView *view = FM_PLACES_VIEW(object);

    switch( prop_id ) {
    case PROP_HOME_DIR:
        if (view->home_dir)
            g_value_set_string(value, view->home_dir);
        else
            g_value_set_string(value, fm_get_home_dir());
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void fm_places_view_class_init(FmPlacesViewClass *klass)
{
    GObjectClass *g_object_class;
    GtkWidgetClass* widget_class;
    GtkTreeViewClass* tv_class;
    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->dispose = fm_places_view_dispose;
    g_object_class->finalize = fm_places_view_finalize;

    widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->key_press_event = on_key_press;
    widget_class->button_press_event = on_button_press;
    widget_class->button_release_event = on_button_release;
    widget_class->drag_motion = on_drag_motion;
    widget_class->drag_leave = on_drag_leave;
    widget_class->drag_drop = on_drag_drop;
    widget_class->drag_data_received = on_drag_data_received;

    tv_class = GTK_TREE_VIEW_CLASS(klass);
    tv_class->row_activated = on_row_activated;
#if !GTK_CHECK_VERSION(3, 0, 0)
    tv_class->set_scroll_adjustments = on_set_scroll_adjustments;
#endif

    g_object_class->get_property = fm_places_view_get_property;
    g_object_class->set_property = fm_places_view_set_property;

    /**
     * FmPlacesView:home-dir-path:
     *
     * The #FmPlacesView:home-dir-path property defines which path will
     * be used on Home item activation. Value of %NULL resets it to the
     * default.
     *
     * Since: 1.2.0
     */
    g_object_class_install_property(g_object_class,
                                    PROP_HOME_DIR,
                                    g_param_spec_string("home-dir-path",
                                                        "Home item directory",
                                                        "What directory path will be used for Home item",
                                                        NULL,
                                                        G_PARAM_READWRITE));

    /**
     * FmPlacesView::chdir:
     * @view: a view instance that emitted the signal
     * @button: the button row was activated with
     * @path: (#FmPath *) new directory path
     *
     * The #FmPlacesView::chdir signal is emitted when current selected
     * directory in view is changed.
     *
     * Since: 0.1.0
     */
    signals[CHDIR] =
        g_signal_new("chdir",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(FmPlacesViewClass, chdir),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__UINT_POINTER,
                     G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_POINTER);

    /**
     * FmPlacesView::item-popup:
     * @view: a view instance that emitted the signal
     * @ui: the #GtkUIManager using to create the menu
     * @act_grp: (#GtkActionGroup *) the menu actions group
     * @fi: (#FmFileInfo *) the item where menu popup is activated
     *
     * The #FmPlacesView::item-popup signal is emitted when context menu
     * is created for any directory in the view. Handler can modify the
     * menu by adding or removing elements.
     *
     * Since: 1.2.0
     */
    signals[ITEM_POPUP] =
        g_signal_new("item-popup",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(FmPlacesViewClass, item_popup),
                     NULL, NULL,
                     fm_marshal_VOID__OBJECT_OBJECT_POINTER,
                     G_TYPE_NONE, 3, G_TYPE_OBJECT, G_TYPE_OBJECT, G_TYPE_POINTER);

    tree_model_row_atom = gdk_atom_intern_static_string("GTK_TREE_MODEL_ROW");
}
