/*
 * command.cpp
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

#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <QMessageBox>

#include "command.h"
#include "commandutils.h"
#include "checkboxdelegate.h"
#include "watchcond.h"
#include "execwindow.h"
#include "listmodel.h"
#include "watchdogdialog.h"
#include "qps.h"
#include "proc.h"
#include "uidstr.h"
#include "dialogs.h"

extern Qps *qps;
extern ControlBar *controlbar;
extern QList<Command *> commands;

Command::Command(QString n, QString cmd, bool /*flag*/)
{
    // printf("Command init\n");
    name = n;
    cmdline = cmd;
    toolbar = false;
    popup = false;

    // toolbutton=new CommandButton(controlbar,name);
    // toolbutton->hide();

    ////toolbutton->setTextLabel (name) ;
    ////toolbutton->setUsesTextLabel ( true );
    /// toolbutton->setAutoRaise(true);
    // QObject::connect(toolbutton, SIGNAL(clicked()),toolbutton,
    // SLOT(exec_command()));
}

Command::~Command()
{
    // toolbutton->hide();
    // delete toolbutton;
}

QString Command::getString()
{
    QString str;
    str.append(name);
    str.append(",");
    str.append(cmdline);
    return str;
}

void Command::putString(QString str)
{
    int idx = str.indexOf(",");
    // idx=str.indexOf(",",idx);
    name = str.mid(0, idx);
    cmdline = str.mid(idx + 1, str.size());
}

// dirty code...
// Description : this command need "select process"
bool Command::IsNeedProc()
{
    int i, len;
    len = cmdline.length();

    for (i = 0; i < len;)
    {
        int v = cmdline.indexOf('%', i);
        if (v < 0)
            break;
        if (++v >= len)
            break;

        char c = cmdline[v].cell(); //.toLatin1().data();
        switch (c)
        {
        case 'p':
        case 'c':
        case 'C':
        case 'u':
            if (cmdline.indexOf("update", v) == v)
            {
                v += 5;
                break;
            }
            // printf("true\n");
            return true;
        default:
            ;
        }
        i = v + 1;
    }
    return false;
}

QString substString(QString str, Procinfo *p)
{
    QString s;
    int len = str.length();
    for (int i = 0;;)
    {
        int v = str.indexOf('%', i);
        if (v < 0)
        {
            s.append(str.rightRef(len - i));
            break;
        }
        else
        {
            s.append(str.midRef(i, v - i));
            if (++v >= len)
                break;
            QString subst;
            // need change to LOCALE(by fasthyun@magicn.com)
            char c = str[v].cell();
            switch (c)
            {
            case 'p':
                subst.setNum(p->pid);
                break;
            case 'c':
                subst = p->command;
                break;
            case 'C':
                subst = p->cmdline;
                break;
            case 'u':
                subst = Uidstr::userName(p->uid);
                break;
            case '%':
                subst = "%";
                break;
            }
            s.append(subst);
            i = v + 1;
        }
    }
    return s;
}

// execute command
void Command::call(Procinfo *p)
{
    QString s;
    QString msg;

    printf("called !\n");

    if (p == nullptr)
    {
        if (cmdline == "%update")
        {
            qps->refresh();
            return;
        }

        s = cmdline;
    }
    else
        s = substString(cmdline, p);

    int ret = system(s.toLatin1().data()); ///
                                           /*
                                                   pr=new QProcess;	// leak?
                                                   if(!wc->command.isEmpty())  //conflict pid's command
                                                   {
                                                           pr->start(wc->command);		// thread run, if
                                              null then
                                              segfault occurs. ?
                                                   }

                                                   connect(pr, SIGNAL(started()), this, SLOT(cmd_started()));
                                                   connect(pr, SIGNAL(finished ( int , QProcess::ExitStatus
                                              )),this,SLOT(cmd_finished ( int , QProcess::ExitStatus )));
                                                   connect(pr, SIGNAL(error ( QProcess::ProcessError )),this,
                                              SLOT(cmd_error(QProcess::ProcessError)));
                                           */

    if (ret)
    {
        msg = tr( "The command:\n\n" );
        msg.append(s);
        if (ret == -1)
        {
            msg.append( tr( "\n\nfailed with the error:\n\n" ) );
            const char *e = static_cast< const char * >( nullptr );
            msg.append( ( errno == EAGAIN )
                        ? tr( "Too many processes" )
                        : ( ( e = strerror( errno ) ) )
                          ? QString::fromLocal8Bit( e )
                          : tr( "Unknown error" ) );
        }
        else if (ret & 0xff)
        {
            msg.append("\n\nwas terminated by signal ");
            msg.append(QString().setNum(ret & 0xff));
            msg.append(".");
        }
        else if (ret == 0x7f00)
        {
            msg.append( tr( "\n\ncould not be executed because it was not "
                            "found,\nor you did not have execute permission." ) );
        }
        else
        {
            msg.append( tr( "\n\nexited with status " ) );
            msg.append(QString().setNum(ret >> 8));
            msg.append(".");
        }
        QMessageBox::warning(nullptr, tr( "Command Failed" ), msg);
    }
}

//----------------------------------------------------------------
QList<watchCond *> watchlist;

extern WatchdogDialog *watchdogDialog;

// except threads, already running process
void watchdog_check_if_start(QString cmd, Procinfo *pi)
{
    /// printf("cmd=%s\n", cmd.toLatin1().data());
    for (int i = 0; i < watchlist.size(); ++i)
    {
        watchCond *wc = watchlist.at(i);
        if (wc->enable == false)
            continue;
        if (wc->cond == WATCH_PROCESS_START)
            if (wc->procname == cmd)
            {
                // printf("Watchdog: start\n");
                if (!pi->isThread())
                    // ExecWindow *mw=new
                    // ExecWindow(wc->message,wc->command,pi->pid,pi->command);
                    // // leak
                        (void) new ExecWindow(wc, pi->pid, pi->command); // leak

                // note :
                //	1.system("./loop"); //block !!
                //	2.pr.setEnvironment(QProcess::systemEnvironment
                //());
            }
    }
}

void watchdog_check_if_finish(QString cmd, Procinfo *pi)
{
    for (int i = 0; i < watchlist.size(); ++i)
    {
        watchCond *w = watchlist.at(i);
        if (w->enable == false)
            continue;
        if (w->cond == WATCH_PROCESS_FINISH)
        {
            if (w->procname == cmd)
            {
                // printf("Watchdog: finish\n");
                if (!pi->isThread())
                    // if(pi->pid==pi->tgid) // not a thread
                    // !
                        (void) new ExecWindow(w, pi->pid, pi->command); // leak
                //	ExecWindow *mw=new
                // ExecWindow(w->message,w->command,pi->pid,pi->command);
            }
        }
    }
}

// NOTYET
void watchdog_syscpu(int /*cpu*/)
{
    //	printf("Watchdog: watchdog_syscpu\n");
    // if(watchdogDialog->flag_ifsyscpu)
    {
        //	if(cpu> watchdogDialog->syscpu_over)
        //		printf("Watchdog: event %d%\n",cpu);
        // QMessageBox::warning(0, "Watchdog", "aaaaa"); //blocking
    }
}
