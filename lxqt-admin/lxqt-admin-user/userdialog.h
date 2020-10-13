/*
 *
 * Copyright (C) 2014  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef USERDIALOG_H
#define USERDIALOG_H

#include <QDialog>
#include "ui_userdialog.h"
#include "usermanager.h"

class UserManager;
class UserInfo;

class UserDialog : public QDialog
{
    Q_OBJECT

public:
    UserDialog(UserManager* userManager, UserInfo* user, QWidget* parent = nullptr);
    ~UserDialog();

    virtual void accept();

private Q_SLOTS:
    void onLoginNameChanged(const QString& text);
    void onFullNameChanged(const QString& text);
    void onHomeDirChanged(const QString& text);

private:
    Ui::UserDialog ui;
    UserManager* mUserManager;
    UserInfo* mUser;

    bool mFullNameChanged;
    bool mHomeDirChanged;
};

#endif // USERDIALOG_H
