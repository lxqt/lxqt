/*
 *      main-win.c
 *
 *      Copyright 2009 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "main-win.h"
#include "fm-folder-view.h"
#include "fm-standard-view.h"
#include "fm-folder-model.h"
#include "fm-path-entry.h"
#include "fm-file-menu.h"
#include "fm-clipboard.h"
#include "fm-gtk-utils.h"
#include "fm-gtk-file-launcher.h"
#include "fm-side-pane.h"
#include "fm-menu-tool-item.h"

static void fm_main_win_finalize              (GObject *object);
static void fm_main_win_dispose              (GObject *object);

G_DEFINE_TYPE(FmMainWin, fm_main_win, GTK_TYPE_WINDOW);

/* static void on_new_tab(GtkAction* act, FmMainWin* win); */
static void on_new_win(GtkAction* act, FmMainWin* win);
static void on_close_win(GtkAction* act, FmMainWin* win);

/* static void on_open_in_new_tab(GtkAction* act, FmMainWin* win); */
static void on_open_in_new_win(GtkAction* act, FmMainWin* win);

static void on_copy_to(GtkAction* act, FmMainWin* win);
static void on_move_to(GtkAction* act, FmMainWin* win);
static void on_rename(GtkAction* act, FmMainWin* win);

static void bounce_action(GtkAction* act, FmMainWin* win);

static void on_go(GtkAction* act, FmMainWin* win);
static void on_go_back(GtkAction* act, FmMainWin* win);
static void on_go_forward(GtkAction* act, FmMainWin* win);
static void on_go_up(GtkAction* act, FmMainWin* win);
static void on_go_home(GtkAction* act, FmMainWin* win);
static void on_go_desktop(GtkAction* act, FmMainWin* win);
static void on_go_trash(GtkAction* act, FmMainWin* win);
static void on_go_computer(GtkAction* act, FmMainWin* win);
static void on_go_network(GtkAction* act, FmMainWin* win);
static void on_go_apps(GtkAction* act, FmMainWin* win);
static void on_reload(GtkAction* act, FmMainWin* win);
static void on_show_hidden(GtkToggleAction* act, FmMainWin* win);
static void on_change_mode(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win);
static void on_sort_by(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win);
static void on_sort_type(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win);
static void on_about(GtkAction* act, FmMainWin* win);
static void on_search(GtkAction* act, FmMainWin* win);

static void on_location(GtkAction* act, FmMainWin* win);

static void on_side_pane_chdir(FmSidePane* sp, guint button, FmPath* path, FmMainWin* win);

static void update_statusbar(FmMainWin* win);
static void on_folder_start_loading(FmFolder* folder, FmMainWin* win);
static void on_folder_finish_loading(FmFolder* folder, FmMainWin* win);
static void on_folder_unmount(FmFolder* folder, FmMainWin* win);
static void on_folder_removed(FmFolder* folder, FmMainWin* win);
static void on_folder_fs_info(FmFolder* folder, FmMainWin* win);
static FmJobErrorAction on_folder_error(FmFolder* folder, GError* err, FmJobErrorSeverity severity, FmMainWin* win);

static void free_folder(FmMainWin* win);

#include "main-win-ui.c" /* ui xml definitions and actions */

static guint n_wins = 0;

static void fm_main_win_class_init(FmMainWinClass *klass)
{
    GObjectClass *g_object_class;
    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->dispose = fm_main_win_dispose;
    g_object_class->finalize = fm_main_win_finalize;
    fm_main_win_parent_class = (GtkWindowClass*)g_type_class_peek(GTK_TYPE_WINDOW);
}

static void on_entry_activate(GtkEntry* entry, FmMainWin* win)
{
    FmPath* path = fm_path_entry_get_path(FM_PATH_ENTRY(entry));
    fm_main_win_chdir(win, path);
    if (win->pathbar_active)
    {
        gtk_widget_set_visible(win->location, FALSE);
        gtk_widget_set_visible(gtk_ui_manager_get_widget(win->ui, "/toolbar/Go"), FALSE);
        gtk_widget_set_visible(win->pathbar, TRUE);
    }
}

static void on_pathbar_chdir(FmPathBar *bar, FmPath *path, FmMainWin *win)
{
    fm_main_win_chdir(win, path);
}

static void on_path_mode(GtkRadioAction *act, GtkRadioAction *cur, FmMainWin *win)
{
    int mode = gtk_radio_action_get_current_value(cur);

    if (mode == win->pathbar_active)
        return;
    win->pathbar_active = mode;
    gtk_widget_set_visible(win->location, mode == 0);
    gtk_widget_set_visible(gtk_ui_manager_get_widget(win->ui, "/toolbar/Go"), mode == 0);
    gtk_widget_set_visible(win->pathbar, mode);
}

