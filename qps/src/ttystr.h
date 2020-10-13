/*
 * ttystr.h
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

#ifndef TTYSTR_H
#define TTYSTR_H

#include "config.h"
#include <QString>

#ifdef LINUX
#include <sys/sysmacros.h>
#endif

#ifdef SOLARIS
#include <QHash>
#endif

class Ttystr
{
  public:
    static QString name(dev_t devnr);

#ifdef SOLARIS
  private:
    static void read_devs();
    static void scandevdir(const char *prefix);

    static QHash<int, char *> dict;
    static bool scanned;
#endif
};

#endif // TTYSTR_H
