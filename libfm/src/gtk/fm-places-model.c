/*
 *      fm-places-model.c
 *
 *      Copyright 2010 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2012-2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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
 * SECTION:fm-places-model
 * @short_description: A model for side panel with places list.
 * @title: FmPlacesModel
 *
 * @include: libfm/fm-gtk.h
 *
 * The #FmPlacesModel represents list of pseudo-folders which contains
 * such items as Home directory, Trash bin, mounted removable drives,
 * bookmarks, etc. It is used by #FmPlacesView to display them in the
 * side panel.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-places-model.h"
#include "fm-file.h"

#include <glib/gi18n-lib.h>

#include "fm-config.h"
#include "fm-monitor.h"
#include "fm-file-info-job.h"

/* standard items order */
typedef enum
{
    FM_PLACES_ID_HOME,
    FM_PLACES_ID_DESKTOP,
    FM_PLACES_ID_TRASH,
    FM_PLACES_ID_ROOT,
    FM_PLACES_ID_APPLICATIONS,
    FM_PLACES_ID_COMPUTER,
    FM_PLACES_ID_NETWORK,
    FM_PLACES_ID_OTHER
} FmPlacesOrder;

struct _FmPlacesItem
{
    FmPlacesType type;
    gboolean mounted : 1; /* used if type == FM_PLACES_ITEM_VOLUME */
    FmPlacesOrder id : 4; /* used if type == FM_PLACES_ITEM_PATH */
    FmIcon* icon;
    FmFileInfo* fi;
    union
    {
        GVolume* volume; /* used if type == FM_PLACES_ITEM_VOLUME */
        GMount* mount; /* used if type == FM_PLACES_ITEM_MOUNT */
        FmBookmarkItem* bm_item; /* used if type == FM_PLACES_ITEM_PATH */
    };
};

struct _FmPlacesModel
{
    GtkListStore parent;

    GVolumeMonitor* vol_mon;
    FmBookmarks* bookmarks;
    GtkTreeRowReference* separator;
    GtkTreeRowReference* trash;
    GFileMonitor* trash_monitor;
    guint trash_idle_handler;
    guint theme_change_handler;
    guint use_trash_change_handler;
    guint pane_icon_size_change_handler;
    guint places_home_change_handler;
    guint places_desktop_change_handler;
    guint places_root_change_handler;
    guint places_computer_change_handler;
    guint places_trash_change_handler;
    guint places_applications_change_handler;
    guint places_network_change_handler;
    guint places_unmounted_change_handler;
    GdkPixbuf* eject_icon;

    GSList* jobs;
};

struct _FmPlacesModelClass
{
    GtkListStoreClass parent_class;
};


static void create_trash_item(FmPlacesModel* model);

static void place_item_free(FmPlacesItem* item)
{
    switch(item->type)
    {
    case FM_PLACES_ITEM_VOLUME:
        g_object_unref(item->volume);
        break;
    case FM_PLACES_ITEM_MOUNT:
        g_object_unref(item->mount);
        break;
    case FM_PLACES_ITEM_PATH:
        if(item->bm_item)
            fm_bookmark_item_unref(item->bm_item);
        break;
    case FM_PLACES_ITEM_NONE:
        ;
    }
    if(G_LIKELY(item->icon))
        g_object_unref(item->icon);
    if(G_LIKELY(item->fi))
        fm_file_info_unref(item->fi);
    g_slice_free(FmPlacesItem, item);
}

static void on_file_info_job_finished(FmFileInfoJob* job, gpointer user_data)
{
    FmPlacesModel* model = FM_PLACES_MODEL(user_data);
    GList* l;
    GtkTreeIter it;
    FmPlacesItem* item;
    FmFileInfo* fi;
    FmPath* path;
    FmIcon* icon;
    GdkPixbuf* pix;

    /* g_debug("file info job finished"); */
    model->jobs = g_slist_remove(model->jobs, job);
    g_signal_handlers_disconnect_by_func(job, on_file_info_job_finished, model);

    if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &it))
        goto finished;

    if(fm_file_info_list_is_empty(job->file_infos))
        goto finished;

    /* optimize for one file case */
    if(fm_file_info_list_get_length(job->file_infos) == 1)
    {
        fi = fm_file_info_list_peek_head(job->file_infos);
        do {
            item = NULL;
            gtk_tree_model_get(GTK_TREE_MODEL(model), &it, FM_PLACES_MODEL_COL_INFO, &item, -1);
            if( item && item->fi && (path = fm_file_info_get_path(item->fi)) && fm_path_equal(path, fm_file_info_get_path(fi)) )
            {
                fm_file_info_unref(item->fi);
                item->fi = fm_file_info_ref(fi);
                break;
            }
        }while(gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &it));
    }
    else
    {
        do {
            item = NULL;
            gtk_tree_model_get(GTK_TREE_MODEL(model), &it, FM_PLACES_MODEL_COL_INFO, &item, -1);
            if( item && item->fi && (path = fm_file_info_get_path(item->fi)) )
            {
                for(l = fm_file_info_list_peek_head_link(job->file_infos); l; l = l->next )
                {
                    fi = FM_FILE_INFO(l->data);
                    if(fm_path_equal(path, fm_file_info_get_path(fi)))
                    {
                        fm_file_info_unref(item->fi);
                        item->fi = fm_file_info_ref(fi);
                        /* only update the icon if the item is not a volume or mount. */
                        if(item->type == FM_PLACES_ITEM_PATH)
                        {
                            icon = fm_file_info_get_icon(fi);
                            /* replace the icon with updated data */
                            if(icon && icon != item->icon)
                            {
                                g_object_unref(item->icon);
                                item->icon = g_object_ref(icon);
                                pix = fm_pixbuf_from_icon(icon, fm_config->pane_icon_size);
                                gtk_list_store_set(GTK_LIST_STORE(model), &it,
                                                   FM_PLACES_MODEL_COL_ICON, pix, -1);
                            }
                        }
                        /* remove the file from list to speed up further loading.
                         * This won't cause problem since nobody else if using the list. */
                        fm_file_info_list_delete_link(job->file_infos, l);
                        break;
                    }
                }
            }
        }while(gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &it));
    }
finished:
    g_object_unref(job);
}

