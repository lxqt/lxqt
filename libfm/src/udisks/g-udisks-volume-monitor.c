/*
 *      g-udisks-volume-monitor.c
 *
 *      Copyright 2010 PCMan <pcman.tw@gmail.com>
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

#include "dbus-utils.h"
#include "udisks.h"

#include "g-udisks-volume-monitor.h"
#include "g-udisks-device.h"
#include "g-udisks-drive.h"
#include "g-udisks-volume.h"

/* FIXME: later we need to remove this when gio-udisks becomes an
 * independent gio module. */
#include "fm-config.h"

static guint sig_drive_changed;
static guint sig_drive_connected;
static guint sig_drive_disconnected;
static guint sig_drive_eject_button;
static guint sig_mount_added;
static guint sig_mount_changed;
static guint sig_mount_premount;
static guint sig_mount_removed;
static guint sig_volume_added;
static guint sig_volume_changed;
static guint sig_volume_removed;

static void g_udisks_device_added(GUDisksDevice* dev, GUDisksVolumeMonitor* mon);
static void g_udisks_device_changed(GUDisksDevice* dev, GUDisksVolumeMonitor* mon);
static void g_udisks_device_removed(GUDisksDevice* dev, GUDisksVolumeMonitor* mon);

typedef void (*GUDisksDeviceEventHandler)(GUDisksDevice*, GUDisksVolumeMonitor*);

typedef struct
{
    GUDisksDevice* dev;
    GUDisksDeviceEventHandler func;
}QueuedEvent;

static void g_udisks_volume_monitor_finalize            (GObject *object);
static GMount* get_mount_for_mount_path(const char *mount_path, GCancellable *cancellable);

static gboolean is_supported(void);
static GList* get_connected_drives(GVolumeMonitor *mon);
static GList* get_volumes(GVolumeMonitor *mon);
static GList* get_mounts(GVolumeMonitor *mon);
static GVolume *get_volume_for_uuid(GVolumeMonitor *mon, const char *uuid);
static GMount *get_mount_for_uuid(GVolumeMonitor *mon, const char *uuid);
static void drive_eject_button(GVolumeMonitor *mon, GDrive *drive);

static void add_device(GUDisksVolumeMonitor* mon, DBusGProxy* proxy, const char* obj_path, gboolean emit_signal);
static void on_device_added(DBusGProxy* proxy, const char* obj_path, gpointer user_data);
static void on_device_removed(DBusGProxy* proxy, const char* obj_path, gpointer user_data);
static void on_device_changed(DBusGProxy* proxy, const char* obj_path, gpointer user_data);

static GList* find_device_l(GUDisksVolumeMonitor* mon, const char* obj_path);
static GList* find_drive_l(GUDisksVolumeMonitor* mon, GUDisksDevice* dev);
static GList* find_volume_l(GUDisksVolumeMonitor* mon, GUDisksDevice* dev);

static inline GUDisksDevice* find_device(GUDisksVolumeMonitor* mon, const char* obj_path)
{
    GList* l = find_device_l(mon, obj_path);
    return l ? G_UDISKS_DEVICE(l->data) : NULL;
}

static inline GUDisksDrive* find_drive(GUDisksVolumeMonitor* mon, GUDisksDevice* dev)
{
    GList* l = find_drive_l(mon, dev);
    return l ? G_UDISKS_DRIVE(l->data) : NULL;
}

static inline GUDisksVolume* find_volume(GUDisksVolumeMonitor* mon, GUDisksDevice* dev)
{
    GList* l = find_volume_l(mon, dev);
    return l ? G_UDISKS_VOLUME(l->data) : NULL;
}


G_DEFINE_TYPE(GUDisksVolumeMonitor, g_udisks_volume_monitor, G_TYPE_NATIVE_VOLUME_MONITOR);


