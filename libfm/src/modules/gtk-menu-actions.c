/*
 *      fm-file-menu.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      Copyright 2018 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#include "fm.h"
#include "fm-folder-view.h"
#include "fm-gtk-utils.h"
#include "gtk-compat.h"

#include "fm-action.h"

static GQuark _fm_actions_qdata_id = 0;
static FmActionCache *_fm_actions_cache = NULL;

static void on_custom_action_file(GtkAction* act, gpointer menu)
{
    GAppInfo* item = g_object_get_qdata(G_OBJECT(act), fm_qdata_id);
    GdkAppLaunchContext* ctx = gdk_display_get_app_launch_context(gdk_display_get_default());
    GList* files = fm_file_info_list_peek_head_link(fm_file_menu_get_file_info_list(menu));
    GError *err = NULL;

    gdk_app_launch_context_set_screen(ctx, gtk_widget_get_screen(GTK_WIDGET(fm_file_menu_get_menu(menu))));
    gdk_app_launch_context_set_timestamp(ctx, gtk_get_current_event_time());

    /* g_debug("item: %s is activated, id:%s", fm_file_action_item_get_name(item),
        fm_file_action_item_get_id(item)); */
    g_app_info_launch(item, files, G_APP_LAUNCH_CONTEXT(ctx), &err);
    if (err)
    {
        fm_show_error(NULL, "output", err->message);
        g_error_free(err);
    }
    g_object_unref(ctx);
}

static void on_custom_action_folder(GtkAction* act, gpointer folder_view)
{
    GAppInfo* item = g_object_get_qdata(G_OBJECT(act), fm_qdata_id);
    GdkAppLaunchContext* ctx = gdk_display_get_app_launch_context(gdk_display_get_default());
    GList* files = g_list_prepend(NULL, fm_folder_view_get_cwd_info(folder_view));
    GError *err = NULL;

    gdk_app_launch_context_set_screen(ctx, gtk_widget_get_screen(folder_view));
    gdk_app_launch_context_set_timestamp(ctx, gtk_get_current_event_time());

    /* g_debug("item: %s is activated, id:%s", fm_file_action_item_get_name(item),
        fm_file_action_item_get_id(item)); */
    g_app_info_launch(item, files, G_APP_LAUNCH_CONTEXT(ctx), &err);
    if (err)
    {
        fm_show_error(NULL, "output", err->message);
        g_error_free(err);
    }
    g_object_unref(ctx);
    g_list_free(files);
}

static void add_custom_action_item(GString* xml, FmActionMenu *root_menu,
                                   GAppInfo* item, GtkActionGroup* act_grp,
                                   GCallback cb, gpointer cb_data)
{
    GtkAction* act;

    if(!item) /* separator */
    {
        g_string_append(xml, "<separator/>");
        return;
    }

    act = gtk_action_new(g_app_info_get_id(item),
#if GLIB_CHECK_VERSION(2, 24, 0)
                         g_app_info_get_display_name(item),
#else
                         g_app_info_get_name(item),
#endif
                         g_app_info_get_description(item),
                         NULL);

    if (FM_IS_ACTION(item))
        g_signal_connect(act, "activate", cb, cb_data);

    gtk_action_set_gicon(act, g_app_info_get_icon(item));
    gtk_action_group_add_action(act_grp, act);
    g_object_unref(act);
    /* hold a reference on the root FmActionMenu object */
    g_object_set_qdata_full(G_OBJECT(act), _fm_actions_qdata_id,
                            g_object_ref(root_menu), g_object_unref);
    /* associate the app info object with the action */
    g_object_set_qdata(G_OBJECT(act), fm_qdata_id, item);
    if (FM_IS_ACTION_MENU(item))
    {
        const GList* subitems = fm_action_menu_get_children(FM_ACTION_MENU(item));
        const GList* l;
        g_string_append_printf(xml, "<menu action='%s'>",
                               g_app_info_get_id(item));
        for(l=subitems; l; l=l->next)
        {
            GAppInfo *subitem = l->data;
            add_custom_action_item(xml, root_menu, subitem, act_grp, cb, cb_data);
        }
        g_string_append(xml, "</menu>");
    }
    else
    {
        g_string_append_printf(xml, "<menuitem action='%s'/>",
                               g_app_info_get_id(item));
    }
}