static void update_volume_or_mount(FmPlacesModel* model, FmPlacesItem* item, GtkTreeIter* it, FmFileInfoJob* job)
{
    GIcon* gicon;
    char* name;
    GdkPixbuf* pix;
    GMount* mount;
    FmPath* path;

    if(item->type == FM_PLACES_ITEM_VOLUME)
    {
        name = g_volume_get_name(item->volume);
        gicon = g_volume_get_icon(item->volume);
        mount = g_volume_get_mount(item->volume);
    }
    else if(G_LIKELY(item->type == FM_PLACES_ITEM_MOUNT))
    {
        name = g_mount_get_name(item->mount);
        gicon = g_mount_get_icon(item->mount);
        mount = g_object_ref(item->mount);
    }
    else
        return; /* FIXME: is it possible? */

    if(item->icon)
        g_object_unref(item->icon);
    item->icon = fm_icon_from_gicon(gicon);
    g_object_unref(gicon);

    if(mount)
    {
        GFile* gf = g_mount_get_root(mount);
        path = fm_path_new_for_gfile(gf);
        g_object_unref(gf);
        g_object_unref(mount);
        item->mounted = TRUE;
    }
    else
    {
        path = NULL;
        item->mounted = FALSE;
    }

    if(!fm_path_equal(fm_file_info_get_path(item->fi), path))
    {
        fm_file_info_set_path(item->fi, path);
        if(path)
        {
            if(job)
                fm_file_info_job_add(job, path);
            else
            {
                job = fm_file_info_job_new(NULL, FM_FILE_INFO_JOB_FOLLOW_SYMLINK);
                fm_file_info_job_add(job, path);
                model->jobs = g_slist_prepend(model->jobs, job);
                g_signal_connect(job, "finished", G_CALLBACK(on_file_info_job_finished), model);
                if (!fm_job_run_async(FM_JOB(job)))
                {
                    model->jobs = g_slist_remove(model->jobs, job);
                    g_object_unref(job);
                    g_critical("fm_job_run_async() failed on mount update");
                }
            }
            fm_path_unref(path);
        }
        else /* we might get it just unmounted so just reset file info */
        {
            fm_file_info_unref(item->fi);
            item->fi = fm_file_info_new();
        }
    }

    pix = fm_pixbuf_from_icon(item->icon, fm_config->pane_icon_size);
    gtk_list_store_set(GTK_LIST_STORE(model), it, FM_PLACES_MODEL_COL_ICON, pix, FM_PLACES_MODEL_COL_LABEL, name, -1);
    g_object_unref(pix);
    g_free(name);
}

static inline FmPlacesItem* add_new_item(GtkListStore* model, FmPlacesType type,
                                         GtkTreeIter *it, GtkTreePath* at)
{
    GtkTreeIter next_it;
    FmPlacesItem* item = g_slice_new0(FmPlacesItem);

    item->fi = fm_file_info_new();
    item->type = type;
    if(at)
    {
        gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &next_it, at);
        gtk_list_store_insert_before(model, it, &next_it);
    }
    else
        gtk_list_store_append(model, it);
    gtk_list_store_set(model, it, FM_PLACES_MODEL_COL_INFO, item, -1);
    return item;
}

static FmPlacesItem* new_path_item(GtkListStore* model, GtkTreeIter* it,
                                   FmPath* path, FmPlacesOrder id,
                                   const char* label, const char* icon_name,
                                   FmFileInfoJob* job)
{
    FmPlacesItem* item = g_slice_new0(FmPlacesItem);
    FmPlacesItem* tst;
    GdkPixbuf* pix;
    GtkTreeIter next_it;

    item->fi = fm_file_info_new();
    item->type = FM_PLACES_ITEM_PATH;
    item->id = id;
    item->icon = fm_icon_from_name(icon_name);
    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &next_it)) do
    {
        tst = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(model), &next_it, FM_PLACES_MODEL_COL_INFO, &tst, -1);
        if(!tst || tst->type != FM_PLACES_ITEM_PATH || tst->id > id)
        {
            gtk_list_store_insert_before(model, it, &next_it);
            goto _added;
        }
        else if(tst->id == id)
        {
            *it = next_it;
            place_item_free(tst);
            g_critical("duplicate places view item");
            goto _added;
        }
    } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &next_it));
    gtk_list_store_append(model, it);
_added:
    pix = fm_pixbuf_from_icon(item->icon, fm_config->pane_icon_size);
    gtk_list_store_set(model, it,
                       FM_PLACES_MODEL_COL_INFO, item,
                       FM_PLACES_MODEL_COL_LABEL, label,
                       FM_PLACES_MODEL_COL_ICON, pix, -1);
    g_object_unref(pix);
    fm_file_info_set_path(item->fi, path);
    if(job)
        fm_file_info_job_add(job, path);
    else
    {
        FmPlacesModel* self = FM_PLACES_MODEL(model);
        job = fm_file_info_job_new(NULL, FM_FILE_INFO_JOB_FOLLOW_SYMLINK);
        fm_file_info_job_add(job, path);
        g_signal_connect(job, "finished", G_CALLBACK(on_file_info_job_finished), model);
        self->jobs = g_slist_prepend(self->jobs, job);
        if (!fm_job_run_async(FM_JOB(job)))
        {
            self->jobs = g_slist_remove(self->jobs, job);
            g_object_unref(job);
            g_critical("fm_job_run_async() failed on update '%s'", label);
        }
    }
    return item;
}

static void remove_path_item(GtkListStore* model, FmPlacesOrder id)
{
    FmPlacesItem* item;
    GtkTreeIter it;

    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &it)) do
    {
        item = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(model), &it, FM_PLACES_MODEL_COL_INFO, &item, -1);
        if(!item || item->type != FM_PLACES_ITEM_PATH || item->id > id)
            return; /* not found! */
        if(item->id == id)
        {
            gtk_list_store_remove(model, &it);
            place_item_free(item);
            return;
        }
    } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &it));
}

static void add_volume_or_mount(FmPlacesModel* model, GObject* volume_or_mount, FmFileInfoJob* job)
{
    FmPlacesItem* item;
    GtkTreePath* tp;
    GtkTreeIter it;
    if(G_IS_VOLUME(volume_or_mount))
    {
        tp = gtk_tree_row_reference_get_path(model->separator);
        item = add_new_item(GTK_LIST_STORE(model), FM_PLACES_ITEM_VOLUME, &it, tp);
        gtk_tree_path_free(tp);
        item->volume = G_VOLUME(g_object_ref(volume_or_mount));
    }
    else if(G_IS_MOUNT(volume_or_mount))
    {
        tp = gtk_tree_row_reference_get_path(model->separator);
        item = add_new_item(GTK_LIST_STORE(model), FM_PLACES_ITEM_MOUNT, &it, tp);
        gtk_tree_path_free(tp);
        item->mount = G_MOUNT(g_object_ref(volume_or_mount));
    }
    else
    {
        /* NOTE: this is impossible, unless a bug exists */
        return;
    }
    update_volume_or_mount(model, item, &it, job);
}

