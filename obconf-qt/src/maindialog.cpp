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
#include <QMessageBox>
#include <QStandardItemModel>
#include <QPushButton>
using namespace Obconf;

MainDialog::MainDialog():
  QDialog(),
  themes(NULL),
  themes_model(new QStandardItemModel()) {

  ui.setupUi(this);
  ui.buttonBox->button(QDialogButtonBox::Close)->setText(tr("Close"));
  setWindowIcon(QIcon(QStringLiteral(PIXMAPS_DIR) + QStringLiteral("/obconf-qt.png")));
  // resize the list widget according to the width of its content.
  ui.listWidget->setMaximumWidth(ui.listWidget->sizeHintForColumn(0) + ui.listWidget->frameWidth() * 2 + 2);
  ui.theme_names->setModel(themes_model);

  theme_setup_tab();
  appearance_setup_tab();
  windows_setup_tab();
  mouse_setup_tab();
  moveresize_setup_tab();
  margins_setup_tab();
  desktops_setup_tab();
  dock_setup_tab();

  // Normally, this is called in ui.setupUi(), but this is not desired behavior.
  // So we edited the generated ui header file with "sed" and generated a fixed version.
  // Then, we need to call it here.
  QMetaObject::connectSlotsByName(this);
}

MainDialog::~MainDialog() {
  if(themes) {
    g_list_foreach(themes, (GFunc)g_free, NULL);
    g_list_free(themes);
  }
  delete themes_model;
}

void MainDialog::accept() {
  QDialog::accept();
}

void MainDialog::reject() {
  /* restore to original settings */

  QDialog::reject();
}

void MainDialog::on_about_clicked() {
  QMessageBox::about(this, tr("About ObConf-Qt"),
                     tr("A preferences manager for Openbox\n\n"
                        "Copyright (c) 2014-2015\n\n"
                        "Authors:\n"
                        "* Hong Jen Yee (PCMan) <pcman.tw@gmail.com>\n\n"
                        "The program is based on ObConf developed by the following developers.\n"
                        "* Dana Jansens <danakj@orodu.net>\n"
                        "* Tim Riley <tr@slackzone.org>\n"
                        "* Javeed Shaikh <syscrash2k@gmail.com>")
                    );
}
