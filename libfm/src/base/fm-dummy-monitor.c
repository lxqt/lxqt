/*
 *      fm-dummy-monitor.c
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

/**
 * SECTION:fm-dummy-monitor
 * @short_description: Replacement of #GFileMonitor for virtual filesystems.
 * @title: FmDummyMonitor
 *
 * @include: libfm/fm.h
 *
 * The #FmDummyMonitor represents dummy #GFileMonitor variant which does
 * not monitor any files but can be used as monitor object.
 */

#include "fm-dummy-monitor.h"

//static void fm_dummy_monitor_finalize           (GObject *object);

G_DEFINE_TYPE(FmDummyMonitor, fm_dummy_monitor, G_TYPE_FILE_MONITOR);

static gboolean cancel()
{
    return TRUE;
}

static void fm_dummy_monitor_class_init(FmDummyMonitorClass *klass)
{
    GFileMonitorClass* fm_class = G_FILE_MONITOR_CLASS(klass);
    fm_class->cancel = cancel;
/*
    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->finalize = fm_dummy_monitor_finalize;
*/
}

/*
static void fm_dummy_monitor_finalize(GObject *object)
{
    FmDummyMonitor *self;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_DUMMY_MONITOR(object));

    self = FM_DUMMY_MONITOR(object);

    G_OBJECT_CLASS(fm_dummy_monitor_parent_class)->finalize(object);
}
*/

static void fm_dummy_monitor_init(FmDummyMonitor *self)
{

}

/**
 * fm_dummy_monitor_new
 *
 * Creates a new dummy #GFileMonitor.
 *
 * Returns: a new dummy #GFileMonitor object.
 */
GFileMonitor *fm_dummy_monitor_new(void)
{
    return (GFileMonitor*)g_object_new(FM_TYPE_DUMMY_MONITOR, NULL);
}

