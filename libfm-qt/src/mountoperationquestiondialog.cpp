/*
 * Copyright (C) 2013 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#include "mountoperationquestiondialog_p.h"
#include "mountoperation.h"
#include <QPushButton>

namespace Fm {

MountOperationQuestionDialog::MountOperationQuestionDialog(MountOperation* op, gchar* message, GStrv choices):
    QMessageBox(),
    mountOperation(op) {

    setIcon(QMessageBox::Question);
    setText(QString::fromUtf8(message));

    choiceCount = g_strv_length(choices);
    choiceButtons = new QAbstractButton*[choiceCount];
    for(int i = 0; i < choiceCount; ++i) {
        // It's not allowed to add custom buttons without standard roles
        // to QMessageBox. So we set role of all buttons to AcceptRole.
        // When any of the set buttons is clicked, exec() always returns "accept".
        QPushButton* button = new QPushButton(QString::fromUtf8(choices[i]));
        addButton(button, QMessageBox::AcceptRole);
        choiceButtons[i] = button;
    }
}

MountOperationQuestionDialog::~MountOperationQuestionDialog() {
    delete []choiceButtons;
}

void MountOperationQuestionDialog::done(int r) {
    GMountOperation* op = mountOperation->mountOperation();

    g_mount_operation_set_choice(op, r);
    g_mount_operation_reply(op, G_MOUNT_OPERATION_HANDLED);

    QDialog::done(r);
}

void MountOperationQuestionDialog::closeEvent(QCloseEvent *event)
{
    GMountOperation* op = mountOperation->mountOperation();

    g_mount_operation_reply(op, G_MOUNT_OPERATION_ABORTED);

    event->accept();
}

} // namespace Fm
