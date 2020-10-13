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

#ifndef LXQT_PLUGIN_MOUNT_DEVICEACTION_H
#define LXQT_PLUGIN_MOUNT_DEVICEACTION_H

#include <QObject>
#include <QSettings>
#include <Solid/Device>

class LXQtMountPlugin;

class DeviceAction: public QObject
{
    Q_OBJECT

public:
    enum ActionId
    {
        ActionNothing,
        ActionInfo,
        ActionMenu
    };

    virtual ~DeviceAction();
    virtual ActionId Type() const throw () = 0;

    static DeviceAction *create(ActionId id, LXQtMountPlugin *plugin, QObject *parent = nullptr);
    static ActionId stringToActionId(const QString &string, ActionId defaultValue);
    static QString actionIdToString(ActionId id);

public slots:
    void onDeviceAdded(Solid::Device device);
    void onDeviceRemoved(Solid::Device device);

protected:
    explicit DeviceAction(LXQtMountPlugin *plugin, QObject *parent = nullptr);
    virtual void doDeviceAdded(Solid::Device device) = 0;
    virtual void doDeviceRemoved(Solid::Device device) = 0;

    LXQtMountPlugin *mPlugin;
    QMap<QString/*!< device udi*/, QString/*!< device description*/> mKnownDeviceDescriptions;
};

#endif // DEVICEACTION_H
