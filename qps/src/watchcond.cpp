/*
 * watchcond.cpp
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

#include "watchcond.h"

QString watchCond::getVal(QString &str, const char *key)
{
    int n; // key.length();
    int idx = str.indexOf(key);
    if (idx < 0)
        return "cant' found";

    idx = str.indexOf("[", idx);
    if (idx < 0)
        return "[ error";
    int idx_end = str.indexOf("]", idx);
    if (idx_end < 0)
        return "] error";
    n = idx_end - idx;
    return str.mid(idx + 1, n - 1);
}
QString watchCond::getstring()
{
    QString string;
    string.clear();
    switch (cond)
    {
    case WATCH_PROCESS_FINISH:
        string.append("if process [" + procname + "] finish, ");
        // string=string.sprintf("if process [%s] finish",procname);
        break;
    case WATCH_PROCESS_START:
        string.append("if process [" + procname + "] start, ");
        break;
    case WATCH_SYS_CPU_OVER:
        string.append("if sys_cpu over [" + QString::number(cpu) + "], ");
        break;
    case WATCH_SYS_CPU_UNDER:
        string.append("if sys_cpu under [" + QString::number(cpu) + "], ");
        break;
    default:
        ;
    }
    if (!command.isEmpty())
        string.append("exec [" + command + "] ");
    if (!message.isEmpty())
        string.append("showmsg [" + message + "] ");
    if (enable)
        string.append("enabled");
    else
        string.append("disabled");

    return string;
}
void watchCond::putstring(QString str)
{
    if (str.contains("if process"))
    {
        if (str.contains("start"))
            cond = WATCH_PROCESS_START;
        if (str.contains("finish"))
            cond = WATCH_PROCESS_FINISH;
        procname = getVal(str, "if process");
    }
    if (str.contains("exec"))
        command = getVal(str, "exec");
    if (str.contains("showmsg"))
        message = getVal(str, "showmsg");
    if (str.contains("enabled"))
        enable = true;
    else
        enable = false;
}
