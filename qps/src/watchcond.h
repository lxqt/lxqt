/*
 * watchcond.h
 * This file is part of qps -- Qt-based visual process status monitor
 *
 * Copyright 1997-1999  Mattias Engdegård
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

#ifndef WATCHCOND_H
#define WATCHCOND_H

#include <QString>

#define WATCH_PROCESS_START 0
#define WATCH_PROCESS_FINISH 1
#define WATCH_PROCESS_CPU_OVER 2
#define WATCH_SYS_CPU_OVER 3
#define WATCH_SYS_CPU_UNDER 4


// if process [name] start, exec [command], showmsg [xxx]
// if process [name] finish exec [command], showmsg [xxx]
// if system cpu over [90%], exec [command], msg [xxx]
// if system cpu under [10%], exec [command], msg [xxx]
// if process [name] cpu over [90%] exec [command] msg [xxx]
// if process [name] start, kill_it, msg [xxx]
// if process [name] start, soundplay [ ], msg [xxx]

class watchCond
{
  public:
    int cond;
    int enable;
    int cpu;
    QString procname;
    QString command;
    QString message;

    watchCond()
    {
        enable = 0;
        //	procname[0]=0;		command[0]=0;
        // message[0]=0;
    }
    // key [txt]	[a]
    // QString getVal(QString &str, QString &key)
    QString getVal(QString &str, const char *key);
    QString getstring();
    void putstring(QString str);
};

#endif // WATCHCOND_H
