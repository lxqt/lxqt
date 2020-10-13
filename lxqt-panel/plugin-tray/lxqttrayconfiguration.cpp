/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2019 LXQt team
 * Authors:
 *   John Lindgren <john@jlindgren.net>
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

#include "lxqttrayconfiguration.h"
#include "ui_lxqttrayconfiguration.h"
#include <QPushButton>
LXQtTrayConfiguration::LXQtTrayConfiguration(PluginSettings *settings, QWidget *parent) :
    LXQtPanelPluginConfigDialog(settings, parent),
    ui(new Ui::LXQtTrayConfiguration)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Reset)->setText(tr("Reset"));
    ui->buttonBox_2->button(QDialogButtonBox::Close)->setText(tr("Close"));
    loadSettings();
    connect(ui->sortIconsCB, &QAbstractButton::toggled, [settings](bool value) {
        settings->setValue(QStringLiteral("sortIcons"), value);
    });
    connect(ui->spacingSB, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [settings](int value) {
        settings->setValue(QStringLiteral("spacing"), value);
    });

    connect(ui->buttonBox_1, &QDialogButtonBox::clicked, this, &LXQtTrayConfiguration::dialogButtonsAction);
}

LXQtTrayConfiguration::~LXQtTrayConfiguration() {}

void LXQtTrayConfiguration::loadSettings()
{
    ui->sortIconsCB->setChecked(settings().value(QStringLiteral("sortIcons"), false).toBool());
    ui->spacingSB->setValue(settings().value(QStringLiteral("spacing"), 0).toInt());
}
