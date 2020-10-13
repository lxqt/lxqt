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


#include "lxqtworldclockconfiguration.h"

#include "ui_lxqtworldclockconfiguration.h"

#include "lxqtworldclockconfigurationtimezones.h"
#include "lxqtworldclockconfigurationmanualformat.h"

#include <LXQt/Globals>
#include <QPushButton>
#include <QInputDialog>


LXQtWorldClockConfiguration::LXQtWorldClockConfiguration(PluginSettings *settings, QWidget *parent) :
    LXQtPanelPluginConfigDialog(settings, parent),
    ui(new Ui::LXQtWorldClockConfiguration),
    mLockCascadeSettingChanges(false),
    mConfigurationTimeZones(nullptr),
    mConfigurationManualFormat(nullptr)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QLatin1String("WorldClockConfigurationWindow"));
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Reset)->setText(tr("Reset"));
    ui->buttonBox_2->button(QDialogButtonBox::Close)->setText(tr("Close"));
    connect(ui->buttonBox_1, SIGNAL(clicked(QAbstractButton*)), this, SLOT(dialogButtonsAction(QAbstractButton*)));
    connect(ui->timeFormatCB, SIGNAL(currentIndexChanged(int)), SLOT(saveSettings()));
    connect(ui->timeShowSecondsCB, SIGNAL(clicked()), SLOT(saveSettings()));
    connect(ui->timePadHourCB, SIGNAL(clicked()), SLOT(saveSettings()));
    connect(ui->timeAMPMCB, SIGNAL(clicked()), SLOT(saveSettings()));
    connect(ui->timezoneGB, SIGNAL(clicked()), SLOT(saveSettings()));
    connect(ui->timezonePositionCB, SIGNAL(currentIndexChanged(int)), SLOT(saveSettings()));
    connect(ui->timezoneFormatCB, SIGNAL(currentIndexChanged(int)), SLOT(saveSettings()));
    connect(ui->dateGB, SIGNAL(clicked()), SLOT(saveSettings()));
    connect(ui->datePositionCB, SIGNAL(currentIndexChanged(int)), SLOT(saveSettings()));
    connect(ui->dateFormatCB, SIGNAL(currentIndexChanged(int)), SLOT(saveSettings()));
    connect(ui->dateShowYearCB, SIGNAL(clicked()), SLOT(saveSettings()));
    connect(ui->dateShowDoWCB, SIGNAL(clicked()), SLOT(saveSettings()));
    connect(ui->datePadDayCB, SIGNAL(clicked()), SLOT(saveSettings()));
    connect(ui->dateLongNamesCB, SIGNAL(clicked()), SLOT(saveSettings()));
    connect(ui->advancedManualGB, SIGNAL(clicked()), SLOT(saveSettings()));
    connect(ui->customisePB, SIGNAL(clicked()), SLOT(customiseManualFormatClicked()));


    connect(ui->timeFormatCB, SIGNAL(currentIndexChanged(int)), SLOT(timeFormatChanged(int)));
    connect(ui->dateGB, SIGNAL(toggled(bool)), SLOT(dateGroupToggled(bool)));
    connect(ui->dateFormatCB, SIGNAL(currentIndexChanged(int)), SLOT(dateFormatChanged(int)));
    connect(ui->advancedManualGB, SIGNAL(toggled(bool)), SLOT(advancedFormatToggled(bool)));

    connect(ui->timeZonesTW, SIGNAL(itemSelectionChanged()), SLOT(updateTimeZoneButtons()));
    connect(ui->addPB, SIGNAL(clicked()), SLOT(addTimeZone()));
    connect(ui->removePB, SIGNAL(clicked()), SLOT(removeTimeZone()));
    connect(ui->setAsDefaultPB, SIGNAL(clicked()), SLOT(setTimeZoneAsDefault()));
    connect(ui->editCustomNamePB, SIGNAL(clicked()), SLOT(editTimeZoneCustomName()));
    connect(ui->moveUpPB, SIGNAL(clicked()), SLOT(moveTimeZoneUp()));
    connect(ui->moveDownPB, SIGNAL(clicked()), SLOT(moveTimeZoneDown()));

    connect(ui->autorotateCB, SIGNAL(clicked()), SLOT(saveSettings()));
    connect(ui->showWeekNumberCB, &QCheckBox::clicked, this, &LXQtWorldClockConfiguration::saveSettings);
    connect(ui->showTooltipCB, SIGNAL(clicked()), SLOT(saveSettings()));

    loadSettings();
}

