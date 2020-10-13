/*
 * Copyright (C) 2014 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef __UTILITS_P_H__
#define __UTILITS_P_H__

#include <QInputDialog>
#include <QTimer>
#include <QLineEdit>

namespace Fm {

// private class used in internal implementation
class FilenameDialog : public QInputDialog {
  Q_OBJECT
public:
  FilenameDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags()):
    QInputDialog(parent, flags),
    selectExtension_(false) {
  }

  void showEvent(QShowEvent * event) override {
    QWidget::showEvent(event);
    if(!selectExtension_) // dot not select filename extension
      QTimer::singleShot(0, this, SLOT(initSelection()));
  }

  bool selectExtension() const {
    return selectExtension_;
  }

  void setSelectExtension(bool value) {
    selectExtension_ = value;
  }

private Q_SLOTS:
  // do not select filename extensions
  void initSelection() {
    // find the QLineEdit child widget
    QLineEdit* lineEdit = findChild<QLineEdit*>();
    if(lineEdit) {
      QString filename = lineEdit->text();
      if(!filename.isEmpty()) {
        // only select filename part without extension name.
        int ext = filename.lastIndexOf(QLatin1Char('.'));
        if(ext != -1) {
          // add special cases for tar.gz, tar.bz2, and other tar.* files
          if(filename.leftRef(ext).endsWith(QStringLiteral(".tar")))
            ext -= 4;
	  // FIXME: should we also handle other special cases?
          lineEdit->setSelection(0, ext);
        }
      }
    }
  }

private:
  bool selectExtension_;
};

} // namespace Fm

#endif
