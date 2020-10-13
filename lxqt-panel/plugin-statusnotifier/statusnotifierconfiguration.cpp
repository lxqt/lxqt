/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2020 LXQt team
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

#include "statusnotifierconfiguration.h"
#include "ui_statusnotifierconfiguration.h"
#include <QPushButton>
#include <QComboBox>

StatusNotifierConfiguration::StatusNotifierConfiguration(PluginSettings *settings, QWidget *parent):
    LXQtPanelPluginConfigDialog(settings, parent),
    ui(new Ui::StatusNotifierConfiguration)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("StatusNotifierConfigurationWindow"));
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Reset)->setText(tr("Reset"));
    ui->buttonBox_2->button(QDialogButtonBox::Close)->setText(tr("Close"));
    if (QPushButton *closeBtn = ui->buttonBox_2->button(QDialogButtonBox::Close))
        closeBtn->setDefault(true);
    connect(ui->buttonBox_1, &QDialogButtonBox::clicked, this, &StatusNotifierConfiguration::dialogButtonsAction);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionsClickable(false);
    ui->tableWidget->sortByColumn(0, Qt::AscendingOrder);

    loadSettings();

    connect(ui->attentionSB, &QAbstractSpinBox::editingFinished, this, &StatusNotifierConfiguration::saveSettings);
}

StatusNotifierConfiguration::~StatusNotifierConfiguration()
{
    delete ui;
}

void StatusNotifierConfiguration::loadSettings()
{
    ui->attentionSB->setValue(settings().value(QStringLiteral("attentionPeriod"), 5).toInt());
    mAutoHideList = settings().value(QStringLiteral("autoHideList")).toStringList();
    mHideList = settings().value(QStringLiteral("hideList")).toStringList();
}

void StatusNotifierConfiguration::saveSettings()
{
    settings().setValue(QStringLiteral("attentionPeriod"), ui->attentionSB->value());
    settings().setValue(QStringLiteral("autoHideList"), mAutoHideList);
    settings().setValue(QStringLiteral("hideList"), mHideList);
}

void StatusNotifierConfiguration::addItems(const QStringList &items)
{
    ui->tableWidget->setRowCount(items.size());
    ui->tableWidget->setSortingEnabled(false);
    int index = 0;
    for (const auto &item : items)
    {
        // first column
        QTableWidgetItem *widgetItem = new QTableWidgetItem(item);
        widgetItem->setFlags(widgetItem->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
        ui->tableWidget->setItem(index, 0, widgetItem);
        // second column
        QComboBox *cb = new QComboBox();
        cb->addItems(QStringList() << tr("Always show") << tr("Auto-hide") << tr("Always hide"));
        if (mAutoHideList.contains(item))
            cb->setCurrentIndex(1);
        else if (mHideList.contains(item))
            cb->setCurrentIndex(2);
        connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, item] (int indx) {
            if (indx == 0)
            {
                mAutoHideList.removeAll(item);
                mHideList.removeAll(item);
            }
            else if (indx == 1)
            {
                mHideList.removeAll(item);
                if (!mAutoHideList.contains(item))
                    mAutoHideList << item;
            }
            else if (indx == 2)
            {
                mAutoHideList.removeAll(item);
                if (!mHideList.contains(item))
                    mHideList << item;
            }
            saveSettings();
        });
        ui->tableWidget->setCellWidget(index, 1, cb);
        ++ index;
    }
    ui->tableWidget->setSortingEnabled(true);
    ui->tableWidget->horizontalHeader()->setSortIndicatorShown(false);
    ui->tableWidget->setCurrentCell(0, 1);
}
