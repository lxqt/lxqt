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

#include <LXQt/ConfigDialog>
#ifdef Q_OS_LINUX
#include "dbustimedatectl.h"
#endif
#ifdef Q_OS_FREEBSD
#include "fbsdtimedatectl.h"
#endif
class DateTimePage;
class TimezonePage;

class TimeAdminDialog: public LXQt::ConfigDialog
{
    Q_OBJECT

public:
    TimeAdminDialog(QWidget * parent = NULL) ;
    ~TimeAdminDialog();

private Q_SLOTS:
    void onChanged();
    void onButtonClicked(QDialogButtonBox::StandardButton button);

private:
    void saveChangesToSystem();
    void loadTimeZones(QStringList & timeZones, QString & currentTimezone);
    void showChangedStar();

private:
#ifdef Q_OS_LINUX
    DbusTimeDateCtl mTimeDateCtl;
#endif
#ifdef Q_OS_FREEBSD
    FBSDTimeDateCtl mTimeDateCtl;
#endif
    DateTimePage * mDateTimeWidget;
    TimezonePage * mTimezoneWidget;
    QString mWindowTitle;
};
