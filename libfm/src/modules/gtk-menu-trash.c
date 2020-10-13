/*
 *      gtk-menu-trash.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2013-2016 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#include "fm-folder-view.h"
#include "fm-gtk-utils.h"

#include <glib/gi18n-lib.h>

static void on_untrash(GtkAction* action, gpointer user_data)
{
    FmFileMenu *data = (FmFileMenu*)user_data;
    GtkMenu *menu = fm_file_menu_get_menu(data);
    GtkWindow *window = GTK_WINDOW(gtk_menu_get_attach_widget(menu));
    FmFileInfoList *file_infos = fm_file_menu_get_file_info_list(data);
    FmPathList *files = fm_path_list_new_from_file_info_list(file_infos);
    fm_untrash_files(window, files);
    fm_path_list_unref(files);
}

static void on_empty_trash(GtkAction* act, gpointer user_data)
{
    GtkWindow *window = GTK_WINDOW(user_data);
    fm_empty_trash(window);
}

static void _update_file_menu_for_trash(GtkWindow* window, GtkUIManager* ui,
                                        GString* xml, GtkActionGroup* act_grp,
                                        FmFileMenu* menu, FmFileInfoList* files,
                                        gboolean single_file)
{
    gboolean can_restore = TRUE, is_trash_root = FALSE;
    GList *l;
    GtkAction *act;

    /* only immediate children of trash:/// can be restored. */
    for(l = fm_file_info_list_peek_head_link(files);l;l=l->next)
    {
        FmPath *trash_path = fm_file_info_get_path(FM_FILE_INFO(l->data));
        if(single_file)
            is_trash_root = fm_path_is_trash_root(trash_path);
        if(!fm_path_get_parent(trash_path) ||
           !fm_path_is_trash_root(fm_path_get_parent(trash_path)))
        {
            can_restore = FALSE;
            break;
        }
    }
    if(can_restore)
    {
        act = gtk_action_new("UnTrash",
                            _("_Restore"),
                            _("Restore trashed files to original paths"),
                            NULL);
        g_signal_connect(act, "activate", G_CALLBACK(on_untrash), menu);
        gtk_action_group_add_action(act_grp, act);
        g_object_unref(act);
        g_string_append(xml, "<popup><placeholder name='ph1'>"
                             "<menuitem action='UnTrash'/>"
                             "</placeholder></popup>");
    }
    else if (is_trash_root)
    {
        act = gtk_action_new("EmptyTrash",
                             _("_Empty Trash Can"),
                             NULL, NULL);
        g_signal_connect(act, "activate", G_CALLBACK(on_empty_trash), window);
        gtk_action_group_add_action(act_grp, act);
        g_string_append(xml, "<popup><placeholder name='ph1'>"
                             "<menuitem action='EmptyTrash'/>"
                             "</placeholder></popup>");
    }
    act = gtk_ui_manager_get_action(ui, "/popup/Open");
    gtk_action_set_visible(act, FALSE);
    act = gtk_ui_manager_get_action(ui, "/popup/Rename");
    gtk_action_set_visible(act, FALSE);
    act = gtk_ui_manager_get_action(ui, "/popup/Copy");
    gtk_action_set_visible(act, FALSE);
    act = gtk_ui_manager_get_action(ui, "/popup/Paste");
    gtk_action_set_visible(act, FALSE);
    /* FIXME: can we cut files here? */
}

static void _update_folder_menu_for_trash(FmFolderView* fv, GtkWindow* window,
                                          GtkUIManager* ui, GtkActionGroup* act_grp,
                                          FmFileInfoList* files)
{
    GtkAction *act;

    /* FIXME: should we show this item for trash root only? */
    act = gtk_action_new("EmptyTrash",
                         _("_Empty Trash Can"),
                         NULL, NULL);
    g_signal_connect(act, "activate", G_CALLBACK(on_empty_trash), window);
    gtk_action_group_add_action(act_grp, act);
    g_object_unref(act);
    gtk_ui_manager_add_ui_from_string(ui, "<popup>"
                                          "<placeholder name='CustomFileOps'>"
                                          "<menuitem action='EmptyTrash'/>"
                                          "</placeholder>"
                                          "</popup>", -1, NULL);
    act = gtk_ui_manager_get_action(ui, "/popup/Rename");
    gtk_action_set_visible(act, FALSE);
    act = gtk_ui_manager_get_action(ui, "/popup/CreateNew");
    gtk_action_set_visible(act, FALSE);
    if (!fm_path_is_trash_root(fm_folder_view_get_cwd(fv)))
    {
        /* we can paste (i.e. trash) files only into trash root */
        act = gtk_ui_manager_get_action(ui, "/popup/Paste");
        gtk_action_set_visible(act, FALSE);
    }
}

FM_DEFINE_MODULE(gtk_menu_scheme, trash)

FmContextMenuSchemeAddonInit fm_module_init_gtk_menu_scheme = {
    NULL,
    NULL,
    _update_file_menu_for_trash,
    _update_folder_menu_for_trash
};
