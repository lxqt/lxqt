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

#ifndef GLOBAL_ACTION_DAEMON__PIPE_UTILS__INCLUDED
#define GLOBAL_ACTION_DAEMON__PIPE_UTILS__INCLUDED

#include <cstddef>
#include <cerrno>

#ifndef __error_t_defined
// FIXME: error_t seems to be GNU specific?
typedef int error_t;
#endif

void initBothPipeEnds(int fd[2]);

error_t createPipe(int fd[2]);

error_t readAll(int fd, void *data, size_t length);
error_t writeAll(int fd, const void *data, size_t length);

void closeBothPipeEnds(int fd[2]);

#endif // GLOBAL_ACTION_DAEMON__PIPE_UTILS__INCLUDED