static void update_statusbar(FmMainWin* win)
{
    char* msg;
    FmFolderModel* model = fm_folder_view_get_model(win->folder_view);
    FmFolder* folder = win->folder;
    if(model && folder)
    {
        FmFileInfoList* files = fm_folder_get_files(folder);
        int total_files = fm_file_info_list_get_length(files);
        int shown_files = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL);

        msg = g_strdup_printf("%d files are listed (%d hidden).", shown_files, (total_files - shown_files) );
        gtk_statusbar_pop(GTK_STATUSBAR(win->statusbar), win->statusbar_ctx);
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), win->statusbar_ctx, msg);
        g_free(msg);

        fm_folder_query_filesystem_info(folder);
    }
}

static FmJobErrorAction on_folder_error(FmFolder* folder, GError* err, FmJobErrorSeverity severity, FmMainWin* win)
{
    if(err->domain == G_IO_ERROR)
    {
        if( err->code == G_IO_ERROR_NOT_MOUNTED && severity < FM_JOB_ERROR_CRITICAL )
        {
            FmPath* path = fm_folder_get_path(folder);
            if(fm_mount_path(GTK_WINDOW(win), path, TRUE))
                return FM_JOB_RETRY;
        }
    }
    fm_show_error(GTK_WINDOW(win), NULL, err->message);
    return FM_JOB_CONTINUE;
}

static void on_folder_start_loading(FmFolder* folder, FmMainWin* win)
{
    FmFolderModel* model;

    g_debug("start-loading");
    fm_set_busy_cursor(GTK_WIDGET(win));

    if(fm_folder_is_incremental(folder))
    {
        /* create a model for the folder and set it to the view
           it is delayed for non-incremental folders since adding rows into
           model is much faster without handlers connected to its signals */
        model = fm_folder_model_new(folder, FALSE);
        fm_folder_view_set_model(win->folder_view, model);
        /* create folder popup and apply shortcuts from it */
        fm_folder_view_add_popup(win->folder_view, GTK_WINDOW(win), NULL);
        g_object_unref(model);
    }
    else
        fm_folder_view_set_model(win->folder_view, NULL);
}

static gboolean update_scroll(gpointer data)
{
    FmMainWin* win = data;
    GtkScrolledWindow* scroll = GTK_SCROLLED_WINDOW(win->folder_view);
    gint scroll_pos;

    /* scroll to recorded position */
    scroll_pos = fm_nav_history_get_scroll_pos(win->nav_history);
    gtk_adjustment_set_value(gtk_scrolled_window_get_vadjustment(scroll),
                             scroll_pos);
    win->update_scroll_id = 0;
    return FALSE;
}

static void on_folder_finish_loading(FmFolder* folder, FmMainWin* win)
{
    FmFolderView* fv = win->folder_view;
    FmPath* path = fm_folder_get_path(folder);
    FmFileInfo* info = fm_folder_get_info(folder);
    FmPathEntry* entry = FM_PATH_ENTRY(win->location);

    fm_path_entry_set_path(entry, path);
    fm_path_bar_set_path(FM_PATH_BAR(win->pathbar), path);

    if(fm_folder_view_get_model(fv) == NULL)
    {
        /* create a model for the folder and set it to the view */
        FmFolderModel* model = fm_folder_model_new(folder, FALSE);
        fm_folder_view_set_model(fv, model);
        /* create folder popup and apply shortcuts from it */
        fm_folder_view_add_popup(win->folder_view, GTK_WINDOW(win), NULL);
        g_object_unref(model);
    }

    /* delaying scrolling since drawing folder view is delayed - don't know why */
    if(!win->update_scroll_id)
        win->update_scroll_id = g_timeout_add(50, update_scroll, win);

    /* update status bar */
    update_statusbar(win);

    if(info) /* not failed */
        gtk_window_set_title(GTK_WINDOW(win), fm_file_info_get_disp_name(info));
    else
        gtk_window_set_title(GTK_WINDOW(win), "(no folder)");

    fm_unset_busy_cursor(GTK_WIDGET(win));
    g_debug("finish-loading");
}

static void on_folder_unmount(FmFolder* folder, FmMainWin* win)
{
    free_folder(win);
    gtk_widget_destroy(GTK_WIDGET(win));
}

static void on_folder_removed(FmFolder* folder, FmMainWin* win)
{
    free_folder(win);
    gtk_widget_destroy(GTK_WIDGET(win));
}

static gboolean open_folder_func(GAppLaunchContext* ctx, GList* folder_infos, gpointer user_data, GError** err)
{
    FmMainWin* win = FM_MAIN_WIN(user_data);
    GList* l = folder_infos;
    FmFileInfo* fi = (FmFileInfo*)l->data;
    fm_main_win_chdir(win, fm_file_info_get_path(fi));
    l=l->next;
    for(; l; l=l->next)
    {
        FmFileInfo* fi = (FmFileInfo*)l->data;
        win = fm_main_win_new();
        fm_main_win_chdir(win, fm_file_info_get_path(fi));
        gtk_window_present(GTK_WINDOW(win));
    }
    return TRUE;
}

