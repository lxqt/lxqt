/*
 *      pcmanfm.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <stdio.h>

#include "fm-gtk.h"
#include "main-win.h"

int main(int argc, char** argv)
{
	FmMainWin* w;
	gtk_init(&argc, &argv);

	fm_gtk_init(NULL);

    /* for debugging RTL */
    /* gtk_widget_set_default_direction(GTK_TEXT_DIR_RTL); */

	w = fm_main_win_new();
	gtk_window_set_default_size(GTK_WINDOW(w), 640, 480);
	gtk_widget_show(GTK_WIDGET(w));

    if(argc > 1)
    {
        FmPath* path = fm_path_new_for_commandline_arg(argv[1]);
        fm_main_win_chdir(w, path);
        fm_path_unref(path);
    }

    GDK_THREADS_ENTER();
	gtk_main();
    GDK_THREADS_LEAVE();

    fm_finalize();

	return 0;
}
