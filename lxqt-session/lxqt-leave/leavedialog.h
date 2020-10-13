/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https:///lxqt.org/
 *
 * Copyright: 2010-2015 LXQt team
 * Authors:
 *   Paulo Lieuthier <paulolieuthier@gmail.com>
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
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef LEAVEDIALOG_H
#define LEAVEDIALOG_H

#include "ui_leavedialog.h"

#include <QDialog>
#include <LXQt/Power>
#include <LXQt/PowerManager>
#include <LXQt/ScreenSaver>

namespace Ui {
    class LeaveDialog;
}

class LeaveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LeaveDialog(QWidget *parent = nullptr);
    ~LeaveDialog() override;

private:
    Ui::LeaveDialog *ui;
    // LXQt::Power is used to know if the actions are doable, while
    // LXQt::PowerManager is used to trigger the actions, while
    // obeying the user option to ask or not for confirmation
    LXQt::Power *mPower;
    LXQt::PowerManager *mPowerManager;
    LXQt::ScreenSaver *mScreensaver;
};


#endif
