/*
 *      fm-terminal.h
 *
 *      Copyright 2012 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifndef __FM_TERMINAL_H__
#define __FM_TERMINAL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define FM_TERMINAL_TYPE               (fm_terminal_get_type())
#define FM_IS_TERMINAL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj), FM_TERMINAL_TYPE))

typedef struct _FmTerminal              FmTerminal;
typedef struct _FmTerminalClass         FmTerminalClass;

/**
 * FmTerminal:
 * @program: archiver program
 * @open_arg: options to insert before &lt;cmd&gt; [&lt;args&gt;] to run command in terminal
 * @noclose_arg: options to insert to run command without closing terminal or %NULL
 * @launch: options if required to launch in current directory
 * @desktop_id: desktop ID to search for icon and descriptions
 * @custom_args: custom arguments (only from libfm.conf)
 *
 * A terminal description. If application should be ran in terminal libfm
 * may do it either default way (closing terminal window after exit):
 * - @program @custom_args @open_arg &lt;cmd&gt; [&lt;args&gt;]
 *
 * or alternate way (not closing terminal window after exit):
 * - @program @custom_args @noclose_arg &lt;cmd&gt; [&lt;args&gt;]
 *
 * If terminal doesn't support not closing terminal window after exit then
 * default way should be used.
 */
struct _FmTerminal
{
    /*< private >*/
    GObject parent;
    /*< public >*/
    char* program;
    char* open_arg;
    char* noclose_arg;
    char* launch;
    char* desktop_id;
    char* custom_args;
    /*< private >*/
    gpointer _reserved1;
    gpointer _reserved2;
};

GType fm_terminal_get_type(void);

void _fm_terminal_init(void);
void _fm_terminal_finalize(void);

FmTerminal* fm_terminal_dup_default(GError **error);
gboolean fm_terminal_launch(const gchar *dir, GError **error);

G_END_DECLS

#endif /* __FM_TERMINAL_H__ */
