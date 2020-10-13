/*
    Copyright (C) 2016  P.L. Lucas <selairi@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __BRIGHTNESS_SETTINGS_H__
#define __BRIGHTNESS_SETTINGS_H__

#include <QDialog>
#include <QTimer>
#include "xrandrbrightness.h"
#include "ui_brightnesssettings.h"

#include <LXQt/lxqtbacklight.h>

class BrightnessSettings: public QDialog
{
Q_OBJECT

public:
    BrightnessSettings(QWidget *parent =0);
    ~BrightnessSettings();

    void revertValues();

signals:
    void monitorReverted(const MonitorInfo & monitor);

public slots:
    void monitorSettingsChanged(MonitorInfo monitor);
    void requestConfirmation();
    void setBacklight();

private:
    void setBacklightSliderValue(int value);

    XRandrBrightness *mBrightness;
    QList<MonitorInfo> mMonitors;
    QList<MonitorInfo> mMonitorsInitial;
    QTimer mConfirmRequestTimer;
    Ui::BrightnessSettings *ui;
    LXQt::Backlight *mBacklight;
    int mLastBacklightValue;
    int mInitialBacklightValue;

};


#endif

