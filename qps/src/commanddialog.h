/*
 * commanddialog.h
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

#ifndef COMMANDDIALOG_H
#define COMMANDDIALOG_H

#include <QDialog>

class QListView;
class QPushButton;
class QLineEdit;
class QCheckBox;
class QModelIndex;

// class CommandDialog : public QWidget
class CommandDialog : public QDialog
{
    Q_OBJECT
  public:
    CommandDialog();

signals:
    void command_change();

  protected slots:
    void new_cmd();
    void add_new();
    void del_current();
    void set_buttons(int);
    void reset();
    void set_select(const QModelIndex &);
    void event_name_midified(const QString &new_name);
    void event_cmd_modified();
    void event_toolbar_checked(bool);

  private:
    QListView *listview;
    QPushButton *new0, *add, *del, *edit, *button_ok;
    QLineEdit *name, *cmdline;
    QCheckBox *qcheck1;
    QCheckBox *qcheck2;
};

#endif // COMMANDDIALOG_H
