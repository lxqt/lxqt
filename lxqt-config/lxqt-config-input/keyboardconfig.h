/*
    Copyright (C) 2013-2014  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

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


#ifndef KEYBOARDCONFIG_H
#define KEYBOARDCONFIG_H

#include <QDialog>
#include "ui_keyboardconfig.h"

namespace LXQt {
  class Settings;
}
class QSettings;

class KeyboardConfig : public QWidget {
  Q_OBJECT

public:
  KeyboardConfig(LXQt::Settings* _settings, QSettings* _qtSettings, QWidget* parent = 0);
  virtual ~KeyboardConfig();

  void accept();
  void applyConfig();

public Q_SLOTS:
  void reset();

Q_SIGNALS:
    void settingsChanged();

private:
  void setLeftHandedMouse();
  void loadSettings();
  void initControls();

private:
  Ui::KeyboardConfig ui;
  LXQt::Settings* settings;
  QSettings* qtSettings;
  int delay;
  int oldDelay;
  int interval;
  int oldInterval;
  int flashTime;
  int oldFlashTime;
  bool beep;
  bool oldBeep;
  bool numlock;
  bool oldNumlock;
};

#endif // KEYBOARDCONFIG_H
