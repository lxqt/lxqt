/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright (C) 2011  Alec Moskvin <alecm@gmx.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include <QFileDialog>
#include "autostartedit.h"
#include "ui_autostartedit.h"
#include <QPushButton>
#include <LXQt/Globals>
AutoStartEdit::AutoStartEdit(QString name, QString command, bool needTray, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutoStartEdit)
{
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui->buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->nameEdit->setFocus();
    ui->nameEdit->setText(name);
    ui->commandEdit->setText(command);
    ui->trayCheckBox->setChecked(needTray);
    connect(ui->browseButton, SIGNAL(clicked()), SLOT(browse()));
}

QString AutoStartEdit::name()
{
    return ui->nameEdit->text();
}

QString AutoStartEdit::command()
{
    return ui->commandEdit->text();
}

bool AutoStartEdit::needTray()
{
    return ui->trayCheckBox->isChecked();
}

void AutoStartEdit::browse()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Select Application"), QSL("/usr/bin/"));
    if (!filePath.isEmpty())
        ui->commandEdit->setText(filePath);
}

AutoStartEdit::~AutoStartEdit()
{
    delete ui;
}
