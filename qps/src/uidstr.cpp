/*
 * uidstr.cpp
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

#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "uidstr.h"

QHash<int, char *> Uidstr::udict;
QHash<int, char *> Uidstr::gdict;

// return user name (possibly numeric)
QString Uidstr::userName(int uid)
{
    char *p = udict.value(uid, NULL);
    if (!p)
    {
        struct passwd *pw = getpwuid(uid);
        if (!pw)
        {
            p = (char *)malloc(11);
            sprintf(p, "%d", uid);
        }
        else
            p = strdup(pw->pw_name);
        udict.insert(uid, p);
        // if(udict.count() > udict.size() * 3)
        // udict.resize(udict.count());
    }
    QString s(p);
    return s;
}

// return group name (possibly numeric)

QString Uidstr::groupName(int gid)
{
    char *p = gdict[gid];
    if (!p)
    {
        struct group *gr = getgrgid(gid);
        if (!gr)
        {
            p = (char *)malloc(11);
            sprintf(p, "%d", gid);
        }
        else
            p = strdup(gr->gr_name);
        gdict.insert(gid, p);
        // if(gdict.count() > gdict.size() * 3)
        // gdict.resize(gdict.count());
    }
    QString s(p);
    return s;
}
