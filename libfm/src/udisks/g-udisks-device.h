//      g-udisks-device.h
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


#ifndef __G_UDISKS_DEVICE_H__
#define __G_UDISKS_DEVICE_H__

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS


#define G_TYPE_UDISKS_DEVICE                (g_udisks_device_get_type())
#define G_UDISKS_DEVICE(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            G_TYPE_UDISKS_DEVICE, GUDisksDevice))
#define G_UDISKS_DEVICE_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST((klass),\
            G_TYPE_UDISKS_DEVICE, GUDisksDeviceClass))
#define G_IS_UDISKS_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            G_TYPE_UDISKS_DEVICE))
#define G_IS_UDISKS_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),\
            G_TYPE_UDISKS_DEVICE))
#define G_UDISKS_DEVICE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj),\
            G_TYPE_UDISKS_DEVICE, GUDisksDeviceClass))

typedef struct _GUDisksDevice            GUDisksDevice;
typedef struct _GUDisksDeviceClass        GUDisksDeviceClass;
typedef struct _GUDisksDevicePrivate        GUDisksDevicePrivate;

struct _GUDisksDevice
{
    GObject parent;
    char* obj_path; /* dbus object path */

    char* dev_file;
    char* dev_file_presentation;
    gboolean is_sys_internal : 1;
    gboolean is_removable : 1;
    gboolean is_read_only : 1;
    gboolean is_drive : 1;
    gboolean is_optic_disc : 1;
    gboolean is_mounted : 1;
    gboolean is_media_available : 1;
    gboolean is_media_change_notification_polling : 1;
    gboolean is_luks : 1;
    gboolean is_luks_clear_text : 1;
    gboolean is_linux_md_component : 1;
    gboolean is_linux_md : 1;
    gboolean is_linux_lvm2lv : 1;
    gboolean is_linux_lvm2pv : 1;
    gboolean is_linux_dmmp_component : 1;
    gboolean is_linux_dmmp : 1;
    gboolean is_ejectable : 1;
    gboolean is_disc_blank : 1;
    gboolean is_hidden : 1;
    gboolean auto_mount : 1;

    guint mounted_by_uid;
    char** mount_paths;
    guint64 dev_size;
    guint64 partition_size;

    guint num_audio_tracks;
    guint luks_unlocked_by_uid;

    char* name;
    char* icon_name;

    char* usage;
    char* type;
    char* uuid;
    char* label;
    char* vender;
    char* model;
    char* conn_iface;
    char* media;
    char* partition_slave;
};

struct _GUDisksDeviceClass
{
    GObjectClass parent_class;
};


GType g_udisks_device_get_type (void);
GUDisksDevice* g_udisks_device_new (const char* obj_path, GHashTable* props);

void g_udisks_device_update(GUDisksDevice* dev, GHashTable* props);

DBusGProxy* g_udisks_device_get_proxy(GUDisksDevice* dev, DBusGConnection* con);

const char* g_udisks_device_get_icon_name(GUDisksDevice* dev);

/* this is only valid if the device contains a optic disc */
const char* g_udisks_device_get_disc_name(GUDisksDevice* dev);

gboolean g_udisks_device_is_volume(GUDisksDevice* dev);

G_END_DECLS

#endif /* __G_UDISKS_DEVICE_H__ */