static gboolean open_search_func(GAppLaunchContext* ctx, GList* folder_infos, gpointer user_data, GError** err)
{
    FmMainWin* win;
    GList* l = folder_infos;
    FmFileInfo* fi = (FmFileInfo*)l->data;
    GSList* cols = NULL;
    const FmFolderViewColumnInfo col_infos[] = {
        {FM_FOLDER_MODEL_COL_NAME},
        {FM_FOLDER_MODEL_COL_DESC},
        {FM_FOLDER_MODEL_COL_DIRNAME},
        {FM_FOLDER_MODEL_COL_SIZE},
        {FM_FOLDER_MODEL_COL_MTIME} };
    guint i;

    win = fm_main_win_new();
    fm_main_win_chdir(win, fm_file_info_get_path(fi));
    fm_standard_view_set_mode(FM_STANDARD_VIEW(win->folder_view), FM_FV_LIST_VIEW);
    for(i = 0; i < G_N_ELEMENTS(col_infos); i++)
        cols = g_slist_append(cols, (gpointer)&col_infos[i]);
    fm_folder_view_set_columns(win->folder_view, cols);
    g_slist_free(cols);
    gtk_window_present(GTK_WINDOW(win));
    l=l->next;
    for(; l; l=l->next)
    {
        /*FmFileInfo* fi = (FmFileInfo*)l->data;
        FIXME: open in new window */
    }
    return TRUE;
}

static void update_files_popup(FmFolderView* fv, GtkWindow* win,
                               GtkUIManager* ui, GtkActionGroup* act_grp,
                               FmFileInfoList* files)
{
    GList *file = fm_file_info_list_peek_head_link(files);
    while (file)
    {
        if (!fm_file_info_is_dir(file->data))
            return;
        file = file->next;
    }
    gtk_action_group_add_actions(act_grp, folder_menu_actions, G_N_ELEMENTS(folder_menu_actions), win);
    g_object_set_qdata_full(G_OBJECT(act_grp), fm_qdata_id,
                            fm_file_info_list_ref(files),
                            (GDestroyNotify)fm_file_info_list_unref);
    gtk_ui_manager_add_ui_from_string(ui, folder_menu_xml, -1, NULL);
}

static void update_sidebar_popup(FmSidePane* sp, GtkUIManager* ui,
                                 GtkActionGroup* act_grp,
                                 FmFileInfo* file, gpointer win)
{
    FmFileInfoList* files;
    if (!file || !fm_file_info_is_dir(file))
        return;
    files = fm_file_info_list_new();
    fm_file_info_list_push_tail(files, file);
    gtk_action_group_add_actions(act_grp, folder_menu_actions, G_N_ELEMENTS(folder_menu_actions), win);
    g_object_set_qdata_full(G_OBJECT(act_grp), fm_qdata_id, files,
                            (GDestroyNotify)fm_file_info_list_unref);
    gtk_ui_manager_add_ui_from_string(ui, folder_menu_xml, -1, NULL);
}

static void on_file_clicked(FmFolderView* fv, FmFolderViewClickType type, FmFileInfo* fi, FmMainWin* win)
{
    switch(type)
    {
    case FM_FV_ACTIVATED: /* file activated */
        break; /* handled by FmFolderView */
    case FM_FV_CONTEXT_MENU:
        break; /* handled by FmFolderView */
    case FM_FV_MIDDLE_CLICK:
        g_debug("middle click!");
    case FM_FV_CLICK_NONE:
        break;
    }
}

static void on_sel_changed(FmFolderView* fv, gint n_files, FmMainWin* win)
{
    /* popup previous message if there is any */
    gtk_statusbar_pop(GTK_STATUSBAR(win->statusbar), win->statusbar_ctx2);
    if(n_files)
    {
        char* msg;
        /* FIXME: display total size of all selected files. */
        if(n_files == 1) /* only one file is selected */
        {
            FmFileInfoList* files = fm_folder_view_dup_selected_files(fv);
            FmFileInfo* fi = fm_file_info_list_peek_head(files);
            const char* size_str = fm_file_info_get_disp_size(fi);
            fm_file_info_list_unref(files);
            if(size_str)
            {
                msg = g_strdup_printf("\"%s\" (%s) %s",
                            fm_file_info_get_disp_name(fi),
                            size_str ? size_str : "",
                            fm_file_info_get_desc(fi));
            }
            else
            {
                msg = g_strdup_printf("\"%s\" %s",
                            fm_file_info_get_disp_name(fi),
                            fm_file_info_get_desc(fi));
            }
        }
        else
            msg = g_strdup_printf("%d items selected", n_files);
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), win->statusbar_ctx2, msg);
        g_free(msg);
    }
}

static void on_bookmark(GtkMenuItem* mi, FmMainWin* win)
{
    FmPath* path = (FmPath*)g_object_get_qdata(G_OBJECT(mi), fm_qdata_id);
    fm_main_win_chdir(win, path);
}

