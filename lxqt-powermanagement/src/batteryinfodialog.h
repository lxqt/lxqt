/*
* Copyright (c) 2014 Christian Surlykke
*
* This file is part of the LXQt project. <https://lxqt.org>
* It is distributed under the LGPL 2.1 or later license.
* Please refer to the LICENSE file for a copy of the license.
*/

#ifndef BATTERYINFODIALOG_H
#define BATTERYINFODIALOG_H

#include "batteryinfoframe.h"

#include <QDialog>
#include <QList>
#include <Solid/Battery>

namespace Ui {
class BatteryInfoDialog;
}

class BatteryInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatteryInfoDialog(QList<Solid::Battery*> batteries, QWidget *parent = nullptr);
    ~BatteryInfoDialog() override;

public slots:
    void toggleShow();

private:
    Ui::BatteryInfoDialog *ui;
};

#endif // BATTERYINFODIALOG_H
