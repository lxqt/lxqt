/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
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


#ifndef LXQT_ADDPLUGINDIALOG_H
#define LXQT_ADDPLUGINDIALOG_H

#include <LXQt/PluginInfo>
#include <QDialog>
#include <QTimer>

#define SEARCH_DELAY 125

namespace Ui {
    class AddPluginDialog;
}

class AddPluginDialog : public QDialog
{
    Q_OBJECT

public:
    AddPluginDialog(QWidget *parent = nullptr);
    ~AddPluginDialog();

signals:
    void pluginSelected(const LXQt::PluginInfo &plugin);

private:
    Ui::AddPluginDialog *ui;
    LXQt::PluginInfoList mPlugins;
    QTimer mSearchTimer;

private slots:
    void filter();
    void emitPluginSelected();
};

#endif // LXQT_ADDPLUGINDIALOG_H
