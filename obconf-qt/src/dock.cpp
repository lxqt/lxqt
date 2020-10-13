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


#define POSITION_TOPLEFT     0
#define POSITION_TOP         1
#define POSITION_TOPRIGHT    2
#define POSITION_LEFT        3
#define POSITION_RIGHT       4
#define POSITION_BOTTOMLEFT  5
#define POSITION_BOTTOM      6
#define POSITION_BOTTOMRIGHT 7
#define POSITION_FLOATING    8

#define DIRECTION_VERTICAL   0
#define DIRECTION_HORIZONTAL 1

static void dock_enable_stuff();

void MainDialog::dock_setup_tab() {
  gchar* s;
  gint pos;

  s = tree_get_string("dock/position", "TopLeft");

  if(!strcasecmp(s, "Top"))              pos = POSITION_TOP;
  else if(!strcasecmp(s, "TopRight"))    pos = POSITION_TOPRIGHT;
  else if(!strcasecmp(s, "Left"))        pos = POSITION_LEFT;
  else if(!strcasecmp(s, "Right"))       pos = POSITION_RIGHT;
  else if(!strcasecmp(s, "BottomLeft"))  pos = POSITION_BOTTOMLEFT;
  else if(!strcasecmp(s, "Bottom"))      pos = POSITION_BOTTOM;
  else if(!strcasecmp(s, "BottomRight")) pos = POSITION_BOTTOMRIGHT;
  else if(!strcasecmp(s, "Floating"))    pos = POSITION_FLOATING;
  else                                    pos = POSITION_TOPLEFT;

  g_free(s);

  ui.dock_position->setCurrentIndex(pos);

  bool is_floating = (pos == POSITION_FLOATING);
  ui.dock_float_x->setEnabled(is_floating);
  ui.dock_float_y->setEnabled(is_floating);

  ui.dock_float_x->setValue(tree_get_int("dock/floatingX", 0));
  ui.dock_float_y->setValue(tree_get_int("dock/floatingY", 0));

  s = tree_get_string("dock/stacking", "Above");
  if(!strcasecmp(s, "Normal"))
    ui.dock_stack_normal->setChecked(true);
  else if(!strcasecmp(s, "Below"))
    ui.dock_stack_bottom->setChecked(true);
  else
    ui.dock_stack_top->setChecked(true);
  g_free(s);

  s = tree_get_string("dock/direction", "Vertical");

  if(!strcasecmp(s, "Horizontal")) pos = DIRECTION_HORIZONTAL;
  else                              pos = DIRECTION_VERTICAL;

  g_free(s);
  ui.dock_direction->setCurrentIndex(pos);

  ui.dock_nostrut->setChecked(tree_get_bool("dock/noStrut", FALSE));

  bool auto_hide = tree_get_bool("dock/autoHide", FALSE);
  ui.dock_hide->setChecked(auto_hide);

  ui.dock_hide_delay->setEnabled(auto_hide);
  ui.dock_hide_delay->setValue(tree_get_int("dock/hideDelay", 300));

  ui.dock_show_delay->setEnabled(auto_hide);
  ui.dock_show_delay->setValue(tree_get_int("dock/showDelay", 300));
}

void MainDialog::on_dock_position_currentIndexChanged(int index) {
  const char* val;
  bool is_floating = false;
  switch(index) {
    case POSITION_TOPLEFT:
    default:
      val = "TopLeft";
      break;
    case POSITION_TOP:
      val = "Top";
      break;
    case POSITION_TOPRIGHT:
      val = "TopRight";
      break;
    case POSITION_LEFT:
      val = "Left";
      break;
    case POSITION_RIGHT:
      val = "Right";
      break;
    case POSITION_BOTTOMLEFT:
      val = "BottomLeft";
      break;
    case POSITION_BOTTOM:
      val = "Bottom";
      break;
    case POSITION_BOTTOMRIGHT:
      val = "BottomRight";
      break;
    case POSITION_FLOATING:
      val = "Floating";
      is_floating = true;
      break;
  }
  tree_set_string("dock/position", val);

  ui.dock_float_x->setEnabled(is_floating);
  ui.dock_float_y->setEnabled(is_floating);
}

void MainDialog::on_dock_float_x_valueChanged(int newValue) {
  tree_set_int("dock/floatingX", newValue);
}

void MainDialog::on_dock_float_y_valueChanged(int newValue) {
  tree_set_int("dock/floatingY", newValue);
}

void MainDialog::on_dock_stack_top_toggled(bool checked) {
  if(checked)
    tree_set_string("dock/stacking", "Above");
}

void MainDialog::on_dock_stack_normal_toggled(bool checked) {
  if(checked)
    tree_set_string("dock/stacking", "Normal");
}

void MainDialog::on_dock_stack_bottom_toggled(bool checked) {
  if(checked)
    tree_set_string("dock/stacking", "Below");
}

void MainDialog::on_dock_direction_currentIndexChanged(int index) {
  const char* val = (index == DIRECTION_VERTICAL ? "Vertical" : "Horizontal");
  tree_set_string("dock/direction", val);
}

void MainDialog::on_dock_nostrut_toggled(bool checked) {
  tree_set_bool("dock/noStrut", checked);
}

void MainDialog::on_dock_hide_toggled(bool checked) {
  tree_set_bool("dock/autoHide", checked);
}

void MainDialog::on_dock_hide_delay_valueChanged(int newValue) {
  tree_set_int("dock/hideDelay", newValue);
}

void MainDialog::on_dock_show_delay_valueChanged(int newValue) {
  tree_set_int("dock/showDelay", newValue);
}


