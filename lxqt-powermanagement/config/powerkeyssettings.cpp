/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2020 LXQt team
 * Authors:
 *   Pedro L. Lucas <selairi@gmail.com>
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

#include "powerkeyssettings.h"
#include "ui_powerkeyssettings.h"
#include "helpers.h"

PowerKeysSettings::PowerKeysSettings(QWidget *parent) :
    QWidget(parent),
    mSettings(),
    mUi(new Ui::PowerKeysSettings)
{
    mUi->setupUi(this);
    fillComboBox(mUi->powerKeyActionComboBox);
    fillComboBox(mUi->suspendKeyActionComboBox);
    fillComboBox(mUi->hibernateKeyActionComboBox);

    connect(mUi->powerKeyActionComboBox, SIGNAL(activated(int)), this, SLOT(saveSettings()));
    connect(mUi->suspendKeyActionComboBox, SIGNAL(activated(int)), this, SLOT(saveSettings()));
    connect(mUi->hibernateKeyActionComboBox, SIGNAL(activated(int)), this, SLOT(saveSettings()));
}


PowerKeysSettings::~PowerKeysSettings()
{
}


void PowerKeysSettings::loadSettings()
{
    setComboBoxToValue(mUi->powerKeyActionComboBox, mSettings.getPowerKeyAction());
    setComboBoxToValue(mUi->suspendKeyActionComboBox, mSettings.getSuspendKeyAction());
    setComboBoxToValue(mUi->hibernateKeyActionComboBox, mSettings.getHibernateKeyAction());
}


void PowerKeysSettings::saveSettings()
{
    mSettings.setPowerKeyAction(currentValue(mUi->powerKeyActionComboBox));
    mSettings.setSuspendKeyAction(currentValue(mUi->suspendKeyActionComboBox));
    mSettings.setHibernateKeyAction(currentValue(mUi->hibernateKeyActionComboBox));
}