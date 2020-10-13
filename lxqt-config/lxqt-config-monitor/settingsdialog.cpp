/*
    Copyright (C) 2015  P.L. Lucas <selairi@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "settingsdialog.h"
#include "managesavedsettings.h"
#include <KScreen/Output>

SettingsDialog::SettingsDialog(const QString &title, LXQt::Settings *settings, KScreen::ConfigPtr config, QWidget *parent)
    : LXQt::ConfigDialog(title, settings, parent)
{
    setButtons(QDialogButtonBox::QDialogButtonBox::Apply | QDialogButtonBox::Close);
    setWindowIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-display")));

    //DaemonSettings *daemon = new DaemonSettings(settings, this);
    //addPage(daemon, QObject::tr("Daemon"), "system-run");

    ManageSavedSettings * savedSettings = new ManageSavedSettings(settings, config, this);
    addPage(savedSettings, QObject::tr("Manage Saved Settings"), QStringLiteral("system-run"));
}
