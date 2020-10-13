/*
 * fm-menu-tool-item.h
 *
 * Copyright (C) 2003 Ricardo Fernandez Pascual
 * Copyright (C) 2004 Paolo Borelli
 *
 * Copyright (C) 2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __FM_MENU_TOOL_ITEM_H__
#define __FM_MENU_TOOL_ITEM_H__

#include <gtk/gtk.h>

#include "fm-seal.h"

G_BEGIN_DECLS

#define FM_TYPE_MENU_TOOL_ITEM         (fm_menu_tool_item_get_type ())
#define FM_MENU_TOOL_ITEM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FM_TYPE_MENU_TOOL_ITEM, FmMenuToolItem))
#define FM_MENU_TOOL_ITEM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), FM_TYPE_MENU_TOOL_ITEM, FmMenuToolItemClass))
#define FM_IS_MENU_TOOL_ITEM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), FM_TYPE_MENU_TOOL_ITEM))
#define FM_IS_MENU_TOOL_ITEM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), FM_TYPE_MENU_TOOL_ITEM))

typedef struct _FmMenuToolItemClass   FmMenuToolItemClass;
typedef struct _FmMenuToolItem        FmMenuToolItem;
typedef struct _FmMenuToolItemPrivate FmMenuToolItemPrivate;

struct _FmMenuToolItem
{
  GtkToolItem parent;

  /*< private >*/
  FmMenuToolItemPrivate *FM_SEAL (priv);
};

/**
 * FmMenuToolItemClass:
 * @parent_class: the parent class
 * @show_menu: the class closure for the #FmMenuToolItem::show-menu signal
 */
struct _FmMenuToolItemClass
{
  GtkToolItemClass parent_class;

  void (*show_menu) (FmMenuToolItem *button);

  /*< private >*/
  /* Padding for future expansion */
  void (*_fm_reserved1) (void);
  void (*_fm_reserved2) (void);
  void (*_fm_reserved3) (void);
  void (*_fm_reserved4) (void);
};

GType        fm_menu_tool_item_get_type(void);
GtkToolItem *fm_menu_tool_item_new(void);
void         fm_menu_tool_item_set_menu(FmMenuToolItem *button, GtkWidget *menu);
GtkWidget   *fm_menu_tool_item_get_menu(FmMenuToolItem *button);

G_END_DECLS

#endif /* __FM_MENU_TOOL_ITEM_H__ */
