/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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

#include <QFileDialog>

#include "advancedsettings.h"
#include "mainwindow.h"


AdvancedSettings::AdvancedSettings(LXQt::Settings* settings, QWidget *parent):
    QWidget(parent),
    mSettings(settings)
{
    setupUi(this);
    restoreSettings();

    connect(serverDecidesBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AdvancedSettings::save);
    connect(spacingBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AdvancedSettings::save);
    connect(widthBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AdvancedSettings::save);
    connect(unattendedBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AdvancedSettings::save);
    connect(blackListEdit, &QLineEdit::editingFinished, this, &AdvancedSettings::save);
    connect(mousebtn, &QCheckBox::clicked, this, &AdvancedSettings::save);
}

AdvancedSettings::~AdvancedSettings()
{
}

void AdvancedSettings::restoreSettings()
{
    int serverDecides = mSettings->value(QL1S("server_decides"), 10).toInt();
    if (serverDecides <= 0)
        serverDecides = 10;
    serverDecidesBox->setValue(serverDecides);

    spacingBox->setValue(mSettings->value(QL1S("spacing"), 6).toInt());
    widthBox->setValue(mSettings->value(QL1S("width"), 300).toInt());
    unattendedBox->setValue(mSettings->value(QL1S("unattendedMaxNum"), 10).toInt());
    blackListEdit->setText(mSettings->value(QL1S("blackList")).toStringList().join (QL1S(",")));

    // true -> Screen with mouse
    // false -> Default, notification will show up on primary screen
    bool screenNotification = mSettings->value(QL1S("screenWithMouse"), false).toBool();

    // TODO: it would be nice to put more options here such as:
    // fixed screen to display notification
    // notification shows in all screens (is it worthy the increased ram usage?)

    if (screenNotification)
        mousebtn->setChecked(true);
    else
        mousebtn->setChecked(false);
}

void AdvancedSettings::save()
{
    mSettings->setValue(QL1S("server_decides"), serverDecidesBox->value());
    mSettings->setValue(QL1S("spacing"), spacingBox->value());
    mSettings->setValue(QL1S("width"), widthBox->value());
    mSettings->setValue(QL1S("unattendedMaxNum"), unattendedBox->value());

    if (mousebtn->isChecked())
        mSettings->setValue(QL1S("screenWithMouse"),true);
    else
        mSettings->setValue(QL1S("screenWithMouse"),false);

    QString blackList = blackListEdit->text();
    if (!blackList.isEmpty())
    {
        QStringList l = blackList.split(QL1S(","), QString::SkipEmptyParts);
        l.removeDuplicates();
        mSettings->setValue(QL1S("blackList"), l);
    }
    else
        mSettings->remove(QL1S("blackList"));
}
