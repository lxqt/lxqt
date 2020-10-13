/*
 * wchan.cpp
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

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "config.h"

#include "wchan.h"

#ifdef LINUX
QHash<int, char *> Wchan::dict;
char *Wchan::sysmap = nullptr;
bool Wchan::sysmap_inited = false;
int Wchan::sysmap_size = 0;

// return malloc:ed hex representation of x
static char *hexstr(unsigned long x)
{
    char *p = (char *)malloc(sizeof(long) * 2 + 1);
    sprintf(p, sizeof(long) == 8 ? "%016lx" : "%08lx", x);
    return p;
}
#endif

// called  by ????
// return wchan symbol (possibly numeric, and empty string if addr=0)

QString Wchan::name(unsigned long /*addr*/)
{
#ifdef LINUX
    return "";

#endif

#ifdef SOLARIS
    char buf[sizeof(long) * 2 + 1];
    sprintf(buf, sizeof(long) == 8 ? "%016lx" : "%08lx", addr);
    return QString(buf);
#endif
}

#ifdef LINUX

// return true if open succeeds
bool Wchan::open_sysmap()
{
    // common places to look for a valid System.map
    static const char *paths[] = {
        "/boot/System.map-%s", "/boot/System.map", "/lib/modules/%s/System.map",
        "/usr/src/linux-%s/System.map", "/usr/src/linux/System.map",
        "/usr/local/src/linux-%s/System.map", "/usr/local/src/linux/System.map",
        nullptr};
    sysmap_inited = true; // don't try again
    for (const char **p = paths; *p; p++)
    {
        char buf[80];
        struct utsname ub;
        uname(&ub);
        int major, minor, lvl;
        if (sscanf(ub.release, "%d.%d.%d", &major, &minor, &lvl) != 3)
            major = -1; // non-standard release, silently accept it
        sprintf(buf, *p, ub.release);
        if (try_sysmap(buf))
        { // try_sysmap
            if (major >= 0)
            {
                char vstr[40];
                sprintf(vstr, "Version_%d", (major << 16) + (minor << 8) + lvl);
                // map a zero page at the end to terminate
                // string
                int ps = getpagesize();

                mmap(sysmap + ((sysmap_size + ps - 1) & ~(ps - 1)), ps,
                     PROT_READ, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
                if (!strstr(sysmap, vstr))
                {
                    fprintf(stderr, "qps warning: %s does "
                                    "not match current "
                                    "kernel\n",
                            buf);
                    munmap(sysmap, sysmap_size + ps);
                    sysmap = nullptr;
                    continue; // search the list for a
                              // better file
                }
            }
            return true;
        }
    }
    return false;
}

// try mapping System.map from path, return true if success
bool Wchan::try_sysmap(const char *path)
{
    int fd = open(path, O_RDONLY);
    struct stat sbuf;
    if (fd >= 0)
    {
        if (fstat(fd, &sbuf) == 0)
        {
            sysmap_size = sbuf.st_size;
            // make room for a zero page after the sysmap
            sysmap = (char *)mmap(nullptr, sysmap_size + getpagesize(), PROT_READ,
                                  MAP_SHARED, fd, 0);
            close(fd);
            if (sysmap != (char *)-1)
                return true;
            sysmap = nullptr;
            return false;
        }
        close(fd);
    }
    return false;
}

inline int Wchan::beginning_of_line(int ofs)
{
    // seek backwards to beginning of line
    while (ofs >= 0 && sysmap[ofs] != '\n')
        ofs--;
    return ofs + 1;
}

char *Wchan::find_sym(unsigned long addr)
{
    // use binary search to find symbol; return malloced string
    int l = 0, r = sysmap_size;
    for (;;)
    {
        unsigned long a;
        char buf[80];
        int m = (l + r) / 2;
        m = beginning_of_line(m);
        if (m == l)
        {
            // see if there is a line further down
            while (m < r - 1 && sysmap[m] != '\n')
                m++;
            if (m < r - 1)
            {
                m++;
            }
            else
            {
                if (r == sysmap_size)
                {
                    // after last item, probably in a
                    // module. give hex addr
                    return hexstr(addr);
                }
                m = l;
                sscanf(sysmap + m, "%lx %*c %s", &a, buf);
                // strip leading sys_ or do_ to reduce field
                // width
                char *p = buf;
                if (strncmp(buf, "do_", 3) == 0)
                    p += 3;
                if (strncmp(buf, "sys_", 4) == 0)
                    p += 4;
                return strdup(p);
            }
        }
        sscanf(sysmap + m, "%lx %*c %s", &a, buf);
        if (addr < a)
        {
            r = m;
        }
        else
        {
            l = m;
        }
    }
}

#endif // LINUX
