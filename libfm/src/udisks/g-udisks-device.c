//      g-udisks-device.c
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

#include "g-udisks-device.h"
#include "dbus-utils.h"
#include "udisks-device.h"
#include <string.h>
#include <glib/gi18n-lib.h>

/* This array is taken from gnome-disk-utility: gdu-volume.c
 * Copyright (C) 2007 David Zeuthen, licensed under GNU LGPL
 */
static const struct
{
        const char *disc_type;
        const char *icon_name;
        const char *ui_name;
        const char *ui_name_blank;
} disc_data[] = {
  /* Translator: The word "blank" is used as an adjective, e.g. we are decsribing discs that are already blank */
  {"optical_cd",             "media-optical-cd-rom",        N_("CD-ROM Disc"),     N_("Blank CD-ROM Disc")},
  {"optical_cd_r",           "media-optical-cd-r",          N_("CD-R Disc"),       N_("Blank CD-R Disc")},
  {"optical_cd_rw",          "media-optical-cd-rw",         N_("CD-RW Disc"),      N_("Blank CD-RW Disc")},
  {"optical_dvd",            "media-optical-dvd-rom",       N_("DVD-ROM Disc"),    N_("Blank DVD-ROM Disc")},
  {"optical_dvd_r",          "media-optical-dvd-r",         N_("DVD-ROM Disc"),    N_("Blank DVD-ROM Disc")},
  {"optical_dvd_rw",         "media-optical-dvd-rw",        N_("DVD-RW Disc"),     N_("Blank DVD-RW Disc")},
  {"optical_dvd_ram",        "media-optical-dvd-ram",       N_("DVD-RAM Disc"),    N_("Blank DVD-RAM Disc")},
  {"optical_dvd_plus_r",     "media-optical-dvd-r-plus",    N_("DVD+R Disc"),      N_("Blank DVD+R Disc")},
  {"optical_dvd_plus_rw",    "media-optical-dvd-rw-plus",   N_("DVD+RW Disc"),     N_("Blank DVD+RW Disc")},
  {"optical_dvd_plus_r_dl",  "media-optical-dvd-dl-r-plus", N_("DVD+R DL Disc"),   N_("Blank DVD+R DL Disc")},
  {"optical_dvd_plus_rw_dl", "media-optical-dvd-dl-r-plus", N_("DVD+RW DL Disc"),  N_("Blank DVD+RW DL Disc")},
  {"optical_bd",             "media-optical-bd-rom",        N_("Blu-Ray Disc"),    N_("Blank Blu-Ray Disc")},
  {"optical_bd_r",           "media-optical-bd-r",          N_("Blu-Ray R Disc"),  N_("Blank Blu-Ray R Disc")},
  {"optical_bd_re",          "media-optical-bd-re",         N_("Blu-Ray RW Disc"), N_("Blank Blu-Ray RW Disc")},
  {"optical_hddvd",          "media-optical-hddvd-rom",     N_("HD DVD Disc"),     N_("Blank HD DVD Disc")},
  {"optical_hddvd_r",        "media-optical-hddvd-r",       N_("HD DVD-R Disc"),   N_("Blank HD DVD-R Disc")},
  {"optical_hddvd_rw",       "media-optical-hddvd-rw",      N_("HD DVD-RW Disc"),  N_("Blank HD DVD-RW Disc")},
  {"optical_mo",             "media-optical-mo",            N_("MO Disc"),         N_("Blank MO Disc")},
  {"optical_mrw",            "media-optical-mrw",           N_("MRW Disc"),        N_("Blank MRW Disc")},
  {"optical_mrw_w",          "media-optical-mrw-w",         N_("MRW/W Disc"),      N_("Blank MRW/W Disc")},
  {NULL, NULL, NULL, NULL}
};

static void g_udisks_device_finalize            (GObject *object);

G_DEFINE_TYPE(GUDisksDevice, g_udisks_device, G_TYPE_OBJECT)


static void g_udisks_device_class_init(GUDisksDeviceClass *klass)
{
    GObjectClass *g_object_class;

    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->finalize = g_udisks_device_finalize;
}

static void clear_props(GUDisksDevice* dev)
{
    g_free(dev->dev_file);
    g_free(dev->dev_file_presentation);
    g_free(dev->name);
    g_free(dev->icon_name);
    g_free(dev->usage);
    g_free(dev->type);
    g_free(dev->uuid);
    g_free(dev->label);
    g_free(dev->vender);
    g_free(dev->model);
    g_free(dev->conn_iface);
    g_free(dev->media);
    g_free(dev->partition_slave);

    g_strfreev(dev->mount_paths);
}

