/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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

#ifndef LXQT_PLUGIN_MOUNT_CONFIGURATION_H
#define LXQT_PLUGIN_MOUNT_CONFIGURATION_H

#include "../panel/lxqtpanelpluginconfigdialog.h"

#define CFG_KEY_ACTION  "newDeviceAction"
#define ACT_SHOW_MENU   "showMenu"
#define ACT_SHOW_INFO   "showInfo"
#define ACT_NOTHING     "nothing"

namespace Ui {
    class Configuration;
}

class Configuration : public LXQtPanelPluginConfigDialog
{
    Q_OBJECT

public:
    explicit Configuration(PluginSettings *settings, QWidget *parent = nullptr);
    ~Configuration();

protected slots:
    virtual void loadSettings();
    void devAddedChanged(int index);

private:
    Ui::Configuration *ui;
};

#endif // LXQTMOUNTCONFIGURATION_H
