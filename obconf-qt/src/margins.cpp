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


void MainDialog::margins_setup_tab() {
  ui.margins_left->setValue(tree_get_int("margins/left", 0));
  ui.margins_right->setValue(tree_get_int("margins/right", 0));
  ui.margins_top->setValue(tree_get_int("margins/top", 0));
  ui.margins_bottom->setValue(tree_get_int("margins/bottom", 0));
}

void MainDialog::on_margins_left_valueChanged(int newValue) {
  tree_set_int("margins/left", newValue);
}

void MainDialog::on_margins_right_valueChanged(int newValue) {
  tree_set_int("margins/right", newValue);
}

void MainDialog::on_margins_top_valueChanged(int newValue) {
  tree_set_int("margins/top", newValue);
}

void MainDialog::on_margins_bottom_valueChanged(int newValue) {
  tree_set_int("margins/bottom", newValue);
}
