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


#include "lxqtpanelpluginconfigdialog.h"


#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QDebug>
/************************************************

 ************************************************/
LXQtPanelPluginConfigDialog::LXQtPanelPluginConfigDialog(PluginSettings &settings, QWidget *parent) :
    QDialog(parent),
    mSettings(settings)
{
}


/************************************************

 ************************************************/
LXQtPanelPluginConfigDialog::~LXQtPanelPluginConfigDialog()
{
}


/************************************************

 ************************************************/
PluginSettings& LXQtPanelPluginConfigDialog::settings() const
{
    return mSettings;
}



/************************************************

 ************************************************/
void LXQtPanelPluginConfigDialog::dialogButtonsAction(QAbstractButton *btn)
{
    QDialogButtonBox *box = qobject_cast<QDialogButtonBox*>(btn->parent());

    if (box && box->buttonRole(btn) == QDialogButtonBox::ResetRole)
    {
        mSettings.loadFromCache();
        loadSettings();
    }
    else
    {
        close();
    }
}


/************************************************

 ************************************************/
void LXQtPanelPluginConfigDialog::setComboboxIndexByData(QComboBox *comboBox, const QVariant &data, int defaultIndex) const
{
    int index = comboBox ->findData(data);
    if (index < 0)
        index = defaultIndex;

    comboBox->setCurrentIndex(index);
}
