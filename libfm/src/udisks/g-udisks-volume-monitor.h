/*
 *      g-udisks-volume-monitor.h
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


#ifndef __G_UDISKS_VOLUME_MONITOR_H__
#define __G_UDISKS_VOLUME_MONITOR_H__

#include <gio/gio.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define G_UDISKS_VOLUME_MONITOR_TYPE                (g_udisks_volume_monitor_get_type())
#define G_UDISKS_VOLUME_MONITOR(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            G_UDISKS_VOLUME_MONITOR_TYPE, GUDisksVolumeMonitor))
#define G_UDISKS_VOLUME_MONITOR_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST((klass),\
            G_UDISKS_VOLUME_MONITOR_TYPE, GUDisksVolumeMonitorClass))
#define G_IS_UDISKS_VOLUME_MONITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            G_UDISKS_VOLUME_MONITOR_TYPE))
#define G_IS_UDISKS_VOLUME_MONITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),\
            G_UDISKS_VOLUME_MONITOR_TYPE))

typedef struct _GUDisksVolumeMonitor            GUDisksVolumeMonitor;
typedef struct _GUDisksVolumeMonitorClass       GUDisksVolumeMonitorClass;

struct _GUDisksVolumeMonitor
{
    GNativeVolumeMonitor parent;
    DBusGConnection* con;
    DBusGProxy* udisks_proxy;

    GList* devices;
    GList* drives;
    GList* volumes;
    guint idle_handler;
    GList* queued_events;
};

struct _GUDisksVolumeMonitorClass
{
    GNativeVolumeMonitorClass parent_class;
};

GType       g_udisks_volume_monitor_get_type        (void);
GNativeVolumeMonitor*   g_udisks_volume_monitor_new         (void);

G_END_DECLS

#endif /* __G_UDISKS_VOLUME_MONITOR_H__ */
