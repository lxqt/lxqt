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


#include "applaunchcontext.h"
#include <QX11Info>
#include <X11/Xlib.h>

typedef struct _FmAppLaunchContext {
  GAppLaunchContext parent;
}FmAppLaunchContext;

G_DEFINE_TYPE(FmAppLaunchContext, fm_app_launch_context, G_TYPE_APP_LAUNCH_CONTEXT)

static char* fm_app_launch_context_get_display(GAppLaunchContext * /*context*/, GAppInfo * /*info*/, GList * /*files*/) {
  Display* dpy = QX11Info::display();
  if(dpy) {
    char* xstr = DisplayString(dpy);
    return g_strdup(xstr);
  }
  return nullptr;
}

static char* fm_app_launch_context_get_startup_notify_id(GAppLaunchContext * /*context*/, GAppInfo * /*info*/, GList * /*files*/) {
  return nullptr;
}

static void fm_app_launch_context_class_init(FmAppLaunchContextClass* klass) {
  GAppLaunchContextClass* app_launch_class = G_APP_LAUNCH_CONTEXT_CLASS(klass);
  app_launch_class->get_display = fm_app_launch_context_get_display;
  app_launch_class->get_startup_notify_id = fm_app_launch_context_get_startup_notify_id;
}

static void fm_app_launch_context_init(FmAppLaunchContext* /*context*/) {
}

FmAppLaunchContext* fm_app_launch_context_new_for_widget(QWidget* /*widget*/) {
  FmAppLaunchContext* context = (FmAppLaunchContext*)g_object_new(FM_TYPE_APP_LAUNCH_CONTEXT, nullptr);
  return context;
}

FmAppLaunchContext* fm_app_launch_context_new() {
  FmAppLaunchContext* context = (FmAppLaunchContext*)g_object_new(FM_TYPE_APP_LAUNCH_CONTEXT, nullptr);
  return context;
}
