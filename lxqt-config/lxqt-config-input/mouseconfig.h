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


#ifndef MOUSECONFIG_H
#define MOUSECONFIG_H

#include <QWidget>
#include "ui_mouseconfig.h"

namespace LXQt {
  class Settings;
}
class QSettings;

class MouseConfig : public QWidget {
  Q_OBJECT

public:
  MouseConfig(LXQt::Settings* _settings, QSettings* _qtSettings, QWidget* parent);
  virtual ~MouseConfig();

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
  Ui::MouseConfig ui;
  LXQt::Settings* settings;
  QSettings* qtSettings;
  int accel;
  int oldAccel;
  int threshold;
  int oldThreshold;
  int doubleClickInterval;
  int oldDoubleClickInterval;
  int wheelScrollLines;
  int oldWheelScrollLines;
  bool leftHanded;
  bool oldLeftHanded;
  bool singleClick;
  bool oldSingleClick;
};

#endif // MOUSECONFIG_H
