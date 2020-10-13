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

#include <QDBusConnection>
#include <QDBusReply>
#include <QObject>
#include <QTimer>
#include <QDebug>

#include "powerbutton.h"

#include <LXQt/Globals>
#include <LXQt/Notification>
#include <LXQt/Power>
#include <lxqt-globalkeys.h>
#include <unistd.h>

PowerButton::PowerButton(QObject *parent) : Watcher(parent)
{

    QDBusInterface manager(QSL("org.freedesktop.login1"),
                           QSL("/org/freedesktop/login1"),
                           QSL("org.freedesktop.login1.Manager"),
                           QDBusConnection::systemBus(), this);


    // QString powerKeyAction = mLogindInterface->property("HandlePowerKey").toString();
    //LXQt::Notification::notify(powerKeyAction, powerKeyAction);

    mFdPower = manager.call(QL1S("Inhibit"), QL1S("handle-power-key"), QL1S("lxqt-powermanager"), 
        QL1S("lxqt-powermanager controls power key"), QL1S("block"));
    QDBusError error = manager.lastError();
    qDebug() << QStringLiteral("PowerButton") << error.name() << error.message() ;
    
    mFdSuspend = manager.call(QL1S("Inhibit"), QL1S("handle-suspend-key"), QL1S("lxqt-powermanager"), 
        QL1S("lxqt-powermanager controls suspend key"), QL1S("block"));
    error = manager.lastError();
    qDebug() << QStringLiteral("SuspendButton") << error.name() << error.message() ;
    
    mFdHibernate = manager.call(QL1S("Inhibit"), QL1S("handle-hibernate-key"), QL1S("lxqt-powermanager"), 
        QL1S("lxqt-powermanager controls hibernate key"), QL1S("block"));
    error = manager.lastError();
    qDebug() << QStringLiteral("HibenateButton") << error.name() << error.message() ;

    //LXQt::Notification::notify(QL1S("powermanager"), QStringLiteral("Fd %1").arg(mFd.value().fileDescriptor()));

    mKeyPowerButton = GlobalKeyShortcut::Client::instance()->addAction(QStringLiteral("XF86PowerOff"), 
        QStringLiteral("/powermanager/keypoweroff"), tr("Power off key action"), this);

    if (mKeyPowerButton) {
        connect(mKeyPowerButton, SIGNAL(activated()), this, SLOT(handleShortcutPoweroff()));
    }
    
    mKeySuspendButton = GlobalKeyShortcut::Client::instance()->addAction(QStringLiteral("XF86Suspend"), 
        QStringLiteral("/powermanager/keysuspend"), tr("Suspend key action"), this);

    if (mKeySuspendButton) {
        connect(mKeySuspendButton, SIGNAL(activated()), this, SLOT(handleShortcutSuspend()));
    }
    
    mKeyHibernateButton = GlobalKeyShortcut::Client::instance()->addAction(QStringLiteral("XF86Sleep"), 
        QStringLiteral("/powermanager/keyhibernate"), tr("Hibernate key action"), this);

    if (mKeyHibernateButton) {
        connect(mKeyHibernateButton, SIGNAL(activated()), this, SLOT(handleShortcutHibernate()));
    }
}


PowerButton::~PowerButton()
{
    close(mFdPower.value().fileDescriptor());
    close(mFdSuspend.value().fileDescriptor());
    close(mFdHibernate.value().fileDescriptor());
}


void PowerButton::handleShortcutPoweroff()
{
    qDebug() << "Power off";
    PowerManagementSettings mSettings;
    runAction(mSettings.getPowerKeyAction());
}

void PowerButton::handleShortcutSuspend()
{
    qDebug() << "Suspend";
    PowerManagementSettings mSettings;
    runAction(mSettings.getSuspendKeyAction());
}

void PowerButton::handleShortcutHibernate()
{
    qDebug() << "Hibernate";
    PowerManagementSettings mSettings;
    runAction(mSettings.getHibernateKeyAction());
}

void PowerButton::runAction(int action)
{
    PowerManagementSettings mSettings;
    if(action == LXQt::Power::PowerMonitorOff) {
        QTimer::singleShot(1000, this, [this]() {
            qDebug() << "LXQt::Power::PowerMonitorOff";
            doAction(LXQt::Power::PowerMonitorOff);
        });
    } else {
        doAction(mSettings.getPowerKeyAction());
    }
}