static void g_udisks_volume_monitor_class_init(GUDisksVolumeMonitorClass *klass)
{
    GObjectClass *g_object_class = G_OBJECT_CLASS(klass);
    GNativeVolumeMonitorClass *parent_class = G_NATIVE_VOLUME_MONITOR_CLASS(klass);
    GVolumeMonitorClass *monitor_class = G_VOLUME_MONITOR_CLASS (klass);

    g_object_class->finalize = g_udisks_volume_monitor_finalize;
    parent_class->get_mount_for_mount_path = get_mount_for_mount_path;

    monitor_class->get_mounts = get_mounts;
    monitor_class->get_volumes = get_volumes;
    monitor_class->get_connected_drives = get_connected_drives;
    monitor_class->get_volume_for_uuid = get_volume_for_uuid;
    monitor_class->get_mount_for_uuid = get_mount_for_uuid;
    monitor_class->is_supported = is_supported;
    monitor_class->drive_eject_button = drive_eject_button;

    sig_drive_changed = g_signal_lookup("drive-changed", G_TYPE_VOLUME_MONITOR);
    sig_drive_connected = g_signal_lookup("drive-connected", G_TYPE_VOLUME_MONITOR);
    sig_drive_disconnected = g_signal_lookup("drive-disconnected", G_TYPE_VOLUME_MONITOR);
    sig_drive_eject_button = g_signal_lookup("drive-eject-button", G_TYPE_VOLUME_MONITOR);
    sig_mount_added = g_signal_lookup("mount-added", G_TYPE_VOLUME_MONITOR);
    sig_mount_changed = g_signal_lookup("mount-changed", G_TYPE_VOLUME_MONITOR);
    sig_mount_premount = g_signal_lookup("mount-premount", G_TYPE_VOLUME_MONITOR);
    sig_mount_removed = g_signal_lookup("mount-removed", G_TYPE_VOLUME_MONITOR);
    sig_volume_added = g_signal_lookup("volume-added", G_TYPE_VOLUME_MONITOR);
    sig_volume_changed = g_signal_lookup("volume-changed", G_TYPE_VOLUME_MONITOR);
    sig_volume_removed = g_signal_lookup("volume-removed", G_TYPE_VOLUME_MONITOR);
}


static void g_udisks_volume_monitor_finalize(GObject *object)
{
    GUDisksVolumeMonitor *self;

    g_return_if_fail(object != NULL);
    g_return_if_fail(G_IS_UDISKS_VOLUME_MONITOR(object));

    self = G_UDISKS_VOLUME_MONITOR(object);

    if(self->idle_handler)
    {
        GList* l;
        g_source_remove(self->idle_handler);
        for(l = self->queued_events; l; l=l->next)
        {
            QueuedEvent* q = (QueuedEvent*)l->data;
            g_object_unref(q->dev);
            g_slice_free(QueuedEvent, q);
        }
        g_list_free(self->queued_events);
    }

    if(self->udisks_proxy)
    {
        dbus_g_proxy_disconnect_signal(self->udisks_proxy, "DeviceAdded", G_CALLBACK(on_device_added), self);
        dbus_g_proxy_disconnect_signal(self->udisks_proxy, "DeviceRemoved", G_CALLBACK(on_device_removed), self);
        dbus_g_proxy_disconnect_signal(self->udisks_proxy, "DeviceChanged", G_CALLBACK(on_device_changed), self);
        g_object_unref(self->udisks_proxy);
    }

    if(self->devices)
    {
        g_list_foreach(self->devices, (GFunc)g_object_unref, NULL);
        g_list_free(self->devices);
    }

    if(self->drives)
    {
        g_list_foreach(self->drives, (GFunc)g_object_unref, NULL);
        g_list_free(self->drives);
    }

    if(self->volumes)
    {
        g_list_foreach(self->volumes, (GFunc)g_object_unref, NULL);
        g_list_free(self->volumes);
    }

    G_OBJECT_CLASS(g_udisks_volume_monitor_parent_class)->finalize(object);
}

static void update_volume_drive(GUDisksVolume* vol, GUDisksVolumeMonitor* mon)
{
    /* set association between drive and volumes here */
    GUDisksDevice* dev;
    if(vol->dev->is_drive) /* it's a drive itself (cdrom is an example) */
        dev = vol->dev;
    else if(vol->dev->partition_slave)
        dev = find_device(mon, vol->dev->partition_slave);
    else
        dev = NULL;

    /* no ref_count handling is needed. volume manager owns all the objects. */
    if(dev)
    {
        GUDisksDrive* drv = find_drive(mon, dev);
        vol->drive = drv;
    }
    else
        vol->drive = NULL;
}

