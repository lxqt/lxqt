//      g-udisks-drive.c
//
//      Copyright 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "g-udisks-drive.h"
#include "g-udisks-device.h"
#include "g-udisks-volume.h"
#include <string.h>
#include "udisks-device.h"
#include "dbus-utils.h"

typedef struct
{
    GUDisksDrive* drv;
    GAsyncReadyCallback callback;
    GMountUnmountFlags flags;
    GMountOperation* op;
    GCancellable* cancellable;
    gpointer user_data;
    DBusGProxy* proxy;
    DBusGProxyCall* call;
    GList* mounts;
}EjectData;

static guint sig_changed;
static guint sig_disconnected;
static guint sig_eject_button;
static guint sig_stop_button;

static void g_udisks_drive_drive_iface_init (GDriveIface * iface);
static void g_udisks_drive_finalize            (GObject *object);

G_DEFINE_TYPE_EXTENDED (GUDisksDrive, g_udisks_drive, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_DRIVE,
                                               g_udisks_drive_drive_iface_init))


static void g_udisks_drive_class_init(GUDisksDriveClass *klass)
{
    GObjectClass *g_object_class;

    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->finalize = g_udisks_drive_finalize;
}


static gboolean g_udisks_drive_can_eject (GDrive* base)
{
    GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    return drv->dev->is_ejectable;
}

static gboolean g_udisks_drive_can_poll_for_media (GDrive* base)
{
    GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    return drv->dev->is_media_change_notification_polling;
}

static gboolean g_udisks_drive_can_start (GDrive* base)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    /* TODO */
    return FALSE;
}

static gboolean g_udisks_drive_can_start_degraded (GDrive* base)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    /* TODO */
    return FALSE;
}

static gboolean g_udisks_drive_can_stop (GDrive* base)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    /* TODO */
    return FALSE;
}

static GList* g_udisks_drive_get_volumes (GDrive* base)
{
    /* FIXME: is it a better idea to save all volumes in GUDisksDrive instead? */
    GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    GList* vols = g_volume_monitor_get_volumes(G_VOLUME_MONITOR(drv->mon));
    GList* l;
    GList* ret = NULL;
    for(l = vols;l;l=l->next)
    {
        GUDisksVolume* vol = G_UDISKS_VOLUME(l->data);
        if(vol->drive == drv)
            ret = g_list_prepend(ret, vol);
        else
            g_object_unref(vol);
    }
    g_list_free(vols);
    return ret;
}

static void on_eject_cancelled(GCancellable* cancellable, gpointer user_data);

static void finish_eject(GSimpleAsyncResult* res, EjectData* data)
{
    g_simple_async_result_complete(res);
    g_object_unref(res);

    g_object_unref(data->drv);
    if(data->cancellable)
    {
        g_signal_handlers_disconnect_by_func(data->cancellable, on_eject_cancelled, data);
        g_object_unref(data->cancellable);
    }
    if(data->proxy)
        g_object_unref(data->proxy);
    g_slice_free(EjectData, data);
}

static void on_ejected(DBusGProxy *proxy, GError *error, gpointer user_data)
{
    EjectData* data = (EjectData*)user_data;
    GSimpleAsyncResult* res;
    if(error)
    {
        error = g_udisks_error_to_gio_error(error);
        res = g_simple_async_result_new_from_error(G_OBJECT(data->drv),
                                                   data->callback,
                                                   data->user_data,
                                                   error);
        g_error_free(error);
    }
    else
    {
        res = g_simple_async_result_new(G_OBJECT(data->drv),
                                        data->callback,
                                        data->user_data,
                                        NULL);
        g_simple_async_result_set_op_res_gboolean(res, TRUE);
    }
    finish_eject(res, data);
}

static void do_eject(EjectData* data)
{
    data->proxy = g_udisks_device_get_proxy(data->drv->dev, data->drv->mon->con);
    data->call = org_freedesktop_UDisks_Device_drive_eject_async(
                        data->proxy,
                        NULL,
                        on_ejected,
                        data);
}

static void unmount_before_eject(EjectData* data);

static void on_unmounted(GObject* mnt, GAsyncResult* res, gpointer input_data)
{
#define data ((EjectData*)input_data)
    GError* err = NULL;
    /* FIXME: with this approach, we could have racing condition.
     * Someone may mount other volumes before we finishing unmounting them all. */
    gboolean success = g_mount_unmount_with_operation_finish(G_MOUNT(mnt), res, &err);
    if(success)
    {
        if(data->mounts) /* we still have some volumes on this drive mounted */
            unmount_before_eject(data);
        else /* all unmounted, do the eject. */
            do_eject(data);
    }
    else
    {
        GSimpleAsyncResult* res;
        GError* error = g_udisks_error_to_gio_error(err);
        g_error_free(err);
        res = g_simple_async_result_new_from_error(G_OBJECT(data->drv),
                                                   data->callback,
                                                   data->user_data,
                                                   err);
        finish_eject(res, data);
        g_error_free(error);
    }
#undef data
}

