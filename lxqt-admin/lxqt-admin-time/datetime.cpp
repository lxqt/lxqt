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

#include "datetime.h"
#include "ui_datetime.h"
#include <QTime>
#include <QTimer>
#include <QTextCharFormat>

DateTimePage::DateTimePage(bool useNtp, bool localRtc, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DateTime),
    mUseNtp(useNtp),
    mLocalRtc(localRtc)
{
    ui->setupUi(this);
    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, this, &DateTimePage::timeout);

    //highlight today
    QDate date = QDate::currentDate();
    QTextCharFormat format = ui->calendar->dateTextFormat(date);
    QBrush brush;
    brush.setColor(Qt::green);
    format.setBackground(brush);
    ui->calendar->setDateTextFormat(date,format);

    reload();
}

DateTimePage::~DateTimePage()
{
    delete ui;
}

void DateTimePage::timeout()
{
    ui->edit_time->blockSignals(true);
    ui->edit_time->setTime(QTime::currentTime());
    ui->edit_time->blockSignals(false);
}

void DateTimePage::reload()
{
    ui->calendar->setSelectedDate(QDate::currentDate());
    ui->edit_time->setTime(QTime::currentTime());

    ui->localRTC->setChecked(mLocalRtc);
    ui->ntp->setChecked(mUseNtp);

    mTimer->start(1000);

    mModified = DateTimePage::ModifiedFlags();
    emit changed();
}

void DateTimePage::on_edit_time_userTimeChanged(const QTime &time)
{
    mModified |= M_TIME;
    mTimer->stop();
    emit changed();
}

QDateTime DateTimePage::dateTime() const
{
    QDateTime dt(ui->calendar->selectedDate(),ui->edit_time->time());
    return dt;
}

bool DateTimePage::useNtp() const
{
    return ui->ntp->isChecked();
}

bool DateTimePage::localRtc() const
{
    return ui->localRTC->isChecked();
}

void DateTimePage::on_calendar_selectionChanged()
{
    QDate date = ui->calendar->selectedDate();
    if (date != QDate::currentDate())
    {
        mModified |= M_DATE;
    }
    else
    {
        mModified &= ~M_DATE;
    }
    emit changed();
}

void DateTimePage::on_ntp_toggled(bool toggled)
{
    if(toggled != mUseNtp)
    {
        mModified |= M_NTP;
    }
    else
    {
        mModified &= ~M_NTP;
    }
    emit changed();
}

void DateTimePage::on_localRTC_toggled(bool toggled)
{
    if(toggled != mLocalRtc)
    {
        mModified |= M_LOCAL_RTC;
    }
    else
    {
        mModified &= ~M_LOCAL_RTC;
    }
    emit changed();
}

