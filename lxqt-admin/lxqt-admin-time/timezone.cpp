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

#include "timezone.h"
#include "ui_timezone.h"

TimezonePage::TimezonePage(const QStringList & zones, const QString & currentimezone, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Timezone),
    mZoneChanged(false),
    mZonesList(zones)
{
    ui->setupUi(this);

    if (!currentimezone.isEmpty())
        ui->label_timezone->setText(currentimezone);
    else
        ui->label_timezone->setText(tr("None"));

    mZonesList.sort();
    ui->list_zones->addItems(mZonesList);

    reload();
}

TimezonePage::~TimezonePage()
{
    delete ui;
}

void TimezonePage::reload()
{
    ui->list_zones->setCurrentRow(-1);
    mZoneChanged = false;
    QList<QListWidgetItem *> list = ui->list_zones->findItems(ui->label_timezone->text(),Qt::MatchExactly);
    if (list.count())
        ui->list_zones->setCurrentItem(list.at(0));

    emit changed();
}

QString TimezonePage::timezone() const
{
    if (ui->list_zones->currentItem())
        return ui->list_zones->currentItem()->text();
    else
        return QString();
}

void TimezonePage::on_list_zones_itemSelectionChanged()
{
    QList<QListWidgetItem*> selected = ui->list_zones->selectedItems();
    if(selected.empty())
        return;
    QListWidgetItem *item = selected.first();
    mZoneChanged = item->text() != ui->label_timezone->text();
    emit changed();
}

void TimezonePage::on_edit_filter_textChanged(const QString &arg1)
{
    QRegExp reg(arg1, Qt::CaseInsensitive,QRegExp::Wildcard);
    ui->list_zones->clear();
    ui->list_zones->addItems(mZonesList.filter(reg));
}
