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

#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QDialog>
#include <memory>


namespace Ui
{
class PasswordDialog;
}


class PasswordDialog : public QDialog {
    Q_OBJECT

public:

    PasswordDialog(QWidget* parent = nullptr);
    
    ~PasswordDialog();

    QString password() const;

    void setPassword(const QString& password);
    
    void setEncryptFileList(bool value);

    bool encryptFileList() const;

    static QString askPassword(QWidget* parent = nullptr);

private Q_SLOTS:
    void onTogglePassword(bool toggled);
    
private:
    std::unique_ptr<Ui::PasswordDialog> ui_;
};

#endif // PASSWORDDIALOG_H
