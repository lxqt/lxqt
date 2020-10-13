/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Christian Surlykke <christian@surlykke.dk>
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

#include <QComboBox>
#include <QDebug>

#include "idlenesswatchersettings.h"
#include "ui_idlenesswatchersettings.h"
#include "helpers.h"

#define MINIMUM_SECONDS 30

IdlenessWatcherSettings::IdlenessWatcherSettings(QWidget *parent) :
    QWidget(parent),
    mSettings(),
    mUi(new Ui::IdlenessWatcherSettings)
{
    mUi->setupUi(this);
    fillComboBox(mUi->idleActionComboBox);
    
    mBacklight = new LXQt::Backlight(this);
    // If if no backlight support then disable backlight control:
    if(! mBacklight->isBacklightAvailable()) {
        mUi->idlenessBacklightWatcherGroupBox->setEnabled(false);
        mUi->idlenessBacklightWatcherGroupBox->setChecked(false);
    } else {
        mBacklightActualValue = mBacklight->getBacklight();
    }
    
    mConnectSignals();
}


void IdlenessWatcherSettings::mConnectSignals()
{
    connect(mUi->idlenessWatcherGroupBox, SIGNAL(clicked()), SLOT(saveSettings()));
    connect(mUi->idleActionComboBox, SIGNAL(activated(int)), SLOT(saveSettings()));
    connect(mUi->idleTimeMinutesSpinBox, SIGNAL(editingFinished()), SLOT(saveSettings()));
    connect(mUi->idleTimeMinutesSpinBox, SIGNAL(valueChanged(int)), SLOT(minutesChanged(int)));
    connect(mUi->idleTimeSecondsSpinBox, SIGNAL(valueChanged(int)), SLOT(secondsChanged(int)));

    connect(mUi->idleTimeSecondsSpinBox, SIGNAL(editingFinished()), SLOT(saveSettings()));
    
    connect(mUi->checkBacklightButton, SIGNAL(pressed()), SLOT(backlightCheckButtonPressed()));
    connect(mUi->checkBacklightButton, SIGNAL(released()), SLOT(backlightCheckButtonReleased()));
    
    connect(mUi->idlenessBacklightWatcherGroupBox, SIGNAL(clicked()), SLOT(saveSettings()));
    connect(mUi->backlightSlider, SIGNAL(valueChanged(int)), SLOT(saveSettings()));
    connect(mUi->idleTimeBacklightTimeEdit, SIGNAL(timeChanged(const QTime &)), SLOT(saveSettings()));
    connect(mUi->onBatteryDischargingCheckBox, SIGNAL(toggled(bool)), SLOT(saveSettings()));
}

void IdlenessWatcherSettings::mDisconnectSignals()
{
    disconnect(mUi->idlenessWatcherGroupBox, SIGNAL(clicked()), this, SLOT(saveSettings()));
    disconnect(mUi->idleActionComboBox, SIGNAL(activated(int)), this, SLOT(saveSettings()));
    disconnect(mUi->idleTimeMinutesSpinBox, SIGNAL(editingFinished()), this, SLOT(saveSettings()));
    disconnect(mUi->idleTimeMinutesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(minutesChanged(int)));
    disconnect(mUi->idleTimeSecondsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(secondsChanged(int)));
    disconnect(mUi->idleTimeSecondsSpinBox, SIGNAL(editingFinished()), this, SLOT(saveSettings()));    
    disconnect(mUi->checkBacklightButton, SIGNAL(pressed()), this, SLOT(backlightCheckButtonPressed()));
    disconnect(mUi->checkBacklightButton, SIGNAL(released()), this, SLOT(backlightCheckButtonReleased()));    
    disconnect(mUi->idlenessBacklightWatcherGroupBox, SIGNAL(clicked()), this, SLOT(saveSettings()));
    disconnect(mUi->backlightSlider, SIGNAL(valueChanged(int)), this, SLOT(saveSettings()));
    disconnect(mUi->idleTimeBacklightTimeEdit, SIGNAL(timeChanged(const QTime &)), this, SLOT(saveSettings()));
    disconnect(mUi->onBatteryDischargingCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveSettings()));
}

IdlenessWatcherSettings::~IdlenessWatcherSettings()
{
    delete mUi;
}

