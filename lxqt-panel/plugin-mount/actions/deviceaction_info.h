/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
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

#ifndef LXQT_PLUGIN_MOUNT_DEVICEACTION_INFO_H
#define LXQT_PLUGIN_MOUNT_DEVICEACTION_INFO_H

#include "deviceaction.h"

#include <QWidget>
#include <QTimer>

class Popup;

class DeviceActionInfo : public DeviceAction
{
    Q_OBJECT
public:
    explicit DeviceActionInfo(LXQtMountPlugin *plugin, QObject *parent = nullptr);
    virtual ActionId Type() const throw () { return ActionInfo; }

protected:
    void doDeviceAdded(Solid::Device device);
    void doDeviceRemoved(Solid::Device device);

private:
    void showMessage(const QString &text);
};

#endif // DEVICEACTION_INFO_H
