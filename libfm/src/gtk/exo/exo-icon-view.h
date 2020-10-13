/*-
 * Copyright (c) 2004-2006  os-cillation e.K.
 * Copyright (c) 2002,2004  Anders Carlsson <andersca@gnu.org>
 *
 * Written by Benedikt Meurer <benny@xfce.org>.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* Modified by Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * on 2009-08-30 for use in libfm */

/*
#if !defined (EXO_INSIDE_EXO_H) && !defined (EXO_COMPILATION)
#error "Only <exo/exo.h> can be included directly, this file may disappear or change contents."
#endif
*/

#ifndef __EXO_ICON_VIEW_H__
#define __EXO_ICON_VIEW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _ExoIconViewPrivate    ExoIconViewPrivate;
typedef struct _ExoIconViewClass      ExoIconViewClass;
typedef struct _ExoIconView           ExoIconView;

#define EXO_TYPE_ICON_VIEW            (exo_icon_view_get_type ())
#define EXO_ICON_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXO_TYPE_ICON_VIEW, ExoIconView))
#define EXO_ICON_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EXO_TYPE_ICON_VIEW, ExoIconViewClass))
#define EXO_IS_ICON_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXO_TYPE_ICON_VIEW))
#define EXO_IS_ICON_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EXO_TYPE_ICON_VIEW))
#define EXO_ICON_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXO_TYPE_ICON_VIEW, ExoIconViewClass))

/**
 * ExoIconViewForeachFunc:
 * @icon_view : an #ExoIconView.
 * @path      : the current path.
 * @user_data : the user data supplied to exo_icon_view_selected_foreach().
 *
 * Callback function prototype, invoked for every selected path in the
 * @icon_view. See exo_icon_view_selected_foreach() for details.
 **/
typedef void (*ExoIconViewForeachFunc) (ExoIconView *icon_view,
                                        GtkTreePath *path,
                                        gpointer     user_data);

/**
 * ExoIconViewSearchEqualFunc:
 * @model       : the #GtkTreeModel being searched.
 * @column      : the search column set by exo_icon_view_set_search_column().
 * @key         : the key string to compare with.
 * @iter        : the #GtkTreeIter of the current item.
 * @search_data : user data from exo_icon_view_set_search_equal_func().
 *
 * A function used for checking whether a row in @model matches a search key string
 * entered by the user. Note the return value is reversed from what you would normally
 * expect, though it has some similarity to strcmp() returning 0 for equal strings.
 *
 * Return value: %FALSE if the row matches, %TRUE otherwise.
 **/
typedef gboolean (*ExoIconViewSearchEqualFunc) (GtkTreeModel *model,
                                                gint          column,
                                                const gchar  *key,
                                                GtkTreeIter  *iter,
                                                gpointer      search_data);

/**
 * ExoIconViewSearchPositionFunc:
 * @icon_view     : an #ExoIconView.
 * @search_dialog : the search dialog window to place.
 * @user_data     : user data from exo_icon_view_set_search_position_func().
 *
 * A function used to place the @search_dialog for the @icon_view.
 **/
typedef void (*ExoIconViewSearchPositionFunc) (ExoIconView *icon_view,
                                               GtkWidget   *search_dialog,
                                               gpointer     user_data);

/**
 * ExoIconViewDropPosition:
 * @EXO_ICON_VIEW_NO_DROP    : no drop indicator.
 * @EXO_ICON_VIEW_DROP_INTO  : drop indicator on an item.
 * @EXO_ICON_VIEW_DROP_LEFT  : drop indicator on the left of an item.
 * @EXO_ICON_VIEW_DROP_RIGHT : drop indicator on the right of an item.
 * @EXO_ICON_VIEW_DROP_ABOVE : drop indicator above an item.
 * @EXO_ICON_VIEW_DROP_BELOW : drop indicator below an item.
 *
 * Specifies whether to display the drop indicator,
 * i.e. where to drop into the icon view.
 **/
typedef enum
{
  EXO_ICON_VIEW_NO_DROP,
  EXO_ICON_VIEW_DROP_INTO,
  EXO_ICON_VIEW_DROP_LEFT,
  EXO_ICON_VIEW_DROP_RIGHT,
  EXO_ICON_VIEW_DROP_ABOVE,
  EXO_ICON_VIEW_DROP_BELOW
} ExoIconViewDropPosition;

/**
 * ExoIconViewLayoutMode:
 * @EXO_ICON_VIEW_LAYOUT_ROWS : layout items in rows.
 * @EXO_ICON_VIEW_LAYOUT_COLS : layout items in columns.
 *
 * Specifies the layouting mode of an #ExoIconView. @EXO_ICON_VIEW_LAYOUT_ROWS
 * is the default, which lays out items vertically in rows from top to bottom.
 * @EXO_ICON_VIEW_LAYOUT_COLS lays out items horizontally in columns from left
 * to right.
 **/
typedef enum
{
  EXO_ICON_VIEW_LAYOUT_ROWS,
  EXO_ICON_VIEW_LAYOUT_COLS
} ExoIconViewLayoutMode;

