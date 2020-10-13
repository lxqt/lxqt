/*
    Copyright (C) 2015  P.L. Lucas <selairi@gmail.com>

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

#ifndef __LOADSETTINGS_H__
#define __LOADSETTINGS_H__


#include <KScreen/GetConfigOperation>
#include <KScreen/SetConfigOperation>
#include <LXQt/Notification>
#include "monitor.h"

class LoadSettings : public QObject
{
    Q_OBJECT

public:
    LoadSettings(QObject *parent = nullptr);
    
    void applyBestSettings();

private:
    QList<MonitorSettings> loadCurrentConfiguration();
    QList<MonitorSettings> loadConfiguration(QString scope);
    LXQt::Notification *mNotification;

    // Configurations
    KScreen::ConfigPtr mConfig;
};

/*! Apply settings.
 */
bool applySettings(KScreen::ConfigPtr config, QList<MonitorSettings> monitors);


#endif // __LOADSETTINGS_H__
