/*
 *      fm-udisks.c
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


#include "fm-udisks.h"
#include "g-udisks-volume-monitor.h"

gboolean _fm_udisks_init()
{
    /* glib < 2.23.2 has errors if an extension poinbt is already registered */
#if !GLIB_CHECK_VERSION(2, 23, 2)
    if(!g_io_extension_point_lookup(G_NATIVE_VOLUME_MONITOR_EXTENSION_POINT_NAME))
#endif
    {
        g_io_extension_point_register(G_NATIVE_VOLUME_MONITOR_EXTENSION_POINT_NAME);
    }

    /* register our own volume monitor to override the one provided in gvfs. */
    g_io_extension_point_implement(G_NATIVE_VOLUME_MONITOR_EXTENSION_POINT_NAME,
        G_UDISKS_VOLUME_MONITOR_TYPE,
        "udisks-monitor", 2 /* the gdu monitor provided by gvfs uses priority 3 */
    );
    return TRUE;
}

void _fm_udisks_finalize()
{
    g_io_scheduler_cancel_all_jobs();
}
