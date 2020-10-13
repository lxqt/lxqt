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

#ifndef ADVANCEDSETTINGS_H
#define ADVANCEDSETTINGS_H

#include <LXQt/Settings>
#include <QWidget>
#include "ui_advancedsettings.h"

class AdvancedSettings : public QWidget, public Ui::AdvancedSettings
{
    Q_OBJECT

public:
    explicit AdvancedSettings(LXQt::Settings* settings, QWidget* parent = nullptr);
    ~AdvancedSettings();

public slots:
    void restoreSettings();

private:
    LXQt::Settings* mSettings;

private slots:
    void save();

};

#endif // MENUCONFIG_H
