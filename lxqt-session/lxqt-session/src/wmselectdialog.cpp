/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2010-2011 LXQt team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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


#include "wmselectdialog.h"
#include "ui_wmselectdialog.h"

#include <LXQt/Globals>
#include <QPushButton>
#include <QTreeWidget>
#include <QVariant>
#include <stdlib.h>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QDebug>

#define TYPE_ROLE   Qt::UserRole + 1
#define SELECT_DLG_TYPE 12345

WmSelectDialog::WmSelectDialog(const WindowManagerList &availableWindowManagers, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WmSelectDialog)
{
    qApp->setStyle(QSL("plastique"));
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    setModal(true);
    connect(ui->wmList, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(accept()));
    connect(ui->wmList, SIGNAL(clicked(QModelIndex)), this, SLOT(selectFileDialog(QModelIndex)));
    connect(ui->wmList, SIGNAL(activated(QModelIndex)), this, SLOT(changeBtnStatus(QModelIndex)));
    for (const WindowManager &wm : availableWindowManagers)
    {
        addWindowManager(wm);
    }


    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(0, tr("Other ..."));
    item->setText(1, tr("Choose your favorite one."));
    item->setData(1, TYPE_ROLE, SELECT_DLG_TYPE);

    ui->wmList->setCurrentItem(ui->wmList->topLevelItem(0));
    ui->wmList->addTopLevelItem(item);
}


WmSelectDialog::~WmSelectDialog()
{
    delete ui;
}


void WmSelectDialog::done( int r )
{
    QString wm = windowManager();
    if (r==1 && !wm.isEmpty() && findProgram(wm))
    {
        QDialog::done( r );
        close();
    }
}


QString WmSelectDialog::windowManager() const
{
    QTreeWidgetItem *item = ui->wmList->currentItem();
    if (item)
        return item->data(0, Qt::UserRole).toString();

    return QString();
}


void WmSelectDialog::addWindowManager(const WindowManager &wm)
{
    QTreeWidgetItem *item = new QTreeWidgetItem();

    item->setText(0, wm.name);
    item->setText(1, wm.comment);
    item->setData(0, Qt::UserRole, wm.command);

    ui->wmList->addTopLevelItem(item);
}


void WmSelectDialog::selectFileDialog(const QModelIndex &/*index*/)
{
    QTreeWidget *wmList = ui->wmList;
    QTreeWidgetItem *item = wmList->currentItem();
    if (item->data(1, TYPE_ROLE) != SELECT_DLG_TYPE)
        return;

    QString fname = QFileDialog::getOpenFileName(this, QString(), QSL("/usr/bin/"));
    if (fname.isEmpty())
        return;

    QFileInfo fi(fname);
    if (!fi.exists() || !fi.isExecutable())
        return;

    QTreeWidgetItem *wmItem = new QTreeWidgetItem();

    wmItem->setText(0, fi.baseName());
    wmItem->setData(0, Qt::UserRole, fi.absoluteFilePath());
    wmList->insertTopLevelItem(wmList->topLevelItemCount() -1, wmItem);
    ui->wmList->setCurrentItem(wmItem);
}

void WmSelectDialog::changeBtnStatus(const QModelIndex &/*index*/)
{
    QString wm = windowManager();
    ui->buttonBox->setEnabled(!wm.isEmpty() && findProgram(wm));
}