static FmPlacesItem* find_volume(FmPlacesModel* model, GVolume* volume, GtkTreeIter* _it)
{
    GtkTreeIter it;
    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &it))
    {
        do
        {
            FmPlacesItem* item;
            gtk_tree_model_get(GTK_TREE_MODEL(model), &it, FM_PLACES_MODEL_COL_INFO, &item, -1);
            if(item == NULL) /* separator item has all columns NULL */
                return item;
            if(item->type == FM_PLACES_ITEM_VOLUME && item->volume == volume)
            {
                *_it = it;
                return item;
            }
        }while(gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &it));
    }
    return NULL;
}

static FmPlacesItem* find_mount(FmPlacesModel* model, GMount* mount, GtkTreeIter* _it)
{
    GtkTreeIter it;
    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &it))
    {
        do
        {
            FmPlacesItem* item;
            gtk_tree_model_get(GTK_TREE_MODEL(model), &it, FM_PLACES_MODEL_COL_INFO, &item, -1);
            if(item == NULL) /* separator item has all columns NULL */
                return item;
            if(item->type == FM_PLACES_ITEM_MOUNT && item->mount == mount)
            {
                *_it = it;
                return item;
            }
        }while(gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &it));
    }
    return NULL;
}

static void on_volume_added(GVolumeMonitor* vm, GVolume* volume, gpointer user_data)
{
    FmPlacesModel* model = FM_PLACES_MODEL(user_data);
    FmPlacesItem* item;
    GMount *mount;
    GtkTreeIter it;

    /* nothing to do if we don't show unmounted volumes */
    if (!fm_config->places_unmounted)
        return;
    /* for some unknown reasons, sometimes we get repeated volume-added 
     * signals and added a device more than one. So, make a sanity check here. */
    item = find_volume(model, volume, &it);
    if (!item)
    {
        /* SF bug #850 - some mounts are announced before appropriate volume
           by GLib, see https://bugzilla.gnome.org/show_bug.cgi?id=730347 */
        mount = g_volume_get_mount(volume);
        if (mount)
        {
            item = find_mount(model, mount, &it);
            if (item)
            {
                g_object_unref(item->mount);
                item->type = FM_PLACES_ITEM_VOLUME;
                item->volume = g_object_ref(volume);
                update_volume_or_mount(model, item, &it, NULL);
            }
            g_object_unref(mount);
        }
    }
    if(!item)
        add_volume_or_mount(model, G_OBJECT(volume), NULL);
}

static void on_volume_removed(GVolumeMonitor* vm, GVolume* volume, gpointer user_data)
{
    FmPlacesModel* model = FM_PLACES_MODEL(user_data);
    FmPlacesItem* item;
    GtkTreeIter it;
    item = find_volume(model, volume, &it);
    if(item)
    {
        gtk_list_store_remove(GTK_LIST_STORE(model), &it);
        place_item_free(item);
    }
}

static void on_volume_changed(GVolumeMonitor* vm, GVolume* volume, gpointer user_data)
{
    FmPlacesModel* model = FM_PLACES_MODEL(user_data);
    FmPlacesItem* item;
    GtkTreeIter it;
    /* g_debug("vol-changed"); */
    item = find_volume(model, volume, &it);
    if(item)
        update_volume_or_mount(model, item, &it, NULL);
}

static void on_mount_added(GVolumeMonitor* vm, GMount* mount, gpointer user_data)
{
    FmPlacesModel* model = FM_PLACES_MODEL(user_data);
    GVolume* vol;

    if (g_mount_is_shadowed(mount))
        return;
    vol = g_mount_get_volume(mount);
    if(vol)
    {
        /* mount-added is also emitted when a volume is newly mounted. */
        FmPlacesItem *item;
        GtkTreeIter it;
        item = find_volume(model, vol, &it);
        /* update the mounted volume and show a button for eject. */
        if(item && item->type == FM_PLACES_ITEM_VOLUME && !fm_file_info_get_path(item->fi))
        {
            GtkTreePath* tp;

            /* we need full update for it to get adequate context menu */
            update_volume_or_mount(model, item, &it, NULL);
            /* inform the view to update mount indicator */
            tp = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &it);
            gtk_tree_model_row_changed(GTK_TREE_MODEL(model), tp, &it);
            gtk_tree_path_free(tp);
        }
        else if (!item)
            /* we might not get volume for it yet or ignored it before */
            add_volume_or_mount(model, G_OBJECT(mount), NULL);
        g_object_unref(vol);
    }
    else /* network mounts and others */
    {
        FmPlacesItem* item;
        GtkTreeIter it;
        /* for some unknown reasons, sometimes we get repeated mount-added 
         * signals and added a device more than one. So, make a sanity check here. */
        item = find_mount(model,  mount, &it);
        if(!item)
            add_volume_or_mount(model, G_OBJECT(mount), NULL);
    }
}

static void on_mount_changed(GVolumeMonitor* vm, GMount* mount, gpointer user_data)
{
    FmPlacesModel* model = FM_PLACES_MODEL(user_data);
    FmPlacesItem* item;
    GtkTreeIter it;
    item = find_mount(model, mount, &it);
    if(item)
        update_volume_or_mount(model, item, &it, NULL);
}

static void on_mount_removed(GVolumeMonitor* vm, GMount* mount, gpointer user_data)
{
    FmPlacesModel* model = FM_PLACES_MODEL(user_data);
    GVolume* vol = g_mount_get_volume(mount);
    if(vol) /* we handle volumes in volume-removed handler */
        g_object_unref(vol);
    else /* network mounts and others */
    {
        GtkTreeIter it;
        FmPlacesItem* item = find_mount(model, mount, &it);
        if(item)
        {
            gtk_list_store_remove(GTK_LIST_STORE(model), &it);
            place_item_free(item);
        }
    }
}

static void add_bookmarks(FmPlacesModel* model, FmFileInfoJob* job)
{
    FmPlacesItem* item;
    GList *bms, *l;
    FmIcon* icon = fm_icon_from_name("folder");
    FmIcon* remote_icon = NULL;
    GdkPixbuf* folder_pix = fm_pixbuf_from_icon(icon, fm_config->pane_icon_size);
    GdkPixbuf* remote_pix = NULL;
    bms = fm_bookmarks_get_all(model->bookmarks);
    for(l=bms;l;l=l->next)
    {
        FmBookmarkItem* bm = (FmBookmarkItem*)l->data;
        GtkTreeIter it;
        GdkPixbuf* pix;
        FmPath* path = bm->path;

        item = add_new_item(GTK_LIST_STORE(model), FM_PLACES_ITEM_PATH, &it, NULL);
        fm_file_info_set_path(item->fi, path);
        fm_file_info_job_add(job, path);
        if(fm_path_is_native(path))
        {
            item->icon = g_object_ref(icon);
            pix = folder_pix;
        }
        else
        {
            if(G_UNLIKELY(!remote_icon))
            {
                remote_icon = fm_icon_from_name("folder-remote");
                remote_pix = fm_pixbuf_from_icon(remote_icon, fm_config->pane_icon_size);
            }
            item->icon = g_object_ref(remote_icon);
            pix = remote_pix;
        }
        item->bm_item = bm;
        item->id = FM_PLACES_ID_OTHER;
        gtk_list_store_set(GTK_LIST_STORE(model), &it,
                           FM_PLACES_MODEL_COL_ICON, pix,
                           FM_PLACES_MODEL_COL_LABEL, bm->name, -1);
    }
    g_list_free(bms);
    g_object_unref(folder_pix);
    g_object_unref(icon);
    if(remote_icon)
    {
        g_object_unref(remote_icon);
        if(remote_pix)
            g_object_unref(remote_pix);
    }
}

