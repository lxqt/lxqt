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

#include "monitor.h"

bool MonitorSavedSettings::operator==(const MonitorSavedSettings &obj) const
{
    if(name != obj.name)
        return false;
    if(date != obj.date)
        return false;
    // TODO: Check QList<MonitorSettings> monitors.
    return true;
}

void saveMonitorSettings(QSettings & settings, const QList<MonitorSettings> monitors)
{
    settings.remove(QStringLiteral("settings"));
    settings.beginWriteArray(QStringLiteral("settings"));
    int i = 0;
    for(const MonitorSettings& monitor : monitors) {
        settings.setArrayIndex(i++);
        saveMonitorSettings(settings, monitor);
    }
    settings.endArray();
}

void saveMonitorSettings(QSettings &settings, const MonitorSettings &monitor)
{
    settings.setValue(QStringLiteral("name"), monitor.name);
    settings.setValue(QStringLiteral("hash"), monitor.hash);
    settings.setValue(QStringLiteral("connected"), monitor.connected);
    if(monitor.connected) {
        settings.setValue(QStringLiteral("enabled"), monitor.enabled);
        settings.setValue(QStringLiteral("primary"), monitor.primary);
        settings.setValue(QStringLiteral("xPos"), monitor.xPos);
        settings.setValue(QStringLiteral("yPos"), monitor.yPos);
        settings.setValue(QStringLiteral("currentMode"), monitor.currentMode);
        settings.setValue(QStringLiteral("currentModeWidth"), monitor.currentModeWidth);
        settings.setValue(QStringLiteral("currentModeHeight"), monitor.currentModeHeight);
        settings.setValue(QStringLiteral("currentModeRate"), monitor.currentModeRate);
        settings.setValue(QStringLiteral("rotation"), monitor.rotation);
    }
}

void loadMonitorSettings(QSettings & settings, QList<MonitorSettings> &monitors)
{
    int size = settings.beginReadArray(QStringLiteral("settings"));
    for(int i=0; i<size; i++) {
        settings.setArrayIndex(i);
        MonitorSettings monitor;
        loadMonitorSettings(settings, monitor);
        monitors.append(monitor);
    }
    settings.endArray();
}

void loadMonitorSettings(QSettings &settings, MonitorSettings &monitor)
{
    monitor.name = settings.value(QStringLiteral("name")).toString();
    monitor.hash = settings.value(QStringLiteral("hash")).toString();
    monitor.connected = settings.value(QStringLiteral("connected")).toBool();
    if(monitor.connected) {
        monitor.enabled = settings.value(QStringLiteral("enabled")).toBool();
        monitor.primary = settings.value(QStringLiteral("primary")).toBool();
        monitor.xPos = settings.value(QStringLiteral("xPos")).toInt();
        monitor.yPos = settings.value(QStringLiteral("yPos")).toInt();
        monitor.currentMode = settings.value(QStringLiteral("currentMode")).toString();
        monitor.currentModeWidth = settings.value(QStringLiteral("currentModeWidth")).toInt();
        monitor.currentModeHeight = settings.value(QStringLiteral("currentModeHeight")).toInt();
        monitor.currentModeRate = settings.value(QStringLiteral("currentModeRate")).toFloat();
        monitor.rotation = settings.value(QStringLiteral("rotation")).toInt();
    }
}

void saveMonitorSettings(QSettings & settings, QList<MonitorSavedSettings> monitors)
{
    settings.remove(QStringLiteral("SavedSettings"));
    settings.beginWriteArray(QStringLiteral("SavedSettings"));
    int i = 0;
    for(MonitorSavedSettings& monitor : monitors) {
        settings.setArrayIndex(i++);
        saveMonitorSettings(settings, monitor);
    }
    settings.endArray();
}

void saveMonitorSettings(QSettings &settings, MonitorSavedSettings &monitor)
{
    settings.setValue(QStringLiteral("name"), monitor.name);
    settings.setValue(QStringLiteral("date"), monitor.date);
    saveMonitorSettings(settings, monitor.monitors);
}

void loadMonitorSettings(QSettings & settings, QList<MonitorSavedSettings> &monitors)
{
    int size = settings.beginReadArray(QStringLiteral("SavedSettings"));
    for(int i=0; i<size; i++) {
        settings.setArrayIndex(i);
        MonitorSavedSettings monitor;
        loadMonitorSettings(settings, monitor);
        monitors.append(monitor);
    }
    settings.endArray();
}

void loadMonitorSettings(QSettings &settings, MonitorSavedSettings &monitor)
{
    monitor.name = settings.value(QStringLiteral("name")).toString();
    monitor.date = settings.value(QStringLiteral("date")).toString();
    loadMonitorSettings(settings, monitor.monitors);
}
