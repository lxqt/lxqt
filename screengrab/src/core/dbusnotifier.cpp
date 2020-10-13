/***************************************************************************
 *   Copyright (C) 2009 - 2013 by Artem 'DOOMer' Galichkin                 *
 *   doomer3d@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDir>
#include <QTimer>

#include <QDebug>

#include "dbusnotifier.h"

#include "config.h"

#define SERVICE_FREEDESKTOP "org.freedesktop.Notifications"
#define PATH_FREEDESKTOP "/org/freedesktop/Notifications"
#define INTERFACE_FREEDESKTOP "org.freedesktop.Notifications"


#define CACHE_DIR "notify-cache"
#define CACHE_PREV "preview.jpg"

DBusNotifier::DBusNotifier(QObject *parent) : QObject(parent)
{
    _notifier = new QDBusInterface(QStringLiteral(SERVICE_FREEDESKTOP), QStringLiteral(PATH_FREEDESKTOP), QStringLiteral(INTERFACE_FREEDESKTOP),
                                   QDBusConnection::sessionBus(), this);
    if (_notifier->lastError().type() != QDBusError::NoError) {
        qWarning() << "Notify: Unable to create interface.";
        return;
    }

    QDir dir(Config::getConfigDir());
    if (!dir.exists(QStringLiteral(CACHE_DIR)))
        dir.mkdir(QStringLiteral(CACHE_DIR));

    _previewPath = dir.absolutePath() + QDir::toNativeSeparators(QDir::separator()) + QStringLiteral(CACHE_PREV);
    _appIconPath = QStringLiteral(SG_ICONPATH);

    _notifyDuration = Config::instance()->getTimeTrayMess() * 1000;

}

DBusNotifier::~DBusNotifier()
{
    if (!_previewPath.isEmpty()) {
        QDir dir(QDir::home());
        dir.remove(_previewPath);
    }
}


void DBusNotifier::displayNotify(const StateNotifyMessage &message)
{
    QList<QVariant> n = prepareNotification(message);

    if (!n.isEmpty()) {
        QDBusReply<uint> reply = _notifier->callWithArgumentList(QDBus::Block,QStringLiteral("Notify"),n);
    }

    deleteLater();
}

QList<QVariant> DBusNotifier::prepareNotification(const StateNotifyMessage& message)
{
    QList<QVariant> args;

    args << QLatin1Literal("Screen Grab");
    args << QVariant(QVariant::UInt); // id

    // app-icon(path to icon on disk)
    args << _appIconPath;

    // summary (notification title)
    args << message.header;

    // message text
    args << message.message;

    // actions is none
    args << QStringList();

    // hints
    args << QVariantMap();

    // timeout
    args << _notifyDuration;

    return args;
}