static void on_bookmarks_changed(FmBookmarks* bm, gpointer user_data)
{
    FmPlacesModel* model = FM_PLACES_MODEL(user_data);
    FmFileInfoJob* job = fm_file_info_job_new(NULL, FM_FILE_INFO_JOB_FOLLOW_SYMLINK);
    GtkTreePath* tp = gtk_tree_row_reference_get_path(model->separator);
    GtkTreeIter it;

    gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &it, tp);
    gtk_tree_path_free(tp);
    /* remove all old bookmarks */
    if(gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &it))
    {
        while(gtk_list_store_remove(GTK_LIST_STORE(model), &it))
            continue;
    }
    add_bookmarks(model, job);

    g_signal_connect(job, "finished", G_CALLBACK(on_file_info_job_finished), model);
    model->jobs = g_slist_prepend(model->jobs, job);
    if (!fm_job_run_async(FM_JOB(job)))
    {
        model->jobs = g_slist_remove(model->jobs, job);
        g_object_unref(job);
        g_critical("fm_job_run_async() failed for bookmark update");
    }
}

static gboolean update_trash_item(gpointer user_data)
{
    FmPlacesModel* model = FM_PLACES_MODEL(user_data);
    if(!g_source_is_destroyed(g_main_current_source()) &&
       fm_config->use_trash && model->trash)
    {
        GFile* gf = fm_file_new_for_uri("trash:///");
        GFileInfo* inf = g_file_query_info(gf, G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT, 0, NULL, NULL);
        g_object_unref(gf);
        if(inf)
        {
            FmIcon* icon;
            const char* icon_name;
            FmPlacesItem* item = NULL;
            GdkPixbuf* pix;
            GtkTreePath* tp = gtk_tree_row_reference_get_path(model->trash);
            GtkTreeIter it;
            guint32 n = g_file_info_get_attribute_uint32(inf, G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT);

            g_object_unref(inf);
            g_assert(tp != NULL); /* FIXME: how can tp be invalid here? */
            icon_name = n > 0 ? "user-trash-full" : "user-trash";
            icon = fm_icon_from_name(icon_name);
            gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &it, tp);
            gtk_tree_model_get(GTK_TREE_MODEL(model), &it, FM_PLACES_MODEL_COL_INFO, &item, -1);
            if(item->icon)
                g_object_unref(item->icon);
            item->icon = icon;
            /* update the icon */
            pix = fm_pixbuf_from_icon(item->icon, fm_config->pane_icon_size);
            gtk_list_store_set(GTK_LIST_STORE(model), &it, FM_PLACES_MODEL_COL_ICON, pix, -1);
            g_object_unref(pix);
            gtk_tree_path_free(tp);
        }
    }
    return FALSE;
}


static void on_trash_changed(GFileMonitor *monitor, GFile *gf, GFile *other, GFileMonitorEvent evt, gpointer user_data)
{
    FmPlacesModel* model = FM_PLACES_MODEL(user_data);
    if(model->trash_idle_handler)
        g_source_remove(model->trash_idle_handler);
    model->trash_idle_handler = gdk_threads_add_idle(update_trash_item, model);
}

static void update_icons(FmPlacesModel* model)
{
    GtkTreeIter it;
    FmIcon* icon;
    GdkPixbuf* pix;

    /* update the eject icon */
    icon = fm_icon_from_name("media-eject");
    pix = fm_pixbuf_from_icon(icon, fm_config->pane_icon_size);
    g_object_unref(icon);
    if(model->eject_icon)
        g_object_unref(model->eject_icon);
    model->eject_icon = pix;

    /* reload icon for every item */
    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &it);
    do{
        FmPlacesItem* item = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(model), &it, FM_PLACES_MODEL_COL_INFO, &item, -1);
        if(item) /* separator item has all columns NULL */
        {
            pix = fm_pixbuf_from_icon(item->icon, fm_config->pane_icon_size);
            gtk_list_store_set(GTK_LIST_STORE(model), &it, FM_PLACES_MODEL_COL_ICON, pix, -1);
            g_object_unref(pix);
        }
    }while( gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &it) );
}

static void on_use_trash_changed(FmConfig* cfg, gpointer user_data)
{
    FmPlacesModel* model = FM_PLACES_MODEL(user_data);
    if(cfg->use_trash && cfg->places_trash && model->trash == NULL)
        create_trash_item(model);
    else if((!cfg->use_trash || !cfg->places_trash) && model->trash)
    {
        FmPlacesItem *item = NULL;
        GtkTreePath* tp = gtk_tree_row_reference_get_path(model->trash);
        GtkTreeIter it;

        gtk_tree_row_reference_free(model->trash);
        model->trash = NULL;
        gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &it, tp);
        gtk_tree_path_free(tp);
        gtk_tree_model_get(GTK_TREE_MODEL(model), &it, FM_PLACES_MODEL_COL_INFO, &item, -1);
        gtk_list_store_remove(GTK_LIST_STORE(model), &it);
        place_item_free(item);

        if(model->trash_monitor)
        {
            g_signal_handlers_disconnect_by_func(model->trash_monitor, on_trash_changed, model);
            g_object_unref(model->trash_monitor);
            model->trash_monitor = NULL;
        }
        if(model->trash_idle_handler)
        {
            g_source_remove(model->trash_idle_handler);
            model->trash_idle_handler = 0;
        }
    }
}

static void on_places_home_changed(FmConfig* cfg, gpointer user_data)
{
    GtkListStore* model = GTK_LIST_STORE(user_data);
    GtkTreeIter it;
    if(cfg->places_home)
    {
        FmPath* path = fm_path_get_home();

        new_path_item(model, &it, path, FM_PLACES_ID_HOME,
                      _("Home Folder"), "user-home", NULL);
    }
    else
    {
        remove_path_item(model, FM_PLACES_ID_HOME);
    }
}

