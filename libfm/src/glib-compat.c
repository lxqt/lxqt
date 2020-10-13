/*
 *      glib-compat.c
 *
 *      Copyright 2011 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 *      This file is a part of the Libfm library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "glib-compat.h"

#if !GLIB_CHECK_VERSION(2, 28, 0)
gboolean
g_signal_accumulator_first_wins (GSignalInvocationHint *ihint,
                                 GValue                *return_accu,
                                 const GValue          *handler_return,
                                 gpointer               dummy)
{
  g_value_copy (handler_return, return_accu);
  return FALSE;
}
#endif
