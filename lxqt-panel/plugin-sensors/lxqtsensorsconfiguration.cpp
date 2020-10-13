/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Łukasz Twarduś <ltwardus@gmail.com>
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
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "lxqtsensorsconfiguration.h"
#include "ui_lxqtsensorsconfiguration.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QDebug>
#include <QPushButton>
#include <QStringList>


LXQtSensorsConfiguration::LXQtSensorsConfiguration(PluginSettings *settings, QWidget *parent) :
    LXQtPanelPluginConfigDialog(settings, parent),
    ui(new Ui::LXQtSensorsConfiguration)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("SensorsConfigurationWindow"));
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Reset)->setText(tr("Reset"));
    ui->buttonBox_2->button(QDialogButtonBox::Close)->setText(tr("Close"));
    // We load settings here cause we have to set up dynamic widgets
    loadSettings();
    connect(ui->buttonBox_1, SIGNAL(clicked(QAbstractButton*)), this, SLOT(dialogButtonsAction(QAbstractButton*)));
    connect(ui->updateIntervalSB, SIGNAL(valueChanged(int)), this, SLOT(saveSettings()));
    connect(ui->tempBarWidthSB, SIGNAL(valueChanged(int)), this, SLOT(saveSettings()));
    connect(ui->detectedChipsCB, SIGNAL(activated(int)), this, SLOT(detectedChipSelected(int)));
    connect(ui->celsiusTempScaleRB, SIGNAL(toggled(bool)), this, SLOT(saveSettings()));
    // We don't need signal from the other radio box as celsiusTempScaleRB will send one
    //connect(ui->fahrenheitTempScaleRB, SIGNAL(toggled(bool)), this, SLOT(saveSettings()));
    connect(ui->warningAboutHighTemperatureChB, SIGNAL(toggled(bool)), this, SLOT(saveSettings()));

    /**
     * Signals for enable/disable and bar color change are set in the loadSettings method because
     * we are creating them dynamically.
     */
}


LXQtSensorsConfiguration::~LXQtSensorsConfiguration()
{
    delete ui;
}


void LXQtSensorsConfiguration::loadSettings()
{
    ui->updateIntervalSB->setValue(settings().value(QStringLiteral("updateInterval")).toInt());
    ui->tempBarWidthSB->setValue(settings().value(QStringLiteral("tempBarWidth")).toInt());

    if (settings().value(QStringLiteral("useFahrenheitScale")).toBool())
    {
        ui->fahrenheitTempScaleRB->setChecked(true);
    }

    // In case of reloading settings we have to clear GUI elements
    ui->detectedChipsCB->clear();

    settings().beginGroup(QStringLiteral("chips"));
    QStringList chipNames = settings().childGroups();

    for (int i = 0; i < chipNames.size(); ++i)
    {
        ui->detectedChipsCB->addItem(chipNames[i]);
    }
    settings().endGroup();

    // Load feature for the first chip if exist
    if (chipNames.size() > 0)
    {
        detectedChipSelected(0);
    }

    ui->warningAboutHighTemperatureChB->setChecked(
            settings().value(QStringLiteral("warningAboutHighTemperature")).toBool());
}


void LXQtSensorsConfiguration::saveSettings()
{
    settings().setValue(QStringLiteral("updateInterval"), ui->updateIntervalSB->value());
    settings().setValue(QStringLiteral("tempBarWidth"), ui->tempBarWidthSB->value());

    if (ui->fahrenheitTempScaleRB->isChecked())
    {
        settings().setValue(QStringLiteral("useFahrenheitScale"), true);
    }
    else
    {
        settings().setValue(QStringLiteral("useFahrenheitScale"), false);
    }

    settings().beginGroup(QStringLiteral("chips"));
    QStringList chipNames = settings().childGroups();

    if (chipNames.size())
    {
        QStringList chipFeatureLabels;
        QPushButton* colorButton = nullptr;
        QCheckBox* enabledCheckbox = nullptr;

        settings().beginGroup(chipNames[ui->detectedChipsCB->currentIndex()]);

        chipFeatureLabels = settings().childGroups();
        for (int j = 0; j < chipFeatureLabels.size(); ++j)
        {
            settings().beginGroup(chipFeatureLabels[j]);

            enabledCheckbox = qobject_cast<QCheckBox*>(ui->chipFeaturesT->cellWidget(j, 0));
            // We know what we are doing so we don't have to check if enabledCheckbox == 0
            settings().setValue(QStringLiteral("enabled"), enabledCheckbox->isChecked());

            colorButton = qobject_cast<QPushButton*>(ui->chipFeaturesT->cellWidget(j, 2));
            // We know what we are doing so we don't have to check if colorButton == 0
            settings().setValue(
                    QStringLiteral("color"),
                    colorButton->palette().color(QPalette::Normal, QPalette::Button).name());

            settings().endGroup();
        }
        settings().endGroup();

    }
    settings().endGroup();

    settings().setValue(QStringLiteral("warningAboutHighTemperature"),
                       ui->warningAboutHighTemperatureChB->isChecked());
}

