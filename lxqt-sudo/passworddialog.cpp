/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015-2018 LXQt team
 * Authors:
 *   Palo Kisa <palo.kisa@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "passworddialog.h"
#include "ui_passworddialog.h"
#include <QIcon>
#include <QClipboard>
#include <QToolButton>
#include <QPushButton>
PasswordDialog::PasswordDialog(const QString & cmd
        , const QString & backendName
        , QWidget * parent/* = 0*/
        , Qt::WindowFlags f/* = 0*/)
    : QDialog(parent, f)
    , ui(new Ui::PasswordDialog)
{
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui->buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->commandL->setText(cmd);
    connect(ui->commandCopyBtn, &QToolButton::clicked, [cmd]() {
        QApplication::clipboard()->setText (cmd);
    });

    ui->backendL->setText(backendName);
    ui->iconL->setPixmap(QIcon::fromTheme(QStringLiteral("dialog-password")).pixmap(64, 64));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("security-high")));
}

PasswordDialog::~PasswordDialog()
{
}

void PasswordDialog::showEvent(QShowEvent * event)
{
    ui->errorL->setText(tr("Attempt #%1").arg(++mAttempt));
    ui->passwordLE->setFocus();
    return QDialog::showEvent(event);
}

QString PasswordDialog::password() const
{
    return ui->passwordLE->text();
}

