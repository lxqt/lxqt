/*
 * wchan.h
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

#ifndef WCHAN_H
#define WCHAN_H

#include <QHash>
#include <QString>

class Wchan
{
  public:
    static QString name(unsigned long addr);

#ifdef LINUX
  private:
    static bool open_sysmap();
    static bool try_sysmap(const char *path);
    static char *find_sym(unsigned long addr);
    static inline int beginning_of_line(int ofs);

    static QHash<int, char *> dict;
    static char *sysmap;       // pointer to mmap()ed System.map
    static bool sysmap_inited; // if an attempt to read sysmap has been made
    static int sysmap_size;
#endif
};

#endif // WCHAN_H
