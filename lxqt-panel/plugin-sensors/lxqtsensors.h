/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Łukasz Twarduś <ltwardus@gmail.com>
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
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef LXQTSENSORS_H
#define LXQTSENSORS_H

#include "sensors.h"
#include "../panel/pluginsettings.h"
#include <QFrame>
#include <QProgressBar>
#include <QSet>
#include <QTimer>


class ProgressBar: public QProgressBar
{
    Q_OBJECT
public:
    ProgressBar(QWidget *parent = nullptr);

    QSize sizeHint() const;
    void setSensorColor(const QString &colorName);
};


class QSettings;
class ILXQtPanelPlugin;
class QBoxLayout;

class LXQtSensors : public QFrame
{
    Q_OBJECT
public:
    LXQtSensors(ILXQtPanelPlugin *plugin, QWidget* parent = nullptr);
    ~LXQtSensors();

    void settingsChanged();
    void realign();
public slots:
    void updateSensorReadings();
    void warningAboutHighTemperature();

private:
    ILXQtPanelPlugin *mPlugin;
    QBoxLayout *mLayout;
    QTimer mUpdateSensorReadingsTimer;
    QTimer mWarningAboutHighTemperatureTimer;
    Sensors mSensors;
    QList<Chip> mDetectedChips;
    QList<ProgressBar*> mTemperatureProgressBars;
    // With set we can handle updates in very easy way :)
    QSet<ProgressBar*> mHighTemperatureProgressBars;
    double celsiusToFahrenheit(double celsius);
    void initDefaultSettings();
    PluginSettings *mSettings;
};


#endif // LXQTSENSORS_H