static void set_props(GUDisksDevice* dev, GHashTable* props)
{
    dev->dev_file = dbus_prop_dup_str(props, "DeviceFile");
    dev->dev_file_presentation = dbus_prop_dup_str(props, "DeviceFilePresentation");
    dev->is_sys_internal = dbus_prop_bool(props, "DeviceIsSystemInternal");
    dev->is_removable = dbus_prop_bool(props, "DeviceIsRemovable");
    dev->is_read_only = dbus_prop_bool(props, "DeviceIsReadOnly");
    dev->is_drive = dbus_prop_bool(props, "DeviceIsDrive");
    dev->is_optic_disc = dbus_prop_bool(props, "DeviceIsOpticalDisc");
    dev->is_mounted = dbus_prop_bool(props, "DeviceIsMounted");
    dev->is_media_available = dbus_prop_bool(props, "DeviceIsMediaAvailable");
    dev->is_media_change_notification_polling = dbus_prop_bool(props, "DeviceIsMediaChangeDetectionPolling");
    dev->is_luks = dbus_prop_bool(props, "DeviceIsLuks");
    dev->is_luks_clear_text = dbus_prop_bool(props, "DeviceIsLuksCleartext");
    dev->is_linux_md_component = dbus_prop_bool(props, "DeviceIsLinuxMdComponent");
    dev->is_linux_md = dbus_prop_bool(props, "DeviceIsLinuxMd");
    dev->is_linux_lvm2lv = dbus_prop_bool(props, "DeviceIsLinuxLvm2LV");
    dev->is_linux_lvm2pv = dbus_prop_bool(props, "DeviceIsLinuxLvm2PV");
    dev->is_linux_dmmp_component = dbus_prop_bool(props, "DeviceIsLinuxDmmpComponent");
    dev->is_linux_dmmp = dbus_prop_bool(props, "DeviceIsLinuxDmmp");

    dev->is_ejectable = dbus_prop_bool(props, "DriveIsMediaEjectable");
    dev->is_disc_blank = dbus_prop_bool(props, "OpticalDiscIsBlank");

    dev->is_hidden = dbus_prop_bool(props, "DevicePresentationHide");
    dev->auto_mount = !dbus_prop_bool(props, "DevicePresentationNopolicy");

    dev->mounted_by_uid = dbus_prop_uint(props, "DeviceMountedByUid");
    dev->mount_paths = dbus_prop_dup_strv(props, "DeviceMountPaths");

    dev->dev_size = dbus_prop_uint64(props, "DeviceSize");
    dev->partition_size = dbus_prop_uint64(props, "PartitionSize");

    dev->luks_unlocked_by_uid = dbus_prop_uint(props, "LuksCleartextUnlockedByUid");
    dev->num_audio_tracks = dbus_prop_uint(props, "OpticalDiscNumAudioTracks");

    dev->name = dbus_prop_dup_str(props, "DevicePresentationName");
    dev->icon_name = dbus_prop_dup_str(props, "DevicePresentationIconName");

    dev->usage = dbus_prop_dup_str(props, "IdUsage");
    dev->type = dbus_prop_dup_str(props, "IdType");
    dev->uuid = dbus_prop_dup_str(props, "IdUuid");
    dev->label = dbus_prop_dup_str(props, "IdLabel");
    dev->vender = dbus_prop_dup_str(props, "DriveVendor");
    dev->model = dbus_prop_dup_str(props, "DriveModel");
    dev->conn_iface = dbus_prop_dup_str(props, "DriveConnectionInterface");
    dev->media = dbus_prop_dup_str(props, "DriveMedia");

    dev->partition_slave = dbus_prop_dup_obj_path(props, "PartitionSlave");

    /* how to support LUKS? */
/*
    'LuksHolder'                              read      'o'
    'LuksCleartextSlave'                      read      'o'
*/

}

static void g_udisks_device_finalize(GObject *object)
{
    GUDisksDevice *self;

    g_return_if_fail(object != NULL);
    g_return_if_fail(G_IS_UDISKS_DEVICE(object));

    self = G_UDISKS_DEVICE(object);

    g_free(self->obj_path);
    clear_props(self);

    G_OBJECT_CLASS(g_udisks_device_parent_class)->finalize(object);
}


static void g_udisks_device_init(GUDisksDevice *self)
{
}


GUDisksDevice *g_udisks_device_new(const char* obj_path, GHashTable* props)
{
    GUDisksDevice* dev = (GUDisksDevice*)g_object_new(G_TYPE_UDISKS_DEVICE, NULL);
    dev->obj_path = g_strdup(obj_path);
    set_props(dev, props);
    return dev;
}

