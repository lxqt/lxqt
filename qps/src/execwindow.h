/*
 * execwindow.h
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

#ifndef EXECWINDOW_H
#define EXECWINDOW_H

#include <QProcess>
#include <QString>

#include "ui_message.h"

class watchCond;

class ExecWindow : public QWidget, private Ui_ExecWindow
{
    Q_OBJECT
  public:
    ExecWindow();
    ExecWindow(QString str, QString exec_cmd, int pid = 0, QString cmd = "");
    ExecWindow(watchCond *wc, int pid = 0, QString cmd = "");
    ~ExecWindow() override;
    void setText(QString str);
    QProcess *pr;
    // QProcess 	proc;
    QString execmd;
    int flag_started;
    watchCond *wcond;
  protected slots:
    void cmd_started();
    void cmd_finished(int exitCode, QProcess::ExitStatus exitStatus);
    void cmd_error(QProcess::ProcessError error);
    void cmd_ok();
};

#endif // EXECWINDOW_H