struct _ExoIconView
{
  GtkContainer        __parent__;

  /*< private >*/
  ExoIconViewPrivate *priv;
};

struct _ExoIconViewClass
{
  GtkContainerClass __parent__;

#if !GTK_CHECK_VERSION(3, 0, 0)
  /* virtual methods */
  void     (*set_scroll_adjustments)    (ExoIconView     *icon_view,
                                         GtkAdjustment   *hadjustment,
                                         GtkAdjustment   *vadjustment);
#endif
  /* signals */
  void     (*item_activated)            (ExoIconView     *icon_view,
                                         GtkTreePath     *path);
  void     (*selection_changed)         (ExoIconView     *icon_view);

  /* Key binding signals */
  void     (*select_all)                (ExoIconView    *icon_view);
  void     (*unselect_all)              (ExoIconView    *icon_view);
  void     (*select_cursor_item)        (ExoIconView    *icon_view);
  void     (*toggle_cursor_item)        (ExoIconView    *icon_view);
  gboolean (*move_cursor)               (ExoIconView    *icon_view,
                                         GtkMovementStep step,
                                         gint            count);
  gboolean (*activate_cursor_item)      (ExoIconView    *icon_view);
  gboolean (*start_interactive_search)  (ExoIconView    *icon_view);

  /*< private >*/
  void (*reserved0) (void);
  void (*reserved1) (void);
  void (*reserved2) (void);
  void (*reserved3) (void);
  void (*reserved4) (void);
  void (*reserved5) (void);
  void (*reserved6) (void);
  void (*reserved7) (void);
  void (*reserved8) (void);
  void (*reserved9) (void);
};

GType                 exo_icon_view_get_type                  (void) G_GNUC_CONST;

GtkWidget            *exo_icon_view_new                       (void);
GtkWidget            *exo_icon_view_new_with_model            (GtkTreeModel             *model);

GtkTreeModel         *exo_icon_view_get_model                 (const ExoIconView        *icon_view);
void                  exo_icon_view_set_model                 (ExoIconView              *icon_view,
                                                               GtkTreeModel             *model);

GtkOrientation        exo_icon_view_get_orientation           (const ExoIconView        *icon_view);
void                  exo_icon_view_set_orientation           (ExoIconView              *icon_view,
                                                               GtkOrientation            orientation);

gint                  exo_icon_view_get_columns               (const ExoIconView        *icon_view);
void                  exo_icon_view_set_columns               (ExoIconView              *icon_view,
                                                               gint                      columns);

gint                  exo_icon_view_get_item_width            (const ExoIconView        *icon_view);
void                  exo_icon_view_set_item_width            (ExoIconView              *icon_view,
                                                               gint                      item_width);

gint                  exo_icon_view_get_spacing               (const ExoIconView        *icon_view);
void                  exo_icon_view_set_spacing               (ExoIconView              *icon_view,
                                                               gint                      spacing);

gint                  exo_icon_view_get_row_spacing           (const ExoIconView        *icon_view);
void                  exo_icon_view_set_row_spacing           (ExoIconView              *icon_view,
                                                               gint                      row_spacing);

gint                  exo_icon_view_get_column_spacing        (const ExoIconView        *icon_view);
void                  exo_icon_view_set_column_spacing        (ExoIconView              *icon_view,
                                                               gint                      column_spacing);

gint                  exo_icon_view_get_margin                (const ExoIconView        *icon_view);
void                  exo_icon_view_set_margin                (ExoIconView              *icon_view,
                                                               gint                      margin);

GtkSelectionMode      exo_icon_view_get_selection_mode        (const ExoIconView        *icon_view);
void                  exo_icon_view_set_selection_mode        (ExoIconView              *icon_view,
                                                               GtkSelectionMode          mode);

ExoIconViewLayoutMode exo_icon_view_get_layout_mode           (const ExoIconView        *icon_view);
void                  exo_icon_view_set_layout_mode           (ExoIconView              *icon_view,
                                                               ExoIconViewLayoutMode     layout_mode);

gboolean              exo_icon_view_get_single_click          (const ExoIconView        *icon_view);
void                  exo_icon_view_set_single_click          (ExoIconView              *icon_view,
                                                               gboolean                  single_click);

guint                 exo_icon_view_get_single_click_timeout  (const ExoIconView        *icon_view);
void                  exo_icon_view_set_single_click_timeout  (ExoIconView              *icon_view,
                                                               guint                     single_click_timeout);

void                  exo_icon_view_widget_to_icon_coords     (const ExoIconView        *icon_view,
                                                               gint                      wx,
                                                               gint                      wy,
                                                               gint                     *ix,
                                                               gint                     *iy);
void                  exo_icon_view_icon_to_widget_coords     (const ExoIconView        *icon_view,
                                                               gint                      ix,
                                                               gint                      iy,
                                                               gint                     *wx,
                                                             gint                     *wy);

GtkTreePath          *exo_icon_view_get_path_at_pos           (const ExoIconView        *icon_view,
                                                               gint                      x,
                                                               gint                      y);
