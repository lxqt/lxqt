/*
 * Copyright (C) 2012 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#ifndef FM_APP_LAUNCHCONTEXT_H
#define FM_APP_LAUNCHCONTEXT_H

#include "libfmqtglobals.h"
#include <gio/gio.h>
#include <QWidget>

#define FM_TYPE_APP_LAUNCH_CONTEXT				(fm_app_launch_context_get_type())
#define FM_APP_LAUNCH_CONTEXT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			FM_TYPE_APP_LAUNCH_CONTEXT, FmAppLaunchContext))
#define FM_APP_LAUNCH_CONTEXT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			FM_TYPE_APP_LAUNCH_CONTEXT, FmAppLaunchContextClass))
#define FM_IS_APP_LAUNCH_CONTEXT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			FM_TYPE_APP_LAUNCH_CONTEXT))
#define FM_IS_APP_LAUNCH_CONTEXT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			FM_TYPE_APP_LAUNCH_CONTEXT))
#define FM_APP_LAUNCH_CONTEXT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj),\
			FM_TYPE_APP_LAUNCH_CONTEXT, FmAppLaunchContextClass))

typedef struct _FmAppLaunchContext FmAppLaunchContext;

typedef struct _FmAppLaunchContextClass {
  GAppLaunchContextClass parent;
}FmAppLaunchContextClass;

FmAppLaunchContext* fm_app_launch_context_new();
FmAppLaunchContext* fm_app_launch_context_new_for_widget(QWidget* widget);
GType fm_app_launch_context_get_type();

#endif // FM_APPLAUNCHCONTEXT_H