static void _fm_actions_init(void)
{
    if (!_fm_actions_cache)
        _fm_actions_cache = fm_action_cache_new();
    if (!_fm_actions_qdata_id)
        _fm_actions_qdata_id = g_quark_from_string("_fm_actions_qdata_id");
}

static void _fm_actions_finalize(void)
{
    if (_fm_actions_cache)
       g_object_unref(G_OBJECT(_fm_actions_cache));
    _fm_actions_cache = NULL;
}

static void
_fm_actions_update_file_menu_for_scheme(GtkWindow* window, GtkUIManager* ui,
                                        GString* xml, GtkActionGroup* act_grp,
                                        FmFileMenu* menu, FmFileInfoList* files,
                                        gboolean single_file)
{
    FmActionMenu *root_menu;
    FmPath *cwd = fm_file_menu_get_cwd(menu);
    FmFolder *folder;
    FmFileInfo *location = NULL;
    const GList *items;

    g_return_if_fail(_fm_actions_cache != NULL && cwd != NULL);
    folder = fm_folder_find_by_path(cwd);
    if (folder)
        location = fm_folder_get_info(folder);
    if (!location)
        return;

    /* add custom file actions */
    root_menu = fm_action_get_for_context(_fm_actions_cache, location, files);
    items = fm_action_menu_get_children(root_menu);
    if(items)
    {
        g_string_append(xml, "<popup><placeholder name='ph3'>");
        const GList* l;
        for(l=items; l; l=l->next)
        {
            GAppInfo *item = l->data;
            add_custom_action_item(xml, root_menu, item, act_grp,
                                   G_CALLBACK(on_custom_action_file), menu);
        }
        g_string_append(xml, "</placeholder></popup>");
    }
    g_object_unref(root_menu);
}

static void
_fm_actions_update_folder_menu_for_scheme(FmFolderView* fv, GtkWindow* window,
                                          GtkUIManager* ui, GtkActionGroup* act_grp,
                                          FmFileInfoList* files)
{
    FmFileInfo *fi = fm_folder_view_get_cwd_info(fv);
    FmActionMenu *root_menu;
    const GList *items;

    g_return_if_fail(_fm_actions_cache != NULL);

    if (fi == NULL) /* incremental folder - no info yet - ignore it */
        return;

    root_menu = fm_action_get_for_location(_fm_actions_cache, fi);
    items = fm_action_menu_get_children(root_menu);
    if(items)
    {
        GString *xml = g_string_new("<popup><placeholder name='CustomCommonOps'>");
        const GList* l;

        for(l=items; l; l=l->next)
        {
            GAppInfo *item = l->data;
            add_custom_action_item(xml, root_menu, item, act_grp,
                                   G_CALLBACK(on_custom_action_folder), fv);
        }
        g_string_append(xml, "</placeholder></popup>");
        gtk_ui_manager_add_ui_from_string(ui, xml->str, xml->len, NULL);
        g_string_free(xml, TRUE);
    }
    g_object_unref(root_menu);
}

/* we catch all schemes to be available on every one */
FM_DEFINE_MODULE(gtk_menu_scheme, *)

FmContextMenuSchemeAddonInit fm_module_init_gtk_menu_scheme = {
    .init = _fm_actions_init,
    .finalize = _fm_actions_finalize,
    .update_file_menu_for_scheme = _fm_actions_update_file_menu_for_scheme,
    .update_folder_menu = _fm_actions_update_folder_menu_for_scheme
};
