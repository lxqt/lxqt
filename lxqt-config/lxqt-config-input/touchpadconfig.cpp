/*
    Copyright (C) 2016-2018 Chih-Hsuan Yen <yan12125@gmail.com>

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

#include "touchpadconfig.h"
#include "touchpaddevice.h"

#include <cmath>
#include <QUrl>
#include <LXQt/AutostartEntry>
#include <LXQt/Settings>

TouchpadConfig::TouchpadConfig(LXQt::Settings* _settings, QWidget* parent):
    QWidget(parent),
    settings(_settings)
{
    ui.setupUi(this);

    devices = TouchpadDevice::enumerate_from_udev();
    for (const TouchpadDevice& device : qAsConst(devices))
    {
        ui.devicesComboBox->addItem(device.name());
    }

    initControls();

    connect(ui.devicesComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this] (int/* index*/) { initControls(); }); // update GUI on device change
    connect(ui.tappingEnabledCheckBox, &QAbstractButton::clicked, this, &TouchpadConfig::settingsChanged);
    connect(ui.naturalScrollingEnabledCheckBox, &QAbstractButton::clicked, this, &TouchpadConfig::settingsChanged);
    connect(ui.tapToDragEnabledCheckBox, &QAbstractButton::clicked, this, &TouchpadConfig::settingsChanged);
    connect(ui.accelSpeedDoubleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TouchpadConfig::settingsChanged);
    connect(ui.noScrollingRadioButton, &QAbstractButton::clicked, this, &TouchpadConfig::settingsChanged);
    connect(ui.twoFingerScrollingRadioButton, &QAbstractButton::clicked, this, &TouchpadConfig::settingsChanged);
    connect(ui.edgeScrollingRadioButton, &QAbstractButton::clicked, this, &TouchpadConfig::settingsChanged);
    connect(ui.buttonScrollingRadioButton, &QAbstractButton::clicked, this, &TouchpadConfig::settingsChanged);
}

TouchpadConfig::~TouchpadConfig()
{
}

void TouchpadConfig::initFeatureControl(QCheckBox* control, int featureEnabled)
{
    if (featureEnabled >= 0)
    {
        control->setEnabled(true);
        control->setCheckState(featureEnabled ? Qt::Checked : Qt::Unchecked);
    }
    else
    {
        control->setEnabled(false);
    }
}

void TouchpadConfig::initControls()
{
    int curDevice = ui.devicesComboBox->currentIndex();
    if (curDevice < 0) {
        return;
    }

    const TouchpadDevice& device = devices[curDevice];
    initFeatureControl(ui.tappingEnabledCheckBox, device.tappingEnabled());
    initFeatureControl(ui.naturalScrollingEnabledCheckBox, device.naturalScrollingEnabled());
    initFeatureControl(ui.tapToDragEnabledCheckBox, device.tapToDragEnabled());

    float accelSpeed = device.accelSpeed();
    if (!std::isnan(accelSpeed)) {
        ui.accelSpeedDoubleSpinBox->setEnabled(true);
        // prevent setAccelSpeed() from being called because the device may have changed
        ui.accelSpeedDoubleSpinBox->blockSignals(true);
        ui.accelSpeedDoubleSpinBox->setValue(accelSpeed);
        ui.accelSpeedDoubleSpinBox->blockSignals(false);
    } else {
        ui.accelSpeedDoubleSpinBox->setEnabled(false);
    }

    int scrollMethodsAvailable = device.scrollMethodsAvailable();
    ui.twoFingerScrollingRadioButton->setEnabled(scrollMethodsAvailable & TWO_FINGER);
    ui.edgeScrollingRadioButton->setEnabled(scrollMethodsAvailable & EDGE);
    ui.buttonScrollingRadioButton->setEnabled(scrollMethodsAvailable & BUTTON);

    ScrollingMethod scrollingMethodEnabled = device.scrollingMethodEnabled();
    if (scrollingMethodEnabled == TWO_FINGER)
    {
        ui.twoFingerScrollingRadioButton->setChecked(true);
    }
    else if (scrollingMethodEnabled == EDGE)
    {
        ui.edgeScrollingRadioButton->setChecked(true);
    }
    else if (scrollingMethodEnabled == BUTTON)
    {
        ui.buttonScrollingRadioButton->setChecked(true);
    }
    else
    {
        ui.noScrollingRadioButton->setChecked(true);
    }
}

void TouchpadConfig::accept()
{
    for (const TouchpadDevice& device : qAsConst(devices))
    {
        device.saveSettings(settings);
    }

    LXQt::AutostartEntry autoStart(QStringLiteral("lxqt-config-touchpad-autostart.desktop"));
    XdgDesktopFile desktopFile(XdgDesktopFile::ApplicationType, QStringLiteral("lxqt-config-touchpad-autostart"), QStringLiteral("lxqt-config-input --load-touchpad"));
    desktopFile.setValue(QStringLiteral("OnlyShowIn"), QStringLiteral("LXQt"));
    desktopFile.setValue(QStringLiteral("Comment"), QStringLiteral("Autostart touchpad settings for lxqt-config-input"));
    autoStart.setFile(desktopFile);
    autoStart.commit();
}

void TouchpadConfig::reset()
{
    for (const TouchpadDevice& device : qAsConst(devices))
    {
        device.setTappingEnabled(device.oldTappingEnabled());
        device.setNaturalScrollingEnabled(device.oldNaturalScrollingEnabled());
        device.setTapToDragEnabled(device.oldTapToDragEnabled());
        device.setAccelSpeed(device.oldAccelSpeed());
        device.setScrollingMethodEnabled(device.oldScrollingMethodEnabled());
    }
    initControls();
    accept();
}

void TouchpadConfig::applyConfig()
{
    int curDevice = ui.devicesComboBox->currentIndex();
    if (curDevice < 0) {
        return;
    }

    bool acceptSetting = false;
    TouchpadDevice& device = devices[curDevice];

    bool enable = ui.tappingEnabledCheckBox->checkState() == Qt::Checked;
    if (enable != (device.tappingEnabled() > 0))
    {
        device.setTappingEnabled(enable);
        acceptSetting = true;
    }

    enable = ui.naturalScrollingEnabledCheckBox->checkState() == Qt::Checked;
    if (enable != (device.naturalScrollingEnabled() > 0))
    {
        device.setNaturalScrollingEnabled(enable);
        acceptSetting = true;
    }

    enable = ui.tapToDragEnabledCheckBox->checkState() == Qt::Checked;
    if (enable != (device.tapToDragEnabled() > 0))
    {
        device.setTapToDragEnabled(enable);
        acceptSetting = true;
    }

    float accelSpeed = static_cast<float>(ui.accelSpeedDoubleSpinBox->value());
    if (accelSpeed != device.accelSpeed())
    {
        device.setAccelSpeed(accelSpeed);
        acceptSetting = true;
    }

    // these radio buttons are auto-exclusive
    ScrollingMethod m = NONE;
    if (ui.noScrollingRadioButton->isChecked())
        m = NONE;
    else if (ui.twoFingerScrollingRadioButton->isChecked())
        m = TWO_FINGER;
    else if (ui.edgeScrollingRadioButton->isChecked())
        m = EDGE;
    else if (ui.buttonScrollingRadioButton->isChecked())
        m = BUTTON;
    if (m != device.scrollingMethodEnabled())
    {
        device.setScrollingMethodEnabled(m);
        acceptSetting = true;
    }

    if (acceptSetting)
        accept();
}