static void create_bookmarks_menu(FmMainWin* win)
{
    GList *items, *l;
    int i = 0;

    items = fm_bookmarks_get_all(win->bookmarks);
    for(l=items;l;l=l->next)
    {
        FmBookmarkItem* item = (FmBookmarkItem*)l->data;
        GtkWidget* mi = gtk_image_menu_item_new_with_label(item->name);
        gtk_widget_show(mi);
        // gtk_image_menu_item_set_image(); // FIXME: set icons for menu items
        g_object_set_qdata_full(G_OBJECT(mi), fm_qdata_id, fm_path_ref(item->path), (GDestroyNotify)fm_path_unref);
        g_signal_connect(mi, "activate", G_CALLBACK(on_bookmark), win);
        gtk_menu_shell_insert(GTK_MENU_SHELL(win->bookmarks_menu), mi, i);
        ++i;
        fm_bookmark_item_unref(item);
    }
    g_list_free(items);
    if(i > 0)
        gtk_menu_shell_insert(GTK_MENU_SHELL(win->bookmarks_menu), gtk_separator_menu_item_new(), i);
}

static void on_bookmarks_changed(FmBookmarks* bm, FmMainWin* win)
{
    /* delete old items first. */
    GList* mis = gtk_container_get_children(GTK_CONTAINER(win->bookmarks_menu));
    GList* l;
    for(l = mis;l;l=l->next)
    {
        GtkWidget* item = (GtkWidget*)l->data;
        if( GTK_IS_SEPARATOR_MENU_ITEM(item) )
            break;
        gtk_widget_destroy(item);
    }
    g_list_free(mis);

    create_bookmarks_menu(win);
}

static void load_bookmarks(FmMainWin* win, GtkUIManager* ui)
{
    GtkWidget* mi = gtk_ui_manager_get_widget(ui, "/menubar/BookmarksMenu");
    win->bookmarks_menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(mi));
    win->bookmarks = fm_bookmarks_dup();
    g_signal_connect(win->bookmarks, "changed", G_CALLBACK(on_bookmarks_changed), win);

    create_bookmarks_menu(win);
}

static void on_history_item(GtkMenuItem* mi, FmMainWin* win)
{
    gpointer i = g_object_get_qdata(G_OBJECT(mi), fm_qdata_id);
    int scroll_pos = gtk_adjustment_get_value(gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(win->folder_view)));
    FmPath* path = fm_nav_history_go_to(win->nav_history, GPOINTER_TO_UINT(i), scroll_pos);
    /* FIXME: should this be driven by a signal emitted on FmNavHistory? */
    fm_main_win_chdir_without_history(win, path);
}

static void on_show_history_menu(FmMenuToolItem* btn, FmMainWin* win)
{
    GtkMenuShell* menu = (GtkMenuShell*)fm_menu_tool_item_get_menu(btn);
    FmPath* path;
    guint n, cur = fm_nav_history_get_cur_index(win->nav_history);

    /* delete old items */
    gtk_container_foreach(GTK_CONTAINER(menu), (GtkCallback)gtk_widget_destroy, NULL);

    for(n = 0; (path = fm_nav_history_get_nth_path(win->nav_history, n)); n++)
    {
        char* str;
        GtkWidget* mi;

        str = fm_path_display_name(path, TRUE);
        if(n == cur)
        {
            mi = gtk_check_menu_item_new_with_label(str);
            gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(mi), TRUE);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi), TRUE);
        }
        else
            mi = gtk_menu_item_new_with_label(str);
        g_free(str);

        g_object_set_qdata_full(G_OBJECT(mi), fm_qdata_id, GUINT_TO_POINTER(n), NULL);
        g_signal_connect(mi, "activate", G_CALLBACK(on_history_item), win);
        gtk_menu_shell_append(menu, mi);
    }
    gtk_widget_show_all( GTK_WIDGET(menu) );
}

static void on_side_pane_chdir(FmSidePane* sp, guint button, FmPath* path, FmMainWin* win)
{
    g_debug("on_side_pane_chdir");
//    g_signal_handlers_block_by_func(win->dirtree_view, on_dirtree_chdir, win);
    fm_main_win_chdir(win, path);
//    g_signal_handlers_unblock_by_func(win->dirtree_view, on_dirtree_chdir, win);
}

