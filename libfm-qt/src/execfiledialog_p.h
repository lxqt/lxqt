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

#ifndef FM_EXECFILEDIALOG_H
#define FM_EXECFILEDIALOG_H

#include "core/basicfilelauncher.h"
#include "core/fileinfo.h"

#include <QDialog>

#include <memory>

namespace Ui {
  class ExecFileDialog;
}

namespace Fm {

class ExecFileDialog : public QDialog {
  Q_OBJECT
public:
  ~ExecFileDialog() override;
  ExecFileDialog(const FileInfo& fileInfo, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

  BasicFileLauncher::ExecAction result() {
    return result_;
  }

protected:
  void accept() override;
  void reject() override;

private:
  Ui::ExecFileDialog* ui;
  BasicFileLauncher::ExecAction result_;
};

}

#endif // FM_EXECFILEDIALOG_H
