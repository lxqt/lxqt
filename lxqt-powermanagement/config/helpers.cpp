/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 *
 * Authors:
 *   Christian Surlykke <christian@surlykke.dk>
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

#include <QComboBox>

#include <LXQt/Power>

#include "helpers.h"

void fillComboBox(QComboBox* comboBox)
{
    comboBox->clear();
    comboBox->addItem(QObject::tr("Nothing"), -1);
    comboBox->addItem(QObject::tr("Ask"), LXQt::Power::PowerShowLeaveDialog);
    comboBox->addItem(QObject::tr("Lock screen"), -2); // FIXME
    comboBox->addItem(QObject::tr("Suspend"), LXQt::Power::PowerSuspend);
    comboBox->addItem(QObject::tr("Hibernate"), LXQt::Power::PowerHibernate);
    comboBox->addItem(QObject::tr("Shutdown"), LXQt::Power::PowerShutdown);
    comboBox->addItem(QObject::tr("Turn Off monitor(s)"), LXQt::Power::PowerMonitorOff);
}

void setComboBoxToValue(QComboBox* comboBox, int value)
{
    for (int index = 0; index < comboBox->count(); index++)
    {
        if (value == comboBox->itemData(index).toInt())
        {
            comboBox->setCurrentIndex(index);
            return;
        }
    }

    comboBox->setCurrentIndex(0);
}

int currentValue(QComboBox *comboBox)
{
    return comboBox->itemData(comboBox->currentIndex()).toInt();
}