static void fm_main_win_init(FmMainWin *win)
{
    GtkWidget *vbox, *menubar;
    GtkBox *pathbox;
    GtkToolItem *toolitem;
    GtkUIManager* ui;
    GtkActionGroup* act_grp;
    GtkAction* act;
    GtkAccelGroup* accel_grp;
    GtkRadioAction *mode_action;
    GSList *radio_group;
    GString *str, *xml;
    static char accel_str[] = "<Ctrl>1";
    GtkShadowType shadow_type;
    int i;
    gboolean is_first;

    ++n_wins;

    vbox = gtk_vbox_new(FALSE, 0);

    win->hpaned = gtk_hpaned_new();
    gtk_paned_set_position(GTK_PANED(win->hpaned), 150);

    /* places left pane */
    win->left_pane = fm_side_pane_new();
    fm_side_pane_set_mode(win->left_pane, FM_SP_PLACES);
    fm_side_pane_set_popup_updater(win->left_pane, update_sidebar_popup, win);
    gtk_paned_add1(GTK_PANED(win->hpaned), GTK_WIDGET(win->left_pane));

    /* folder view */
    win->folder_view = (FmFolderView*)fm_standard_view_new(FM_FV_ICON_VIEW, update_files_popup, open_folder_func);
    fm_folder_view_set_show_hidden(win->folder_view, FALSE);
    fm_folder_view_set_selection_mode(win->folder_view, GTK_SELECTION_MULTIPLE);
    g_signal_connect(win->folder_view, "clicked", G_CALLBACK(on_file_clicked), win);
    g_signal_connect(win->folder_view, "sel-changed", G_CALLBACK(on_sel_changed), win);

    gtk_paned_add2(GTK_PANED(win->hpaned), GTK_WIDGET(win->folder_view));

    /* link places view with folder view. */
    g_signal_connect(win->left_pane, "chdir", G_CALLBACK(on_side_pane_chdir), win);

    /* create menu bar and toolbar */
    ui = gtk_ui_manager_new();
    act_grp = gtk_action_group_new("Main");
    gtk_action_group_add_actions(act_grp, main_win_actions, G_N_ELEMENTS(main_win_actions), win);
    gtk_action_group_add_toggle_actions(act_grp, main_win_toggle_actions, G_N_ELEMENTS(main_win_toggle_actions), win);
    gtk_action_group_add_radio_actions(act_grp, main_win_sort_type_actions, G_N_ELEMENTS(main_win_sort_type_actions), GTK_SORT_ASCENDING, G_CALLBACK(on_sort_type), win);
    gtk_action_group_add_radio_actions(act_grp, main_win_sort_by_actions, G_N_ELEMENTS(main_win_sort_by_actions), 0, G_CALLBACK(on_sort_by), win);
    gtk_action_group_add_radio_actions(act_grp, main_win_pathbar_actions, G_N_ELEMENTS(main_win_pathbar_actions), 0, G_CALLBACK(on_path_mode), win);

    /* implement gtk_action_group_add_radio_actions() for dynamic list */
    radio_group = NULL;
    is_first = TRUE;
    str = g_string_new("Mode:");
    xml = g_string_new("<menubar><menu action='ViewMenu'><placeholder name='ph1'>");
    accel_str[6] = '1';
    for(i = 0; i < fm_standard_view_get_n_modes(); i++)
    {
        if(fm_standard_view_get_mode_label(i))
        {
            g_string_append(str, fm_standard_view_mode_to_str(i));
            mode_action = gtk_radio_action_new(str->str,
                                               fm_standard_view_get_mode_label(i),
                                               fm_standard_view_get_mode_tooltip(i),
                                               fm_standard_view_get_mode_icon(i),
                                               i);
            gtk_radio_action_set_group(mode_action, radio_group);
            radio_group = gtk_radio_action_get_group(mode_action);
            gtk_action_group_add_action_with_accel(act_grp,
                                                   GTK_ACTION(mode_action),
                                                   accel_str);
            if (is_first) /* work on first one only */
                g_signal_connect(mode_action, "changed", G_CALLBACK(on_change_mode), win);
            is_first = FALSE;
            g_object_unref(mode_action);
            g_string_append_printf(xml, "<menuitem action='%s'/>", str->str);
            accel_str[6]++; /* <Ctrl>2 and so on */
            g_string_truncate(str, 5);
        }
    }
    g_string_append(xml, "</placeholder></menu></menubar>");
    g_string_free(str, TRUE);

    accel_grp = gtk_ui_manager_get_accel_group(ui);
    gtk_window_add_accel_group(GTK_WINDOW(win), accel_grp);

    gtk_ui_manager_insert_action_group(ui, act_grp, 0);
    gtk_ui_manager_add_ui_from_string(ui, main_menu_xml, -1, NULL);
    /* add ui generated above */
    gtk_ui_manager_add_ui_from_string(ui, xml->str, xml->len, NULL);
    g_string_free(xml, TRUE);

    /* disable "Find Files" button if module isn't available */
    if (!fm_module_is_in_use("vfs", "search"))
    {
        act = gtk_ui_manager_get_action(ui, "/menubar/GoMenu/Search");
        gtk_action_set_sensitive(act, FALSE);
    }
    /* disable "Applications" button if module isn't available */
    if (!fm_module_is_in_use("vfs", "menu"))
    {
        act = gtk_ui_manager_get_action(ui, "/menubar/GoMenu/Apps");
        gtk_action_set_sensitive(act, FALSE);
    }

    menubar = gtk_ui_manager_get_widget(ui, "/menubar");

    win->toolbar = gtk_ui_manager_get_widget(ui, "/toolbar");
    /* FIXME: should make these optional */
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(win->toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_toolbar_set_style(GTK_TOOLBAR(win->toolbar), GTK_TOOLBAR_ICONS);

    /* create history button manually and add a popup menu to it */
    toolitem = fm_menu_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(win->toolbar), toolitem, 2);
    gtk_widget_show(GTK_WIDGET(toolitem));

    /* set up history menu */
    win->nav_history = fm_nav_history_new();
    win->history_menu = gtk_menu_new();
    fm_menu_tool_item_set_menu(FM_MENU_TOOL_ITEM(toolitem), win->history_menu);
    g_signal_connect(toolitem, "show-menu", G_CALLBACK(on_show_history_menu), win);

    gtk_box_pack_start( (GtkBox*)vbox, menubar, FALSE, TRUE, 0 );
    gtk_box_pack_start( (GtkBox*)vbox, win->toolbar, FALSE, TRUE, 0 );

    /* load bookmarks menu */
    load_bookmarks(win, ui);

    /* the location bar */
    win->location = (GtkWidget*)fm_path_entry_new();
    g_signal_connect(win->location, "activate", G_CALLBACK(on_entry_activate), win);
    win->pathbar = (GtkWidget*)fm_path_bar_new();
    g_signal_connect(win->pathbar, "chdir", G_CALLBACK(on_pathbar_chdir), win);
    pathbox = (GtkBox*)gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(pathbox, win->location, TRUE, TRUE, 0);
    gtk_box_pack_start(pathbox, win->pathbar, TRUE, TRUE, 0);
    win->pathbar_active = FALSE;

    toolitem = gtk_tool_item_new();
    gtk_container_add( GTK_CONTAINER(toolitem), GTK_WIDGET(pathbox) );
    gtk_tool_item_set_expand(GTK_TOOL_ITEM(toolitem), TRUE);
    gtk_toolbar_insert((GtkToolbar*)win->toolbar, toolitem, gtk_toolbar_get_n_items(GTK_TOOLBAR(win->toolbar)) - 1 );

    gtk_box_pack_start( (GtkBox*)vbox, win->hpaned, TRUE, TRUE, 0 );

    /* status bar */
    win->statusbar = gtk_statusbar_new();
    /* status bar column showing volume free space */
    gtk_widget_style_get(win->statusbar, "shadow-type", &shadow_type, NULL);
    win->vol_status = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(win->vol_status), shadow_type);
    gtk_box_pack_start(GTK_BOX(win->statusbar), win->vol_status, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(win->vol_status), gtk_label_new(NULL));

    gtk_box_pack_start( (GtkBox*)vbox, win->statusbar, FALSE, TRUE, 0 );
    win->statusbar_ctx = gtk_statusbar_get_context_id(GTK_STATUSBAR(win->statusbar), "status");
    win->statusbar_ctx2 = gtk_statusbar_get_context_id(GTK_STATUSBAR(win->statusbar), "status2");


    g_object_unref(act_grp);
    win->ui = ui;

    gtk_container_add( (GtkContainer*)win, vbox );
    gtk_widget_show_all(vbox);
    gtk_widget_hide(win->pathbar);

    gtk_window_set_default_size(GTK_WINDOW(win), 640, 480);
    gtk_widget_realize(GTK_WIDGET(win));

    fm_folder_view_set_show_hidden(win->folder_view, FALSE);
    fm_main_win_chdir(win, fm_path_get_home());

    gtk_widget_grab_focus(GTK_WIDGET(win->folder_view));
}


