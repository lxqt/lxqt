/*
    Copyright (C) 2014  P.L. Lucas <selairi@gmail.com>
    Copyright (C) 2013  <copyright holder> <email>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef MONITORSETTINGSDIALOG_H
#define MONITORSETTINGSDIALOG_H

#include "ui_monitorsettingsdialog.h"
#include "timeoutdialog.h"

#include <QDialog>
#include <QTimer>
#include <KScreen/GetConfigOperation>
#include <KScreen/SetConfigOperation>

class MonitorSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    MonitorSettingsDialog();
    ~MonitorSettingsDialog() override;

    void accept() override;
    void reject() override;

private:
    void applyConfiguration(bool saveConfigOk);
    void cancelConfiguration();

private Q_SLOTS:
    void loadConfiguration(KScreen::ConfigPtr config);
    void showSettingsDialog();

private:
    void saveConfiguration(KScreen::ConfigPtr config);

    Ui::MonitorSettingsDialog ui;

    // Configutarions
    KScreen::ConfigPtr mOldConfig;
    KScreen::ConfigPtr mConfig;
};

#endif // MONITORSETTINGSDIALOG_H