static void on_places_desktop_changed(FmConfig* cfg, gpointer user_data)
{
    GtkListStore* model = GTK_LIST_STORE(user_data);
    GtkTreeIter it;
    if(cfg->places_desktop &&
       g_file_test(g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP), G_FILE_TEST_IS_DIR))
    {
        new_path_item(model, &it, fm_path_get_desktop(), FM_PLACES_ID_DESKTOP,
                      _("Desktop"), "user-desktop", NULL);
    }
    else
    {
        remove_path_item(model, FM_PLACES_ID_DESKTOP);
    }
}

static void on_places_applications_changed(FmConfig* cfg, gpointer user_data)
{
    GtkListStore* model = GTK_LIST_STORE(user_data);
    GtkTreeIter it;

    if (!fm_module_is_in_use("vfs", "menu"))
        ; /* not in use */
    else if(cfg->places_applications)
    {
        new_path_item(model, &it, fm_path_get_apps_menu(),
                      FM_PLACES_ID_APPLICATIONS, _("Applications"),
                      "system-software-install", NULL);
    }
    else
    {
        remove_path_item(model, FM_PLACES_ID_APPLICATIONS);
    }
}

static void on_places_root_changed(FmConfig* cfg, gpointer user_data)
{
    GtkListStore* model = GTK_LIST_STORE(user_data);
    GtkTreeIter it;
    if(cfg->places_root)
    {
        new_path_item(model, &it, fm_path_get_root(), FM_PLACES_ID_ROOT,
                      _("Filesystem Root"), "drive-harddisk", NULL);
    }
    else
    {
        remove_path_item(model, FM_PLACES_ID_ROOT);
    }
}

static void on_places_computer_changed(FmConfig* cfg, gpointer user_data)
{
    GtkListStore* model = GTK_LIST_STORE(user_data);
    GtkTreeIter it;
    if(cfg->places_computer)
    {
        FmPath* path = fm_path_new_for_uri("computer:///");

        new_path_item(model, &it, path, FM_PLACES_ID_COMPUTER,
                      _("Devices"), "computer", NULL);
        fm_path_unref(path);
    }
    else
    {
        remove_path_item(model, FM_PLACES_ID_COMPUTER);
    }
}

static void on_places_network_changed(FmConfig* cfg, gpointer user_data)
{
    GtkListStore* model = GTK_LIST_STORE(user_data);
    GtkTreeIter it;
    if(cfg->places_network)
    {
        FmPath* path = fm_path_new_for_uri("network:///");

        new_path_item(model, &it, path, FM_PLACES_ID_NETWORK,
                      _("Network"), GTK_STOCK_NETWORK, NULL);
        fm_path_unref(path);
    }
    else
    {
        remove_path_item(model, FM_PLACES_ID_NETWORK);
    }
}

static void on_pane_icon_size_changed(FmConfig* cfg, gpointer user_data)
{
    FmPlacesModel* model = FM_PLACES_MODEL(user_data);
    update_icons(model);
}

static void create_trash_item(FmPlacesModel* model)
{
    GtkTreeIter it;
    GtkTreePath* trash_path;
    GFile* gf;

    gf = fm_file_new_for_uri("trash:///");
    if (!g_file_query_exists(gf, NULL))
    {
        g_object_unref(gf);
        return;
    }
    model->trash_monitor = fm_monitor_directory(gf, NULL);
    g_signal_connect(model->trash_monitor, "changed", G_CALLBACK(on_trash_changed), model);
    g_object_unref(gf);

    new_path_item(GTK_LIST_STORE(model), &it, fm_path_get_trash(),
                  FM_PLACES_ID_TRASH, _("Trash Can"), "user-trash", NULL);
    trash_path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &it);
    model->trash = gtk_tree_row_reference_new(GTK_TREE_MODEL(model), trash_path);
    gtk_tree_path_free(trash_path);

    if(0 == model->trash_idle_handler)
        model->trash_idle_handler = gdk_threads_add_idle(update_trash_item, model);
}

static void on_places_unmounted_changed(FmConfig* cfg, gpointer user_data)
{
    FmPlacesModel *model = FM_PLACES_MODEL(user_data);
    GVolume *vol;
    FmPlacesItem *item;
    GList *vols, *l;
    FmFileInfoJob *job = NULL;
    GtkTreeIter it;

    if (cfg->places_unmounted)
        job = fm_file_info_job_new(NULL, FM_FILE_INFO_JOB_NONE);
    vols = g_volume_monitor_get_volumes(model->vol_mon);
    for (l = vols; l; l = l->next)
    {
        vol = G_VOLUME(l->data);
        item = find_volume(model, vol, &it);
        if (item)
        {
            if (!cfg->places_unmounted) /* it is there but should be not */
            {
                gtk_list_store_remove(GTK_LIST_STORE(model), &it);
                place_item_free(item);
            }
        }
        else if (cfg->places_unmounted) /* not found but should be added */
            add_volume_or_mount(model, G_OBJECT(vol), job);
        g_object_unref(vol);
    }
    if (job)
    {
        g_signal_connect(job, "finished", G_CALLBACK(on_file_info_job_finished), model);
        model->jobs = g_slist_prepend(model->jobs, job);
        if (!fm_job_run_async(FM_JOB(job)))
        {
            model->jobs = g_slist_remove(model->jobs, job);
            g_object_unref(job);
            g_critical("fm_job_run_async() failed on volumes update");
        }
    }
    g_list_free(vols);
}