FmMainWin* fm_main_win_new(void)
{
    return (FmMainWin*)g_object_new(FM_MAIN_WIN_TYPE, NULL);
}

static void free_folder(FmMainWin* win)
{
    if(win->folder)
    {
        g_signal_handlers_disconnect_by_func(win->folder, on_folder_start_loading, win);
        g_signal_handlers_disconnect_by_func(win->folder, on_folder_finish_loading, win);
        g_signal_handlers_disconnect_by_func(win->folder, on_folder_fs_info, win);
        g_signal_handlers_disconnect_by_func(win->folder, on_folder_error, win);
        g_signal_handlers_disconnect_by_func(win->folder, on_folder_removed, win);
        g_signal_handlers_disconnect_by_func(win->folder, on_folder_unmount, win);
        g_object_unref(win->folder);
        win->folder = NULL;
    }
}

static void fm_main_win_dispose(GObject *object)
{
    FmMainWin *win = FM_MAIN_WIN(object);
    g_debug("fm_main_win_dispose");
    free_folder(win);

    if(win->nav_history)
    {
        g_object_unref(win->nav_history);
        win->nav_history = NULL;
    }
    if(win->ui)
    {
        g_object_unref(win->ui);
        win->ui = NULL;
    }
    if(win->bookmarks)
    {
        g_object_unref(win->bookmarks);
        win->bookmarks = NULL;
    }
    if(win->update_scroll_id)
    {
        g_source_remove(win->update_scroll_id);
        win->update_scroll_id = 0;
    }
    (* G_OBJECT_CLASS(fm_main_win_parent_class)->dispose)(object);
}

static void fm_main_win_finalize(GObject *object)
{
    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_MAIN_WIN(object));

    --n_wins;

    if (G_OBJECT_CLASS(fm_main_win_parent_class)->finalize)
        (* G_OBJECT_CLASS(fm_main_win_parent_class)->finalize)(object);

    if(n_wins == 0)
        gtk_main_quit();
}