gboolean              exo_icon_view_get_item_at_pos           (const ExoIconView        *icon_view,
                                                               gint                      x,
                                                               gint                      y,
                                                               GtkTreePath             **path,
                                                               GtkCellRenderer         **cell);

gboolean              exo_icon_view_get_visible_range         (const ExoIconView        *icon_view,
                                                               GtkTreePath             **start_path,
                                                               GtkTreePath             **end_path);

void                  exo_icon_view_selected_foreach          (ExoIconView              *icon_view,
                                                               ExoIconViewForeachFunc    func,
                                                               gpointer                  data);
void                  exo_icon_view_select_path               (ExoIconView              *icon_view,
                                                               GtkTreePath              *path);
void                  exo_icon_view_unselect_path             (ExoIconView              *icon_view,
                                                               GtkTreePath              *path);
gboolean              exo_icon_view_path_is_selected          (const ExoIconView        *icon_view,
                                                               GtkTreePath              *path);
GList                *exo_icon_view_get_selected_items        (const ExoIconView        *icon_view);
gint                  exo_icon_view_count_selected_items      (const ExoIconView        *icon_view);
void                  exo_icon_view_select_all                (ExoIconView              *icon_view);
void                  exo_icon_view_unselect_all              (ExoIconView              *icon_view);
void                  exo_icon_view_item_activated            (ExoIconView              *icon_view,
                                                               GtkTreePath              *path);

gboolean              exo_icon_view_get_cursor                (const ExoIconView        *icon_view,
                                                               GtkTreePath             **path,
                                                               GtkCellRenderer         **cell);
void                  exo_icon_view_set_cursor                (ExoIconView              *icon_view,
                                                               GtkTreePath              *path,
                                                               GtkCellRenderer          *cell,
                                                               gboolean                  start_editing);

void                  exo_icon_view_scroll_to_path            (ExoIconView              *icon_view,
                                                               GtkTreePath              *path,
                                                               gboolean                  use_align,
                                                               gfloat                    row_align,
                                                               gfloat                    col_align);

/* Drag-and-Drop support */
void                  exo_icon_view_enable_model_drag_source  (ExoIconView              *icon_view,
                                                               GdkModifierType           start_button_mask,
                                                               const GtkTargetEntry     *targets,
                                                               gint                      n_targets,
                                                               GdkDragAction             actions);
void                  exo_icon_view_enable_model_drag_dest    (ExoIconView              *icon_view,
                                                               const GtkTargetEntry     *targets,
                                                               gint                      n_targets,
                                                               GdkDragAction             actions);
void                  exo_icon_view_unset_model_drag_source   (ExoIconView              *icon_view);
void                  exo_icon_view_unset_model_drag_dest     (ExoIconView              *icon_view);
void                  exo_icon_view_set_reorderable           (ExoIconView              *icon_view,
                                                               gboolean                  reorderable);
gboolean              exo_icon_view_get_reorderable           (ExoIconView              *icon_view);


/* These are useful to implement your own custom stuff. */
void                  exo_icon_view_set_drag_dest_item        (ExoIconView              *icon_view,
                                                               GtkTreePath              *path,
                                                               ExoIconViewDropPosition   pos);
void                  exo_icon_view_get_drag_dest_item        (ExoIconView              *icon_view,
                                                               GtkTreePath             **path,
                                                               ExoIconViewDropPosition  *pos);
gboolean              exo_icon_view_get_dest_item_at_pos      (ExoIconView              *icon_view,
                                                               gint                      drag_x,
                                                               gint                      drag_y,
                                                               GtkTreePath             **path,
                                                               ExoIconViewDropPosition  *pos);
GdkPixbuf            *exo_icon_view_create_drag_icon          (ExoIconView              *icon_view,
                                                               GtkTreePath              *path);


/* Interactive search support */
gboolean                      exo_icon_view_get_enable_search         (const ExoIconView            *icon_view);
void                          exo_icon_view_set_enable_search         (ExoIconView                  *icon_view,
                                                                       gboolean                      enable_search);
gint                          exo_icon_view_get_search_column         (const ExoIconView            *icon_view);
void                          exo_icon_view_set_search_column         (ExoIconView                  *icon_view,
                                                                       gint                          search_column);
ExoIconViewSearchEqualFunc    exo_icon_view_get_search_equal_func     (const ExoIconView            *icon_view);
void                          exo_icon_view_set_search_equal_func     (ExoIconView                  *icon_view,
                                                                       ExoIconViewSearchEqualFunc    search_equal_func,
                                                                       gpointer                      search_equal_data,
                                                                       GDestroyNotify                search_equal_destroy);
ExoIconViewSearchPositionFunc exo_icon_view_get_search_position_func  (const ExoIconView            *icon_view);
void                          exo_icon_view_set_search_position_func  (ExoIconView                  *icon_view,
                                                                       ExoIconViewSearchPositionFunc search_position_func,
                                                                       gpointer                      search_position_data,
                                                                       GDestroyNotify                search_position_destroy);

G_END_DECLS

#endif /* __EXO_ICON_VIEW_H__ */