LXQtWorldClockConfiguration::~LXQtWorldClockConfiguration()
{
    delete ui;
}

void LXQtWorldClockConfiguration::loadSettings()
{
    mLockCascadeSettingChanges = true;

    bool longTimeFormatSelected = false;

    QString formatType = settings().value(QLatin1String("formatType"), QString()).toString();
    QString dateFormatType = settings().value(QLatin1String("dateFormatType"), QString()).toString();
    bool advancedManual = settings().value(QLatin1String("useAdvancedManualFormat"), false).toBool();
    mManualFormat = settings().value(QLatin1String("customFormat"), tr("'<b>'HH:mm:ss'</b><br/><font size=\"-2\">'ddd, d MMM yyyy'<br/>'TT'</font>'")).toString();

    // backward compatibility
    if (formatType == QLatin1String("custom"))
    {
        formatType = QLatin1String("short-timeonly");
        dateFormatType = QLatin1String("short");
        advancedManual = true;
    }
    else if (formatType == QLatin1String("short"))
    {
        formatType = QLatin1String("short-timeonly");
        dateFormatType = QLatin1String("short");
        advancedManual = false;
    }
    else if ((formatType == QLatin1String("full")) ||
             (formatType == QLatin1String("long")) ||
             (formatType == QLatin1String("medium")))
    {
        formatType = QLatin1String("long-timeonly");
        dateFormatType = QLatin1String("long");
        advancedManual = false;
    }


    if (formatType == QLatin1String("short-timeonly"))
        ui->timeFormatCB->setCurrentIndex(0);
    else if (formatType == QLatin1String("long-timeonly"))
    {
        ui->timeFormatCB->setCurrentIndex(1);
        longTimeFormatSelected = true;
    }
    else // if (formatType == QLatin1String("custom-timeonly"))
        ui->timeFormatCB->setCurrentIndex(2);

    ui->timeShowSecondsCB->setChecked(settings().value(QLatin1String("timeShowSeconds"), false).toBool());
    ui->timePadHourCB->setChecked(settings().value(QLatin1String("timePadHour"), false).toBool());
    ui->timeAMPMCB->setChecked(settings().value(QLatin1String("timeAMPM"), false).toBool());
    ui->showTooltipCB->setChecked(settings().value(QLatin1String("showTooltip"), false).toBool());

    bool customTimeFormatSelected = ui->timeFormatCB->currentIndex() == ui->timeFormatCB->count() - 1;
    ui->timeCustomW->setEnabled(customTimeFormatSelected);

    ui->timezoneGB->setEnabled(!longTimeFormatSelected);

    // timezone
    ui->timezoneGB->setChecked(settings().value(QLatin1String("showTimezone"), false).toBool() && !longTimeFormatSelected);

    QString timezonePosition = settings().value(QLatin1String("timezonePosition"), QString()).toString();
    if (timezonePosition == QLatin1String("above"))
        ui->timezonePositionCB->setCurrentIndex(1);
    else if (timezonePosition == QLatin1String("before"))
        ui->timezonePositionCB->setCurrentIndex(2);
    else if (timezonePosition == QLatin1String("after"))
        ui->timezonePositionCB->setCurrentIndex(3);
    else // if (timezonePosition == QLatin1String("below"))
        ui->timezonePositionCB->setCurrentIndex(0);

    QString timezoneFormatType = settings().value(QLatin1String("timezoneFormatType"), QString()).toString();
    if (timezoneFormatType == QLatin1String("short"))
        ui->timezoneFormatCB->setCurrentIndex(0);
    else if (timezoneFormatType == QLatin1String("long"))
        ui->timezoneFormatCB->setCurrentIndex(1);
    else if (timezoneFormatType == QLatin1String("offset"))
        ui->timezoneFormatCB->setCurrentIndex(2);
    else if (timezoneFormatType == QLatin1String("abbreviation"))
        ui->timezoneFormatCB->setCurrentIndex(3);
    else // if (timezoneFormatType == QLatin1String("iana"))
        ui->timezoneFormatCB->setCurrentIndex(4);

    // date
    bool dateIsChecked = settings().value(QLatin1String("showDate"), false).toBool();
    ui->dateGB->setChecked(dateIsChecked);

    QString datePosition = settings().value(QLatin1String("datePosition"), QString()).toString();
    if (datePosition == QLatin1String("above"))
        ui->datePositionCB->setCurrentIndex(1);
    else if (datePosition == QLatin1String("before"))
        ui->datePositionCB->setCurrentIndex(2);
    else if (datePosition == QLatin1String("after"))
        ui->datePositionCB->setCurrentIndex(3);
    else // if (datePosition == QLatin1String("below"))
        ui->datePositionCB->setCurrentIndex(0);

    if (dateFormatType == QLatin1String("short"))
        ui->dateFormatCB->setCurrentIndex(0);
    else if (dateFormatType == QLatin1String("long"))
        ui->dateFormatCB->setCurrentIndex(1);
    else if (dateFormatType == QLatin1String("iso"))
        ui->dateFormatCB->setCurrentIndex(2);
    else // if (dateFormatType == QLatin1String("custom"))
        ui->dateFormatCB->setCurrentIndex(3);

    ui->dateShowYearCB->setChecked(settings().value(QLatin1String("dateShowYear"), false).toBool());
    ui->dateShowDoWCB->setChecked(settings().value(QLatin1String("dateShowDoW"), false).toBool());
    ui->datePadDayCB->setChecked(settings().value(QLatin1String("datePadDay"), false).toBool());
    ui->dateLongNamesCB->setChecked(settings().value(QLatin1String("dateLongNames"), false).toBool());

    bool customDateFormatSelected = ui->dateFormatCB->currentIndex() == ui->dateFormatCB->count() - 1;
    ui->dateCustomW->setEnabled(dateIsChecked && customDateFormatSelected);


    ui->advancedManualGB->setChecked(advancedManual);


    mDefaultTimeZone = settings().value(QStringLiteral("defaultTimeZone"), QString()).toString();

    ui->timeZonesTW->setRowCount(0);

    const QList<QMap<QString, QVariant> > list = settings().readArray(QLatin1String("timeZones"));
    int i = 0;
    for (const auto &map : list)
    {
        ui->timeZonesTW->setRowCount(ui->timeZonesTW->rowCount() + 1);

        QString timeZoneName = map.value(QLatin1String("timeZone"), QString()).toString();
        if (mDefaultTimeZone.isEmpty())
            mDefaultTimeZone = timeZoneName;

        ui->timeZonesTW->setItem(i, 0, new QTableWidgetItem(timeZoneName));
        ui->timeZonesTW->setItem(i, 1, new QTableWidgetItem(map.value(QLatin1String("customName"),
                                                                      QString()).toString()));

        setBold(i, mDefaultTimeZone == timeZoneName);
        ++i;
    }

    ui->timeZonesTW->resizeColumnsToContents();


    ui->autorotateCB->setChecked(settings().value(QStringLiteral("autoRotate"), true).toBool());
    ui->showWeekNumberCB->setChecked(settings().value(QL1S("showWeekNumber"), true).toBool());

    mLockCascadeSettingChanges = false;
}