void on_about(GtkAction* act, FmMainWin* win)
{
    const char* authors[]={"Hong Jen Yee <pcman.tw@gmail.com>", NULL};
    GtkWidget* dlg = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dlg), "libfm-demo");
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dlg), authors);
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dlg), "A demo program for libfm");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dlg), "http://pcmanfm.sf.net/");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

void on_show_hidden(GtkToggleAction* act, FmMainWin* win)
{
    gboolean active = gtk_toggle_action_get_active(act);
    fm_folder_view_set_show_hidden(win->folder_view, active);
    fm_side_pane_set_show_hidden(win->left_pane, active);
    update_statusbar(win);
}

void on_change_mode(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win)
{
    int mode = gtk_radio_action_get_current_value(cur);
    fm_standard_view_set_mode(FM_STANDARD_VIEW(win->folder_view), mode);
}

void on_sort_by(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win)
{
    guint val = gtk_radio_action_get_current_value(cur);
    FmFolderModel *model = fm_folder_view_get_model(win->folder_view);

    if(model)
        fm_folder_model_set_sort(model, val, FM_SORT_DEFAULT);
}

void on_sort_type(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win)
{
    guint val = gtk_radio_action_get_current_value(cur);
    FmFolderModel *model = fm_folder_view_get_model(win->folder_view);
    FmSortMode mode;

    if(model)
    {
        fm_folder_model_get_sort(model, NULL, &mode);
        mode &= ~FM_SORT_ORDER_MASK;
        mode |= (val == GTK_SORT_ASCENDING) ? FM_SORT_ASCENDING : FM_SORT_DESCENDING;
        fm_folder_model_set_sort(model, FM_FOLDER_MODEL_COL_DEFAULT, mode);
    }
}

void on_new_win(GtkAction* act, FmMainWin* win)
{
    win = fm_main_win_new();
    fm_main_win_chdir(win, fm_path_get_home());
    gtk_window_present(GTK_WINDOW(win));
}

void on_close_win(GtkAction* act, FmMainWin* win)
{
    gtk_widget_destroy(GTK_WIDGET(win));
}

/* void on_open_in_new_tab(GtkAction* act, FmMainWin* win);
{
}
*/

void on_open_in_new_win(GtkAction* act, FmMainWin* win)
{
    GObject* act_grp;
    FmFileInfoList* sels;
    g_object_get(act, "action-group", &act_grp, NULL);
    sels = g_object_get_qdata(act_grp, fm_qdata_id);
    g_object_unref(act_grp);
    GList* l;
    for( l = fm_file_info_list_peek_head_link(sels); l; l=l->next )
    {
        FmFileInfo* fi = (FmFileInfo*)l->data;
        win = fm_main_win_new();
        fm_main_win_chdir(win, fm_file_info_get_path(fi));
        gtk_window_present(GTK_WINDOW(win));
    }
}

static void on_search(GtkAction* act, FmMainWin* win)
{
    FmPath* cwd = fm_folder_get_path(win->folder);
    GList* l = g_list_append(NULL, cwd);
    fm_launch_search_simple(GTK_WINDOW(win), NULL, l, open_search_func, win);
    g_list_free(l);
}

void on_go(GtkAction* act, FmMainWin* win)
{
    fm_main_win_chdir_by_name( win, gtk_entry_get_text(GTK_ENTRY(win->location)));
}

void on_go_back(GtkAction* act, FmMainWin* win)
{
    if(fm_nav_history_can_back(win->nav_history))
    {
        guint prev = fm_nav_history_get_cur_index(win->nav_history) + 1;
        int scroll_pos = gtk_adjustment_get_value(gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(win->folder_view)));
        FmPath* path = fm_nav_history_go_to(win->nav_history, prev, scroll_pos);
        /* FIXME: should this be driven by a signal emitted on FmNavHistory? */
        fm_main_win_chdir_without_history(win, path);
    }
}

void on_go_forward(GtkAction* act, FmMainWin* win)
{
    guint cur = fm_nav_history_get_cur_index(win->nav_history);

    if(cur > 0)
    {
        int scroll_pos = gtk_adjustment_get_value(gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(win->folder_view)));
        FmPath* path = fm_nav_history_go_to(win->nav_history, cur - 1, scroll_pos);
        /* FIXME: should this be driven by a signal emitted on FmNavHistory? */
        fm_main_win_chdir_without_history(win, path);
    }
}

void on_go_up(GtkAction* act, FmMainWin* win)
{
    FmPath* parent = fm_path_get_parent(fm_folder_get_path(win->folder));
    if(parent)
        fm_main_win_chdir( win, parent);
}

void on_go_home(GtkAction* act, FmMainWin* win)
{
    fm_main_win_chdir_by_name( win, fm_get_home_dir());
}

void on_go_desktop(GtkAction* act, FmMainWin* win)
{
    fm_main_win_chdir_by_name( win, g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP));
}

void on_go_trash(GtkAction* act, FmMainWin* win)
{
    fm_main_win_chdir_by_name( win, "trash:///");
}

