/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
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

#include "spacerconfiguration.h"
#include "ui_spacerconfiguration.h"
#include <QPushButton>

//Note: strings can't actually be translated here (in static initialization time)
//      the QT_TR_NOOP here is just for qt translate tools to get the strings for translation
const QStringList SpacerConfiguration::msTypes = {
    QStringLiteral(QT_TR_NOOP("lined"))
    , QStringLiteral(QT_TR_NOOP("dotted"))
    , QStringLiteral(QT_TR_NOOP("invisible"))
};

SpacerConfiguration::SpacerConfiguration(PluginSettings *settings, QWidget *parent) :
    LXQtPanelPluginConfigDialog(settings, parent),
    ui(new Ui::SpacerConfiguration)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("SpacerConfigurationWindow"));
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Close)->setText(tr("Close"));
    //Note: translation is needed here in runtime (translator is attached already)
    for (auto const & type : msTypes)
        ui->typeCB->addItem(tr(type.toStdString().c_str()), type);

    loadSettings();

    connect(ui->sizeSB, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SpacerConfiguration::sizeChanged);
    connect(ui->typeCB, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this, &SpacerConfiguration::typeChanged);
    //Note: if there will be more than 2 radio buttons for width/size type, this simple setting logic will break
    connect(ui->sizeExpandRB, &QAbstractButton::toggled, this, &SpacerConfiguration::widthTypeChanged);
}

SpacerConfiguration::~SpacerConfiguration()
{
    delete ui;
}

void SpacerConfiguration::loadSettings()
{
    ui->sizeSB->setValue(settings().value(QStringLiteral("size"), 8).toInt());
    ui->typeCB->setCurrentIndex(ui->typeCB->findData(settings().value(QStringLiteral("spaceType"), msTypes[0]).toString()));
    const bool expandable = settings().value(QStringLiteral("expandable"), false).toBool();
    ui->sizeExpandRB->setChecked(expandable);
    ui->sizeFixedRB->setChecked(!expandable);
    ui->sizeSB->setDisabled(expandable);
}

void SpacerConfiguration::sizeChanged(int value)
{
    settings().setValue(QStringLiteral("size"), value);
}

void SpacerConfiguration::typeChanged(int index)
{
    settings().setValue(QStringLiteral("spaceType"), ui->typeCB->itemData(index, Qt::UserRole));
}

void SpacerConfiguration::widthTypeChanged(bool expandableChecked)
{
    settings().setValue(QStringLiteral("expandable"), expandableChecked);
}
