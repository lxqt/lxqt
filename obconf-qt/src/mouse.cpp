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

static xmlNodePtr saved_custom = NULL;

#define TITLEBAR_MAXIMIZE 0
#define TITLEBAR_SHADE    1
#define TITLEBAR_CUSTOM   2

//static void MainDialog::on_titlebar_doubleclick_custom_activate(GtkMenuItem* w,
//    gpointer data);
void MainDialog::mouse_setup_tab() {
  gint a;
  
  ui.focus_mouse->setChecked(tree_get_bool("focus/followMouse", FALSE));
  ui.focus_delay->setValue(tree_get_int("focus/focusDelay", 0));
  ui.focus_raise->setChecked(tree_get_bool("focus/raiseOnFocus", FALSE));
  ui.focus_notlast->setChecked(!tree_get_bool("focus/focusLast", TRUE));
  ui.focus_under_mouse->setChecked(tree_get_bool("focus/underMouse", FALSE));
  ui.doubleclick_time->setValue(tree_get_int("mouse/doubleClickTime", 200));
  
  // w = get_widget("titlebar_doubleclick");
  a = read_doubleclick_action();
  
  if(a == TITLEBAR_CUSTOM) {
    ui.titlebar_doubleclick->addItem(tr("Custom actions"));
    /*
     *    GtkWidget* i = gtk_menu_item_new_with_label(_());
     *    g_signal_connect(i, "activate",
     *                     G_CALLBACK(on_titlebar_doubleclick_custom_activate),
     *                     NULL);
     *    gtk_menu_shell_append
     *    (GTK_MENU_SHELL
     *     (gtk_option_menu_get_menu
     *      (GTK_OPTION_MENU(w))), i);
     */
  }
  ui.titlebar_doubleclick->setCurrentIndex(a);
  mouse_enable_stuff();
}

void MainDialog::mouse_enable_stuff() {
  bool b = ui.focus_mouse->isChecked();
  ui.focus_delay->setEnabled(b);
  ui.focus_delay_label->setEnabled(b);
  // ui.focus_delay_label_units->setEnabled(b);
  ui.focus_raise->setEnabled(b);
  ui.focus_notlast->setEnabled(b);
  ui.focus_under_mouse->setEnabled(b);
}

void MainDialog::on_focus_mouse_toggled(bool checked) {
  tree_set_bool("focus/followMouse", checked);
  mouse_enable_stuff();
}

void MainDialog::on_focus_delay_valueChanged(int newValue) {
  tree_set_int("focus/focusDelay", newValue);
}

void MainDialog::on_focus_raise_toggled(bool checked) {
  tree_set_bool("focus/raiseOnFocus", checked);
}

void MainDialog::on_focus_notlast_toggled(bool checked) {
  tree_set_bool("focus/focusLast", !checked);
}

void MainDialog::on_focus_under_mouse_toggled(bool checked) {
  tree_set_bool("focus/underMouse", checked);
}

void MainDialog::on_titlebar_doubleclick_currentIndexChanged(int index) {
  write_doubleclick_action(index);
}

void MainDialog::on_doubleclick_time_valueChanged(int newValue) {
  tree_set_int("mouse/doubleClickTime", newValue);
}

int MainDialog::read_doubleclick_action() {
  xmlNodePtr n, top, c;
  gint max = 0, shade = 0, other = 0;
  
  top = tree_get_node("mouse/context:name=Titlebar"
  "/mousebind:button=Left:action=DoubleClick", NULL);
  n = top->children;
  
  /* save the current state */
  saved_custom = xmlCopyNode(top, 1);
  
  /* remove the namespace from all the nodes under saved_custom..
   *     without recursion! */
  c = saved_custom;
  
  while(c) {
    xmlSetNs(c, NULL);
    
    if(c->children)
      c = c->children;
    else if(c->next)
      c = c->next;
    
    while(c->parent && !c->parent->next)
      c = c->parent;
    
    if(!c->parent)
      c = NULL;
  }
  
  while(n) {
    if(!xmlStrcmp(n->name, (const xmlChar*)"action")) {
      if(obt_xml_attr_contains(n, "name", "ToggleMaximizeFull"))
        ++max;
      else if(obt_xml_attr_contains(n, "name", "ToggleShade"))
        ++shade;
      else
        ++other;
      
    }
    
    n = n->next;
  }
  
  if(max == 1 && shade == 0 && other == 0)
    return TITLEBAR_MAXIMIZE;
  
  if(max == 0 && shade == 1 && other == 0)
    return TITLEBAR_SHADE;
  
  return TITLEBAR_CUSTOM;
}

void MainDialog::write_doubleclick_action(int a) {
  xmlNodePtr n;
  
  n = tree_get_node("mouse/context:name=Titlebar"
  "/mousebind:button=Left:action=DoubleClick", NULL);
  
  /* remove all children */
  while(n->children) {
    xmlUnlinkNode(n->children);
    xmlFreeNode(n->children);
  }
  
  if(a == TITLEBAR_MAXIMIZE) {
    n = xmlNewChild(n, NULL, (xmlChar*)"action", NULL);
    xmlSetProp(n, (xmlChar*)"name", (xmlChar*)"ToggleMaximizeFull");
  }
  else if(a == TITLEBAR_SHADE) {
    n = xmlNewChild(n, NULL, (xmlChar*)"action", NULL);
    xmlSetProp(n, (xmlChar*)"name", (xmlChar*)"ToggleShade");
  }
  else {
    xmlNodePtr c = saved_custom->children;
    
    while(c) {
      xmlAddChild(n, xmlCopyNode(c, 1));
      c = c->next;
    }
  }
  
  tree_apply();
}

