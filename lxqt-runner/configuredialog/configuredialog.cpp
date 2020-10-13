/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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


#include "configuredialog.h"
#include "ui_configuredialog.h"

#include <LXQt/Globals>
#include <LXQt/Settings>

#include <QSettings>
#include <QGuiApplication>
#include <QComboBox>
#include <QDebug>
#include <QKeySequence>
#include <QPushButton>
#include <QCloseEvent>
#include <QAction>
#include <QPushButton>


/************************************************

 ************************************************/
ConfigureDialog::ConfigureDialog(QSettings *settings, const QString &defaultShortcut, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigureDialog),
    mSettings(settings),
    mOldSettings(new LXQt::SettingsCache(settings)),
    mDefaultShortcut(defaultShortcut)
{
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Reset)->setText(tr("Reset"));
    ui->buttonBox_2->button(QDialogButtonBox::Close)->setText(tr("Close"));
    connect(ui->buttonBox_1->button(QDialogButtonBox::Reset), SIGNAL(clicked()), this, SLOT(resetSettings()));
    // Position .................................
    ui->positionCbx->addItem(tr("Top edge of the screen"), QVariant(ConfigureDialog::PositionTop));
    ui->positionCbx->addItem(tr("Center of the screen"), QVariant(ConfigureDialog::PositionCenter));
    connect(ui->positionCbx, SIGNAL(currentIndexChanged(int)), this, SLOT(positionCbxChanged(int)));

    // Monitor ..................................

    ui->monitorCbx->addItem(tr("Focused screen"), QVariant(-1));

    const int monCnt = QGuiApplication::screens().size();
    for (int i = 0; i < monCnt; ++i)
        ui->monitorCbx->addItem(tr("Always on screen %1").arg(i + 1), QVariant(i));

    ui->monitorCbx->setEnabled(monCnt > 1);
    connect(ui->monitorCbx, SIGNAL(currentIndexChanged(int)), this, SLOT(monitorCbxChanged(int)));


    // Shortcut .................................
    connect(ui->shortcutEd, SIGNAL(shortcutGrabbed(QString)), this, SLOT(shortcutChanged(QString)));

    connect(ui->shortcutEd->addMenuAction(tr("Reset")), SIGNAL(triggered()), this, SLOT(shortcutReset()));

    settingsChanged();

    connect(ui->historyUseCb, &QAbstractButton::toggled, [this] (bool checked) { mSettings->setValue(QL1S("dialog/history_use"), checked); });
    connect(ui->historyFirstCb, &QAbstractButton::toggled, [this] (bool checked) { mSettings->setValue(QL1S("dialog/history_first"), checked); });
    connect(ui->listShownItemsSB, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this] (int i) { mSettings->setValue(QL1S("dialog/list_shown_items"), i); });
}


/************************************************

 ************************************************/
void ConfigureDialog::settingsChanged()
{
    if (mSettings->value(QL1S("dialog/show_on_top"), true).toBool())
        ui->positionCbx->setCurrentIndex(0);
    else
        ui->positionCbx->setCurrentIndex(1);

    ui->monitorCbx->setCurrentIndex(mSettings->value(QL1S("dialog/monitor"), -1).toInt() + 1);
    ui->shortcutEd->setText(mSettings->value(QL1S("dialog/shortcut"), QL1S("Alt+F2")).toString());
    const bool history_use = mSettings->value(QL1S("dialog/history_use"), true).toBool();
    ui->historyUseCb->setChecked(history_use);
    ui->historyFirstCb->setChecked(mSettings->value(QL1S("dialog/history_first"), true).toBool());
    ui->historyFirstCb->setEnabled(history_use);
    ui->listShownItemsSB->setValue(mSettings->value(QL1S("dialog/list_shown_items"), 4).toInt());
}


/************************************************

 ************************************************/
ConfigureDialog::~ConfigureDialog()
{
    delete mOldSettings;
    delete ui;
}


/************************************************

 ************************************************/
void ConfigureDialog::shortcutChanged(const QString &text)
{
    ui->shortcutEd->setText(text);
    mSettings->setValue(QL1S("dialog/shortcut"), text);
}


/************************************************

 ************************************************/
void ConfigureDialog::shortcutReset()
{
    shortcutChanged(mDefaultShortcut);
}


/************************************************

 ************************************************/
void ConfigureDialog::positionCbxChanged(int index)
{
    mSettings->setValue(QL1S("dialog/show_on_top"), index == 0);
}


/************************************************

 ************************************************/
void ConfigureDialog::monitorCbxChanged(int index)
{
    mSettings->setValue(QL1S("dialog/monitor"), index - 1);
}


/************************************************

 ************************************************/
void ConfigureDialog::resetSettings()
{
    mOldSettings->loadToSettings();
    ui->shortcutEd->clear();
    settingsChanged();
}