void LXQtWorldClockConfiguration::saveSettings()
{
    if (mLockCascadeSettingChanges)
        return;

    QString formatType;
    switch (ui->timeFormatCB->currentIndex())
    {
    case 0:
        formatType = QLatin1String("short-timeonly");
        break;

    case 1:
        formatType = QLatin1String("long-timeonly");
        break;

    case 2:
        formatType = QLatin1String("custom-timeonly");
        break;
    }
    settings().setValue(QLatin1String("formatType"), formatType);

    settings().setValue(QLatin1String("timeShowSeconds"), ui->timeShowSecondsCB->isChecked());
    settings().setValue(QLatin1String("timePadHour"), ui->timePadHourCB->isChecked());
    settings().setValue(QLatin1String("timeAMPM"), ui->timeAMPMCB->isChecked());

    settings().setValue(QLatin1String("showTimezone"), ui->timezoneGB->isChecked());

    QString timezonePosition;
    switch (ui->timezonePositionCB->currentIndex())
    {
    case 0:
        timezonePosition = QLatin1String("below");
        break;

    case 1:
        timezonePosition = QLatin1String("above");
        break;

    case 2:
        timezonePosition = QLatin1String("before");
        break;

    case 3:
        timezonePosition = QLatin1String("after");
        break;
    }
    settings().setValue(QLatin1String("timezonePosition"), timezonePosition);

    QString timezoneFormatType;
    switch (ui->timezoneFormatCB->currentIndex())
    {
    case 0:
        timezoneFormatType = QLatin1String("short");
        break;

    case 1:
        timezoneFormatType = QLatin1String("long");
        break;

    case 2:
        timezoneFormatType = QLatin1String("offset");
        break;

    case 3:
        timezoneFormatType = QLatin1String("abbreviation");
        break;

    case 4:
        timezoneFormatType = QLatin1String("iana");
        break;
    }
    settings().setValue(QLatin1String("timezoneFormatType"), timezoneFormatType);

    settings().setValue(QLatin1String("showDate"), ui->dateGB->isChecked());

    QString datePosition;
    switch (ui->datePositionCB->currentIndex())
    {
    case 0:
        datePosition = QLatin1String("below");
        break;

    case 1:
        datePosition = QLatin1String("above");
        break;

    case 2:
        datePosition = QLatin1String("before");
        break;

    case 3:
        datePosition = QLatin1String("after");
        break;
    }
    settings().setValue(QLatin1String("datePosition"), datePosition);

    QString dateFormatType;
    switch (ui->dateFormatCB->currentIndex())
    {
    case 0:
        dateFormatType = QLatin1String("short");
        break;

    case 1:
        dateFormatType = QLatin1String("long");
        break;

    case 2:
        dateFormatType = QLatin1String("iso");
        break;

    case 3:
        dateFormatType = QLatin1String("custom");
        break;
    }
    settings().setValue(QLatin1String("dateFormatType"), dateFormatType);

    settings().setValue(QLatin1String("dateShowYear"), ui->dateShowYearCB->isChecked());
    settings().setValue(QLatin1String("dateShowDoW"), ui->dateShowDoWCB->isChecked());
    settings().setValue(QLatin1String("datePadDay"), ui->datePadDayCB->isChecked());
    settings().setValue(QLatin1String("dateLongNames"), ui->dateLongNamesCB->isChecked());

    settings().setValue(QLatin1String("customFormat"), mManualFormat);

    settings().remove(QLatin1String("timeZones"));
    QList<QMap<QString, QVariant> > array;
    int size = ui->timeZonesTW->rowCount();
    for (int i = 0; i < size; ++i)
    {
        QMap<QString, QVariant> map;
        map[QLatin1String("timeZone")] = ui->timeZonesTW->item(i, 0)->text();
        map[QLatin1String("customName")] = ui->timeZonesTW->item(i, 1)->text();
        array << map;
    }
    settings().setArray(QLatin1String("timeZones"), array);

    settings().setValue(QLatin1String("defaultTimeZone"), mDefaultTimeZone);
    settings().setValue(QLatin1String("useAdvancedManualFormat"), ui->advancedManualGB->isChecked());
    settings().setValue(QLatin1String("autoRotate"), ui->autorotateCB->isChecked());
    settings().setValue(QL1S("showWeekNumber"), ui->showWeekNumberCB->isChecked());
    settings().setValue(QLatin1String("showTooltip"), ui->showTooltipCB->isChecked());
}

