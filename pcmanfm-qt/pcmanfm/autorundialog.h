/*

    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

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


#ifndef PCMANFM_AUTORUNDIALOG_H
#define PCMANFM_AUTORUNDIALOG_H

#include <QDialog>
#include "ui_autorun.h"
#include <gio/gio.h>

namespace PCManFM {

class AutoRunDialog : public QDialog {
Q_OBJECT

public:
  explicit AutoRunDialog(GVolume* volume, GMount* mount, QWidget* parent = 0, Qt::WindowFlags f = Qt::WindowFlags());
  virtual ~AutoRunDialog();

  virtual void accept();

private Q_SLOTS:

private:

  static void onContentTypeFinished(GMount* mount, GAsyncResult* res, AutoRunDialog* pThis);

private:
  Ui::AutoRunDialog ui;
  GCancellable* cancellable;
  GList* applications;
  GMount* mount_;
};

}

#endif // PCMANFM_AUTORUNDIALOG_H
