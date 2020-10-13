/*
* Copyright (c) 2014 Christian Surlykke
*
* This file is part of the LXQt project. <https://lxqt.org>
* It is distributed under the LGPL 2.1 or later license.
* Please refer to the LICENSE file for a copy of the license.
*/

#include "batteryinfodialog.h"
#include "ui_batteryinfodialog.h"

#include <LXQt/Globals>
#include <QPushButton>
#include <QFormLayout>
#include <QTabWidget>
#include <QDebug>

BatteryInfoDialog::BatteryInfoDialog(QList<Solid::Battery*> batteries, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BatteryInfoDialog)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    setWindowTitle(tr("Battery Info"));

    if (batteries.size() == 1)
    {
        BatteryInfoFrame *batteryInfoFrame = new BatteryInfoFrame(batteries[0]);
        ui->verticalLayout->insertWidget(0, batteryInfoFrame);
    }
    else
    {
        QTabWidget *tabWidget = new QTabWidget(this);
        ui->verticalLayout->insertWidget(0, tabWidget);
        for (Solid::Battery *const battery : qAsConst(batteries))
        {
            BatteryInfoFrame *batteryInfoFrame = new BatteryInfoFrame(battery);
            tabWidget->addTab(batteryInfoFrame, QSL("BAT"));
        }
    }
}

BatteryInfoDialog::~BatteryInfoDialog()
{
    delete ui;
}

void BatteryInfoDialog::toggleShow()
{
    qDebug() << "toggleShow";
    isVisible() ? hide() : show();
}
