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

#include "groupdialog.h"
#include <QMessageBox>
#include "usermanager.h"
#include <QDebug>
#include <QPushButton>
#define DEFAULT_GID_MIN 1000
#define DEFAULT_GID_MAX 32768

GroupDialog::GroupDialog(UserManager* userManager, GroupInfo* group, QWidget *parent, Qt::WindowFlags f):
    QDialog(parent, f),
    mUserManager(userManager),
    mGroup(group)
{
    ui.setupUi(this);
    ui.groupName->setText(group->name());
    ui.gid->setValue(group->gid());
    ui.buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui.buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    const QStringList& members = group->members(); // all users in this group
    // load all users
    for(const UserInfo* user: userManager->users())
    {
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(user->name());
        item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsSelectable);
        if(members.indexOf(user->name()) != -1) // the user is in this group
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
        QVariant obj = QVariant::fromValue<void*>((void*)user);
        item->setData(Qt::UserRole, obj);
        ui.userList->addItem(item);
    }
}
GroupDialog::~GroupDialog()
{
}

void GroupDialog::accept()
{
    QString groupName = ui.groupName->text();
    if(groupName.isEmpty())
    {
        QMessageBox::critical(this, tr("Error"), tr("The group name cannot be empty."));
        return;
    }
    mGroup->setName(groupName);
    mGroup->setGid(ui.gid->value());

    // update users
    mGroup->removeAllMemberss();
    for(int row = 0; row < ui.userList->count(); ++row) {
        QListWidgetItem* item = ui.userList->item(row);
        if(item->checkState() == Qt::Checked) {
            mGroup->addMember(item->text());
        }
    }
    QDialog::accept();
}
