/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *  Palo Kisa <palo.kisa@gmail.com>
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

#if !defined(UdevNotifier_h)
#define UdevNotifier_h
#if defined(WITH_LIBUDEV_MONITOR)

#include <QObject>
#include <QScopedPointer>

class UdevNotifier : public QObject
{
    Q_OBJECT
public:
    UdevNotifier(QString const & subsystem, QObject * parent = nullptr);
    ~UdevNotifier() override;

signals:
    void deviceAdded(QString path);
    void deviceRemoved(QString path);
    void deviceChanged(QString path);
    void deviceOnline(QString path);
    void deviceOffline(QString path);

private slots:
    void eventReady(int socket);

private:
    class Impl;

    QScopedPointer<Impl> d;
};

#endif //WITH_LIBUDEV_MONITOR
#endif //UdevNotifier_h
