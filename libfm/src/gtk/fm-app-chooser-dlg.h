/*
 *      fm-app-chooser-dlg.h
 *      
 *      Copyright 2010 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#ifndef __FM_APP_CHOOSER_DLG_H__
#define __FM_APP_CHOOSER_DLG_H__

#include <gtk/gtk.h>
#include <gio/gio.h>
#include "fm-mime-type.h"

G_BEGIN_DECLS

GtkDialog* fm_app_chooser_dlg_new(FmMimeType* mime_type, gboolean can_set_default);
GAppInfo* fm_app_chooser_dlg_dup_selected_app(GtkDialog* dlg, gboolean* set_default);

GAppInfo* fm_choose_app_for_mime_type(GtkWindow* parent, FmMimeType* mime_type, gboolean can_set_default);

G_END_DECLS

#endif /* __FM_APP_CHOOSER_DLG_H__ */
