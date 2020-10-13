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

#include <QPushButton>
#include <QTimeZone>

#include "lxqtworldclockconfigurationtimezones.h"

#include "ui_lxqtworldclockconfigurationtimezones.h"


LXQtWorldClockConfigurationTimeZones::LXQtWorldClockConfigurationTimeZones(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LXQtWorldClockConfigurationTimeZones)
{
    setObjectName(QStringLiteral("WorldClockConfigurationTimeZonesWindow"));
    setWindowModality(Qt::WindowModal);
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui->buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    connect(ui->timeZonesTW, SIGNAL(itemSelectionChanged()), SLOT(itemSelectionChanged()));
    connect(ui->timeZonesTW, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(itemDoubleClicked(QTreeWidgetItem*,int)));
}
LXQtWorldClockConfigurationTimeZones::~LXQtWorldClockConfigurationTimeZones()
{
    delete ui;
}

QString LXQtWorldClockConfigurationTimeZones::timeZone()
{
    return mTimeZone;
}

void LXQtWorldClockConfigurationTimeZones::itemSelectionChanged()
{
    QList<QTreeWidgetItem*> items = ui->timeZonesTW->selectedItems();
    if (!items.empty())
        mTimeZone = items[0]->data(0, Qt::UserRole).toString();
    else
        mTimeZone.clear();
}

void LXQtWorldClockConfigurationTimeZones::itemDoubleClicked(QTreeWidgetItem* /*item*/, int /*column*/)
{
    if (!mTimeZone.isEmpty())
        accept();
}

QTreeWidgetItem* LXQtWorldClockConfigurationTimeZones::makeSureParentsExist(const QStringList &parts, QMap<QString, QTreeWidgetItem*> &parentItems)
{
    if (parts.length() == 1)
        return 0;

    QStringList parentParts = parts.mid(0, parts.length() - 1);

    QString parentPath = parentParts.join(QLatin1String("/"));

    QMap<QString, QTreeWidgetItem*>::Iterator I = parentItems.find(parentPath);
    if (I != parentItems.end())
        return I.value();

    QTreeWidgetItem* newItem = new QTreeWidgetItem(QStringList() << parts[parts.length() - 2]);

    QTreeWidgetItem* parentItem = makeSureParentsExist(parentParts, parentItems);

    if (!parentItem)
        ui->timeZonesTW->addTopLevelItem(newItem);
    else
        parentItem->addChild(newItem);

    parentItems[parentPath] = newItem;

    return newItem;
}

int LXQtWorldClockConfigurationTimeZones::updateAndExec()
{
    QDateTime now = QDateTime::currentDateTime();

    ui->timeZonesTW->clear();

    QMap<QString, QTreeWidgetItem*> parentItems;

    const auto timeZones = QTimeZone::availableTimeZoneIds();
    for(const QByteArray &ba : timeZones)
    {
        QTimeZone timeZone(ba);
        QString ianaId(QString::fromUtf8(ba));
        QStringList qStrings(QString::fromUtf8((ba)).split(QLatin1Char('/')));

        if ((qStrings.size() == 1) && (qStrings[0].startsWith(QLatin1String("UTC"))))
            qStrings.prepend(tr("UTC"));

        if (qStrings.size() == 1)
            qStrings.prepend(tr("Other"));

        QTreeWidgetItem *tzItem = new QTreeWidgetItem(QStringList() << qStrings[qStrings.length() - 1] << timeZone.displayName(now) << timeZone.comment() << QLocale::countryToString(timeZone.country()));
        tzItem->setData(0, Qt::UserRole, ianaId);

        makeSureParentsExist(qStrings, parentItems)->addChild(tzItem);
    }

    QStringList qStrings = QStringList() << tr("Other") << QLatin1String("local");
    QTreeWidgetItem *tzItem = new QTreeWidgetItem(QStringList() << qStrings[qStrings.length() - 1] << QString() << tr("Local timezone") << QString());
    tzItem->setData(0, Qt::UserRole, qStrings[qStrings.length() - 1]);
    makeSureParentsExist(qStrings, parentItems)->addChild(tzItem);

    ui->timeZonesTW->sortByColumn(0, Qt::AscendingOrder);

    return exec();
}
