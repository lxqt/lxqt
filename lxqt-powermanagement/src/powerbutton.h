/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2020 LXQt team
 * Authors:
 *   Pedro L. Lucas <selairi@gmail.com>
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

#ifndef POWERBUTTON_H
#define POWERBUTTON_H

#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include "../config/powermanagementsettings.h"
#include "watcher.h"

namespace GlobalKeyShortcut
{
class Action;
}


class PowerButton : public Watcher
{
    Q_OBJECT

public:
    PowerButton(QObject *parent);
    ~PowerButton();

private Q_SLOTS:
    void handleShortcutPoweroff();
    void handleShortcutSuspend();
    void handleShortcutHibernate();
    
private:
    QDBusInterface *mLogindInterface;
    QDBusInterface *mLogindPropertiesInterface;
    QDBusReply<QDBusUnixFileDescriptor> mFdPower, mFdSuspend, mFdHibernate;
    GlobalKeyShortcut::Action *mKeyPowerButton;
    GlobalKeyShortcut::Action *mKeySuspendButton;
    GlobalKeyShortcut::Action *mKeyHibernateButton;
    
    void runAction(int action);
};

#endif // LID_H
