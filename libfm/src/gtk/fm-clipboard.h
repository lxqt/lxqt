/*
 *      fm-clipboard.h
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


#ifndef __FM_CLIPBOARD_H__
#define __FM_CLIPBOARD_H__

#include <gtk/gtk.h>
#include "fm-path.h"

G_BEGIN_DECLS

#define fm_clipboard_cut_files(src_widget, files)	\
	fm_clipboard_cut_or_copy_files(src_widget, files, TRUE)

#define fm_clipboard_copy_files(src_widget, files)	\
	fm_clipboard_cut_or_copy_files(src_widget, files, FALSE)

gboolean fm_clipboard_cut_or_copy_files(GtkWidget* src_widget, FmPathList* files, gboolean _is_cut);

gboolean fm_clipboard_paste_files(GtkWidget* dest_widget, FmPath* dest_dir);

gboolean fm_clipboard_have_files(GtkWidget* dest_widget);

G_END_DECLS

#endif /* __FM_CLIPBOARD_H__ */
