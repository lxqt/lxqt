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
#ifndef IDLE_SETTINGS_H
#define IDLE_SETTINGS_H

#include <QGroupBox>
#include <LXQt/Settings>
#include <LXQt/lxqtbacklight.h>

#include "powermanagementsettings.h"

namespace Ui {
    class IdlenessWatcherSettings;
}

class IdlenessWatcherSettings : public QWidget
{
    Q_OBJECT

public:
    explicit IdlenessWatcherSettings(QWidget *parent = nullptr);
    ~IdlenessWatcherSettings() override;

public Q_SLOTS:
    void loadSettings();

private Q_SLOTS:
    void minutesChanged(int newVal);
    void secondsChanged(int newVal);
    void saveSettings();
    void backlightCheckButtonPressed();
    void backlightCheckButtonReleased();

private:
    PowerManagementSettings mSettings;
    Ui::IdlenessWatcherSettings *mUi;
    LXQt::Backlight *mBacklight;
    int mBacklightActualValue;
    
    void mConnectSignals();
    void mDisconnectSignals();
};

#endif // IDLE_SETTINGS_H
