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
#include <X11/Xlib.h>

using namespace Obconf;

extern RrInstance* rrinst; // defined in obconf-qt.cpp

static int num_desktops;

static void desktops_read_names();
static void desktops_write_names();
static void desktops_write_number();

static void enable_stuff();

void MainDialog::desktops_setup_tab() {
  num_desktops = tree_get_int("desktops/number", 4);
  ui.desktop_num->setValue(num_desktops);

  gint i;

  desktops_read_names();

  i = tree_get_int("desktops/popupTime", 875);
  ui.desktop_popup->setChecked(i != 0);
  ui.desktop_popup_time->setValue(i ? i : 875);
}

void MainDialog::on_desktop_num_valueChanged(int newValue) {
  num_desktops = newValue;
  desktops_write_number();
  desktops_read_names();
}

void MainDialog::on_desktop_names_itemChanged(QListWidgetItem * item) {
  QString new_text = item->text();

  if(new_text.isEmpty())
    item->setText(tr("(Unnamed desktop)"));

  desktops_write_names();
}

void MainDialog::desktops_read_names() {
  xmlNodePtr n;
  gint i;

  ui.desktop_names->clear();

  i = 0;
  n = tree_get_node("desktops/names", NULL)->children;

  while(n) {
    gchar* name;

    if(!xmlStrcmp(n->name, (const xmlChar*)"name")) {
      name = obt_xml_node_string(n);
      QString desktop_name = QString::fromUtf8(name);

      if(desktop_name.isEmpty())
        desktop_name = tr("(Unnamed desktop)");

      QListWidgetItem* item = new QListWidgetItem(desktop_name);
      item->setFlags(item->flags() | Qt::ItemIsEditable);
      ui.desktop_names->addItem(item);
      ++i;
    }

    n = n->next;
  }

  while(i < num_desktops) {
    QListWidgetItem* item = new QListWidgetItem(tr("(Unnamed desktop)"));
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    ui.desktop_names->addItem(item);
    ++i;
  }
}

void MainDialog::desktops_write_names() {
  xmlNodePtr n, c;

  // delete all existing keys
  n = tree_get_node("desktops/names", NULL);

  while((c = n->children)) {
    xmlUnlinkNode(c);
    xmlFreeNode(c);
  }

  int i;

  for(i = 0; i < ui.desktop_names->count(); ++i) {
    QListWidgetItem* item = ui.desktop_names->item(i);
    QString text = item->text();
    xmlNewTextChild(n, NULL, (xmlChar*)"name", (xmlChar*)text.toUtf8().constData());
  }

  tree_apply();
  /* make openbox re-set the property */
  XDeleteProperty(QX11Info::display(), QX11Info::appRootWindow(),
                  XInternAtom(QX11Info::display(), "_NET_DESKTOP_NAMES", False));
}

void MainDialog::desktops_write_number() {
  XEvent ce;
  tree_set_int("desktops/number", num_desktops);
  ce.xclient.type = ClientMessage;
  ce.xclient.message_type =
    XInternAtom(QX11Info::display(), "_NET_NUMBER_OF_DESKTOPS", False);
  ce.xclient.display = QX11Info::display();
  ce.xclient.window = QX11Info::appRootWindow();
  ce.xclient.format = 32;
  ce.xclient.data.l[0] = num_desktops;
  ce.xclient.data.l[1] = 0;
  ce.xclient.data.l[2] = 0;
  ce.xclient.data.l[3] = 0;
  ce.xclient.data.l[4] = 0;
  XSendEvent(QX11Info::display(), QX11Info::appRootWindow(), FALSE,
             SubstructureNotifyMask | SubstructureRedirectMask,
             &ce);
}

void MainDialog::on_desktop_popup_toggled(bool checked) {
  if(checked) {
    tree_set_int("desktops/popupTime", ui.desktop_popup_time->value());
  }
  else
    tree_set_int("desktops/popupTime", 0);
}

void MainDialog::on_desktop_popup_time_valueChanged(int newValue) {
  tree_set_int("desktops/popupTime", newValue);
}

