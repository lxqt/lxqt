/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2014 LXQt team
 * Authors:
 *   Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef TIMEZONE_H
#define TIMEZONE_H

#include <QWidget>

namespace Ui {
class Timezone;
}

class QListWidgetItem;
class TimezonePage : public QWidget
{
    Q_OBJECT

public:
    explicit TimezonePage(const QStringList & zones, const QString & currentimezone, QWidget *parent = 0);
    ~TimezonePage();
    QString timezone() const;
    inline bool isChanged() const {return mZoneChanged;}

public slots:
    void reload();

Q_SIGNALS:
    void changed();

private slots:
    void on_list_zones_itemSelectionChanged();
    void on_edit_filter_textChanged(const QString &arg1);

private:
    Ui::Timezone *ui;
    bool mZoneChanged;
    QString mCurrentTimeZone;
    QStringList mZonesList;
};

#endif // TIMEZONE_H
