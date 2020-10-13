/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
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

#ifndef FONTSCONFIG_H
#define FONTSCONFIG_H

#include <QWidget>
#include <QFont>
#include <LXQt/Settings>
#include "fontconfigfile.h"

class QTreeWidgetItem;
class QSettings;

namespace Ui {
    class FontsConfig;
}

class FontsConfig : public QWidget
{
    Q_OBJECT

public:
    explicit FontsConfig(LXQt::Settings *settings, QSettings *qtSettings, QWidget *parent = 0);
    ~FontsConfig();

    void updateQtFont();

public Q_SLOTS:
    void initControls();

signals:
    void settingsChanged();
    void updateOtherSettings();

private:
    Ui::FontsConfig *ui;
    QSettings *mQtSettings;
    LXQt::Settings *mSettings;
    FontConfigFile mFontConfigFile;
};

#endif // FONTSCONFIG_H