static void unmount_before_eject(EjectData* data)
{
    GMount* mnt = G_MOUNT(data->mounts->data);
    data->mounts = g_list_delete_link(data->mounts, data->mounts);
    /* pop the first GMount in the list. */
    g_mount_unmount_with_operation(mnt, data->flags, data->op,
                                 data->cancellable,
                                 on_unmounted, data);
    /* FIXME: Notify volume monitor!! */
    g_object_unref(mnt);
}

static void on_eject_cancelled(GCancellable* cancellable, gpointer user_data)
{
    EjectData* data = (EjectData*)user_data;
    /* cancel the dbus call if needed */
    if(data->call)
        dbus_g_proxy_cancel_call(data->proxy, data->call);
}

static void g_udisks_drive_eject_with_operation (GDrive* base, GMountUnmountFlags flags, GMountOperation* mount_operation, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    GList* vols = g_udisks_drive_get_volumes(base);
    GList* mounts = NULL;
    EjectData* data;

    /* umount all volumes/mounts first */
    if(vols)
    {
        GList* l;
        for(l = vols; l; l=l->next)
        {
            GVolume* vol = G_VOLUME(l->data);
            GMount* mnt = g_volume_get_mount(vol);
            if(mnt)
                mounts = g_list_prepend(mounts, mnt);
            g_object_unref(vol);
        }
        g_list_free(vols);
    }

    data = g_slice_new0(EjectData);
    data->drv = g_object_ref(drv);
    data->callback = callback;
    data->cancellable = cancellable ? g_object_ref(cancellable) : NULL;
    data->flags = flags;
    data->op = mount_operation ? g_object_ref(mount_operation) : NULL;
    data->user_data = user_data;

    if(cancellable)
        g_signal_connect(cancellable, "cancelled", G_CALLBACK(on_eject_cancelled), data);

    if(mounts) /* unmount all GMounts first, and do eject in ready callback */
    {
        /* NOTE: is this really needed?
         * I read the source code of UDisks and found it calling "eject"
         * command internally. According to manpage of "eject", it unmounts
         * partitions before ejecting the device. So I don't think that we
         * need to unmount ourselves. However, without this, we won't have
         * correct "mount-pre-unmount" signals. So, let's do it. */
        data->mounts = mounts;
        unmount_before_eject(data);
    }
    else /* no volume is mounted. it's safe to do eject directly. */
        do_eject(data);
}

static gboolean g_udisks_drive_eject_with_operation_finish (GDrive* base, GAsyncResult* res, GError** error)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    return !g_simple_async_result_propagate_error(G_SIMPLE_ASYNC_RESULT(res), error);
}

static void g_udisks_drive_eject (GDrive* base, GMountUnmountFlags flags, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    g_udisks_drive_eject_with_operation(base, flags, NULL, cancellable, callback, user_data);
}

static gboolean g_udisks_drive_eject_finish (GDrive* base, GAsyncResult* res, GError** error)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    return g_udisks_drive_eject_with_operation_finish(base, res, error);
}

static char** g_udisks_drive_enumerate_identifiers (GDrive* base)
{
    char** kinds = g_new0(char*, 4);
    kinds[0] = g_strdup(G_VOLUME_IDENTIFIER_KIND_LABEL);
    kinds[1] = g_strdup(G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    kinds[2] = g_strdup(G_VOLUME_IDENTIFIER_KIND_UUID);
    kinds[3] = NULL;
    return kinds;
}

static GIcon* g_udisks_drive_get_icon (GDrive* base)
{
    GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    if(!drv->icon)
    {
        const char* icon_name = g_udisks_device_get_icon_name(drv->dev);
        drv->icon = g_themed_icon_new_with_default_fallbacks(icon_name);
    }
    return (GIcon*)g_object_ref(drv->icon);
}

static char* g_udisks_drive_get_identifier (GDrive* base, const char* kind)
{
    GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    if(kind)
    {
        if(strcmp(kind, G_VOLUME_IDENTIFIER_KIND_LABEL) == 0)
            return g_strdup(drv->dev->label);
        else if(strcmp(kind, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE) == 0)
            return g_strdup(drv->dev->dev_file);
        else if(strcmp(kind, G_VOLUME_IDENTIFIER_KIND_UUID) == 0)
            return g_strdup(drv->dev->uuid);
    }
    return NULL;
}

static char* g_udisks_drive_get_name (GDrive* base)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    /* TODO */
    return g_strdup("");
}