static void fm_places_model_init(FmPlacesModel *self)
{
    GType types[] = {GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER};
    GtkTreeIter it;
    GList *vols, *l;
    FmIcon *icon;
    FmFileInfoJob* job = fm_file_info_job_new(NULL, FM_FILE_INFO_JOB_FOLLOW_SYMLINK);
    GtkListStore* model = &self->parent;
    FmPath *path;
    GtkTreePath* tp;

    gtk_list_store_set_column_types(&self->parent, FM_PLACES_MODEL_N_COLS, types);

    self->theme_change_handler = g_signal_connect_swapped(gtk_icon_theme_get_default(), "changed",
                                            G_CALLBACK(update_icons), self);

    self->use_trash_change_handler = g_signal_connect(fm_config, "changed::use_trash",
                                             G_CALLBACK(on_use_trash_changed), self);

    self->places_home_change_handler = g_signal_connect(fm_config, "changed::places_home",
                                             G_CALLBACK(on_places_home_changed), self);
    self->places_desktop_change_handler = g_signal_connect(fm_config, "changed::places_desktop",
                                             G_CALLBACK(on_places_desktop_changed), self);
    self->places_root_change_handler = g_signal_connect(fm_config, "changed::places_root",
                                             G_CALLBACK(on_places_root_changed), self);
    self->places_computer_change_handler = g_signal_connect(fm_config, "changed::places_computer",
                                             G_CALLBACK(on_places_computer_changed), self);
    self->places_trash_change_handler = g_signal_connect(fm_config, "changed::places_trash",
                                             G_CALLBACK(on_use_trash_changed), self);
    self->places_applications_change_handler = g_signal_connect(fm_config, "changed::places_applications",
                                             G_CALLBACK(on_places_applications_changed), self);
    self->places_network_change_handler = g_signal_connect(fm_config, "changed::places_network",
                                             G_CALLBACK(on_places_network_changed), self);
    self->places_unmounted_change_handler = g_signal_connect(fm_config, "changed::places_unmounted",
                                             G_CALLBACK(on_places_unmounted_changed), self);

    self->pane_icon_size_change_handler = g_signal_connect(fm_config, "changed::pane_icon_size",
                                             G_CALLBACK(on_pane_icon_size_changed), self);
    icon = fm_icon_from_name("media-eject");
    self->eject_icon = fm_pixbuf_from_icon(icon, fm_config->pane_icon_size);
    g_object_unref(icon);

    if(fm_config->places_home)
    {
        path = fm_path_get_home();
        new_path_item(model, &it, path, FM_PLACES_ID_HOME,
                      _("Home Folder"), "user-home", job);
    }

    /* Only show desktop in side pane when the user has a desktop dir. */
    if(fm_config->places_desktop &&
       g_file_test(g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP), G_FILE_TEST_IS_DIR))
    {
        new_path_item(model, &it, fm_path_get_desktop(), FM_PLACES_ID_DESKTOP,
                      _("Desktop"), "user-desktop", job);
    }

    if(fm_config->places_root)
    {
        new_path_item(model, &it, fm_path_get_root(), FM_PLACES_ID_ROOT,
                      _("Filesystem Root"), "drive-harddisk", job);
    }

    if(fm_config->places_computer)
    {
        path = fm_path_new_for_uri("computer:///");
        new_path_item(model, &it, path, FM_PLACES_ID_COMPUTER,
                      _("Devices"), "computer", job);
        fm_path_unref(path);
    }

    if(fm_config->places_applications && fm_module_is_in_use("vfs", "menu"))
    {
        new_path_item(model, &it, fm_path_get_apps_menu(),
                      FM_PLACES_ID_APPLICATIONS, _("Applications"),
                      "system-software-install", job);
    }

    if(fm_config->places_network)
    {
        path = fm_path_new_for_uri("network:///");
        new_path_item(model, &it, path, FM_PLACES_ID_NETWORK,
                      _("Network"), GTK_STOCK_NETWORK, job);
        fm_path_unref(path);
    }

    /* volumes */
    self->vol_mon = g_volume_monitor_get();
    if(self->vol_mon)
    {
        g_signal_connect(self->vol_mon, "volume-added", G_CALLBACK(on_volume_added), self);
        g_signal_connect(self->vol_mon, "volume-removed", G_CALLBACK(on_volume_removed), self);
        g_signal_connect(self->vol_mon, "volume-changed", G_CALLBACK(on_volume_changed), self);
        g_signal_connect(self->vol_mon, "mount-added", G_CALLBACK(on_mount_added), self);
        g_signal_connect(self->vol_mon, "mount-changed", G_CALLBACK(on_mount_changed), self);
        g_signal_connect(self->vol_mon, "mount-removed", G_CALLBACK(on_mount_removed), self);
    }

    /* separator */
    gtk_list_store_append(model, &it);
    tp = gtk_tree_model_get_path(GTK_TREE_MODEL(self), &it);
    self->separator = gtk_tree_row_reference_new(GTK_TREE_MODEL(self), tp);
    gtk_tree_path_free(tp);
    /* separator has all columns NULL */

    if(fm_config->use_trash && fm_config->places_trash)
        create_trash_item(self);

    /* add volumes to side-pane */
    /* respect fm_config->places_unmounted */
    if (fm_config->places_unmounted)
    {
        vols = g_volume_monitor_get_volumes(self->vol_mon);
        for(l=vols;l;l=l->next)
        {
            GVolume* vol = G_VOLUME(l->data);
            add_volume_or_mount(self, G_OBJECT(vol), job);
            g_object_unref(vol);
        }
        g_list_free(vols);
    }

    /* add mounts to side-pane */
    vols = g_volume_monitor_get_mounts(self->vol_mon);
    for(l=vols;l;l=l->next)
    {
        GMount* mount = G_MOUNT(l->data);
        GVolume* volume = g_mount_get_volume(mount);
        if(volume)
            g_object_unref(volume);
        else /* network mounts or others */
            add_volume_or_mount(self, G_OBJECT(mount), job);
        g_object_unref(mount);
    }
    g_list_free(vols);

    self->bookmarks = fm_bookmarks_dup(); /* bookmarks */
    if(self->bookmarks)
        g_signal_connect(self->bookmarks, "changed", G_CALLBACK(on_bookmarks_changed), self);

    /* add bookmarks to side pane */
    add_bookmarks(self, job);

    g_signal_connect(job, "finished", G_CALLBACK(on_file_info_job_finished), self);
    self->jobs = g_slist_prepend(self->jobs, job);
    if (!fm_job_run_async(FM_JOB(job)))
    {
        self->jobs = g_slist_remove(self->jobs, job);
        g_object_unref(job);
        g_critical("fm_job_run_async() failed on places view init");
    }
}

/**
 * fm_places_model_get_separator_path
 * @model: a places model instance
 *
 * Retrieves path to separator between places and bookmark items. Returned
 * path should be freed with gtk_tree_path_free() after usage.
 *
 * Returns: (transfer full): the path to separator.
 *
 * Since: 0.1.14
 */
GtkTreePath* fm_places_model_get_separator_path(FmPlacesModel* model)
{
    return gtk_tree_row_reference_get_path(model->separator);
}

/**
 * fm_places_model_get_bookmarks
 * @model: a places model instance
 *
 * Retrieves list of bookmarks that is used by the @model. Returned data
 * are owned by places model and should not be freed by caller.
 *
 * Returns: (transfer none): list of bookmarks.
 *
 * Since: 1.0.0
 */
FmBookmarks* fm_places_model_get_bookmarks(FmPlacesModel* model)
{
    return model->bookmarks;
}

/**
 * fm_places_model_iter_is_separator
 * @model: a places model instance
 * @it: model iterator to inspect
 *
 * Checks if the row described in @it is a separator.
 *
 * Returns: %TRUE if the row is a separator.
 *
 * Since: 0.1.14
 */
gboolean fm_places_model_iter_is_separator(FmPlacesModel* model, GtkTreeIter* it)
{
    gpointer item = NULL;

    if(it == NULL)
        return FALSE;
    gtk_tree_model_get(GTK_TREE_MODEL(model), it, FM_PLACES_MODEL_COL_INFO, &item, -1);
    return (item == NULL);
}

