/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018  <copyright holder> <email>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "passworddialog.h"
#include "ui_passworddialog.h"
#include <QInputDialog>
#include <QPushButton>

PasswordDialog::PasswordDialog(QWidget* parent):
    QDialog{parent},
    ui_{new Ui::PasswordDialog} {

    ui_->setupUi(this);
    ui_->buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui_->buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    connect(ui_->showPassword, &QCheckBox::toggled, this, &PasswordDialog::onTogglePassword);
}
PasswordDialog::~PasswordDialog() {
}
QString PasswordDialog::password() const {
    return ui_->passwordEdit->text();
}

void PasswordDialog::setPassword(const QString& password) {
    ui_->passwordEdit->setText(password);
}

void PasswordDialog::setEncryptFileList(bool value) {
    ui_->encryptFileList->setChecked(value);
}

bool PasswordDialog::encryptFileList() const {
    return ui_->encryptFileList->isChecked();
}


void PasswordDialog::onTogglePassword(bool toggled) {
    ui_->passwordEdit->setEchoMode(toggled ? QLineEdit::Normal : QLineEdit::Password);
}

// static
QString PasswordDialog::askPassword(QWidget* parent) {
    QInputDialog dlg;
    return QInputDialog::getText(parent, tr("Password"), tr("Password:"), QLineEdit::Password);
}

