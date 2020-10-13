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

#include "mainwindow.h"

#include <LXQt/Globals>

#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QRegExp>
#include "userdialog.h"
#include "groupdialog.h"
#include "usermanager.h"

MainWindow::MainWindow():
    QMainWindow(),
    mUserManager(new UserManager(this))
{
    ui.setupUi(this);
    ui.userList->sortByColumn(0, Qt::AscendingOrder);
    ui.groupList->sortByColumn(0, Qt::AscendingOrder);

    connect(ui.actionAdd, &QAction::triggered, this, &MainWindow::onAdd);
    connect(ui.actionDelete, &QAction::triggered, this, &MainWindow::onDelete);
    connect(ui.actionProperties, &QAction::triggered, this, &MainWindow::onEditProperties);
    connect(ui.actionChangePasswd, &QAction::triggered, this, &MainWindow::onChangePasswd);
    connect(ui.actionRefresh, &QAction::triggered, this, &MainWindow::reload);

#ifdef Q_OS_FREEBSD //Disable group gpasswd for FreeBSD
    connect(ui.tabWidget, &QTabWidget::currentChanged, [this](int index) {
        if(index==1) {
            ui.actionChangePasswd->setEnabled(false);
        } else {
            ui.actionChangePasswd->setEnabled(true);
        }
    });
#endif
    connect(ui.userList, &QListWidget::activated, this, &MainWindow::onRowActivated);
    connect(ui.groupList, &QListWidget::activated, this, &MainWindow::onRowActivated);
    connect(ui.showSystemUsers, &QCheckBox::stateChanged, this, &MainWindow::reload);

    connect(mUserManager, &UserManager::changed, this, &MainWindow::reload);
    reload();
}

MainWindow::~MainWindow()
{
}

void MainWindow::reloadUsers()
{
    QList<QTreeWidgetItem *> items;
    const auto& users = mUserManager->users();
    items.reserve(users.size());
    for(const UserInfo* user: users)
    {
        uid_t uid = user->uid();
        if(ui.showSystemUsers->isChecked() || (uid > 499 && !user->shell().isEmpty())) // exclude system users
        {
            QTreeWidgetItem* item = new QTreeWidgetItem();
            item->setData(0, Qt::DisplayRole, user->name());
            QVariant obj = QVariant::fromValue<void*>((void*)user);
            item->setData(0, Qt::UserRole, obj);
            item->setData(1, Qt::DisplayRole, uid);
            item->setData(2, Qt::DisplayRole, user->fullName());
            GroupInfo* group = mUserManager->findGroupInfo(user->gid());
            if(group != nullptr) {
                item->setData(3, Qt::DisplayRole, group->name());
            }
            item->setData(4, Qt::DisplayRole, user->homeDir());
            items.append(item);
        }
    }
    ui.userList->clear();
    ui.userList->addTopLevelItems(items);
}

void MainWindow::reloadGroups()
{
    // load groups
    QList<QTreeWidgetItem *> items;
    const auto& groups = mUserManager->groups();
    items.reserve(groups.size());
    for(const GroupInfo* group: groups)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setData(0, Qt::DisplayRole, group->name());
        QVariant obj = QVariant::fromValue<void*>((void*)group);
        item->setData(0, Qt::UserRole, obj);
        item->setData(1, Qt::DisplayRole, group->gid());
        item->setData(2, Qt::DisplayRole, group->members().join(QL1S(", ")));
        items.append(item);
    }
    ui.groupList->clear();
    ui.groupList->addTopLevelItems(items);
}

void MainWindow::reload() {
    reloadUsers();
    reloadGroups();
}

UserInfo *MainWindow::userFromItem(QTreeWidgetItem *item)
{
    if(item)
    {
        QVariant obj = item->data(0, Qt::UserRole);
        return reinterpret_cast<UserInfo*>(obj.value<void*>());
    }
    return nullptr;
}

GroupInfo* MainWindow::groupFromItem(QTreeWidgetItem *item)
{
    if(item)
    {
        QVariant obj = item->data(0, Qt::UserRole);
        return reinterpret_cast<GroupInfo*>(obj.value<void*>());
    }
    return nullptr;
}

