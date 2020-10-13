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


#include "mouseconfig.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <LXQt/Settings>
#include <QDir>
#include <QFile>
#include <QStringBuilder>
#include <QDebug>

// FIXME: how to support XCB or Wayland?
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#ifdef Q_WS_X11
extern void qt_x11_apply_settings_in_all_apps();
#endif

MouseConfig::MouseConfig(LXQt::Settings* _settings, QSettings* _qtSettings, QWidget* parent):
  QWidget(parent),
  settings(_settings),
  qtSettings(_qtSettings),
  accel(20),
  oldAccel(20),
  threshold(10),
  oldThreshold(10),
  doubleClickInterval(400),
  oldDoubleClickInterval(400),
  wheelScrollLines(3),
  oldWheelScrollLines(3),
  leftHanded(false),
  oldLeftHanded(false),
  singleClick(false),
  oldSingleClick(false) {

  ui.setupUi(this);

  /* read the config flie */
  loadSettings();
  initControls();

  connect(ui.mouseLeftHanded, &QAbstractButton::clicked, this, &MouseConfig::settingsChanged);
  connect(ui.doubleClickInterval, QOverload<int>::of(&QSpinBox::valueChanged), this, &MouseConfig::settingsChanged);
  connect(ui.wheelScrollLines, QOverload<int>::of(&QSpinBox::valueChanged), this, &MouseConfig::settingsChanged);
  connect(ui.singleClick, &QAbstractButton::clicked, this, &MouseConfig::settingsChanged);
}

MouseConfig::~MouseConfig() {
}

void MouseConfig::initControls() {
  ui.mouseLeftHanded->setChecked(leftHanded);

  ui.singleClick->setChecked(singleClick);

  ui.doubleClickInterval->blockSignals(true);
  ui.doubleClickInterval->setValue(doubleClickInterval);
  ui.doubleClickInterval->blockSignals(false);

  ui.wheelScrollLines->blockSignals(true);
  ui.wheelScrollLines->setValue(wheelScrollLines);
  ui.wheelScrollLines->blockSignals(false);
}

/* This function is taken from Gnome's control-center 2.6.0.3 (gnome-settings-mouse.c) and was modified*/
#define DEFAULT_PTR_MAP_SIZE 128
void MouseConfig::setLeftHandedMouse() {
  unsigned char* buttons;
  unsigned char* more_buttons;
  int n_buttons, i;
  int idx_1 = 0, idx_3 = 1;

  buttons = (unsigned char*)malloc(DEFAULT_PTR_MAP_SIZE);
  if(!buttons) {
    return;
  }
  n_buttons = XGetPointerMapping(QX11Info::display(), buttons, DEFAULT_PTR_MAP_SIZE);

  if(n_buttons > DEFAULT_PTR_MAP_SIZE) {
    more_buttons = (unsigned char*)realloc(buttons, n_buttons);
    if(!more_buttons) {
      free(buttons);
      return;
    }
    buttons = more_buttons;
    n_buttons = XGetPointerMapping(QX11Info::display(), buttons, n_buttons);
  }

  for(i = 0; i < n_buttons; i++) {
    if(buttons[i] == 1)
      idx_1 = i;
    else if(buttons[i] == ((n_buttons < 3) ? 2 : 3))
      idx_3 = i;
  }

  if((leftHanded && idx_1 < idx_3) ||
      (!leftHanded && idx_1 > idx_3)) {
    buttons[idx_1] = ((n_buttons < 3) ? 2 : 3);
    buttons[idx_3] = 1;
    XSetPointerMapping(QX11Info::display(), buttons, n_buttons);
  }
  free(buttons);
}

void MouseConfig::applyConfig()
{
  bool acceptSetting = false;
  bool applyX11 = false;

  if(leftHanded != ui.mouseLeftHanded->isChecked())
  {
    leftHanded = ui.mouseLeftHanded->isChecked();
    setLeftHandedMouse();
    acceptSetting = true;
  }

  if(doubleClickInterval != ui.doubleClickInterval->value())
  {
    doubleClickInterval = ui.doubleClickInterval->value();
    acceptSetting = applyX11 = true;
  }

  if(wheelScrollLines != ui.wheelScrollLines->value())
  {
    wheelScrollLines = ui.wheelScrollLines->value();
    acceptSetting = applyX11 = true;
  }

  if(singleClick != ui.singleClick->isChecked())
  {
    singleClick = ui.singleClick->isChecked();
    acceptSetting = applyX11 = true;
  }

  if(acceptSetting)
    accept();

#ifdef Q_WS_X11
  if(applyX11)
  {
    qtSettings->sync();
    qt_x11_apply_settings_in_all_apps();
  }
#endif
}

void MouseConfig::loadSettings() {
  oldSingleClick = singleClick = qtSettings->value(QLatin1String("single_click_activate"), false).toBool();

  qtSettings->beginGroup(QLatin1String("Qt"));
  oldDoubleClickInterval = doubleClickInterval = qtSettings->value(QLatin1String("doubleClickInterval"), 400).toInt();
  oldWheelScrollLines = wheelScrollLines = qtSettings->value(QLatin1String("wheelScrollLines"), 3).toInt();
  qtSettings->endGroup();

  settings->beginGroup(QLatin1String("Mouse"));
  oldAccel = accel = settings->value(QLatin1String("accel_factor"), 20).toInt();
  oldThreshold = threshold = settings->value(QLatin1String("accel_threshold"), 10).toInt();
  oldLeftHanded = leftHanded = settings->value(QLatin1String("left_handed"), false).toBool();
  settings->endGroup();
}

void MouseConfig::accept() {
  qtSettings->setValue(QStringLiteral("single_click_activate"), singleClick);

  qtSettings->beginGroup(QLatin1String("Qt"));
  qtSettings->setValue(QLatin1String("doubleClickInterval"), doubleClickInterval);
  qtSettings->setValue(QLatin1String("wheelScrollLines"), wheelScrollLines);
  qtSettings->endGroup();

  settings->beginGroup(QStringLiteral("Mouse"));
  settings->setValue(QStringLiteral("accel_factor"), accel);
  settings->setValue(QStringLiteral("accel_threshold"), threshold);
  settings->setValue(QStringLiteral("left_handed"), leftHanded);
  settings->endGroup();
}

void MouseConfig::reset() {
  /* restore to original settings */
  /* mouse */
  accel = oldAccel;
  threshold = oldThreshold;
  leftHanded = oldLeftHanded;
  singleClick = oldSingleClick;
  doubleClickInterval = oldDoubleClickInterval;
  wheelScrollLines = oldWheelScrollLines;
  XChangePointerControl(QX11Info::display(), True, True,
                        accel, 10, threshold);
  setLeftHandedMouse();

  initControls();
  accept();
}
