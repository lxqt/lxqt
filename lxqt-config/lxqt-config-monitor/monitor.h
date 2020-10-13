/*
    Copyright (C) 2014  P.L. Lucas <selairi@gmail.com>

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

#ifndef _MONITOR_H_
#define _MONITOR_H_

#include <QSettings>
#include <QString>
#include <QList>

//Settings to be stored or read from settings file.
struct MonitorSettings {
    MonitorSettings()
        : name(),
          hash(),
          connected(false),
          enabled(false),
          primary(false),
          currentMode(),
          currentModeWidth(-1),
          currentModeHeight(-1),
          currentModeRate(0.0f),
          xPos(-1),
          yPos(-1),
          rotation(-1)
    {
    }

    QString name;
    QString hash;
    bool connected;
    bool enabled;
    bool primary;
    QString currentMode;
    int currentModeWidth;
    int currentModeHeight;
    float currentModeRate;
    int xPos;
    int yPos;
    int rotation;
};

struct MonitorSavedSettings {
    QString name;
    QString date;
    QList<MonitorSettings> monitors;
    
    bool operator==(const MonitorSavedSettings &obj) const;
};

/**This function saves a list of MonitorSettings in QSettings file.
 * Before using this function, QSettings group must be opened.
 */
void saveMonitorSettings(QSettings &settings, const QList<MonitorSettings> monitors);
void saveMonitorSettings(QSettings &settings, const MonitorSettings &monitor);

/**This function loads a list of MonitorSettings from QSettings file.
 * Before using this function, QSettings group must be opened.
 */
void loadMonitorSettings(QSettings &settings, QList<MonitorSettings> &monitors);
void loadMonitorSettings(QSettings &settings, MonitorSettings &monitor);

/**This function saves a list of MonitorSavedSettings in QSettings file.
 * Before using this function, QSettings group must be opened.
 */
void saveMonitorSettings(QSettings &settings, QList<MonitorSavedSettings> monitors);
void saveMonitorSettings(QSettings &settings, MonitorSavedSettings &monitor);

/**This function loads a list of MonitorSavedSettings in QSettings file.
 * Before using this function, QSettings group must be opened.
 */
void loadMonitorSettings(QSettings &settings, QList<MonitorSavedSettings> &monitors);
void loadMonitorSettings(QSettings &settings, MonitorSavedSettings &monitor);

#endif // _MONITOR_H_
