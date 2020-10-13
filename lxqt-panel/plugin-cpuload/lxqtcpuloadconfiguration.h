/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
 * Authors:
 *   Maciej Płaza <plaza.maciej@gmail.com>
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


#ifndef LXQTCPULOADCONFIGURATION_H
#define LXQTCPULOADCONFIGURATION_H

#include "../panel/lxqtpanelpluginconfigdialog.h"
#include "../panel/pluginsettings.h"

class QSettings;
class QAbstractButton;

namespace Ui {
    class LXQtCpuLoadConfiguration;
}

class LXQtCpuLoadConfiguration : public LXQtPanelPluginConfigDialog
{
    Q_OBJECT

public:
    explicit LXQtCpuLoadConfiguration(PluginSettings *settings, QWidget *parent = nullptr);
    ~LXQtCpuLoadConfiguration();

private:
    Ui::LXQtCpuLoadConfiguration *ui;

    /*
      Fills Bar orientation combobox
    */
    void fillBarOrientations();

private slots:
    /*
      Saves settings in conf file.
    */
    void loadSettings();
    void showTextChanged(bool value);
    void barWidthChanged(int value);
    void updateIntervalChanged(double value);
    void barOrientationChanged(int index);

};

#endif // LXQTCPULOADCONFIGURATION_H
