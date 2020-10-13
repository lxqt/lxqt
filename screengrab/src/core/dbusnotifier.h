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

#ifndef DBUSNOTIFIER_H
#define DBUSNOTIFIER_H

#include <QObject>

#include "core.h"

class QDBusInterface;

class DBusNotifier : public QObject
{
    Q_OBJECT
public:
    explicit DBusNotifier(QObject *parent = 0);
    ~DBusNotifier();

    void displayNotify(const StateNotifyMessage& message);

private:
    QList<QVariant> prepareNotification(const StateNotifyMessage& message);

    QDBusInterface *_notifier;
    int _notifyDuration;
    QString _appIconPath;
    QString _previewPath;
};

#endif // DBUSNOTIFIER_H