void LXQtWorldClockConfiguration::timeFormatChanged(int index)
{
    bool longTimeFormatSelected = index == 1;
    bool customTimeFormatSelected = index == 2;
    ui->timeCustomW->setEnabled(customTimeFormatSelected);
    ui->timezoneGB->setEnabled(!longTimeFormatSelected);
}

void LXQtWorldClockConfiguration::dateGroupToggled(bool dateIsChecked)
{
    bool customDateFormatSelected = ui->dateFormatCB->currentIndex() == ui->dateFormatCB->count() - 1;
    ui->dateCustomW->setEnabled(dateIsChecked && customDateFormatSelected);
}

void LXQtWorldClockConfiguration::dateFormatChanged(int index)
{
    bool customDateFormatSelected = index == ui->dateFormatCB->count() - 1;
    bool dateIsChecked = ui->dateGB->isChecked();
    ui->dateCustomW->setEnabled(dateIsChecked && customDateFormatSelected);
}

void LXQtWorldClockConfiguration::advancedFormatToggled(bool on)
{
    bool longTimeFormatSelected = ui->timeFormatCB->currentIndex() == 1;
    ui->timeGB->setEnabled(!on);
    ui->timezoneGB->setEnabled(!on && !longTimeFormatSelected);
    ui->dateGB->setEnabled(!on);
}

