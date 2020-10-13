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

#ifndef GROUPDIALOG_H
#define GROUPDIALOG_H

#include <QDialog>
#include "ui_groupdialog.h"

class GroupInfo;
class UserManager;

class GroupDialog : public QDialog
{
    Q_OBJECT

public:
    GroupDialog(UserManager* userManager, GroupInfo* group, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~GroupDialog();

    virtual void accept();

private:
    Ui::GroupDialog ui;
    UserManager* mUserManager;
    GroupInfo* mGroup;
};

#endif // GROUPDIALOG_H
