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

#ifndef GLOBAL_ACTION_CONFIG__MAIN_WINDOW__INCLUDED
#define GLOBAL_ACTION_CONFIG__MAIN_WINDOW__INCLUDED


#include "ui_main_window.h"
#include "../daemon/meta_types.h"


class Actions;
class DefaultModel;
class QItemSelectionModel;
class QSortFilterProxyModel;
class EditActionDialog;

class MainWindow : public QDialog, private Ui::MainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

public slots:
    void daemonDisappeared();
    void daemonAppeared();

    void multipleActionsBehaviourChanged(MultipleActionsBehaviour behaviour);

protected:
    void changeEvent(QEvent *e) override;

protected slots:
    void selectionChanged(const QItemSelection &, const QItemSelection &);

    void on_add_PB_clicked();
    void on_modify_PB_clicked();
    void on_swap_PB_clicked();
    void on_remove_PB_clicked();
    void on_default_PB_clicked();

    void on_multipleActionsBehaviour_CB_currentIndexChanged(int);

    void on_actions_TV_doubleClicked(const QModelIndex &);

private:
    Actions *mActions;
    DefaultModel *mDefaultModel;
    QSortFilterProxyModel *mSortFilterProxyModel;
    QItemSelectionModel *mSelectionModel;
    EditActionDialog *mEditActionDialog;

    void editAction(const QModelIndex &);
};

#endif // GLOBAL_ACTION_CONFIG__MAIN_WINDOW__INCLUDED
