/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
 * Authors:
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#ifndef GLOBAL_ACTION_CONFIG__EDIT_ACTION_DIALOG__INCLUDED
#define GLOBAL_ACTION_CONFIG__EDIT_ACTION_DIALOG__INCLUDED


#include "ui_edit_action_dialog.h"


class Actions;

class EditActionDialog : public QDialog, private Ui::EditActionDialog
{
    Q_OBJECT

public:
    explicit EditActionDialog(Actions *actions, QWidget *parent = nullptr);

    bool load(qulonglong id);

protected slots:
    void when_accepted();

    void on_shortcut_SS_shortcutGrabbed(const QString &);
    void on_command_RB_clicked(bool);
    void on_dbus_method_RB_clicked(bool);

protected:
    void changeEvent(QEvent *e) override;

private:
    Actions *mActions;
    qulonglong mId;
    QString mShortcut;
};

#endif // GLOBAL_ACTION_CONFIG__EDIT_ACTION_DIALOG__INCLUDED
