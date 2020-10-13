/*
 * watchdogdialog.h
 * This file is part of qps -- Qt-based visual process status monitor
 *
 * Copyright 1997-1999 Mattias Engdegård
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef WATCHDOGDIALOG_H
#define WATCHDOGDIALOG_H

#include "ui_watchdog.h"

#include <QDialog>
#include <QModelIndex>
#include <QString>

class ListModel;
class QShowEvent;

class WatchdogDialog : public QDialog, private Ui_EventDialog
{
    Q_OBJECT
  public:
    WatchdogDialog();
    ListModel *listmodel;
    // signals:
    //    void command_change();
    void checkCombo();
  protected slots:
    void _new();
    void apply();
    void add();
    void del();
    void condChanged(const QString &str);
    void Changed(const QString &str);

    void comboChanged(int);
    void eventcat_slected(const QModelIndex &idx);

  protected:
    void showEvent(QShowEvent *event) override;

    //  void set_select( const QModelIndex & );
    //  void event_name_midified(const QString &new_name);
    //  void event_cmd_modified();
    // void event_toolbar_checked(bool);
  private:
};

#endif // WATCHDOGDIALOG_H