/**
 * fm_places_model_path_is_separator
 * @model: a places model instance
 * @tp: the row path to inspect
 *
 * Checks if the row by @tp is a separator between places and bookmarks.
 *
 * Returns: %TRUE if the row is a separator.
 *
 * Since: 0.1.14
 */
gboolean fm_places_model_path_is_separator(FmPlacesModel* model, GtkTreePath* tp)
{
    GtkTreePath* sep_tp;
    gboolean ret = FALSE;

    if(tp)
    {
        sep_tp = gtk_tree_row_reference_get_path(model->separator);
        ret = gtk_tree_path_compare(sep_tp, tp) == 0;
        gtk_tree_path_free(sep_tp);
    }
    return ret;
}

/**
 * fm_places_model_path_is_bookmark
 * @model: a places model instance
 * @tp: the row path to inspect
 *
 * Checks if the row by @tp lies within bookmark items.
 *
 * Returns: %TRUE if the row is a bookmark item.
 *
 * Since: 0.1.14
 */
gboolean fm_places_model_path_is_bookmark(FmPlacesModel* model, GtkTreePath* tp)
{
    GtkTreePath* sep_tp;
    gboolean ret = FALSE;

    if(tp)
    {
        sep_tp = gtk_tree_row_reference_get_path(model->separator);
        ret = gtk_tree_path_compare(sep_tp, tp) < 0;
        gtk_tree_path_free(sep_tp);
    }
    return ret;
}

/**
 * fm_places_model_path_is_places
 * @model: a places model instance
 * @tp: the row path to inspect
 *
 * Checks if the row by @tp lies above separator, i.e. within "places".
 *
 * Returns: %TRUE if the row is a places item.
 *
 * Since: 0.1.14
 */
gboolean fm_places_model_path_is_places(FmPlacesModel* model, GtkTreePath* tp)
{
    GtkTreePath* sep_tp;
    gboolean ret = FALSE;

    if(tp)
    {
        sep_tp = gtk_tree_row_reference_get_path(model->separator);
        ret = gtk_tree_path_compare(sep_tp, tp) > 0;
        gtk_tree_path_free(sep_tp);
    }
    return ret;
}

static gboolean row_draggable(GtkTreeDragSource* drag_source, GtkTreePath* tp)
{
    FmPlacesModel* model;
    g_return_val_if_fail(FM_IS_PLACES_MODEL(drag_source), FALSE);
    model = (FmPlacesModel*)drag_source;
    return fm_places_model_path_is_bookmark(model, tp);
}

static void fm_places_model_drag_source_init(GtkTreeDragSourceIface *iface)
{
    iface->row_draggable = row_draggable;
}

G_DEFINE_TYPE_WITH_CODE (FmPlacesModel, fm_places_model, GTK_TYPE_LIST_STORE,
             G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_SOURCE,
                        fm_places_model_drag_source_init))

static void fm_places_model_dispose(GObject *object)
{
    FmPlacesModel *self;
    GtkTreeIter it;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_PLACES_MODEL(object));
    self = (FmPlacesModel*)object;

    if(self->jobs)
    {
        GSList* l;
        for(l = self->jobs; l; l=l->next)
        {
            g_signal_handlers_disconnect_by_func(l->data, on_file_info_job_finished, self);
            fm_job_cancel(FM_JOB(l->data));
            g_object_unref(l->data);
        }
        g_slist_free(self->jobs);
        self->jobs = NULL;
    }

    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(self), &it))
    {
        do
        {
            FmPlacesItem* item;
            gtk_tree_model_get(GTK_TREE_MODEL(self), &it, FM_PLACES_MODEL_COL_INFO, &item, -1);
            if(G_LIKELY(item))
                place_item_free(item);
        }while(gtk_tree_model_iter_next(GTK_TREE_MODEL(self), &it));
    }

    gtk_tree_row_reference_free(self->separator);
    self->separator = NULL;

    gtk_tree_row_reference_free(self->trash);
    self->trash = NULL;

    if(self->theme_change_handler)
    {
        g_signal_handler_disconnect(gtk_icon_theme_get_default(), self->theme_change_handler);
        self->theme_change_handler = 0;
    }
    if(self->use_trash_change_handler)
    {
        g_signal_handler_disconnect(fm_config, self->use_trash_change_handler);
        self->use_trash_change_handler = 0;
    }
    if(self->places_home_change_handler)
    {
        g_signal_handler_disconnect(fm_config, self->places_home_change_handler);
        self->places_home_change_handler = 0;
    }
    if(self->places_desktop_change_handler)
    {
        g_signal_handler_disconnect(fm_config, self->places_desktop_change_handler);
        self->places_desktop_change_handler = 0;
    }
    if(self->places_root_change_handler)
    {
        g_signal_handler_disconnect(fm_config, self->places_root_change_handler);
        self->places_root_change_handler = 0;
    }
    if(self->places_computer_change_handler)
    {
        g_signal_handler_disconnect(fm_config, self->places_computer_change_handler);
        self->places_computer_change_handler = 0;
    }
    if(self->places_trash_change_handler)
    {
        g_signal_handler_disconnect(fm_config, self->places_trash_change_handler);
        self->places_trash_change_handler = 0;
    }
    if(self->places_applications_change_handler)
    {
        g_signal_handler_disconnect(fm_config, self->places_applications_change_handler);
        self->places_applications_change_handler = 0;
    }
    if(self->places_network_change_handler)
    {
        g_signal_handler_disconnect(fm_config, self->places_network_change_handler);
        self->places_network_change_handler = 0;
    }
    if(self->places_unmounted_change_handler)
    {
        g_signal_handler_disconnect(fm_config, self->places_unmounted_change_handler);
        self->places_unmounted_change_handler = 0;
    }
    if(self->pane_icon_size_change_handler)
    {
        g_signal_handler_disconnect(fm_config, self->pane_icon_size_change_handler);
        self->pane_icon_size_change_handler = 0;
    }

    if(self->vol_mon)
    {
        g_signal_handlers_disconnect_by_func(self->vol_mon, on_volume_added, self);
        g_signal_handlers_disconnect_by_func(self->vol_mon, on_volume_removed, self);
        g_signal_handlers_disconnect_by_func(self->vol_mon, on_volume_changed, self);
        g_signal_handlers_disconnect_by_func(self->vol_mon, on_mount_added, self);
        g_signal_handlers_disconnect_by_func(self->vol_mon, on_mount_changed, self);
        g_signal_handlers_disconnect_by_func(self->vol_mon, on_mount_removed, self);
        g_object_unref(self->vol_mon);
        self->vol_mon = NULL;
    }

    if(self->bookmarks)
    {
        g_signal_handlers_disconnect_by_func(self->bookmarks, on_bookmarks_changed, self);
        g_object_unref(self->bookmarks);
        self->bookmarks = NULL;
    }

    if(self->trash_monitor)
    {
        g_signal_handlers_disconnect_by_func(self->trash_monitor, on_trash_changed, self);
        g_object_unref(self->trash_monitor);
        self->trash_monitor = NULL;
    }

    if(self->trash_idle_handler)
    {
        g_source_remove(self->trash_idle_handler);
        self->trash_idle_handler = 0;
    }

    if(self->eject_icon)
        g_object_unref(self->eject_icon);
    self->eject_icon = NULL;

    G_OBJECT_CLASS(fm_places_model_parent_class)->dispose(object);
}

