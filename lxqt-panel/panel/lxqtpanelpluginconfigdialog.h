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


#ifndef LXQTPANELPLUGINCONFIGDIALOG_H
#define LXQTPANELPLUGINCONFIGDIALOG_H

#include <QAbstractButton>
#include <QDialog>
#include "lxqtpanelglobals.h"
#include "pluginsettings.h"

class QComboBox;

class LXQT_PANEL_API LXQtPanelPluginConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LXQtPanelPluginConfigDialog(PluginSettings &settings, QWidget *parent = nullptr);
    explicit LXQtPanelPluginConfigDialog(PluginSettings *settings, QWidget *parent = nullptr) : LXQtPanelPluginConfigDialog(*settings, parent) {}
    virtual ~LXQtPanelPluginConfigDialog();

    PluginSettings &settings() const;

protected slots:
    /*
      Saves settings in conf file.
    */
    virtual void loadSettings() = 0;
    virtual void dialogButtonsAction(QAbstractButton *btn);

protected:
    void setComboboxIndexByData(QComboBox *comboBox, const QVariant &data, int defaultIndex = 0) const;

private:
    PluginSettings &mSettings;

};

#endif // LXQTPANELPLUGINCONFIGDIALOG_H