void LXQtWorldClockConfiguration::customiseManualFormatClicked()
{
    if (!mConfigurationManualFormat)
    {
        mConfigurationManualFormat = new LXQtWorldClockConfigurationManualFormat(this);
        connect(mConfigurationManualFormat, SIGNAL(manualFormatChanged()), this, SLOT(manualFormatChanged()));
    }

    mConfigurationManualFormat->setManualFormat(mManualFormat);

    QString oldManualFormat = mManualFormat;

    mManualFormat = (mConfigurationManualFormat->exec() == QDialog::Accepted) ? mConfigurationManualFormat->manualFormat() : oldManualFormat;

    saveSettings();
}

void LXQtWorldClockConfiguration::manualFormatChanged()
{
    mManualFormat = mConfigurationManualFormat->manualFormat();
    saveSettings();
}

void LXQtWorldClockConfiguration::updateTimeZoneButtons()
{
    QList<QTableWidgetItem*> selectedItems = ui->timeZonesTW->selectedItems();
    int selectedCount = selectedItems.count() / 2;
    int allCount = ui->timeZonesTW->rowCount();

    ui->removePB->setEnabled(selectedCount != 0);
    bool canSetAsDefault = (selectedCount == 1);
    if (canSetAsDefault)
    {
        if (selectedItems[0]->column() == 0)
            canSetAsDefault = (selectedItems[0]->text() != mDefaultTimeZone);
        else
            canSetAsDefault = (selectedItems[1]->text() != mDefaultTimeZone);
    }

    bool canMoveUp = false;
    bool canMoveDown = false;
    if ((selectedCount != 0) && (selectedCount != allCount))
    {
        bool skipBottom = true;
        for (int i = allCount - 1; i >= 0; --i)
        {
            if (ui->timeZonesTW->item(i, 0)->isSelected())
            {
                if (!skipBottom)
                {
                    canMoveDown = true;
                    break;
                }
            }
            else
                skipBottom = false;
        }

        bool skipTop = true;
        for (int i = 0; i < allCount; ++i)
        {
            if (ui->timeZonesTW->item(i, 0)->isSelected())
            {
                if (!skipTop)
                {
                    canMoveUp = true;
                    break;
                }
            }
            else
                skipTop = false;
        }
    }
    ui->setAsDefaultPB->setEnabled(canSetAsDefault);
    ui->editCustomNamePB->setEnabled(selectedCount == 1);
    ui->moveUpPB->setEnabled(canMoveUp);
    ui->moveDownPB->setEnabled(canMoveDown);
}

int LXQtWorldClockConfiguration::findTimeZone(const QString& timeZone)
{
    QList<QTableWidgetItem*> items = ui->timeZonesTW->findItems(timeZone, Qt::MatchExactly);
    for (const QTableWidgetItem* item : qAsConst(items))
        if (item->column() == 0)
            return item->row();
    return -1;
}

void LXQtWorldClockConfiguration::addTimeZone()
{
    if (!mConfigurationTimeZones)
        mConfigurationTimeZones = new LXQtWorldClockConfigurationTimeZones(this);

    if (mConfigurationTimeZones->updateAndExec() == QDialog::Accepted)
    {
        QString timeZone = mConfigurationTimeZones->timeZone();
        if (timeZone != QString())
        {
            if (findTimeZone(timeZone) == -1)
            {
                int row = ui->timeZonesTW->rowCount();
                ui->timeZonesTW->setRowCount(row + 1);
                QTableWidgetItem *item = new QTableWidgetItem(timeZone);
                ui->timeZonesTW->setItem(row, 0, item);
                ui->timeZonesTW->setItem(row, 1, new QTableWidgetItem(QString()));
                if (mDefaultTimeZone.isEmpty())
                    setDefault(row);
            }
        }
    }

    saveSettings();
}