void MainWindow::onAdd()
{
    if(ui.tabWidget->currentIndex() == PageUsers)
    {
        UserInfo newUser;
        UserDialog dlg(mUserManager, &newUser, this);
        if(dlg.exec() == QDialog::Accepted)
        {
            mUserManager->addUser(&newUser);
            QByteArray newPasswd;
            if(getNewPassword(newUser.name(), newPasswd)) {
                mUserManager->changePassword(&newUser, newPasswd);
            }
        }
    }
    else if (ui.tabWidget->currentIndex() == PageGroups)
    {
        GroupInfo newGroup;
        GroupDialog dlg(mUserManager, &newGroup, this);
        if(dlg.exec() == QDialog::Accepted)
        {
            mUserManager->addGroup(&newGroup);
        }
    }
}

void MainWindow::onDelete()
{
    if(ui.tabWidget->currentIndex() == PageUsers)
    {
        QTreeWidgetItem* item = ui.userList->currentItem();
        UserInfo* user = userFromItem(item);
        if(user)
        {
            if(QMessageBox::question(this, tr("Confirm"), tr("Are you sure you want to delete the selected user?"), QMessageBox::Ok|QMessageBox::Cancel) == QMessageBox::Ok)
            {
                mUserManager->deleteUser(user);
            }
        }
    }
    else if(ui.tabWidget->currentIndex() == PageGroups)
    {
        QTreeWidgetItem* item = ui.groupList->currentItem();
        GroupInfo* group = groupFromItem(item);
        if(group)
        {
            if(QMessageBox::question(this, tr("Confirm"), tr("Are you sure you want to delete the selected group?"), QMessageBox::Ok|QMessageBox::Cancel) == QMessageBox::Ok)
            {
                mUserManager->deleteGroup(group);
            }
        }
    }
}

bool MainWindow::getNewPassword(const QString& name, QByteArray& passwd) {
    QInputDialog dlg(this);
    dlg.setTextEchoMode(QLineEdit::Password);
    dlg.setLabelText(tr("Input the new password for %1:").arg(name));
    QLineEdit* edit = dlg.findChild<QLineEdit*>(QString());
    if(edit) {
        // NOTE: do we need to add a validator to limit the input?
        // QRegExpValidator* validator = new QRegExpValidator(QRegExp(QStringLiteral("\\w*")), edit);
        // edit->setValidator(validator);
    }
    if(dlg.exec() == QDialog::Accepted) {
        passwd = dlg.textValue().toUtf8();
        if(passwd.isEmpty()) {
            if(QMessageBox::question(this, tr("Confirm"), tr("Are you sure you want to set a empty password?"), QMessageBox::Ok|QMessageBox::Cancel) != QMessageBox::Ok)
                return false;
        }
        return true;
    }
    return false;
}

void MainWindow::onChangePasswd() {
    QString name;
    UserInfo* user = nullptr;
    GroupInfo* group = nullptr;
    if(ui.tabWidget->currentIndex() == PageUsers)
    {
        QTreeWidgetItem* item = ui.userList->currentItem();
        user = userFromItem(item);
        if (!user)
            return;

        name = user->name();
    }
    else if(ui.tabWidget->currentIndex() == PageGroups)
    {
        QTreeWidgetItem* item = ui.groupList->currentItem();
        group = groupFromItem(item);
        if (!group)
            return;

        name = group->name();
    }

    QByteArray newPasswd;
    if(getNewPassword(name, newPasswd)) {
        if(user) {
            mUserManager->changePassword(user, newPasswd);
        }
        if(group) {
            mUserManager->changePassword(group, newPasswd);
        }
    }
}

void MainWindow::onRowActivated(const QModelIndex& index)
{
    onEditProperties();
}

void MainWindow::onEditProperties()
{
    if(ui.tabWidget->currentIndex() == PageUsers)
    {
        QTreeWidgetItem* item = ui.userList->currentItem();
        UserInfo* user = userFromItem(item);
        if(user) {
            UserInfo newSettings(*user);
            UserDialog dlg(mUserManager, &newSettings, this);
            if(dlg.exec() == QDialog::Accepted)
            {
                mUserManager->modifyUser(user, &newSettings);
            }
        }
    }
    else if(ui.tabWidget->currentIndex() == PageGroups)
    {
        QTreeWidgetItem* item = ui.groupList->currentItem();
        GroupInfo* group = groupFromItem(item);
        if(group) {
            GroupInfo newSettings(*group);
            GroupDialog dlg(mUserManager, &newSettings, this);
            if(dlg.exec() == QDialog::Accepted)
            {
                mUserManager->modifyGroup(group, &newSettings);
            }
        }
    }
}