void g_udisks_device_update(GUDisksDevice* dev, GHashTable* props)
{
    clear_props(dev);
    set_props(dev, props);
}

DBusGProxy* g_udisks_device_get_proxy(GUDisksDevice* dev, DBusGConnection* con)
{
    DBusGProxy* proxy = dbus_g_proxy_new_for_name(con,
                            "org.freedesktop.UDisks",
                            dev->obj_path,
                            "org.freedesktop.UDisks.Device");
    return proxy;
}

const char* g_udisks_device_get_icon_name(GUDisksDevice* dev)
{
    const char* icon_name = NULL;
    if(dev->icon_name && *dev->icon_name)
        icon_name = dev->icon_name;
    else if(dev->media && *dev->media) /* by media type */
    {
        if(dev->is_optic_disc)
        {
            if(dev->num_audio_tracks > 0)
                icon_name = "media-optical-audio";
            else
            {
                guint i;
                icon_name = "media-optical";
                for( i = 0; i < G_N_ELEMENTS(disc_data); ++i)
                {
                    if(strcmp(dev->media, disc_data[i].disc_type) == 0)
                    {
                        if(dev->is_disc_blank)
                            icon_name = disc_data[i].icon_name;
                        break;
                    }
                }
            }
        }
        else
        {
            if(strcmp (dev->media, "flash_cf") == 0)
                icon_name = "media-flash-cf";
            else if(strcmp (dev->media, "flash_ms") == 0)
                icon_name = "media-flash-ms";
            else if(strcmp (dev->media, "flash_sm") == 0)
                icon_name = "media-flash-sm";
            else if(strcmp (dev->media, "flash_sd") == 0)
                icon_name = "media-flash-sd";
            else if(strcmp (dev->media, "flash_sdhc") == 0)
                icon_name = "media-flash-sd";
            else if(strcmp (dev->media, "flash_mmc") == 0)
                icon_name = "media-flash-sd";
            else if(strcmp (dev->media, "floppy") == 0)
                icon_name = "media-floppy";
            else if(strcmp (dev->media, "floppy_zip") == 0)
                icon_name = "media-floppy-zip";
            else if(strcmp (dev->media, "floppy_jaz") == 0)
                icon_name = "media-floppy-jaz";
            else if(g_str_has_prefix (dev->media, "flash"))
                icon_name = "media-flash";
        }
    }
    else if(dev->conn_iface && *dev->conn_iface) /* by connection interface */
    {
        if(g_str_has_prefix(dev->conn_iface, "ata"))
            icon_name = dev->is_removable ? "drive-removable-media-ata" : "drive-harddisk-ata";
        else if(g_str_has_prefix (dev->conn_iface, "scsi"))
            icon_name = dev->is_removable ? "drive-removable-media-scsi" : "drive-harddisk-scsi";
        else if(strcmp (dev->conn_iface, "usb") == 0)
            icon_name = dev->is_removable ? "drive-removable-media-usb" : "drive-harddisk-usb";
        else if (strcmp (dev->conn_iface, "firewire") == 0)
            icon_name = dev->is_removable ? "drive-removable-media-ieee1394" : "drive-harddisk-ieee1394";
    }

    if(!icon_name)
    {
        if(dev->is_removable)
            icon_name = "drive-removable-media";
        else
            icon_name = "drive-harddisk";
    }
    return icon_name;
}

const char* g_udisks_device_get_disc_name(GUDisksDevice* dev)
{
    const char* name = NULL;
    if(!dev->is_optic_disc)
        return NULL;
    if(dev->media && *dev->media)
    {
        if(dev->num_audio_tracks > 0 && g_str_has_prefix(dev->media, "optical_cd"))
            name = "Audio CD";
        else
        {
            guint i;
            for( i = 0; i < G_N_ELEMENTS(disc_data); ++i)
            {
                if(strcmp(dev->media, disc_data[i].disc_type) == 0)
                {
                    if(dev->is_disc_blank)
                        name = disc_data[i].ui_name_blank;
                    else
                        name = disc_data[i].ui_name;
                    break;
                }
            }
        }
    }

    if(!name)
    {
        if(dev->is_disc_blank)
            name = _("Blank Optical Disc");
        else
            name = _("Optical Disc");
    }
    return name;
}

gboolean g_udisks_device_is_volume(GUDisksDevice* dev)
{
    /* also treat blank optical discs as volumes here to be compatible with gvfs.
     * FIXME: this is useless unless we support burn:///
     * So, should we support this? Personally I think it's a bad idea. */
    return (g_strcmp0(dev->usage, "filesystem") == 0 || dev->is_disc_blank);
}

