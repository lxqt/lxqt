/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  PCMan <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#define TIMER_DURATION 10

#include <QIcon>
#include <QStyle>
#include <QPushButton>
#include "timeoutdialog.h"

TimeoutDialog::TimeoutDialog(QWidget* parent, Qt::WindowFlags f) :
    QDialog(parent, f)
{
    ui.setupUi(this);
    ui.buttonBox_1->button(QDialogButtonBox::Yes)->setText(tr("Yes"));
    ui.buttonBox_2->button(QDialogButtonBox::No)->setText(tr("No"));
    QIcon icon = style()->standardIcon(QStyle::SP_MessageBoxQuestion);
    int size = style()->pixelMetric(QStyle::PM_MessageBoxIconSize);
    ui.icon->setPixmap(icon.pixmap(QSize(size, size)));
    connect(&timer, &QTimer::timeout, this, &TimeoutDialog::onTimeout);
    adjustSize();
}

TimeoutDialog::~TimeoutDialog()
{
}

void TimeoutDialog::showEvent(QShowEvent* e)
{
    timer.start(1000);
    QDialog::showEvent(e);
}

void TimeoutDialog::onTimeout()
{
    int maximum = ui.progressBar->maximum();
    int time = ui.progressBar->value() + maximum / TIMER_DURATION;

    // if time is finished, settings are restored.
    if (time >= maximum) {
        timer.stop();
        reject();
    }
    else {
        int remaining = maximum / TIMER_DURATION - TIMER_DURATION * time / maximum;
        ui.remainingTime->setText(tr("%n second(s) remaining", nullptr, remaining));
        ui.progressBar->setValue(time);
    }
}
