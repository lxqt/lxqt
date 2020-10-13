/*
 *      fm-dummy-monitor.h
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


#ifndef __FM_DUMMY_MONITOR_H__
#define __FM_DUMMY_MONITOR_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define FM_TYPE_DUMMY_MONITOR				(fm_dummy_monitor_get_type())
#define FM_DUMMY_MONITOR(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			FM_TYPE_DUMMY_MONITOR, FmDummyMonitor))
#define FM_DUMMY_MONITOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			FM_TYPE_DUMMY_MONITOR, FmDummyMonitorClass))
#define FM_IS_DUMMY_MONITOR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			FM_TYPE_DUMMY_MONITOR))
#define FM_IS_DUMMY_MONITOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			FM_TYPE_DUMMY_MONITOR))

typedef struct _FmDummyMonitor			FmDummyMonitor;
typedef struct _FmDummyMonitorClass		FmDummyMonitorClass;

struct _FmDummyMonitor
{
	GFileMonitor parent;
};

struct _FmDummyMonitorClass
{
    /*< private >*/
	GFileMonitorClass parent_class;
};

GType fm_dummy_monitor_get_type(void);
GFileMonitor* fm_dummy_monitor_new(void);

G_END_DECLS

#endif /* __FM_DUMMY_MONITOR_H__ */
