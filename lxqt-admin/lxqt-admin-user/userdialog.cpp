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

#include "userdialog.h"

#include <LXQt/Globals>
#include <QPushButton>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QDebug>
#include "usermanager.h"
#define DEFAULT_UID_MIN 1000
#define DEFAULT_UID_MAX 32768

UserDialog::UserDialog(UserManager* userManager, UserInfo* user, QWidget* parent):
    QDialog(parent),
    mUserManager(userManager),
    mUser(user),
    mFullNameChanged(false),
    mHomeDirChanged(false)
{
    ui.setupUi(this);
    ui.buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui.buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    bool isNewUser = (user->uid() == 0 && user->name().isEmpty());
    // load all groups
    for(const GroupInfo* group: mUserManager->groups())
    {
        ui.mainGroup->addItem(group->name());
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(group->name());
        item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsSelectable);
        if(!isNewUser)
        {
            if(group->hasMember(user->name())) // the user is in this group
                item->setCheckState(Qt::Checked);
            else
                item->setCheckState(Qt::Unchecked);
        }
        else
            item->setCheckState(Qt::Unchecked);
        ui.groupList->addItem(item);
    }
    connect(ui.loginName, SIGNAL(textChanged(QString)), SLOT(onLoginNameChanged(QString)));

    ui.loginShell->addItems(mUserManager->availableShells());

    if(isNewUser) // new user
    {
        ui.mainGroup->setCurrentIndex(-1);
        ui.loginShell->setCurrentIndex(-1);
    }
    else  // edit an existing user
    {
        ui.loginName->setText(user->name());
        ui.uid->setValue(user->uid());
        ui.fullName->setText(user->fullName());
        ui.loginShell->setEditText(user->shell());
        ui.homeDir->setText(user->homeDir());
        GroupInfo* group = userManager->findGroupInfo(user->gid());
        if(group)
            ui.mainGroup->setEditText(group->name());
    }
}

UserDialog::~UserDialog()
{
}

void UserDialog::onLoginNameChanged(const QString& text)
{
    if(!mFullNameChanged)
    {
        ui.fullName->blockSignals(true);
        ui.fullName->setText(text);
        ui.fullName->blockSignals(false);
    }

    if(!mHomeDirChanged)
    {
        ui.homeDir->blockSignals(true);
        ui.homeDir->setText(QSL("/home/") + text);
        ui.homeDir->blockSignals(false);
    }
}

void UserDialog::onFullNameChanged(const QString& text)
{
   mFullNameChanged = true;
}

void UserDialog::onHomeDirChanged(const QString& text)
{
    mHomeDirChanged = true;
}

void UserDialog::accept()
{
    mUser->setUid(ui.uid->value());
    QString loginName = ui.loginName->text();
    if(loginName.isEmpty())
    {
        QMessageBox::critical(this, tr("Error"), tr("The user name cannot be empty."));
        return;
    }
    mUser->setName(loginName);
    mUser->setFullName(ui.fullName->text());

    mUser->setHomeDir(ui.homeDir->text());
    mUser->setShell(ui.loginShell->currentText());
    // main group
    QString groupName = ui.mainGroup->currentText();
    if(!groupName.isEmpty()) {
        GroupInfo* group = mUserManager->findGroupInfo(groupName);
        if(group)
            mUser->setGid(group->gid());
    }

    // other groups
    mUser->removeAllGroups();
    for(int row = 0; row < ui.groupList->count(); ++row) {
        QListWidgetItem* item = ui.groupList->item(row);
        if(item->checkState() == Qt::Checked) {
            mUser->addGroup(item->text());
        }
    }
    QDialog::accept();
}
