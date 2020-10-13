/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 *            2014 LXQt team
 * Authors:
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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


#ifndef LXQT_PANEL_WORLDCLOCK_CONFIGURATION_TIMEZONES_H
#define LXQT_PANEL_WORLDCLOCK_CONFIGURATION_TIMEZONES_H

#include <QDialog>
#include <QAbstractButton>


namespace Ui {
    class LXQtWorldClockConfigurationTimeZones;
}

class QTreeWidgetItem;

class LXQtWorldClockConfigurationTimeZones : public QDialog
{
    Q_OBJECT

public:
    explicit LXQtWorldClockConfigurationTimeZones(QWidget *parent = nullptr);
    ~LXQtWorldClockConfigurationTimeZones();

    int updateAndExec();

    QString timeZone();

public slots:
    void itemSelectionChanged();
    void itemDoubleClicked(QTreeWidgetItem*,int);

private:
    Ui::LXQtWorldClockConfigurationTimeZones *ui;

    QString mTimeZone;

    QTreeWidgetItem* makeSureParentsExist(const QStringList &parts, QMap<QString, QTreeWidgetItem*> &parentItems);
};

#endif // LXQT_PANEL_WORLDCLOCK_CONFIGURATION_TIMEZONES_H
