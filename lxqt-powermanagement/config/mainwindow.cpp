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

#include "mainwindow.h"
#include "lidwatchersettings.h"
#include "batterywatchersettings.h"
#include "idlenesswatchersettings.h"
#include "powerkeyssettings.h"

#include <LXQt/Globals>

MainWindow::MainWindow(QWidget *parent) :
    LXQt::ConfigDialog(tr("Power Management Settings"), new PowerManagementSettings(parent))
{
    BatteryWatcherSettings* batteryWatcherSettings = new BatteryWatcherSettings(this);
    addPage(batteryWatcherSettings, tr("Battery"), QSL("battery"));
    connect(this, SIGNAL(reset()), batteryWatcherSettings, SLOT(loadSettings()));

    LidWatcherSettings *lidwatcherSettings = new LidWatcherSettings(this);
    addPage(lidwatcherSettings, tr("Lid"), QSL("laptop-lid"));
    connect(this, SIGNAL(reset()), lidwatcherSettings, SLOT(loadSettings()));

    IdlenessWatcherSettings* idlenessWatcherSettings = new IdlenessWatcherSettings(this);
    addPage(idlenessWatcherSettings, tr("Idle"), (QStringList() << QSL("user-idle") << QSL("user-away")));
    connect(this, SIGNAL(reset()), idlenessWatcherSettings, SLOT(loadSettings()));

    PowerKeysSettings* powerKeysSettings = new PowerKeysSettings(this);
    addPage(powerKeysSettings, tr("Power keys"), QSL("system-shutdown"));
    connect(this, SIGNAL(reset()), powerKeysSettings, SLOT(loadSettings()));

    emit reset();
}