void IdlenessWatcherSettings::loadSettings()
{
    mDisconnectSignals();
    mUi->idlenessWatcherGroupBox->setChecked(mSettings.isIdlenessWatcherEnabled());
    setComboBoxToValue(mUi->idleActionComboBox, mSettings.getIdlenessAction());

    int idlenessTimeSeconds = mSettings.getIdlenessTimeSecs();
    // if less than minimum, change to 15 minutes
    if (idlenessTimeSeconds < MINIMUM_SECONDS)
        idlenessTimeSeconds = 900;
    int idlenessTimeMinutes = idlenessTimeSeconds / 60;
    idlenessTimeSeconds = idlenessTimeSeconds % 60;
    mUi->idleTimeMinutesSpinBox->setValue(idlenessTimeMinutes);
    mUi->idleTimeSecondsSpinBox->setValue(idlenessTimeSeconds);
    
    if(mBacklight->isBacklightAvailable()) {
        mUi->idlenessBacklightWatcherGroupBox->setChecked(mSettings.isIdlenessBacklightWatcherEnabled());
        mUi->idleTimeBacklightTimeEdit->setTime(mSettings.getIdlenessBacklightTime());
        mUi->backlightSlider->setValue(mSettings.getBacklight());
        mUi->onBatteryDischargingCheckBox->setChecked(mSettings.isIdlenessBacklightOnBatteryDischargingEnabled());
    }
    mConnectSignals();
}

void IdlenessWatcherSettings::minutesChanged(int newVal)
{
    if (newVal < 1 && mUi->idleTimeSecondsSpinBox->value() < MINIMUM_SECONDS)
    {
        mUi->idleTimeSecondsSpinBox->setValue(MINIMUM_SECONDS);
    }
}

void IdlenessWatcherSettings::secondsChanged(int newVal)
{
    if (newVal > 59)
    {
        mUi->idleTimeSecondsSpinBox->setValue(0);
        mUi->idleTimeMinutesSpinBox->setValue(mUi->idleTimeMinutesSpinBox->value() + 1);
    }
    else if (mUi->idleTimeMinutesSpinBox->value() < 1 && newVal < MINIMUM_SECONDS)
    {
        mUi->idleTimeMinutesSpinBox->setValue(0);
        mUi->idleTimeSecondsSpinBox->setValue(MINIMUM_SECONDS);
    }
    else if (newVal < 0)
    {
        mUi->idleTimeMinutesSpinBox->setValue(mUi->idleTimeMinutesSpinBox->value() - 1);
        mUi->idleTimeSecondsSpinBox->setValue(59);
    }
}



void IdlenessWatcherSettings::saveSettings()
{
    mSettings.setIdlenessWatcherEnabled(mUi->idlenessWatcherGroupBox->isChecked());
    mSettings.setIdlenessAction(currentValue(mUi->idleActionComboBox));

    int idleTimeSecs = 60 * mUi->idleTimeMinutesSpinBox->value() + mUi->idleTimeSecondsSpinBox->value();
    // if less than minimum, change 15 minutes
    if (idleTimeSecs < MINIMUM_SECONDS)
    {
        idleTimeSecs = 900;
        mUi->idleTimeMinutesSpinBox->setValue(15);
        mUi->idleTimeSecondsSpinBox->setValue(0);
    }

    mSettings.setIdlenessTimeSecs(idleTimeSecs);
    
    mSettings.setIdlenessBacklightWatcherEnabled(mBacklight->isBacklightAvailable() ? mUi->idlenessBacklightWatcherGroupBox->isChecked() : false);
    mSettings.setIdlenessBacklightTime(mUi->idleTimeBacklightTimeEdit->time());
    mSettings.setBacklight(mUi->backlightSlider->value());
    mSettings.setIdlenessBacklightOnBatteryDischargingEnabled(mUi->onBatteryDischargingCheckBox->isChecked());
}

void IdlenessWatcherSettings::backlightCheckButtonPressed()
{
    if(mBacklight->isBacklightAvailable()) {
        mBacklightActualValue = mBacklight->getBacklight();
        mBacklight->setBacklight((float)mBacklightActualValue * (float)(mUi->backlightSlider->value())/100.0f);
    }
}

void IdlenessWatcherSettings::backlightCheckButtonReleased()
{
    if(mBacklight->isBacklightAvailable()) {
        mBacklight->setBacklight(mBacklightActualValue);
    }
}

