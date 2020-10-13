/*
 * ttystr.cpp
 *
 * Copyright 1997-1999 Mattias Engdegård
 * This file is part of qps -- Qt-based visual process status monitor
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h> //major() minor()
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dirent.h>
#include "ttystr.h"

#ifdef LINUX

// Table of various tty devices (mostly serial boards)

struct Ttytab
{
    short dev_maj; // major number of tty
    char prefix;
    char callout; // 1 if callout device, 0 if tty
};

static Ttytab ttytab[] = {{19, 'C', 0}, // Cyclades
                          {20, 'b', 1},
                          {22, 'D', 0}, // Digiboard
                          {23, 'd', 1},
                          {24, 'E', 0}, // Stallion
                          {25, 'e', 1},
                          {32, 'X', 0}, // Specialix
                          {33, 'x', 1},
                          {46, 'R', 0}, // Comtrol Rocketport
                          {47, 'r', 1},
                          {48, 'L', 0}, // SDL RISCom
                          {49, 'l', 1},
                          {57, 'P', 0}, // Hayes ESP
                          {58, 'p', 1},
                          {71, 'F', 0}, // Computone IntelliPort II
                          {72, 'f', 1},
                          {75, 'W', 0}, // Specialix IO8+
                          {76, 'w', 1},
                          {78, 'M', 0}, // PAM Software's multimodem boards
                          {79, 'm', 1},
                          {105, 'V', 0}, // Comtrol VS-1000
                          {106, 'v', 1},
                          {0, 0, 0}};

// Return tty name of a device. Remove leading "tty" if any.
// Return "-" if devnr is zero, "?" if unknown.
QString Ttystr::name(dev_t devnr)
{
    if (devnr == 0)
        return "-";

    int dmin = minor(devnr);
    int dmaj = major(devnr);
    char name[40];
    switch (dmaj)
    {
    case 3:
        // BSD-style pty slaves
        name[0] = "pqrstuvwxyzabcde"[(dmin >> 4) & 0xf];
        name[1] = "0123456789abcdef"[dmin & 0xf];
        name[2] = '\0';
        break;

    case 4:
        if (dmin < 64)
        {
            // virtual console 0..63
            sprintf(name, "%d", dmin);
        }
        else if (dmin < 128)
        {
            // serial port 0..63
            sprintf(name, "S%d", dmin - 64);
        }
        else if (dmin >= 192 && dmin < 256)
        {
            // obsolete tty devices
            name[0] = "pqrs"[(dmin >> 4) & 0x3];
            name[1] = "0123456789abcdef"[dmin & 0xf];
            name[2] = '\0';
        }
        else
            strcpy(name, "?");
        break;

    case 5:
        // alternate tty devices
        switch (dmin)
        {
        case 0:
            strcpy(name, "tty");
            break;

        case 1:
            strcpy(name, "console");
            break;

        case 2:
            strcpy(name, "ptmx");
            break;

        default:
            if (dmin >= 64 && dmin < 128)
            {
                // callout devices 0..63
                sprintf(name, "cua%d", dmin - 64);
            }
            else
                strcpy(name, "?");
        }

    default:
        if (dmaj >= 136 && dmaj < 144)
        {
            // Unix98 pty slaves
            sprintf(name, "pts/%d", ((dmaj - 136) << 8) + dmin);
            break;
        }

        // Various serial tty devices
        for (int i = 0; ttytab[i].dev_maj; i++)
        {
            if (dmaj == ttytab[i].dev_maj)
            {
                sprintf(name, "%s%c%d", ttytab[i].callout ? "cu" : "",
                        ttytab[i].prefix, dmin);
                break;
            }
        }
        strcpy(name, "?");
    }
    return name;
}

#endif // LINUX

#ifdef SOLARIS

QHash<int, char *> Ttystr::dict;
bool Ttystr::scanned = FALSE;

// return tty name, '-' if no tty, "??" if unidentifiable tty
QString Ttystr::name(dev_t devnr)
{
    if (!scanned)
    {
        read_devs();
        scanned = TRUE; // just scan /dev once
    }
    QString s;
    if (devnr == (dev_t)-1)
        s = "-";
    else
    {
        char *ts = dict.value(devnr, NULL);

        if (ts)
            s = ts;
        else
            s = "??";
    }
    return s;
}

// scan device directory or subdirectory thereof
void Ttystr::scandevdir(const char *prefix)
{
    char dirname[80] = "/dev";
    strcat(dirname, prefix);
    DIR *d = opendir(dirname);
    if (!d)
        return;
    struct dirent *e;
    struct stat sb;
    char name[80];
    while ((e = readdir(d)) != 0)
    {
        sprintf(name, "/dev%s/%s", prefix, e->d_name);
        if (stat(name, &sb) >= 0)
        {
            if (S_ISCHR(sb.st_mode))
            {
                dev_t d = sb.st_rdev;
                if (!dict.value(d, NULL))
                {
                    dict.insert(sb.st_rdev, strdup(name + 5)); // skip "/dev/"
                }
            }
        }
    }
    closedir(d);
}

void Ttystr::read_devs()
{
    // scan /dev and various subdirectories. In case of duplicates, pick
    // the first name encountered
    scandevdir("/term");
    scandevdir("");
    scandevdir("/pts");
    scandevdir("/cua");
}

#endif // SOLARIS
