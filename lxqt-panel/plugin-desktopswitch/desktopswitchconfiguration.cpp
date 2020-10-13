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

#include "desktopswitchconfiguration.h"
#include "ui_desktopswitchconfiguration.h"
#include <KWindowSystem>
#include <QTimer>
#include <QPushButton>
DesktopSwitchConfiguration::DesktopSwitchConfiguration(PluginSettings *settings, QWidget *parent) :
    LXQtPanelPluginConfigDialog(settings, parent),
    ui(new Ui::DesktopSwitchConfiguration)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("DesktopSwitchConfigurationWindow"));
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Close)->setText(tr("Close"));
    connect(ui->buttonBox_1, SIGNAL(clicked(QAbstractButton*)), this, SLOT(dialogButtonsAction(QAbstractButton*)));

    loadSettings();

    connect(ui->rowsSB, SIGNAL(valueChanged(int)), this, SLOT(rowsChanged(int)));
    connect(ui->labelTypeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(labelTypeChanged(int)));
    connect(ui->showOnlyActiveCB, &QAbstractButton::toggled, [this] (bool checked) { this->settings().setValue(QStringLiteral("showOnlyActive"), checked); });

    loadDesktopsNames();
}

DesktopSwitchConfiguration::~DesktopSwitchConfiguration()
{
    delete ui;
}

void DesktopSwitchConfiguration::loadSettings()
{
    ui->rowsSB->setValue(settings().value(QStringLiteral("rows"), 1).toInt());
    ui->labelTypeCB->setCurrentIndex(settings().value(QStringLiteral("labelType"), 0).toInt());
    ui->showOnlyActiveCB->setChecked(settings().value(QStringLiteral("showOnlyActive"), false).toBool());
}

void DesktopSwitchConfiguration::loadDesktopsNames()
{
    int n = KWindowSystem::numberOfDesktops();
    for (int i = 1; i <= n; i++)
    {
        QLineEdit *edit = new QLineEdit(KWindowSystem::desktopName(i), this);
        ((QFormLayout *) ui->namesGroupBox->layout())->addRow(QStringLiteral("Desktop %1:").arg(i), edit);

        // C++11 rocks!
        QTimer *timer = new QTimer(this);
        timer->setInterval(400);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, [=] { KWindowSystem::setDesktopName(i, edit->text()); });
        connect(edit, &QLineEdit::textEdited, [=] { timer->start(); });
    }
}

void DesktopSwitchConfiguration::rowsChanged(int value)
{
    settings().setValue(QStringLiteral("rows"), value);
}

void DesktopSwitchConfiguration::labelTypeChanged(int type)
{
    settings().setValue(QStringLiteral("labelType"), type);
}