void LXQtSensorsConfiguration::changeProgressBarColor()
{
    QAbstractButton* btn = qobject_cast<QAbstractButton*>(sender());

    if (btn)
    {
        QPalette pal = btn->palette();
        QColor color = QColorDialog::getColor(pal.color(QPalette::Normal, QPalette::Button), this);

        if (color.isValid())
        {
            pal.setColor(QPalette::Normal, QPalette::Button, color);
            btn->setPalette(pal);
            saveSettings();
        }
    }
    else
    {
        qDebug() << "LXQtSensorsConfiguration::changeProgressBarColor():" << "invalid button cast";
    }
}


void LXQtSensorsConfiguration::detectedChipSelected(int index)
{
    settings().beginGroup(QStringLiteral("chips"));
    QStringList chipNames = settings().childGroups();
    QStringList chipFeatureLabels;
    QPushButton* colorButton = nullptr;
    QCheckBox* enabledCheckbox = nullptr;
    QTableWidgetItem *chipFeatureLabel = nullptr;

    if (index < chipNames.size())
    {
        qDebug() << "Selected chip: " << ui->detectedChipsCB->currentText();

        // In case of reloading settings we have to clear GUI elements
        ui->chipFeaturesT->setRowCount(0);

        // Add detected chips and features
        QStringList chipFeaturesLabels;
        chipFeaturesLabels << tr("Enabled") << tr("Label") << tr("Color");
        ui->chipFeaturesT->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        ui->chipFeaturesT->setHorizontalHeaderLabels(chipFeaturesLabels);

        settings().beginGroup(chipNames[index]);
        chipFeatureLabels = settings().childGroups();
        for (int j = 0; j < chipFeatureLabels.size(); ++j)
        {
            settings().beginGroup(chipFeatureLabels[j]);

            ui->chipFeaturesT->insertRow(j);

            enabledCheckbox = new QCheckBox(ui->chipFeaturesT);
            enabledCheckbox->setChecked(settings().value(QStringLiteral("enabled")).toBool());
            // Connect here after the setChecked call because we don't want to send signal
            connect(enabledCheckbox, SIGNAL(stateChanged(int)), this, SLOT(saveSettings()));
            ui->chipFeaturesT->setCellWidget(j, 0, enabledCheckbox);

            chipFeatureLabel = new QTableWidgetItem(chipFeatureLabels[j]);
            chipFeatureLabel->setFlags(Qt::ItemIsEnabled);
            ui->chipFeaturesT->setItem(j, 1, chipFeatureLabel);

            colorButton = new QPushButton(ui->chipFeaturesT);
            connect(colorButton, SIGNAL(clicked()), this, SLOT(changeProgressBarColor()));
            QPalette pal = colorButton->palette();
            pal.setColor(QPalette::Normal, QPalette::Button,
                         QColor(settings().value(QStringLiteral("color")).toString()));
            colorButton->setPalette(pal);
            ui->chipFeaturesT->setCellWidget(j, 2, colorButton);

            settings().endGroup();
        }
        settings().endGroup();
    }
    else
    {
        qDebug() << "Invalid chip index: " << index;
    }

    settings().endGroup();
}
