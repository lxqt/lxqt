/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   tree.h for ObConf, the configuration tool for Openbox
   Copyright (c) 2003        Dana Jansens

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   See the COPYING file for a copy of the GNU General Public License.
*/

#ifndef obconf__tree_h
#define obconf__tree_h

#include <obt/xml.h>

xmlNodePtr tree_get_node(const gchar *path, const gchar *def);

void tree_delete_node(const gchar *path);

gchar* tree_get_string(const gchar *node, const gchar *def);
gint tree_get_int(const gchar *node, gint def);
gboolean tree_get_bool(const gchar *node, gboolean def);

void tree_set_string(const gchar *node, const gchar *value);
void tree_set_int(const gchar *node, const gint value);
void tree_set_bool(const gchar *node, const gboolean value);

void tree_apply(); /* save the rc.xml and force reconfigure from Openbox */

#endif
