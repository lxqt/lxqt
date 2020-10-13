/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright (C) 2013  Alec Moskvin <alecm@gmx.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef IDLENESSWATCHER_H
#define IDLENESSWATCHER_H

#include <LXQt/lxqtbacklight.h>
#include "../config/powermanagementsettings.h"
#include "watcher.h"

class IdlenessWatcher : public Watcher
{
    Q_OBJECT

public:
    explicit IdlenessWatcher(QObject* parent = nullptr);
    ~IdlenessWatcher() override;

private Q_SLOTS:
    void setup();
    void timeoutReached(int identifier);
    void resumingFromIdle();
    void onBatteryChanged(int newState, const QString &udi = QString());
    void onSettingsChanged();

private:
    PowerManagementSettings mPSettings;
    int mIdleWatcher;
    int mIdleBacklightWatcher;
    LXQt::Backlight *mBacklight;
    int mBacklightActualValue;
    bool mDischarging;
};

#endif // IDLENESSWATCHER_H
