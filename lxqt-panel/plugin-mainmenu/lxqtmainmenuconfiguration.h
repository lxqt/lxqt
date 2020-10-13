/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
 * Authors:
 *   Maciej Płaza <plaza.maciej@gmail.com>
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


#ifndef LXQTMAINMENUCONFIGURATION_H
#define LXQTMAINMENUCONFIGURATION_H

#include "../panel/lxqtpanelpluginconfigdialog.h"
#include "../panel/pluginsettings.h"

class QAbstractButton;

namespace Ui {
    class LXQtMainMenuConfiguration;
}

namespace GlobalKeyShortcut {
    class Action;
}

class LXQtMainMenuConfiguration : public LXQtPanelPluginConfigDialog
{
    Q_OBJECT

public:
    explicit LXQtMainMenuConfiguration(PluginSettings *settings,
                                       GlobalKeyShortcut::Action *shortcut,
                                       const QString &defaultShortcut,
                                       QWidget *parent = nullptr);
    ~LXQtMainMenuConfiguration();

private:
    Ui::LXQtMainMenuConfiguration *ui;
    QString mDefaultShortcut;
    GlobalKeyShortcut::Action * mShortcut;

private slots:
    void globalShortcutChanged(const QString &oldShortcut, const QString &newShortcut);
    void shortcutChanged(const QString &value);
    /*
      Saves settings in conf file.
    */
    void loadSettings();
    void textButtonChanged(const QString &value);
    void showTextChanged(bool value);
    void chooseIcon();
    void chooseMenuFile();
    void shortcutReset();
    void customFontChanged(bool value);
    void customFontSizeChanged(int value);
};

#endif // LXQTMAINMENUCONFIGURATION_H
