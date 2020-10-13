/*
 *      fm-file-monitor.h
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *
 *      This file is a part of the Libfm library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __FM_FILE_MONITOR_H__
#define __FM_FILE_MONITOR_H__

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

GFileMonitor* fm_monitor_directory(GFile* gf, GError** err);

void _fm_monitor_init();
void _fm_monitor_finalize();

GFileMonitor* fm_monitor_lookup_monitor(GFile* gf);
GFileMonitor* fm_monitor_lookup_dummy_monitor(GFile* gf);

G_END_DECLS

#endif /* __FM_FILE_MONITOR_H__ */