static GDriveStartStopType g_udisks_drive_get_start_stop_type (GDrive* base)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    /* TODO */
    return G_DRIVE_START_STOP_TYPE_UNKNOWN;
}

static gboolean g_udisks_drive_has_media (GDrive* base)
{
    GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    return drv->dev->is_media_available;
}

static gboolean g_udisks_drive_has_volumes (GDrive* base)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    return FALSE;
}

static gboolean g_udisks_drive_is_media_check_automatic (GDrive* base)
{
    GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    /* FIXME: is this correct? */
    return drv->dev->is_media_change_notification_polling;
}

static gboolean g_udisks_drive_is_media_removable (GDrive* base)
{
    GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    return drv->dev->is_removable;
}

static void g_udisks_drive_poll_for_media (GDrive* base, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    /* TODO */
}

static gboolean g_udisks_drive_poll_for_media_finish (GDrive* base, GAsyncResult* res, GError** error)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    return FALSE;
}

static void g_udisks_drive_start (GDrive* base, GDriveStartFlags flags, GMountOperation* mount_operation, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    /* TODO */
}

static gboolean g_udisks_drive_start_finish (GDrive* base, GAsyncResult* res, GError** error)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    /* TODO */
    return FALSE;
}

static void g_udisks_drive_stop (GDrive* base, GMountUnmountFlags flags, GMountOperation* mount_operation, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    /* TODO */
}

static gboolean g_udisks_drive_stop_finish (GDrive* base, GAsyncResult* res, GError** error)
{
    //GUDisksDrive* drv = G_UDISKS_DRIVE(base);
    /* TODO */
    return FALSE;
}


void g_udisks_drive_drive_iface_init (GDriveIface * iface)
{
    iface->get_name = g_udisks_drive_get_name;
    iface->get_icon = g_udisks_drive_get_icon;
    iface->has_volumes = g_udisks_drive_has_volumes;
    iface->get_volumes = g_udisks_drive_get_volumes;
    iface->is_media_removable = g_udisks_drive_is_media_removable;
    iface->has_media = g_udisks_drive_has_media;
    iface->is_media_check_automatic = g_udisks_drive_is_media_check_automatic;
    iface->can_eject = g_udisks_drive_can_eject;
    iface->can_poll_for_media = g_udisks_drive_can_poll_for_media;
    iface->eject = g_udisks_drive_eject;
    iface->eject_finish = g_udisks_drive_eject_finish;
    iface->eject_with_operation = g_udisks_drive_eject_with_operation;
    iface->eject_with_operation_finish = g_udisks_drive_eject_with_operation_finish;
    iface->poll_for_media = g_udisks_drive_poll_for_media;
    iface->poll_for_media_finish = g_udisks_drive_poll_for_media_finish;
    iface->get_identifier = g_udisks_drive_get_identifier;
    iface->enumerate_identifiers = g_udisks_drive_enumerate_identifiers;

    iface->get_start_stop_type = g_udisks_drive_get_start_stop_type;
    iface->can_start = g_udisks_drive_can_start;
    iface->can_start_degraded = g_udisks_drive_can_start_degraded;
    iface->can_stop = g_udisks_drive_can_stop;
    iface->start = g_udisks_drive_start;
    iface->start_finish = g_udisks_drive_start_finish;
    iface->stop = g_udisks_drive_stop;
    iface->stop_finish = g_udisks_drive_stop_finish;

    sig_changed = g_signal_lookup("changed", G_TYPE_DRIVE);
    sig_disconnected = g_signal_lookup("disconnected", G_TYPE_DRIVE);
    sig_eject_button = g_signal_lookup("eject-button", G_TYPE_DRIVE);
    sig_stop_button = g_signal_lookup("stop-button", G_TYPE_DRIVE);

}

static void g_udisks_drive_finalize(GObject *object)
{
    GUDisksDrive *self;

    g_return_if_fail(object != NULL);
    g_return_if_fail(G_IS_UDISKS_DRIVE(object));

    self = G_UDISKS_DRIVE(object);
    if(self->dev)
        g_object_unref(self->dev);

    G_OBJECT_CLASS(g_udisks_drive_parent_class)->finalize(object);
}


static void g_udisks_drive_init(GUDisksDrive *self)
{

}

GUDisksDrive *g_udisks_drive_new(GUDisksVolumeMonitor* mon, GUDisksDevice* dev)
{
    GUDisksDrive* drv = (GUDisksDrive*)g_object_new(G_TYPE_UDISKS_DRIVE, NULL);
    drv->dev = g_object_ref(dev);
    drv->mon = mon;
    return drv;
}


void g_udisks_drive_changed(GUDisksDrive* drv)
{
    g_signal_emit(drv, sig_changed, 0);
}

void g_udisks_drive_disconnected(GUDisksDrive* drv)
{
    g_signal_emit(drv, sig_disconnected, 0);
}
