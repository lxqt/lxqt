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

#include "execfiledialog_p.h"
#include "ui_exec-file.h"
#include "core/iconinfo.h"

namespace Fm {

ExecFileDialog::ExecFileDialog(const FileInfo &fileInfo, QWidget* parent, Qt::WindowFlags f):
    QDialog(parent, f),
    ui(new Ui::ExecFileDialog()),
    result_(BasicFileLauncher::ExecAction::DIRECT_EXEC) {

    ui->setupUi(this);
    // show file icon
    auto gicon = fileInfo.icon();
    if(gicon) {
        ui->icon->setPixmap(gicon->qicon().pixmap(QSize(48, 48)));
    }

    QString msg;
    if(fileInfo.isDesktopEntry()) {
        msg = tr("This file '%1' seems to be a desktop entry.\nWhat do you want to do with it?")
              .arg(fileInfo.displayName());
        ui->exec->setDefault(true);
        ui->execTerm->hide();
    }
    else if(fileInfo.isText()) {
        msg = tr("This text file '%1' seems to be an executable script.\nWhat do you want to do with it?")
              .arg(fileInfo.displayName());
        ui->execTerm->setDefault(true);
    }
    else {
        msg = tr("This file '%1' is executable. Do you want to execute it?")
              .arg(fileInfo.displayName());
        ui->exec->setDefault(true);
        ui->open->hide();
    }
    ui->msg->setText(msg);
}

ExecFileDialog::~ExecFileDialog() {
    delete ui;
}

void ExecFileDialog::accept() {
    QObject* _sender = sender();
    if(_sender == ui->exec) {
        result_ = BasicFileLauncher::ExecAction::DIRECT_EXEC;
    }
    else if(_sender == ui->execTerm) {
        result_ = BasicFileLauncher::ExecAction::EXEC_IN_TERMINAL;
    }
    else if(_sender == ui->open) {
        result_ = BasicFileLauncher::ExecAction::OPEN_WITH_DEFAULT_APP;
    }
    else {
        result_ = BasicFileLauncher::ExecAction::CANCEL;
    }
    QDialog::accept();
}

void ExecFileDialog::reject() {
    result_ = BasicFileLauncher::ExecAction::CANCEL;
    QDialog::reject();
}

} // namespace Fm
