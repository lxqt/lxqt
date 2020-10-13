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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

class UserInfo;
class GroupInfo;
class UserManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
  enum
  {
      PageUsers = 0,
      PageGroups
  };

public:
    explicit MainWindow();
    virtual ~MainWindow();

private:
    UserInfo* userFromItem(QTreeWidgetItem* item);
    GroupInfo* groupFromItem(QTreeWidgetItem *item);
    bool getNewPassword(const QString& name, QByteArray& passwd);
    void reloadUsers();
    void reloadGroups();

private Q_SLOTS:
    void onAdd();
    void onDelete();
    void onEditProperties();
    void onChangePasswd();
    void reload();
    void onRowActivated(const QModelIndex& index);

private:
    Ui::MainWindow ui;
    UserManager* mUserManager;
};

#endif // MAINWINDOW_H