static void fm_places_model_class_init(FmPlacesModelClass *klass)
{
    GObjectClass *g_object_class;

    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->dispose = fm_places_model_dispose;
}


/**
 * fm_places_model_new
 *
 * Creates new places model.
 *
 * Returns: (transfer full): a new #FmPlacesModel object.
 *
 * Since: 0.1.14
 */
FmPlacesModel *fm_places_model_new(void)
{
    return g_object_new(FM_TYPE_PLACES_MODEL, NULL);
}

/**
 * fm_places_model_mount_indicator_cell_data_func
 * @cell_layout: the cell layout
 * @render: the cell renderer
 * @tree_model: a places model instance
 * @it: the row iterator
 * @user_data: unused
 *
 * 
 *
 * Since: 0.1.15
 */
void fm_places_model_mount_indicator_cell_data_func(GtkCellLayout *cell_layout,
                                           GtkCellRenderer *render,
                                           GtkTreeModel *tree_model,
                                           GtkTreeIter *it,
                                           gpointer user_data)
{
    FmPlacesItem* item = NULL;
    GdkPixbuf* pix = NULL;
    g_return_if_fail(FM_IS_PLACES_MODEL(tree_model));
    gtk_tree_model_get(tree_model, it, FM_PLACES_MODEL_COL_INFO, &item, -1);
    if(item && item->mounted)
        pix = ((FmPlacesModel*)tree_model)->eject_icon;
    g_object_set(render, "pixbuf", pix, NULL);
}

/**
 * fm_places_model_get_iter_by_fm_path
 * @model: a places model instance
 * @iter: the row iterator pointer
 * @path: a file path to search
 *
 * Tries to find an item in the @model by the @path. If item was found
 * within @model then sets @iter to match the found item.
 *
 * Returns: %TRUE if item was found.
 *
 * Since: 1.0.0
 */
gboolean fm_places_model_get_iter_by_fm_path(FmPlacesModel* model, GtkTreeIter* iter, FmPath* path)
{
    GtkTreeIter it;
    GtkTreeModel* model_ = GTK_TREE_MODEL(model);
    if(gtk_tree_model_get_iter_first(model_, &it))
    {
        FmPlacesItem* item;
        do{
            item = NULL;
            gtk_tree_model_get(model_, &it, FM_PLACES_MODEL_COL_INFO, &item, -1);
            if(item && item->fi && fm_path_equal(fm_file_info_get_path(item->fi), path))
            {
                *iter = it;
                return TRUE;
            }
        }while(gtk_tree_model_iter_next(model_, &it));
    }
    return FALSE;
}


/**
 * fm_places_item_get_type
 * @item: a places model item
 *
 * Retrieves type of @item.
 *
 * Returns: type of item.
 *
 * Since: 1.0.0
 */
FmPlacesType fm_places_item_get_type(FmPlacesItem* item)
{
    return item->type;
}

/**
 * fm_places_item_is_mounted
 * @item: a places model item
 *
 * Checks if the row is a mounted volume.
 *
 * Returns: %TRUE if the row is a mounted volume.
 *
 * Since: 1.0.0
 */
gboolean fm_places_item_is_mounted(FmPlacesItem* item)
{
    return item->mounted ? TRUE : FALSE;
}

/**
 * fm_places_item_get_icon
 * @item: a places model item
 *
 * Retrieves icom image for the row. Returned data are owned by places
 * model and should not be freed by caller.
 *
 * Returns: (transfer none): icon descriptor.
 *
 * Since: 1.0.0
 */
FmIcon* fm_places_item_get_icon(FmPlacesItem* item)
{
    return item->icon;
}

/**
 * fm_places_item_get_info
 * @item: a places model item
 *
 * Retrieves file info for the row. Returned data are owned by places
 * model and should not be freed by caller.
 *
 * Returns: (transfer none): file info descriptor.
 *
 * Since: 1.0.0
 */
FmFileInfo* fm_places_item_get_info(FmPlacesItem* item)
{
    return item->fi;
}

/**
 * fm_places_item_get_volume
 * @item: a places model item
 *
 * Retrieves volume descriptor for the row. Returned data are owned by
 * places model and should not be freed by caller.
 *
 * Returns: (transfer none): volume descriptor or %NULL if @item isn't a
 * mountable volume.
 *
 * Since: 1.0.0
 */
GVolume* fm_places_item_get_volume(FmPlacesItem* item)
{
    return item->type == FM_PLACES_ITEM_VOLUME ? item->volume : NULL;
}

/**
 * fm_places_item_get_mount
 * @item: a places model item
 *
 * Rertieves mount descriptor for the row. Returned data are owned by
 * places model and should not be freed by caller.
 *
 * Returns: (transfer none): mount descriptor or %NULL if @item isn't a
 * mounted path.
 *
 * Since: 1.0.0
 */
GMount* fm_places_item_get_mount(FmPlacesItem* item)
{
    return item->type == FM_PLACES_ITEM_MOUNT ? item->mount : NULL;
}

/**
 * fm_places_item_get_path
 * @item: a places model item
 *
 * Retrieves path for the row. Returned data are owned by places model
 * and should not be freed by caller.
 *
 * Returns: (transfer none): item path.
 *
 * Since: 1.0.0
 */
FmPath* fm_places_item_get_path(FmPlacesItem* item)
{
    return item->fi ? fm_file_info_get_path(item->fi) : NULL;
}

/**
 * fm_places_item_get_bookmark_item
 * @item: a places model item
 *
 * Retrieves bookmark descriptor for the row. Returned data are owned by
 * places model and should not be freed by caller.
 *
 * Returns: (transfer none): bookmark descriptor or %NULL if @item isn't
 * a bookmark.
 *
 * Since: 1.0.0
 */
FmBookmarkItem* fm_places_item_get_bookmark_item(FmPlacesItem* item)
{
    return item->type == FM_PLACES_ITEM_PATH ? item->bm_item : NULL;
}
