/*
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright (C) 2012  Alec Moskvin <alecm@gmx.com>
 * Copyright (C) 2018  Luís Pereira <luis.artur.pereira@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef LXQTCONFIGDIALOG_P_H
#define LXQTCONFIGDIALOG_P_H

#include <QHash>
#include <QList>
#include <QSize>
#include <QStringList>

namespace Ui {
class ConfigDialog;
}

class QAbstractButton;
namespace LXQt
{

class ConfigDialog;
class Settings;
class SettingsCache;

class Q_DECL_HIDDEN ConfigDialogPrivate
{
    Q_DECLARE_PUBLIC(ConfigDialog)
    ConfigDialog* const q_ptr;

public:
    ConfigDialogPrivate(ConfigDialog *q, Settings* settings);
    ~ConfigDialogPrivate();

    void init();
    void dialogButtonsAction(QAbstractButton* button);
    void updateIcons();

    SettingsCache* mCache;
    QList<QStringList> mIcons;
    QSize mMaxSize;
    Ui::ConfigDialog* ui;
    QHash<QString, QWidget*> mPages;
};

} // namespace LXQt

#endif // LXQTCONFIGDIALOG_P_H
