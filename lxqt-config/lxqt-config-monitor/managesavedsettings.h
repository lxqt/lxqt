/*
    Copyright (C) 2015  P.L. Lucas <selairi@gmail.com>

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


#ifndef _SAVESETTINGS_H_
#define _SAVESETTINGS_H_

#include "ui_managesavedsettings.h"
#include "monitor.h"
#include <LXQt/Settings>
#include <KScreen/Output>
#include <KScreen/EDID>
#include <KScreen/Config>

class ManageSavedSettings : public QDialog {
  Q_OBJECT

public:
  ManageSavedSettings(LXQt::Settings *applicationSettings, KScreen::ConfigPtr config, QWidget* parent = nullptr);

  Ui::ManageSavedSettings ui;

public slots:
  /*! Load settings to QListWidgets.
      edids is hardware code to detect hardware compatible settings.
   */
  void loadSettings();

  void showSelectedConfig(QListWidgetItem * item);

  void onDeleteItem();

  void onRenameItem();

  void onApplyItem();

private:
  LXQt::Settings *applicationSettings;
  KScreen::ConfigPtr config;
  bool isHardwareCompatible(const MonitorSavedSettings &settings);
  
};

#endif // _SAVESETTINGS_H_
