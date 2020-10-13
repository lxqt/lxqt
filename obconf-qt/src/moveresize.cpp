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

#define POPUP_NONPIXEL 0
#define POPUP_ALWAYS   1
#define POPUP_NEVER    2

#define POSITION_CENTER 0
#define POSITION_TOP    1
#define POSITION_FIXED  2

#define EDGE_CENTER 0
#define EDGE_LEFT   1
#define EDGE_RIGHT  2

void MainDialog::moveresize_setup_tab() {
  gchar* s;
  gint pos, i;
  gboolean opp;

  ui.resize_contents->setChecked(tree_get_bool("resize/drawContents", TRUE));
  ui.resist_window->setValue(tree_get_int("resistance/strength", 10));
  ui.resist_edge->setValue(tree_get_int("resistance/screen_edge_strength", 20));

  s = tree_get_string("resize/popupShow", "NonPixel");

  if(!strcasecmp(s, "Always"))     pos = POPUP_ALWAYS;
  else if(!strcasecmp(s, "Never")) pos = POPUP_NEVER;
  else                              pos = POPUP_NONPIXEL;

  g_free(s);

  ui.resize_popup->setCurrentIndex(pos);
  ui.drag_threshold->setValue(tree_get_int("mouse/dragThreshold", 8));
  s = tree_get_string("resize/popupPosition", "Center");

  if(!strcasecmp(s, "Top"))   pos = POSITION_TOP;

  if(!strcasecmp(s, "Fixed")) pos = POSITION_FIXED;
  else                         pos = POSITION_CENTER;

  g_free(s);
  ui.resize_position->setCurrentIndex(pos);

  s = tree_get_string("resize/popupFixedPosition/x", "0");
  char* fixed_pos = s;
  opp = fixed_pos[0] == '-';

  if(fixed_pos[0] == '-' || fixed_pos[0] == '+')
    ++fixed_pos;

  if(!strcasecmp(fixed_pos, "Center")) pos = EDGE_CENTER;
  else if(opp) pos = EDGE_RIGHT;
  else pos = EDGE_LEFT;

  ui.fixed_x_popup->setCurrentIndex(pos);
  ui.fixed_x_pos->setValue(MAX(atoi(fixed_pos), 0));
  g_free(s);
  
  s = tree_get_string("resize/popupFixedPosition/y", "0");
  opp = s[0] == '-';

  if(!strcasecmp(s, "Center")) pos = EDGE_CENTER;
  else if(opp) pos = EDGE_RIGHT;
  else pos = EDGE_LEFT;

  ui.fixed_y_popup->setCurrentIndex(pos);
  ui.fixed_y_pos->setValue(MAX(atoi(s), 0));
    
  g_free(s);

  i = tree_get_int("mouse/screenEdgeWarpTime", 400);

  ui.warp_edge->setChecked(i != 0);
  ui.warp_edge_time->setValue(i ? i : 400);

  moveresize_enable_stuff();
}

void MainDialog::moveresize_enable_stuff() {
  bool enabled;

  enabled = (ui.resize_popup->currentIndex() != POPUP_NEVER);
  ui.resize_position->setEnabled(enabled);

  enabled = ui.warp_edge->isChecked();
  ui.warp_edge_time->setEnabled(enabled);

  enabled = (ui.resize_position->currentIndex() == POSITION_FIXED);
  ui.fixed_x_popup->setEnabled(enabled);
  ui.fixed_y_popup->setEnabled(enabled);

  if(!enabled) {
    ui.fixed_x_pos->setEnabled(false);
    ui.fixed_y_pos->setEnabled(false);
  }
  else {
    enabled = (ui.fixed_x_popup->currentIndex() != EDGE_CENTER);
    ui.fixed_x_pos->setEnabled(enabled);

    enabled = (ui.fixed_y_popup->currentIndex() != EDGE_CENTER);
    ui.fixed_y_pos->setEnabled(enabled);
  }
}

void MainDialog::on_resist_window_valueChanged(int newValue) {
  tree_set_int("resistance/strength", newValue);
}

void MainDialog::on_resist_edge_valueChanged(int newValue) {
  tree_set_int("resistance/screen_edge_strength", newValue);
}

void MainDialog::on_resize_contents_toggled(bool checked) {
  tree_set_bool("resize/drawContents", checked);
}

void MainDialog::on_resize_popup_currentIndexChanged(int index) {
  switch(index) {
    case POPUP_NONPIXEL:
      tree_set_string("resize/popupShow", "NonPixel");
      break;
    case POPUP_ALWAYS:
      tree_set_string("resize/popupShow", "Always");
      break;
    case POPUP_NEVER:
      tree_set_string("resize/popupShow", "Never");
      break;
  }

  moveresize_enable_stuff();
}

void MainDialog::on_drag_threshold_valueChanged(int newValue) {
  tree_set_int("mouse/dragThreshold", newValue);
}

void MainDialog::on_resize_position_currentIndexChanged(int index) {
  /*
  #define POSITION_CENTER 0
  #define POSITION_TOP    1
  #define POSITION_FIXED  2
  */
  const char* strs[] = {
    "Center",
    "Top",
    "Fixed"
  };

  if(index >= 0 && index < G_N_ELEMENTS(strs)) {
    tree_set_string("resize/popupPosition", strs[index]);
    moveresize_enable_stuff();
  }
}

void MainDialog::on_fixed_x_popup_currentIndexChanged(int index) {
  write_fixed_position("x");
  moveresize_enable_stuff();
}

void MainDialog::on_fixed_y_popup_currentIndexChanged(int index) {
  write_fixed_position("y");
  moveresize_enable_stuff();
}


void MainDialog::write_fixed_position(const char* coord) {
  g_assert(!strcmp(coord, "x") || !strcmp(coord, "y"));
  QComboBox* popup = (*coord == 'x' ? ui.fixed_x_popup : ui.fixed_y_popup);

  int edge = popup->currentIndex();
  g_assert(edge == EDGE_CENTER || edge == EDGE_LEFT || edge == EDGE_RIGHT);

  char* val;

  if(edge == EDGE_CENTER)
    val = g_strdup("center");
  else {
    QSpinBox* spin = (*coord == 'x' ? ui.fixed_x_pos : ui.fixed_y_pos) ;
    int i = spin->value();

    if(edge == EDGE_LEFT)
      val = g_strdup_printf("%d", i);
    else
      val = g_strdup_printf("-%d", i);
  }

  char* valname = g_strdup_printf("resize/popupFixedPosition/%s", coord);
  tree_set_string(valname, val);
  g_free(valname);
  g_free(val);
}

void MainDialog::on_fixed_x_pos_valueChanged(int newValue) {
  write_fixed_position("x");
}

void MainDialog::on_fixed_y_pos_valueChanged(int newValue) {
  write_fixed_position("y");
}

void MainDialog::on_warp_edge_toggled(bool checked) {
  if(checked) {
    tree_set_int("mouse/screenEdgeWarpTime", ui.warp_edge_time->value());
  }
  else
    tree_set_int("mouse/screenEdgeWarpTime", 0);

  moveresize_enable_stuff();
}

void MainDialog::on_warp_edge_time_valueChanged(int newValue) {
  tree_set_int("mouse/screenEdgeWarpTime", newValue);
}