static void g_udisks_volume_monitor_init(GUDisksVolumeMonitor *self)
{
    self->con = dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);
    if(self->con)
    {
        GPtrArray* ret;
        /* FIXME: handle disconnecting from dbus */
        self->udisks_proxy = dbus_g_proxy_new_for_name(self->con, "org.freedesktop.UDisks", "/org/freedesktop/UDisks", "org.freedesktop.UDisks");

        if(org_freedesktop_UDisks_enumerate_devices(self->udisks_proxy, &ret, NULL))
        {
            guint i;
            char** paths = (char**)ret->pdata;
            for(i=0; i<ret->len;++i)
                add_device(self, self->udisks_proxy, paths[i], FALSE);
            g_ptr_array_free(ret, TRUE);
        }

        dbus_g_proxy_add_signal(self->udisks_proxy, "DeviceAdded", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_add_signal(self->udisks_proxy, "DeviceRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_add_signal(self->udisks_proxy, "DeviceChanged", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);

        dbus_g_proxy_connect_signal(self->udisks_proxy, "DeviceAdded", G_CALLBACK(on_device_added), self, NULL);
        dbus_g_proxy_connect_signal(self->udisks_proxy, "DeviceRemoved", G_CALLBACK(on_device_removed), self, NULL);
        dbus_g_proxy_connect_signal(self->udisks_proxy, "DeviceChanged", G_CALLBACK(on_device_changed), self, NULL);

        /* find drives for volumes */
        if(self->volumes && self->drives)
            g_list_foreach(self->volumes, (GFunc)update_volume_drive, self);
    }
}


GNativeVolumeMonitor *g_udisks_volume_monitor_new(void)
{
    return g_object_new(G_UDISKS_VOLUME_MONITOR_TYPE, NULL);
}

GMount* get_mount_for_mount_path(const char *mount_path, GCancellable *cancellable)
{
    /* TODO */

    return NULL;
}


gboolean is_supported(void)
{
    return TRUE;
}

GList* get_connected_drives(GVolumeMonitor *mon)
{
    GUDisksVolumeMonitor* umon = G_UDISKS_VOLUME_MONITOR(mon);
    GList* drvs = g_list_copy(umon->drives);
    g_list_foreach(drvs, (GFunc)g_object_ref, NULL);
    return drvs;
}

GList* get_volumes(GVolumeMonitor *mon)
{
    GUDisksVolumeMonitor* umon = G_UDISKS_VOLUME_MONITOR(mon);
    GList* vols = g_list_copy(umon->volumes);
    g_list_foreach(vols, (GFunc)g_object_ref, NULL);
    return vols;
}

GList* get_mounts(GVolumeMonitor *mon)
{
    GUDisksVolumeMonitor* umon = G_UDISKS_VOLUME_MONITOR(mon);
    GList* l;
    GList* mnts = NULL;
    for(l = umon->volumes; l; l=l->next)
    {
        GUDisksVolume* vol = G_UDISKS_VOLUME(l->data);
        if(vol->mount)
            mnts = g_list_prepend(mnts, g_object_ref(vol->mount));
    }
    return mnts;
}

GVolume *get_volume_for_uuid(GVolumeMonitor *mon, const char *uuid)
{
    GUDisksVolumeMonitor* umon = G_UDISKS_VOLUME_MONITOR(mon);
    GList* l;
    for(l = umon->volumes; l; l=l->next)
    {
        GUDisksVolume* vol = G_UDISKS_VOLUME(l->data);
        if(g_strcmp0(vol->dev->uuid, uuid) == 0)
            return (GVolume*)g_object_ref(vol);
    }
    return NULL;
}

GMount *get_mount_for_uuid(GVolumeMonitor *mon, const char *uuid)
{
    GUDisksVolumeMonitor* umon = G_UDISKS_VOLUME_MONITOR(mon);
    GList* l;
    for(l = umon->volumes; l; l=l->next)
    {
        GUDisksVolume* vol = G_UDISKS_VOLUME(l->data);
        if(g_strcmp0(vol->dev->uuid, uuid) == 0)
            return g_volume_get_mount(G_VOLUME(vol));
    }
    return NULL;
}

/* signal added in 2.17 */
void drive_eject_button(GVolumeMonitor *mon, GDrive *drive)
{
    /* TODO */
    //GUDisksVolumeMonitor* umon = G_UDISKS_VOLUME_MONITOR(mon);

}

GList* find_device_l(GUDisksVolumeMonitor* mon, const char* obj_path)
{
    GList* l;
    for(l = mon->devices; l; l=l->next)
    {
        GUDisksDevice* dev = G_UDISKS_DEVICE(l->data);
        if(g_strcmp0(dev->obj_path, obj_path) == 0)
            return l;
    }
    return NULL;
}

GList* find_drive_l(GUDisksVolumeMonitor* mon, GUDisksDevice* dev)
{
    GList* l;
    for(l = mon->drives; l; l=l->next)
    {
        GUDisksDrive* drv = G_UDISKS_DRIVE(l->data);
        if(G_UNLIKELY(drv->dev == dev))
            return l;
    }
    return NULL;
}

GList* find_volume_l(GUDisksVolumeMonitor* mon, GUDisksDevice* dev)
{
    GList* l;
    for(l = mon->volumes; l; l=l->next)
    {
        GUDisksVolume* vol = G_UDISKS_VOLUME(l->data);
        if(G_UNLIKELY(vol->dev == dev))
            return l;
    }
    return NULL;
}

static void add_drive(GUDisksVolumeMonitor* mon, GUDisksDevice* dev, gboolean emit_signal)
{
    if(!find_drive(mon, dev))
    {
        GUDisksDrive* drv = g_udisks_drive_new(mon, dev);
        mon->drives = g_list_prepend(mon->drives, drv);
        if(emit_signal)
            g_signal_emit(mon, sig_drive_connected, 0, drv);
    }
}

static void add_volume(GUDisksVolumeMonitor* mon, GUDisksDevice* dev, gboolean emit_signal)
{
    if(!find_volume(mon, dev))
    {
        GUDisksVolume* vol = g_udisks_volume_new(mon, dev);
        mon->volumes = g_list_prepend(mon->volumes, vol);
        if(emit_signal)
            g_signal_emit(mon, sig_volume_added, 0, vol);
    }
}

static void remove_drive(GUDisksVolumeMonitor* mon, GUDisksDevice* dev)
{
    GList* l = find_drive_l(mon, dev);
    if(l)
    {
        GUDisksDrive* drv = G_UDISKS_DRIVE(l->data);
        mon->drives = g_list_delete_link(mon->drives, l);
        g_signal_emit(mon, sig_drive_disconnected, 0, drv);
        g_udisks_drive_disconnected(drv);
        drv->mon = NULL;
        for(l = mon->volumes; l; l=l->next)
        {
            GUDisksVolume* vol = G_UDISKS_VOLUME(l->data);
            if(vol->drive == drv)
            {
                vol->drive = NULL;
                /* FIXME: emit sigal for volume change */
            }
        }
        g_object_unref(drv);
    }
}

static void remove_volume(GUDisksVolumeMonitor* mon, GUDisksDevice* dev)
{
    GList* l = find_volume_l(mon, dev);
    if(l)
    {
        GUDisksVolume* vol = G_UDISKS_VOLUME(l->data);
        mon->volumes = g_list_delete_link(mon->volumes, l);
        g_signal_emit(mon, sig_volume_removed, 0, vol);
        g_udisks_volume_removed(vol);
        vol->mon = NULL;
        vol->drive = NULL;
        g_object_unref(vol);
    }
}

static void _g_udisks_device_added(GUDisksDevice* dev, GUDisksVolumeMonitor* mon, gboolean emit_signal)
{
    /* FIXME: how should we treat sys internal devices?
     * make this optional */
    if(!dev->is_hidden && (!dev->is_sys_internal || fm_config->show_internal_volumes) )
    {
        if(dev->is_drive)
            add_drive(mon, dev, emit_signal);

        if(g_udisks_device_is_volume(dev))
            add_volume(mon, dev, emit_signal);
    }
}

static void g_udisks_device_added(GUDisksDevice* dev, GUDisksVolumeMonitor* mon)
{
    g_debug("g_udisks_device_added");
    _g_udisks_device_added(dev, mon, TRUE);
}

static void g_udisks_device_changed(GUDisksDevice* dev, GUDisksVolumeMonitor* mon)
{
    GUDisksDrive* drv = find_drive(mon, dev);
    GUDisksVolume* vol = find_volume(mon, dev);
    /*
    gboolean is_drive = dev->is_drive;
    char* usage = g_strdup(dev->usage);
    */
    g_debug("g_udisks_device_changed");
    if(drv)
    {
        g_signal_emit(mon, sig_drive_changed, 0, drv);
        g_udisks_drive_changed(drv);
        /* it's no longer a drive */
        if(!dev->is_drive)
            remove_drive(mon, dev);
    }
    else
    {
        if(dev->is_drive)
            add_drive(mon, dev, TRUE);
    }

    if(vol)
    {
        update_volume_drive(vol, mon);
        g_signal_emit(mon, sig_volume_changed, 0, vol);
        g_udisks_volume_changed(vol);

        /* it's no longer a volume */
        if(!g_udisks_device_is_volume(dev))
            remove_volume(mon, dev);
    }
    else
    {
        /* we got a usable volume now */
        if(g_udisks_device_is_volume(dev))
            add_volume(mon, dev, TRUE);
    }
}

static void g_udisks_device_removed(GUDisksDevice* dev, GUDisksVolumeMonitor* mon)
{
    g_debug("g_udisks_device_removed");

    if(dev->is_drive)
        remove_drive(mon, dev);

    remove_volume(mon, dev);
}

static gboolean on_idle(GUDisksVolumeMonitor* mon)
{
    GList* l;
    g_debug("on_idle: %d", g_list_length(mon->queued_events));
    for(l = mon->queued_events; l; l=l->next)
    {
        QueuedEvent* q = (QueuedEvent*)l->data;
        q->func(q->dev, mon);
        g_object_unref(q->dev);
        g_slice_free(QueuedEvent, q);
    }
    g_list_free(mon->queued_events);
    mon->queued_events = NULL;
    mon->idle_handler = 0;
    return FALSE;
}

static void g_udisks_device_queue_event(GUDisksDevice* dev, GUDisksDeviceEventHandler func, GUDisksVolumeMonitor* mon)
{
    QueuedEvent* q = g_slice_new(QueuedEvent);
    q->dev = g_object_ref(dev);
    q->func = func;
    mon->queued_events = g_list_append(mon->queued_events, q);

    if(0 == mon->idle_handler)
        mon->idle_handler = g_idle_add_full(G_PRIORITY_HIGH, (GSourceFunc)on_idle, mon, NULL);
}

static void add_device(GUDisksVolumeMonitor* mon, DBusGProxy* proxy, const char* obj_path, gboolean emit_signal)
{
    if(!find_device(mon, obj_path))
    {
        DBusGProxy *dev_proxy = dbus_g_proxy_new_for_name(mon->con,
                                            "org.freedesktop.UDisks",
                                            obj_path,
                                            "org.freedesktop.DBus.Properties");
        GError* err = NULL;
        GHashTable* props = dbus_get_all_props(dev_proxy, "org.freedesktop.UDisks.Device", &err);
        if(props)
        {
            GUDisksDevice* dev = g_udisks_device_new(obj_path, props);
            g_hash_table_destroy(props);

            mon->devices = g_list_prepend(mon->devices, dev);
            if(emit_signal)
                g_udisks_device_queue_event(dev, g_udisks_device_added, mon);
            else
                _g_udisks_device_added(dev, mon, FALSE);
        }
        else
        {
            g_debug("%s", err->message);
            g_error_free(err);
        }
        g_object_unref(dev_proxy);
    }
    g_debug("device_added: %s", obj_path);
}

void on_device_added(DBusGProxy* proxy, const char* obj_path, gpointer user_data)
{
    GUDisksVolumeMonitor* mon = G_UDISKS_VOLUME_MONITOR(user_data);
    add_device(mon, proxy, obj_path, TRUE);
}


void on_device_removed(DBusGProxy* proxy, const char* obj_path, gpointer user_data)
{
    GUDisksVolumeMonitor* mon = G_UDISKS_VOLUME_MONITOR(user_data);
    GList* l;
    l = find_device_l(mon, obj_path);
    if(l)
    {
        GUDisksDevice* dev = G_UDISKS_DEVICE(l->data);
        mon->devices = g_list_delete_link(mon->devices, l);

        g_udisks_device_queue_event(dev, g_udisks_device_removed, mon);
        g_object_unref(dev);
    }
    g_debug("device_removed: %s", obj_path);

}

void on_device_changed(DBusGProxy* proxy, const char* obj_path, gpointer user_data)
{
    GUDisksVolumeMonitor* mon = G_UDISKS_VOLUME_MONITOR(user_data);
    GUDisksDevice* dev = find_device(mon, obj_path);
    if(dev)
    {
        DBusGProxy *dev_proxy = dbus_g_proxy_new_for_name(mon->con,
                                            "org.freedesktop.UDisks",
                                            obj_path,
                                            "org.freedesktop.DBus.Properties");

        GError* err = NULL;
        GHashTable* props = dbus_get_all_props(dev_proxy, "org.freedesktop.UDisks.Device", &err);
        if(props)
        {
            g_udisks_device_update(dev, props);
            g_hash_table_destroy(props);

            g_udisks_device_queue_event(dev, g_udisks_device_changed, mon);
        }
        g_object_unref(dev_proxy);
    }
    g_debug("device_changed: %s", obj_path);
}

