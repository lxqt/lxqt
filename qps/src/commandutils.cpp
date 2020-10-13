/*
 * commandutils.cpp
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

#include "commandutils.h"
#include <command.h>

extern QList<Command *> commands;

int find_command(QString s)
{
    for (int i = 0; i < commands.size(); i++)
        if (s == commands[i]->name)
            return i;
    return -1;
}

// DEL has "&" end of the string ?
bool hasAmpersand(QString cmdline)
{
    QString str;
    int len;
    str = cmdline.simplified();

    if (str == "%update")
        return true; // internal command

    len = str.length();
    if (str[len - 1] == '&')
        return true;
    else
        return false;
}

void check_command(int /*idx*/) {}

//
void check_commandAll()
{
    // int idx;

    return;
    for (int i = 0; i < commands.size(); i++)
    {
        if (hasAmpersand(commands[i]->cmdline) == false)
            commands[i]->cmdline.append("&");
    }
}

// after read ~/.qpsrc
void add_default_command()
{

    // int idx;

    /*
    idx=find_command("Update");
    if (idx>=0)
            commands[idx]->cmdline="%update";
    else
            commands.add(new Command("Update","%update",true));
    */

    /*
     *	PAUSED
    idx=find_command("Quit");
    if (idx>=0)
            commands[idx]->cmdline="killall qps";
    else	commands.add(new Command("Quit","killall qps",false));
    */

    // check_commandAll(); DEL?
}
