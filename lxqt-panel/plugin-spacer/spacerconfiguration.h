/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
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


#ifndef SPACERCONFIGURATION_H
#define SPACERCONFIGURATION_H

#include "../panel/lxqtpanelpluginconfigdialog.h"
#include "../panel/pluginsettings.h"

class QAbstractButton;

namespace Ui {
    class SpacerConfiguration;
}

class SpacerConfiguration : public LXQtPanelPluginConfigDialog
{
    Q_OBJECT

public:
    explicit SpacerConfiguration(PluginSettings *settings, QWidget *parent = nullptr);
    ~SpacerConfiguration();

public:
    static const QStringList msTypes;

private:
    Ui::SpacerConfiguration *ui;

private slots:
    /*
       Saves settings in conf file.
    */
    void loadSettings();
    void sizeChanged(int value);
    void typeChanged(int index);
    void widthTypeChanged(bool expandableChecked);
};

#endif // SPACERCONFIGURATION_H
