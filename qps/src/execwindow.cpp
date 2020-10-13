/*
 * execwindow.cpp
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

#include "execwindow.h"
#include "watchcond.h"

QList<ExecWindow *> execlist;

// ExecWindow
ExecWindow::ExecWindow()
{
    setupUi(this);
    //	connect(okButton, SIGNAL(clicked()), this, SLOT(close()));
    //	show();
}

ExecWindow::~ExecWindow() {}

// eventcat_id;
ExecWindow::ExecWindow(watchCond *wc, int pid, QString cmd)
{
    setupUi(this);
    setWindowTitle( tr( "Qps Watchdog" ) );

    wcond = wc;
    if (wc->cond == WATCH_PROCESS_START)
    {
        textEdit->append(cmd + "(" + QString::number(pid) + ")" + " start");
    }
    if (wc->cond == WATCH_PROCESS_FINISH)
        textEdit->append(cmd + "(" + QString::number(pid) + ")" + " finished");

    flag_started = false;

    pr = new QProcess;          // leak?
    if (!wc->command.isEmpty()) // conflict pid's command
    {
        pr->start(wc->command); // thread run, if null then segfault occurs. ?
    }

    connect(okButton, SIGNAL(clicked()), this, SLOT(cmd_ok()));

    connect(pr, SIGNAL(started()), this, SLOT(cmd_started()));
    connect(pr, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(cmd_finished(int, QProcess::ExitStatus)));
    connect(pr, SIGNAL(error(QProcess::ProcessError)), this,
            SLOT(cmd_error(QProcess::ProcessError)));

    show();

    execlist.append(this);
}

ExecWindow::ExecWindow(QString /*str*/, QString /*exec_cmd*/, int /*pid*/, QString /*cmd*/)
{
    setupUi(this);
    //
}

// QProcess: Destroyed while process is still running.(Segmentation fault)
void ExecWindow::cmd_ok()
{
    if (pr->state() == QProcess::Running)
    {
        //	pr->kill();
        pr->terminate();
        return;
    }
    close(); // Qt::WA_DeleteOnClose
}

// slot : catch terminate signal.
void ExecWindow::cmd_finished(int /*exitCode*/, QProcess::ExitStatus exitStatus)
{
    textEdit->append( tr( "%1 exit with code %2" ).arg( wcond->command )
                                                  .arg( exitStatus ) );
    okButton->setText( tr( "Close" ) );
    delete pr;
}

void ExecWindow::cmd_started()
{
    textEdit->append( tr( "%1 [running]" ).arg( wcond->command ) );
    okButton->setText( tr( "terminate command" ) );
    flag_started = true;
}

void ExecWindow::cmd_error(QProcess::ProcessError e)
{
    // not found command
    // Error ? :
    if (e == QProcess::FailedToStart)
        textEdit->append( tr( "Error %1 : [%2] Maybe command not found" ).arg( e )
                                                                         .arg( wcond->command ) );
    delete pr;
}

void ExecWindow::setText(QString str)
{
    textEdit->append(str);
    //	label->setText(str);
}
