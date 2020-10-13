/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *   Dmitriy Zhukov <zjesclean@gmail.com>
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

#include <LXQt/Globals>

#include <QDebug>
#include <QProcess>
#include <QPushButton>
#include "kbdstateconfig.h"
#include "ui_kbdstateconfig.h"
#include "settings.h"

KbdStateConfig::KbdStateConfig(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::KbdStateConfig)
{
    setAttribute(Qt::WA_DeleteOnClose);
    m_ui->setupUi(this);
    m_ui->buttonBox_1->button(QDialogButtonBox::Reset)->setText(tr("Reset"));
    m_ui->buttonBox_2->button(QDialogButtonBox::Close)->setText(tr("Close"));
    connect(m_ui->showCaps,   &QCheckBox::clicked, this, &KbdStateConfig::save);
    connect(m_ui->showNum,    &QCheckBox::clicked, this, &KbdStateConfig::save);
    connect(m_ui->showScroll, &QCheckBox::clicked, this, &KbdStateConfig::save);
    connect(m_ui->showLayout, &QGroupBox::clicked, this, &KbdStateConfig::save);
    connect(m_ui->layoutFlagPattern, &QLineEdit::textEdited, this, &KbdStateConfig::save);
    connect(m_ui->modes, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            [this](int){
        KbdStateConfig::save();
    }
    );

    connect(m_ui->buttonBox_1, &QDialogButtonBox::clicked, [this](QAbstractButton *btn){
        if (m_ui->buttonBox_1->buttonRole(btn) == QDialogButtonBox::ResetRole){
            Settings::instance().restore();
            load();
        }
    });

    connect(m_ui->configureLayouts, &QPushButton::clicked, this, &KbdStateConfig::configureLayouts);

    load();
}

KbdStateConfig::~KbdStateConfig()
{
    delete m_ui;
}

void KbdStateConfig::load()
{
    Settings & sets = Settings::instance();

    m_ui->showCaps->setChecked(sets.showCapLock());
    m_ui->showNum->setChecked(sets.showNumLock());
    m_ui->showScroll->setChecked(sets.showScrollLock());
    m_ui->showLayout->setChecked(sets.showLayout());
    m_ui->layoutFlagPattern->setText(sets.layoutFlagPattern());

    switch(sets.keeperType()){
    case KeeperType::Global:
        m_ui->switchGlobal->setChecked(true);
        break;
    case KeeperType::Window:
        m_ui->switchWindow->setChecked(true);
        break;
    case KeeperType::Application:
        m_ui->switchApplication->setChecked(true);
        break;
    }
}

void KbdStateConfig::save()
{
    Settings & sets = Settings::instance();

    sets.setShowCapLock(m_ui->showCaps->isChecked());
    sets.setShowNumLock(m_ui->showNum->isChecked());
    sets.setShowScrollLock(m_ui->showScroll->isChecked());
    sets.setShowLayout(m_ui->showLayout->isChecked());
    sets.setLayoutFlagPattern(m_ui->layoutFlagPattern->text());

    if (m_ui->switchGlobal->isChecked())
        sets.setKeeperType(KeeperType::Global);
    if (m_ui->switchWindow->isChecked())
        sets.setKeeperType(KeeperType::Window);
    if (m_ui->switchApplication->isChecked())
        sets.setKeeperType(KeeperType::Application);
}

void KbdStateConfig::configureLayouts()
{
    QProcess::startDetached(QL1S("lxqt-config-input --show-page \"Keyboard Layout\""));
}
