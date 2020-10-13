/*
    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    Part of the code in this file is taken from obconf:
    Copyright (c) 2003-2007   Dana Jansens
    Copyright (c) 2003        Tim Riley

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "maindialog.h"
#include <obrender/render.h>
#include "tree.h"

#include <QX11Info>
// FIXME: how to support XCB or Wayland?
#include <X11/Xlib.h>

using namespace Obconf;

extern RrInstance* rrinst; // defined in obconf-qt.cpp

#define PLACE_ON_FIXED 0
#define PLACE_ON_ALL 0
#define PLACE_ON_ACTIVE 1
#define PLACE_ON_MOUSE 2
#define PLACE_ON_PRIMARY 3

void MainDialog::windows_setup_tab() {
  gchar* s;
  ui.focus_new->setChecked(tree_get_bool("focus/focusNew", TRUE));

  s = tree_get_string("placement/policy", "Smart");
  ui.place_mouse->setChecked(!g_ascii_strcasecmp(s, "UnderMouse"));
  g_free(s);

  int index;
  s = tree_get_string("placement/monitor", "Any");

  if(!g_ascii_strcasecmp(s, "Active"))
    index = PLACE_ON_ACTIVE;
  else if(!g_ascii_strcasecmp(s, "Mouse"))
    index = PLACE_ON_MOUSE;
  else if(!g_ascii_strcasecmp(s, "Primary"))
    index = PLACE_ON_PRIMARY;
  else
    index = PLACE_ON_ALL;

  g_free(s);
  ui.place_active_popup->setCurrentIndex(index);

  s = tree_get_string("placement/primaryMonitor", "");
  if(!g_ascii_strcasecmp(s, "Active"))
    index = PLACE_ON_ACTIVE;
  else if(!g_ascii_strcasecmp(s, "Mouse"))
    index = PLACE_ON_MOUSE;
  else {
    index = PLACE_ON_FIXED;
    ui.fixed_monitor->setValue(tree_get_int("placement/primaryMonitor", 1));
  }
  g_free(s);
  ui.primary_monitor_popup->setCurrentIndex(index);

  windows_enable_stuff();
}

void MainDialog::windows_enable_stuff() {
  bool enabled = (ui.primary_monitor_popup->currentIndex() == PLACE_ON_FIXED);
  ui.fixed_monitor->setEnabled(enabled);
}

void MainDialog::on_primary_monitor_popup_currentIndexChanged(int index) {
  /*
  #define PLACE_ON_FIXED 0
  #define PLACE_ON_ACTIVE 1
  #define PLACE_ON_MOUSE 2
  #define PLACE_ON_PRIMARY 3
  */
  const char* strs[] = {
    NULL,
    "Active",
    "Mouse"
  };
  if(index < G_N_ELEMENTS(strs) && index >= 0) {
    if(index == PLACE_ON_FIXED) {
      tree_set_int("placement/primaryMonitor", ui.fixed_monitor->value());
    }
    else {
      tree_set_string("placement/primaryMonitor", strs[index]);
    }
    windows_enable_stuff();
  }
}

void MainDialog::on_fixed_monitor_valueChanged(int newValue) {
  tree_set_int("placement/primaryMonitor", newValue);
}

void MainDialog::on_focus_new_toggled(bool checked) {
  tree_set_bool("focus/focusNew", checked);
}

void MainDialog::on_place_mouse_toggled(bool checked) {
  tree_set_string("placement/policy",
                  (checked ?
                  "UnderMouse" : "Smart"));
  windows_enable_stuff();
}

void MainDialog::on_place_active_popup_currentIndexChanged(int index) {
  /*
   *  #define PLACE_ON_ALL 0
   *  #define PLACE_ON_ACTIVE 1
   *  #define PLACE_ON_MOUSE 2
   *  #define PLACE_ON_PRIMARY 3
   */
  const char* strs[] = {
    "Any",
    "Active",
    "Mouse",
    "Primary"
  };
  if(index < G_N_ELEMENTS(strs) && index >= 0)
    tree_set_string("placement/monitor", strs[index]);
}
