/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
 * Authors:
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "pipe_utils.h"

#include <unistd.h>
#include <fcntl.h>


void initBothPipeEnds(int fd[2])
{
    fd[STDIN_FILENO] = -1;
    fd[STDOUT_FILENO] = -1;
}

error_t createPipe(int fd[2])
{
    error_t result = static_cast<error_t>(0);
    if (pipe(fd) < 0)
    {
        result = static_cast<error_t>(errno);
    }
    if (!result)
    {
        fcntl(fd[STDIN_FILENO], F_SETFD, FD_CLOEXEC);
        fcntl(fd[STDOUT_FILENO], F_SETFD, FD_CLOEXEC);
    }
    return result;
}

error_t readAll(int fd, void *data, size_t length)
{
    while (length)
    {
        ssize_t bytes_read = read(fd, data, length);
        if (bytes_read < 0)
        {
            return static_cast<error_t>(errno);
        }
        if (!bytes_read)
        {
            return static_cast<error_t>(-1);
        }
        data = reinterpret_cast<char *>(data) + bytes_read;
        length -= bytes_read;
    }
    return static_cast<error_t>(0);
}

error_t writeAll(int fd, const void *data, size_t length)
{
    while (length)
    {
        ssize_t bytes_written = write(fd, data, length);
        if (bytes_written < 0)
        {
            return static_cast<error_t>(errno);
        }
        if (!bytes_written)
        {
            return static_cast<error_t>(-1);
        }
        data = reinterpret_cast<const char *>(data) + bytes_written;
        length -= bytes_written;
    }
    return static_cast<error_t>(0);
}

void closeBothPipeEnds(int fd[2])
{
    if (fd[STDIN_FILENO] != -1)
    {
        close(fd[STDIN_FILENO]);
        fd[STDIN_FILENO] = -1;
    }
    if (fd[STDOUT_FILENO] != -1)
    {
        close(fd[STDOUT_FILENO]);
        fd[STDOUT_FILENO] = -1;
    }
}