void on_go_computer(GtkAction* act, FmMainWin* win)
{
    fm_main_win_chdir_by_name( win, "computer:///");
}

void on_go_network(GtkAction* act, FmMainWin* win)
{
    fm_main_win_chdir_by_name( win, "network:///");
}

void on_go_apps(GtkAction* act, FmMainWin* win)
{
    fm_main_win_chdir(win, fm_path_get_apps_menu());
}

static void on_reload(GtkAction* act, FmMainWin* win)
{
    if(win->folder)
        fm_folder_reload(win->folder);
}

void fm_main_win_chdir_by_name(FmMainWin* win, const char* path_str)
{
    FmPath* path;
    char* tmp;
    path = fm_path_new_for_str(path_str);
    fm_main_win_chdir(win, path);
    tmp = fm_path_display_name(path, FALSE);
    gtk_entry_set_text(GTK_ENTRY(win->location), tmp);
    g_free(tmp);
    fm_path_unref(path);
}

static void on_folder_fs_info(FmFolder* folder, FmMainWin* win)
{
    guint64 free, total;
    if(fm_folder_get_filesystem_info(folder, &total, &free))
    {
        char total_str[ 64 ];
        char free_str[ 64 ];
        char buf[128];

        fm_file_size_to_str(free_str, sizeof(free_str), free, TRUE);
        fm_file_size_to_str(total_str, sizeof(total_str), total, TRUE);
        g_snprintf( buf, G_N_ELEMENTS(buf),
                    "Free space: %s (Total: %s)", free_str, total_str );
        gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(win->vol_status))), buf);
        gtk_widget_show(win->vol_status);
    }
    else
    {
        gtk_widget_hide(win->vol_status);
    }
}

void fm_main_win_chdir_without_history(FmMainWin* win, FmPath* path)
{
    free_folder(win);
    win->folder = fm_folder_from_path(path);
    g_signal_connect(win->folder, "start-loading", G_CALLBACK(on_folder_start_loading), win);
    g_signal_connect(win->folder, "finish-loading", G_CALLBACK(on_folder_finish_loading), win);
    g_signal_connect(win->folder, "removed", G_CALLBACK(on_folder_removed), win);
    g_signal_connect(win->folder, "unmount", G_CALLBACK(on_folder_unmount), win);
    g_signal_connect(win->folder, "error", G_CALLBACK(on_folder_error), win);
    g_signal_connect(win->folder, "fs-info", G_CALLBACK(on_folder_fs_info), win);

    on_folder_start_loading(win->folder, win);
    if(fm_folder_is_loaded(win->folder))
    {
        on_folder_finish_loading(win->folder, win);
        on_folder_fs_info(win->folder, win);
    }

    g_signal_handlers_block_by_func(win->left_pane, on_side_pane_chdir, win);
    fm_side_pane_chdir(FM_SIDE_PANE(win->left_pane), path);
    g_signal_handlers_unblock_by_func(win->left_pane, on_side_pane_chdir, win);

    update_statusbar(win);
    /* fm_nav_history_set_cur(); */
}

void fm_main_win_chdir(FmMainWin* win, FmPath* path)
{
    FmPath* cwd = win->folder ? fm_folder_get_path(win->folder) : NULL;
    int scroll_pos;
    /* it should grab focus in any case, see bug #3589448 */
    gtk_widget_grab_focus(GTK_WIDGET(win->folder_view));
    if(cwd && path && fm_path_equal(cwd, path))
        return;
    scroll_pos = gtk_adjustment_get_value(gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(win->folder_view)));
    fm_nav_history_chdir(win->nav_history, path, scroll_pos);
    fm_main_win_chdir_without_history(win, path);
}

void on_copy_to(GtkAction* act, FmMainWin* win)
{
    FmPathList* files = fm_folder_view_dup_selected_file_paths(win->folder_view);
    if(files)
        fm_copy_files_to(GTK_WINDOW(win), files);
}

void on_move_to(GtkAction* act, FmMainWin* win)
{
    FmPathList* files = fm_folder_view_dup_selected_file_paths(win->folder_view);
    if(files)
        fm_move_files_to(GTK_WINDOW(win), files);
}

void on_rename(GtkAction* act, FmMainWin* win)
{
    FmPathList* files = fm_folder_view_dup_selected_file_paths(win->folder_view);
    if(files)
        fm_rename_file(GTK_WINDOW(win), fm_path_list_peek_head(files));
        /* FIXME: is it ok to only rename the first selected file here. */
}

static void bounce_action(GtkAction* act, FmMainWin* win)
{
    g_debug("bouncing action %s to popup", gtk_action_get_name(act));
    fm_folder_view_bounce_action(act, win->folder_view);
}

void on_location(GtkAction* act, FmMainWin* win)
{
    gtk_widget_grab_focus(win->location);
    if (win->pathbar_active)
    {
        gtk_widget_set_visible(win->location, TRUE);
        gtk_widget_set_visible(gtk_ui_manager_get_widget(win->ui, "/toolbar/Go"), TRUE);
        gtk_widget_set_visible(win->pathbar, FALSE);
    }
}