void LXQtWorldClockConfiguration::removeTimeZone()
{
    const auto selectedItems = ui->timeZonesTW->selectedItems();
    for (const QTableWidgetItem *item : selectedItems)
        if (item->column() == 0)
        {
            if (item->text() == mDefaultTimeZone)
                mDefaultTimeZone.clear();
            ui->timeZonesTW->removeRow(item->row());
        }

    if ((mDefaultTimeZone.isEmpty()) && ui->timeZonesTW->rowCount())
        setDefault(0);

    saveSettings();
}

void LXQtWorldClockConfiguration::setBold(QTableWidgetItem *item, bool value)
{
    if (item)
    {
        QFont font = item->font();
        font.setBold(value);
        item->setFont(font);
    }
}

void LXQtWorldClockConfiguration::setBold(int row, bool value)
{
    setBold(ui->timeZonesTW->item(row, 0), value);
    setBold(ui->timeZonesTW->item(row, 1), value);
}

void LXQtWorldClockConfiguration::setDefault(int row)
{
    setBold(row, true);
    mDefaultTimeZone = ui->timeZonesTW->item(row, 0)->text();
}

void LXQtWorldClockConfiguration::setTimeZoneAsDefault()
{
    setBold(findTimeZone(mDefaultTimeZone), false);

    setDefault(ui->timeZonesTW->selectedItems()[0]->row());

    saveSettings();
}

void LXQtWorldClockConfiguration::editTimeZoneCustomName()
{
    int row = ui->timeZonesTW->selectedItems()[0]->row();

    QString oldName = ui->timeZonesTW->item(row, 1)->text();

    QInputDialog d(this);
    d.setWindowTitle(tr("Input custom time zone name"));
    d.setLabelText(tr("Custom name"));
    d.setTextValue(oldName);
    d.setWindowModality(Qt::WindowModal);
    if (d.exec())
    {
        ui->timeZonesTW->item(row, 1)->setText(d.textValue());

        saveSettings();
    }
}

void LXQtWorldClockConfiguration::moveTimeZoneUp()
{
    int m = ui->timeZonesTW->rowCount();
    bool skipTop = true;
    for (int i = 0; i < m; ++i)
    {
        if (ui->timeZonesTW->item(i, 0)->isSelected())
        {
            if (!skipTop)
            {
                QTableWidgetItem *itemP0 = ui->timeZonesTW->takeItem(i - 1, 0);
                QTableWidgetItem *itemP1 = ui->timeZonesTW->takeItem(i - 1, 1);
                QTableWidgetItem *itemT0 = ui->timeZonesTW->takeItem(i, 0);
                QTableWidgetItem *itemT1 = ui->timeZonesTW->takeItem(i, 1);

                ui->timeZonesTW->setItem(i - 1, 0, itemT0);
                ui->timeZonesTW->setItem(i - 1, 1, itemT1);
                ui->timeZonesTW->setItem(i, 0, itemP0);
                ui->timeZonesTW->setItem(i, 1, itemP1);

                itemT0->setSelected(true);
                itemT1->setSelected(true);
                itemP0->setSelected(false);
                itemP1->setSelected(false);
            }
        }
        else
            skipTop = false;
    }

    saveSettings();
}

void LXQtWorldClockConfiguration::moveTimeZoneDown()
{
    int m = ui->timeZonesTW->rowCount();
    bool skipBottom = true;
    for (int i = m - 1; i >= 0; --i)
    {
        if (ui->timeZonesTW->item(i, 0)->isSelected())
        {
            if (!skipBottom)
            {
                QTableWidgetItem *itemN0 = ui->timeZonesTW->takeItem(i + 1, 0);
                QTableWidgetItem *itemN1 = ui->timeZonesTW->takeItem(i + 1, 1);
                QTableWidgetItem *itemT0 = ui->timeZonesTW->takeItem(i, 0);
                QTableWidgetItem *itemT1 = ui->timeZonesTW->takeItem(i, 1);

                ui->timeZonesTW->setItem(i + 1, 0, itemT0);
                ui->timeZonesTW->setItem(i + 1, 1, itemT1);
                ui->timeZonesTW->setItem(i, 0, itemN0);
                ui->timeZonesTW->setItem(i, 1, itemN1);

                itemT0->setSelected(true);
                itemT1->setSelected(true);
                itemN0->setSelected(false);
                itemN1->setSelected(false);
            }
        }
        else
            skipBottom = false;
    }

    saveSettings();
}
