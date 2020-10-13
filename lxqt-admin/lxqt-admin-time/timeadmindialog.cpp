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

#include "timeadmindialog.h"
#include <QLabel>
#include <QCloseEvent>
#include <QMessageBox>
#include <QDateTime>
#include <QMap>
#include <QDebug>

#include <LXQt/Globals>
#include <LXQt/Settings>

#include "datetime.h"
#include "timezone.h"

#define ZONETAB_PATH "/usr/share/zoneinfo/zone.tab"

TimeAdminDialog::TimeAdminDialog(QWidget *parent):
    LXQt::ConfigDialog(tr("Time and date configuration"),new LXQt::Settings(QSL("TimeDate")), parent)
{
    setMinimumSize(QSize(400,400));
    mWindowTitle = windowTitle();

    mDateTimeWidget = new DateTimePage(mTimeDateCtl.useNtp(), mTimeDateCtl.localRtc(), this);
    addPage(mDateTimeWidget,tr("Date and time"));
    connect(this, &LXQt::ConfigDialog::reset, mDateTimeWidget,&DateTimePage::reload);
    connect(mDateTimeWidget,&DateTimePage::changed,this,&TimeAdminDialog::onChanged);

    QStringList zones;
    QString currentZone;
    loadTimeZones(zones,currentZone);
    mTimezoneWidget = new TimezonePage(zones,currentZone,this);
    addPage(mTimezoneWidget,tr("Timezone"));
    connect(this,&TimeAdminDialog::reset,mTimezoneWidget,&TimezonePage::reload);
    connect(mTimezoneWidget,&TimezonePage::changed,this,&TimeAdminDialog::onChanged);

    setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(this, &LXQt::ConfigDialog::clicked, this, &TimeAdminDialog::onButtonClicked);
    adjustSize();
}

TimeAdminDialog::~TimeAdminDialog()
{
    delete mSettings;
    mSettings = nullptr;
}

void TimeAdminDialog::onChanged()
{
    showChangedStar();
}


void TimeAdminDialog::showChangedStar()
{
    if(mTimezoneWidget->isChanged() || mDateTimeWidget->modified())
        setWindowTitle(mWindowTitle + QL1C('*'));
    else
        setWindowTitle(mWindowTitle);
}

void TimeAdminDialog::loadTimeZones(QStringList & timeZones, QString & currentTimezone)
{
    currentTimezone = mTimeDateCtl.timeZone();

    timeZones.clear();
    QFile file(QSL(ZONETAB_PATH));
    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray line;
        while(!file.atEnd())
        {
            line = file.readLine().trimmed();
            if(line.isEmpty() || line[0] == '#') // skip comments or empty lines
                continue;
            QList<QByteArray> items = line.split('\t');
            if(items.length() <= 2)
                continue;
            timeZones.append(QLatin1String(items[2]));
        }
        file.close();
    }
}



void TimeAdminDialog::saveChangesToSystem()
{
    QString errorMessage;
    if(mTimezoneWidget->isChanged())
    {
        QString timeZone = mTimezoneWidget->timezone();
        if(!timeZone.isEmpty())
        {
            if(false == mTimeDateCtl.setTimeZone(timeZone, errorMessage)) {
                QMessageBox::critical(this, tr("Error"), errorMessage);
            }
        }
    }

    auto modified = mDateTimeWidget->modified();
    bool useNtp = mDateTimeWidget->useNtp();
    if(modified.testFlag(DateTimePage::M_NTP))
    {
        if(false == mTimeDateCtl.setUseNtp(useNtp, errorMessage)) {
            QMessageBox::critical(this, tr("Error"), errorMessage);
        }
    }

    if(modified.testFlag(DateTimePage::M_LOCAL_RTC))
    {
        if(false == mTimeDateCtl.setLocalRtc(mDateTimeWidget->localRtc(), errorMessage)) {
            QMessageBox::critical(this, tr("Error"), errorMessage);
        }
    }

    // we can only change the date & time explicitly when NTP is disabled.
    if(false == useNtp)
    {
        if(modified.testFlag(DateTimePage::M_DATE) || modified.testFlag(DateTimePage::M_TIME))
        {
            if(false == mTimeDateCtl.setDateTime(mDateTimeWidget->dateTime(), errorMessage)) {
                QMessageBox::critical(this, tr("Error"), errorMessage);
            }
        }
    }
#ifdef Q_OS_FREEBSD
    mTimeDateCtl.pkexec();
    if(modified.testFlag(DateTimePage::M_LOCAL_RTC)) {
    const QString infoMsg = mDateTimeWidget->localRtc() ?  tr("Change RTC to be in localtime requires a reboot") : tr("Change RTC to be in UTC requires a reboot");
    QMessageBox::information(this,tr("Reboot required"),infoMsg);
    }
#endif
}

void TimeAdminDialog::onButtonClicked(QDialogButtonBox::StandardButton button)
{
    if(button == QDialogButtonBox::Ok)
    {
        saveChangesToSystem();
        accept();
    }
    else if(button == QDialogButtonBox::Cancel)
    {
        reject();
    }
}
