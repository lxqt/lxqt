/*-
 * Copyright (c) 2008       Jannis Pohlmann <jannis@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* #ifdef HAVE_MATH_H */
#include <math.h>
/* #endif */
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gdk/gdkkeysyms.h>

/*
#include <exo/exo-config.h>
#include <exo/exo-enum-types.h>
#include <exo/exo-icon-view.h>
#include <exo/exo-marshal.h>
#include <exo/exo-private.h>
#include <exo/exo-string.h>
#include <exo/exo-alias.h>
*/
#include "exo-icon-view.h"
#include "exo-string.h"
#include "exo-marshal.h"
#include "exo-private.h"

/* libfm specific */
#include "gtk-compat.h"

/* from exo/exo-marshal.h */
#if defined(G_PARAM_STATIC_NAME) && defined(G_PARAM_STATIC_NICK) && defined(G_PARAM_STATIC_BLURB)
#define EXO_PARAM_READABLE  (G_PARAM_READABLE \
                           | G_PARAM_STATIC_NAME \
                           | G_PARAM_STATIC_NICK \
                           | G_PARAM_STATIC_BLURB)
#define EXO_PARAM_WRITABLE  (G_PARAM_WRITABLE \
                           | G_PARAM_STATIC_NAME \
                           | G_PARAM_STATIC_NICK \
                           | G_PARAM_STATIC_BLURB)
#define EXO_PARAM_READWRITE (G_PARAM_READWRITE \
                           | G_PARAM_STATIC_NAME \
                           | G_PARAM_STATIC_NICK \
                           | G_PARAM_STATIC_BLURB)
#else
#define EXO_PARAM_READABLE  (G_PARAM_READABLE)
#define EXO_PARAM_WRITABLE  (G_PARAM_WRITABLE)
#define EXO_PARAM_READWRITE (G_PARAM_READWRITE)
#endif

#define             I_(string)  g_intern_static_string(string)

/* from exo/exo-enum-types.h */
GType
exo_icon_view_layout_mode_get_type (void)
{
    static GType type = 0;
    if (type == 0) {
    static const GEnumValue values[] = {
    { EXO_ICON_VIEW_LAYOUT_ROWS, "EXO_ICON_VIEW_LAYOUT_ROWS", "rows" },
    { EXO_ICON_VIEW_LAYOUT_COLS, "EXO_ICON_VIEW_LAYOUT_COLS", "cols" },
    { 0, NULL, NULL }
    };
    type = g_enum_register_static ("ExoIconViewLayoutMode", values);
  }
    return type;
}
#define EXO_TYPE_ICON_VIEW_LAYOUT_MODE (exo_icon_view_layout_mode_get_type())
/* enumerations from "exo-mount-point.h" */


/* the search dialog timeout (in ms) */
#define EXO_ICON_VIEW_SEARCH_DIALOG_TIMEOUT (5000)

#define SCROLL_EDGE_SIZE 15



/* Property identifiers */
enum
{
  PROP_0,
  PROP_PIXBUF_COLUMN,
  PROP_TEXT_COLUMN,
  PROP_MARKUP_COLUMN,
  PROP_SELECTION_MODE,
  PROP_LAYOUT_MODE,
  PROP_ORIENTATION,
  PROP_MODEL,
  PROP_COLUMNS,
  PROP_ITEM_WIDTH,
  PROP_SPACING,
  PROP_ROW_SPACING,
  PROP_COLUMN_SPACING,
  PROP_MARGIN,
  PROP_REORDERABLE,
  PROP_SINGLE_CLICK,
  PROP_SINGLE_CLICK_TIMEOUT,
  PROP_ENABLE_SEARCH,
  PROP_SEARCH_COLUMN,
  /* For scrollable interface */
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HSCROLL_POLICY,
  PROP_VSCROLL_POLICY
};

/* Signal identifiers */
enum
{
  ITEM_ACTIVATED,
  SELECTION_CHANGED,
  SELECT_ALL,
  UNSELECT_ALL,
  SELECT_CURSOR_ITEM,
  TOGGLE_CURSOR_ITEM,
  MOVE_CURSOR,
  ACTIVATE_CURSOR_ITEM,
  START_INTERACTIVE_SEARCH,
  LAST_SIGNAL
};

/* Icon view flags */
typedef enum
{
  EXO_ICON_VIEW_DRAW_KEYFOCUS = (1l << 0),  /* whether to draw keyboard focus */
  EXO_ICON_VIEW_ITERS_PERSIST = (1l << 1),  /* whether current model provides persistent iterators */
} ExoIconViewFlags;

#define EXO_ICON_VIEW_SET_FLAG(icon_view, flag)   G_STMT_START{ (EXO_ICON_VIEW (icon_view)->priv->flags |= flag); }G_STMT_END
#define EXO_ICON_VIEW_UNSET_FLAG(icon_view, flag) G_STMT_START{ (EXO_ICON_VIEW (icon_view)->priv->flags &= ~(flag)); }G_STMT_END
#define EXO_ICON_VIEW_FLAG_SET(icon_view, flag)   ((EXO_ICON_VIEW (icon_view)->priv->flags & (flag)) == (flag))



typedef struct _ExoIconViewCellInfo ExoIconViewCellInfo;
typedef struct _ExoIconViewChild    ExoIconViewChild;
typedef struct _ExoIconViewItem     ExoIconViewItem;



#define EXO_ICON_VIEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), EXO_TYPE_ICON_VIEW, ExoIconViewPrivate))
#define EXO_ICON_VIEW_CELL_INFO(obj)   ((ExoIconViewCellInfo *) (obj))
#define EXO_ICON_VIEW_CHILD(obj)       ((ExoIconViewChild *) (obj))
#define EXO_ICON_VIEW_ITEM(obj)        ((ExoIconViewItem *) (obj))



static void                 exo_icon_view_cell_layout_init               (GtkCellLayoutIface     *iface);
static void                 exo_icon_view_dispose                        (GObject                *object);
static void                 exo_icon_view_finalize                       (GObject                *object);
static void                 exo_icon_view_get_property                   (GObject                *object,
                                                                          guint                   prop_id,
                                                                          GValue                 *value,
                                                                          GParamSpec             *pspec);
static void                 exo_icon_view_set_property                   (GObject                *object,
                                                                          guint                   prop_id,
                                                                          const GValue           *value,
                                                                          GParamSpec             *pspec);
static void                 exo_icon_view_realize                        (GtkWidget              *widget);
static void                 exo_icon_view_unrealize                      (GtkWidget              *widget);
#if GTK_CHECK_VERSION(3, 0, 0)
static void                 exo_icon_view_state_flags_changed            (GtkWidget *widget,
                                                                          GtkStateFlags previous_state);
#else
static void                 exo_icon_view_state_changed                  (GtkWidget *widget,
                                                                          GtkStateType previous_state);
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
static void                 exo_icon_view_get_preferred_width            (GtkWidget              *widget,
                                                                          gint                   *minimal_width,
                                                                          gint                   *natural_width);
static void                 exo_icon_view_get_preferred_height           (GtkWidget              *widget,
                                                                          gint                   *minimal_height,
                                                                          gint                   *natural_height);
#endif
static void                 exo_icon_view_size_request                   (GtkWidget              *widget,
                                                                          GtkRequisition         *requisition);
static void                 exo_icon_view_size_allocate                  (GtkWidget              *widget,
                                                                          GtkAllocation          *allocation);
static void                 exo_icon_view_style_set                      (GtkWidget              *widget,
                                                                          GtkStyle               *previous_style);
#if GTK_CHECK_VERSION(3, 0, 0)
static gboolean             exo_icon_view_draw                           (GtkWidget              *widget,
                                                                          cairo_t                *cr);
#else
static gboolean             exo_icon_view_expose_event                   (GtkWidget              *widget,
                                                                          GdkEventExpose         *event);
#endif
static gboolean             exo_icon_view_motion_notify_event            (GtkWidget              *widget,
                                                                          GdkEventMotion         *event);
static gboolean             exo_icon_view_button_press_event             (GtkWidget              *widget,
                                                                          GdkEventButton         *event);
static gboolean             exo_icon_view_button_release_event           (GtkWidget              *widget,
                                                                          GdkEventButton         *event);
static gboolean             exo_icon_view_scroll_event                   (GtkWidget              *widget,
                                                                          GdkEventScroll         *event);
static gboolean             exo_icon_view_key_press_event                (GtkWidget              *widget,
                                                                          GdkEventKey            *event);
static gboolean             exo_icon_view_focus_out_event                (GtkWidget              *widget,
                                                                          GdkEventFocus          *event);
static gboolean             exo_icon_view_leave_notify_event             (GtkWidget              *widget,
                                                                          GdkEventCrossing       *event);
static void                 exo_icon_view_remove                         (GtkContainer           *container,
                                                                          GtkWidget              *widget);
static void                 exo_icon_view_forall                         (GtkContainer           *container,
                                                                          gboolean                include_internals,
                                                                          GtkCallback             callback,
                                                                          gpointer                callback_data);
static AtkObject           *exo_icon_view_get_accessible                 (GtkWidget              *widget);
#if !GTK_CHECK_VERSION(3, 0, 0)
static void                 exo_icon_view_set_adjustments                (ExoIconView            *icon_view,
                                                                          GtkAdjustment          *hadj,
                                                                          GtkAdjustment          *vadj);
#else
static void                 exo_icon_view_set_hadjustment                (ExoIconView            *icon_view,
                                                                          GtkAdjustment          *hadj);
static void                 exo_icon_view_set_vadjustment                (ExoIconView            *icon_view,
                                                                          GtkAdjustment          *vadj);
#endif
static void                 exo_icon_view_real_select_all                (ExoIconView            *icon_view);
static void                 exo_icon_view_real_unselect_all              (ExoIconView            *icon_view);
static void                 exo_icon_view_real_select_cursor_item        (ExoIconView            *icon_view);
static void                 exo_icon_view_real_toggle_cursor_item        (ExoIconView            *icon_view);
static gboolean             exo_icon_view_real_activate_cursor_item      (ExoIconView            *icon_view);
static gboolean             exo_icon_view_real_start_interactive_search  (ExoIconView            *icon_view);
static void                 exo_icon_view_adjustment_changed             (GtkAdjustment          *adjustment,
                                                                          ExoIconView            *icon_view);
static gint                 exo_icon_view_layout_cols                    (ExoIconView            *icon_view,
                                                                          gint                    item_height,
                                                                          gint                   *x,
                                                                          gint                   *maximum_height,
                                                                          gint                    max_rows);
static gint                 exo_icon_view_layout_rows                    (ExoIconView            *icon_view,
                                                                          gint                    item_width,
                                                                          gint                   *y,
                                                                          gint                   *maximum_width,
                                                                          gint                    max_cols);
static void                 exo_icon_view_layout                         (ExoIconView            *icon_view);
static void                 exo_icon_view_paint_item                     (ExoIconView            *icon_view,
                                                                          ExoIconViewItem        *item,
                                                                          GdkRectangle           *area,
#if GTK_CHECK_VERSION(3, 0, 0)
                                                                          cairo_t                *drawable,
#else
                                                                          GdkDrawable            *drawable,
#endif
                                                                          gint                    x,
                                                                          gint                    y,
                                                                          gboolean                draw_focus);
static void                 exo_icon_view_queue_draw_item                (ExoIconView            *icon_view,
                                                                          ExoIconViewItem        *item);
static void                 exo_icon_view_queue_layout                   (ExoIconView            *icon_view);
static void                 exo_icon_view_set_cursor_item                (ExoIconView            *icon_view,
                                                                          ExoIconViewItem        *item,
                                                                          gint                    cursor_cell);
static void                 exo_icon_view_start_rubberbanding            (ExoIconView            *icon_view,
                                                                          gint                    x,
                                                                          gint                    y);
static void                 exo_icon_view_stop_rubberbanding             (ExoIconView            *icon_view);
static void                 exo_icon_view_update_rubberband_selection    (ExoIconView            *icon_view);
static gboolean             exo_icon_view_item_hit_test                  (ExoIconView            *icon_view,
                                                                          ExoIconViewItem        *item,
                                                                          gint                    x,
                                                                          gint                    y,
                                                                          gint                    width,
                                                                          gint                    height);
static gboolean             exo_icon_view_unselect_all_internal          (ExoIconView            *icon_view);
static void                 exo_icon_view_calculate_item_size            (ExoIconView            *icon_view,
                                                                          ExoIconViewItem        *item);
static void                 exo_icon_view_calculate_item_size2           (ExoIconView            *icon_view,
                                                                          ExoIconViewItem        *item,
                                                                          gint                   *max_width,
                                                                          gint                   *max_height);
static void                 exo_icon_view_update_rubberband              (gpointer                data);
static void                 exo_icon_view_invalidate_sizes               (ExoIconView            *icon_view);
static void                 exo_icon_view_add_move_binding               (GtkBindingSet          *binding_set,
                                                                          guint                   keyval,
                                                                          guint                   modmask,
                                                                          GtkMovementStep         step,
                                                                          gint                    count);
static gboolean             exo_icon_view_real_move_cursor               (ExoIconView            *icon_view,
                                                                          GtkMovementStep         step,
                                                                          gint                    count);
static void                 exo_icon_view_move_cursor_up_down            (ExoIconView            *icon_view,
                                                                          gint                    count);
static void                 exo_icon_view_move_cursor_page_up_down       (ExoIconView            *icon_view,
                                                                          gint                    count);
static void                 exo_icon_view_move_cursor_left_right         (ExoIconView            *icon_view,
                                                                          gint                    count);
static void                 exo_icon_view_move_cursor_start_end          (ExoIconView            *icon_view,
                                                                          gint                    count);
static void                 exo_icon_view_scroll_to_item                 (ExoIconView            *icon_view,
                                                                          ExoIconViewItem        *item);
static void                 exo_icon_view_select_item                    (ExoIconView            *icon_view,
                                                                          ExoIconViewItem        *item);
static void                 exo_icon_view_unselect_item                  (ExoIconView            *icon_view,
                                                                          ExoIconViewItem        *item);
static gboolean             exo_icon_view_select_all_between             (ExoIconView            *icon_view,
                                                                          ExoIconViewItem        *anchor,
                                                                          ExoIconViewItem        *cursor);
static ExoIconViewItem *    exo_icon_view_get_item_at_coords             (const ExoIconView      *icon_view,
                                                                          gint                    x,
                                                                          gint                    y,
                                                                          gboolean                only_in_cell,
                                                                          ExoIconViewCellInfo   **cell_at_pos);
static void                 exo_icon_view_get_cell_area                  (ExoIconView            *icon_view,
                                                                          ExoIconViewItem        *item,
                                                                          ExoIconViewCellInfo    *cell_info,
                                                                          GdkRectangle           *cell_area);
static ExoIconViewCellInfo *exo_icon_view_get_cell_info                  (ExoIconView            *icon_view,
                                                                          GtkCellRenderer        *renderer);
static void                 exo_icon_view_set_cell_data                  (const ExoIconView      *icon_view,
                                                                          ExoIconViewItem        *item);
static void                 exo_icon_view_cell_layout_pack_start         (GtkCellLayout          *layout,
                                                                          GtkCellRenderer        *renderer,
                                                                          gboolean                expand);
static void                 exo_icon_view_cell_layout_pack_end           (GtkCellLayout          *layout,
                                                                          GtkCellRenderer        *renderer,
                                                                          gboolean                expand);
static void                 exo_icon_view_cell_layout_add_attribute      (GtkCellLayout          *layout,
                                                                          GtkCellRenderer        *renderer,
                                                                          const gchar            *attribute,
                                                                          gint                    column);
static void                 exo_icon_view_cell_layout_clear              (GtkCellLayout          *layout);
static void                 exo_icon_view_cell_layout_clear_attributes   (GtkCellLayout          *layout,
                                                                          GtkCellRenderer        *renderer);
static void                 exo_icon_view_cell_layout_set_cell_data_func (GtkCellLayout          *layout,
                                                                          GtkCellRenderer        *cell,
                                                                          GtkCellLayoutDataFunc   func,
                                                                          gpointer                func_data,
                                                                          GDestroyNotify          destroy);
static void                 exo_icon_view_cell_layout_reorder            (GtkCellLayout          *layout,
                                                                          GtkCellRenderer        *cell,
                                                                          gint                    position);
static void                 exo_icon_view_item_activate_cell             (ExoIconView            *icon_view,
                                                                          ExoIconViewItem        *item,
                                                                          ExoIconViewCellInfo    *cell_info,
                                                                          GdkEvent               *event);
static void                 exo_icon_view_put                            (ExoIconView            *icon_view,
                                                                          GtkWidget              *widget,
                                                                          ExoIconViewItem        *item,
                                                                          gint                    cell);
static void                 exo_icon_view_remove_widget                  (GtkCellEditable        *editable,
                                                                          ExoIconView            *icon_view);
static void                 exo_icon_view_start_editing                  (ExoIconView            *icon_view,
                                                                          ExoIconViewItem        *item,
                                                                          ExoIconViewCellInfo    *cell_info,
                                                                          GdkEvent               *event);
static void                 exo_icon_view_stop_editing                   (ExoIconView            *icon_view,
                                                                          gboolean                cancel_editing);

/* Source side drag signals */
static void exo_icon_view_drag_begin       (GtkWidget        *widget,
                                            GdkDragContext   *context);
static void exo_icon_view_drag_end         (GtkWidget        *widget,
                                            GdkDragContext   *context);
static void exo_icon_view_drag_data_get    (GtkWidget        *widget,
                                            GdkDragContext   *context,
                                            GtkSelectionData *selection_data,
                                            guint             info,
                                            guint             drag_time);
static void exo_icon_view_drag_data_delete (GtkWidget        *widget,
                                            GdkDragContext   *context);

/* Target side drag signals */
static void     exo_icon_view_drag_leave         (GtkWidget        *widget,
                                                  GdkDragContext   *context,
                                                  guint             drag_time);
static gboolean exo_icon_view_drag_motion        (GtkWidget        *widget,
                                                  GdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  guint             drag_time);
static gboolean exo_icon_view_drag_drop          (GtkWidget        *widget,
                                                  GdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  guint             drag_time);
static void     exo_icon_view_drag_data_received (GtkWidget        *widget,
                                                  GdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  GtkSelectionData *selection_data,
                                                  guint             info,
                                                  guint             drag_time);
static gboolean exo_icon_view_maybe_begin_drag   (ExoIconView      *icon_view,
                                                  GdkEventMotion   *event);

static void     remove_scroll_timeout            (ExoIconView *icon_view);

/* single-click autoselection support */
static gboolean exo_icon_view_single_click_timeout          (gpointer user_data);
static void     exo_icon_view_single_click_timeout_destroy  (gpointer user_data);

/* Interactive search support */
static void     exo_icon_view_search_activate           (GtkEntry       *entry,
                                                         ExoIconView    *icon_view);
static void     exo_icon_view_search_dialog_hide        (GtkWidget      *search_dialog,
                                                         ExoIconView    *icon_view);
static void     exo_icon_view_search_ensure_directory   (ExoIconView    *icon_view);
static void     exo_icon_view_search_init               (GtkWidget      *search_entry,
                                                         ExoIconView    *icon_view);
static gboolean exo_icon_view_search_iter               (ExoIconView    *icon_view,
                                                         GtkTreeModel   *model,
                                                         GtkTreeIter    *iter,
                                                         const gchar    *text,
                                                         gint           *count,
                                                         gint            n);
static void     exo_icon_view_search_move               (GtkWidget      *widget,
                                                         ExoIconView    *icon_view,
                                                         gboolean        move_up);
#if GTK_CHECK_VERSION(2, 20, 0)
static void     exo_icon_view_search_preedit_changed    (GtkEntry       *entry,
                                                         gchar          *preedit,
                                                         ExoIconView    *icon_view);
#else
static void     exo_icon_view_search_preedit_changed    (GtkIMContext   *im_context,
                                                         ExoIconView    *icon_view);
#endif
static gboolean exo_icon_view_search_start              (ExoIconView    *icon_view,
                                                         gboolean        keybinding);
static gboolean exo_icon_view_search_equal_func         (GtkTreeModel   *model,
                                                         gint            column,
                                                         const gchar    *key,
                                                         GtkTreeIter    *iter,
                                                         gpointer        user_data);
static void     exo_icon_view_search_position_func      (ExoIconView    *icon_view,
                                                         GtkWidget      *search_dialog,
                                                         gpointer        user_data);
static gboolean exo_icon_view_search_button_press_event (GtkWidget      *widget,
                                                         GdkEventButton *event,
                                                         ExoIconView    *icon_view);
static gboolean exo_icon_view_search_delete_event       (GtkWidget      *widget,
                                                         GdkEventAny    *event,
                                                         ExoIconView    *icon_view);
static gboolean exo_icon_view_search_key_press_event    (GtkWidget      *widget,
                                                         GdkEventKey    *event,
                                                         ExoIconView    *icon_view);
static gboolean exo_icon_view_search_scroll_event       (GtkWidget      *widget,
                                                         GdkEventScroll *event,
                                                         ExoIconView    *icon_view);
static gboolean exo_icon_view_search_timeout            (gpointer        user_data);
static void     exo_icon_view_search_timeout_destroy    (gpointer        user_data);



struct _ExoIconViewCellInfo
{
  GtkCellRenderer      *cell;
  guint                 expand : 1;
  guint                 pack : 1;
  guint                 editing : 1;
  gint                  position;
  GSList               *attributes;
  GtkCellLayoutDataFunc func;
  gpointer              func_data;
  GDestroyNotify        destroy;
  gboolean              is_text;
};

struct _ExoIconViewChild
{
  ExoIconViewItem *item;
  GtkWidget       *widget;
  gint             cell;
};

struct _ExoIconViewItem
{
  GtkTreeIter iter;

  /* Bounding box (a value of -1 for width indicates
   * that the item needs to be layouted first)
   */
  GdkRectangle area;

  /* Individual cells.
   * box[i] is the actual area occupied by cell i,
   * before, after are used to calculate the cell
   * area relative to the box.
   * See exo_icon_view_get_cell_area().
   */
  gint n_cells;
  GdkRectangle *box;
  gint index;
  gint *before;
  gint *after;

  guint row : ((sizeof (guint) / 2) * 8) - 1;
  guint col : ((sizeof (guint) / 2) * 8) - 1;
  guint selected : 1;
  guint selected_before_rubberbanding : 1;
};

struct _ExoIconViewPrivate
{
  gint width, height;
  gint rows, cols;

  GtkSelectionMode selection_mode;

  ExoIconViewLayoutMode layout_mode;

  GdkWindow *bin_window;

  GList *children;

  GtkTreeModel *model;

  GList *items;

  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;
#if GTK_CHECK_VERSION(3, 0, 0)
  /* GtkScrollablePolicy needs to be checked when
   * driving the scrollable adjustment values */
  guint hscroll_policy : 1;
  guint vscroll_policy : 1;
#endif

  gint layout_idle_id;

  gboolean doing_rubberband;
  gint rubberband_x_1, rubberband_y_1;
  gint rubberband_x2, rubberband_y2;

  gint scroll_timeout_id;
  gint scroll_value_diff;
  gint event_last_x, event_last_y;

  ExoIconViewItem *anchor_item;
  ExoIconViewItem *cursor_item;
  ExoIconViewItem *edited_item;
  GtkCellEditable *editable;
  ExoIconViewItem *prelit_item;

  ExoIconViewItem *last_single_clicked;

  GList *cell_list;
  gint n_cells;

  gint cursor_cell;

  GtkOrientation orientation;

  gint columns;
  gint item_width;
  gint spacing;
  gint row_spacing;
  gint column_spacing;
  gint margin;

  gint text_column;
  gint markup_column;
  gint pixbuf_column;

  gint pixbuf_cell;
  gint text_cell;

  /* Drag-and-drop. */
  GdkModifierType start_button_mask;
  gint pressed_button;
  gint press_start_x;
  gint press_start_y;

  GtkTargetList *source_targets;
  GdkDragAction source_actions;

  GtkTargetList *dest_targets;
  GdkDragAction dest_actions;

  GtkTreeRowReference *dest_item;
  ExoIconViewDropPosition dest_pos;

  /* delayed scrolling */
  GtkTreeRowReference          *scroll_to_path;
  gfloat                        scroll_to_row_align;
  gfloat                        scroll_to_col_align;
  guint                         scroll_to_use_align : 1;

  /* misc flags */
  guint                         source_set : 1;
  guint                         dest_set : 1;
  guint                         reorderable : 1;
  guint                         empty_view_drop :1;

  guint                         ctrl_pressed : 1;
  guint                         shift_pressed : 1;

  guint                         dnd_locked : 1;

  /* Single-click support
   * The single_click_timeout is the timeout after which the
   * prelited item will be automatically selected in single
   * click mode (0 to disable).
   */
  guint                         single_click : 1;
  guint                         single_click_timeout;
  guint                         single_click_timeout_id;
  guint                         single_click_timeout_state;

  /* Interactive search support */
  guint                         enable_search : 1;
  guint                         search_imcontext_changed : 1;
  gint                          search_column;
  gint                          search_selected_iter;
  gint                          search_timeout_id;
  gboolean                      search_disable_popdown;
  ExoIconViewSearchEqualFunc    search_equal_func;
  gpointer                      search_equal_data;
  GDestroyNotify                search_equal_destroy;
  ExoIconViewSearchPositionFunc search_position_func;
  gpointer                      search_position_data;
  GDestroyNotify                search_position_destroy;
  gint                          search_entry_changed_id;
  GtkWidget                    *search_entry;
  GtkWidget                    *search_window;

  /* ExoIconViewFlags */
  guint flags;
};



static guint icon_view_signals[LAST_SIGNAL];



G_DEFINE_TYPE_WITH_CODE (ExoIconView, exo_icon_view, GTK_TYPE_CONTAINER,
#if GTK_CHECK_VERSION(3, 0, 0)
    G_IMPLEMENT_INTERFACE (GTK_TYPE_SCROLLABLE, NULL)
#endif
    G_IMPLEMENT_INTERFACE (GTK_TYPE_CELL_LAYOUT, exo_icon_view_cell_layout_init))



static void
exo_icon_view_class_init (ExoIconViewClass *klass)
{
  GtkContainerClass *gtkcontainer_class;
  GtkWidgetClass    *gtkwidget_class;
  GtkBindingSet     *gtkbinding_set;
  GObjectClass      *gobject_class;

  /* add our private data to the type's instances */
  g_type_class_add_private (klass, sizeof (ExoIconViewPrivate));

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = exo_icon_view_dispose;
  gobject_class->finalize = exo_icon_view_finalize;
  gobject_class->set_property = exo_icon_view_set_property;
  gobject_class->get_property = exo_icon_view_get_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->realize = exo_icon_view_realize;
  gtkwidget_class->unrealize = exo_icon_view_unrealize;
  gtkwidget_class->get_accessible = exo_icon_view_get_accessible;
#if GTK_CHECK_VERSION(3, 0, 0)
  gtkwidget_class->state_flags_changed = exo_icon_view_state_flags_changed;
  gtkwidget_class->get_preferred_width = exo_icon_view_get_preferred_width;
  gtkwidget_class->get_preferred_height = exo_icon_view_get_preferred_height;
#else
  gtkwidget_class->state_changed = exo_icon_view_state_changed;
  gtkwidget_class->size_request = exo_icon_view_size_request;
#endif
  gtkwidget_class->size_allocate = exo_icon_view_size_allocate;
  gtkwidget_class->style_set = exo_icon_view_style_set;
#if GTK_CHECK_VERSION(3, 0, 0)
  gtkwidget_class->draw = exo_icon_view_draw;
#else
  gtkwidget_class->expose_event = exo_icon_view_expose_event;
#endif
  gtkwidget_class->motion_notify_event = exo_icon_view_motion_notify_event;
  gtkwidget_class->button_press_event = exo_icon_view_button_press_event;
  gtkwidget_class->button_release_event = exo_icon_view_button_release_event;
  gtkwidget_class->scroll_event = exo_icon_view_scroll_event;
  gtkwidget_class->key_press_event = exo_icon_view_key_press_event;
  gtkwidget_class->focus_out_event = exo_icon_view_focus_out_event;
  gtkwidget_class->leave_notify_event = exo_icon_view_leave_notify_event;
  gtkwidget_class->drag_begin = exo_icon_view_drag_begin;
  gtkwidget_class->drag_end = exo_icon_view_drag_end;
  gtkwidget_class->drag_data_get = exo_icon_view_drag_data_get;
  gtkwidget_class->drag_data_delete = exo_icon_view_drag_data_delete;
  gtkwidget_class->drag_leave = exo_icon_view_drag_leave;
  gtkwidget_class->drag_motion = exo_icon_view_drag_motion;
  gtkwidget_class->drag_drop = exo_icon_view_drag_drop;
  gtkwidget_class->drag_data_received = exo_icon_view_drag_data_received;

  gtkcontainer_class = GTK_CONTAINER_CLASS (klass);
  gtkcontainer_class->remove = exo_icon_view_remove;
  gtkcontainer_class->forall = exo_icon_view_forall;


#if !GTK_CHECK_VERSION(3, 0, 0)
  klass->set_scroll_adjustments = exo_icon_view_set_adjustments;
#endif
  klass->select_all = exo_icon_view_real_select_all;
  klass->unselect_all = exo_icon_view_real_unselect_all;
  klass->select_cursor_item = exo_icon_view_real_select_cursor_item;
  klass->toggle_cursor_item = exo_icon_view_real_toggle_cursor_item;
  klass->move_cursor = exo_icon_view_real_move_cursor;
  klass->activate_cursor_item = exo_icon_view_real_activate_cursor_item;
  klass->start_interactive_search = exo_icon_view_real_start_interactive_search;

  /**
   * ExoIconView:column-spacing:
   *
   * The column-spacing property specifies the space which is inserted between
   * the columns of the icon view.
   *
   * Since: 0.3.1
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLUMN_SPACING,
                                   g_param_spec_int ("column-spacing",
                                                     _("Column Spacing"),
                                                     _("Space which is inserted between grid column"),
                                                     0, G_MAXINT, 6,
                                                     EXO_PARAM_READWRITE));

  /**
   * ExoIconView:columns:
   *
   * The columns property contains the number of the columns in which the
   * items should be displayed. If it is -1, the number of columns will
   * be chosen automatically to fill the available area.
   *
   * Since: 0.3.1
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLUMNS,
                                   g_param_spec_int ("columns",
                                                     _("Number of columns"),
                                                     _("Number of columns to display"),
                                                     -1, G_MAXINT, -1,
                                                     EXO_PARAM_READWRITE));

  /**
   * ExoIconView:enable-search:
   *
   * View allows user to search through columns interactively.
   *
   * Since: 0.3.1.3
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ENABLE_SEARCH,
                                   g_param_spec_boolean ("enable-search",
                                                         _("Enable Search"),
                                                         _("View allows user to search through columns interactively"),
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));


  /**
   * ExoIconView:item-width:
   *
   * The item-width property specifies the width to use for each item.
   * If it is set to -1, the icon view will automatically determine a
   * suitable item size.
   *
   * Since: 0.3.1
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ITEM_WIDTH,
                                   g_param_spec_int ("item-width",
                                                     _("Width for each item"),
                                                     _("The width used for each item"),
                                                     -1, G_MAXINT, -1,
                                                     EXO_PARAM_READWRITE));

  /**
   * ExoIconView:layout-mode:
   *
   * The layout-mode property specifies the way items are layed out in
   * the #ExoIconView. This can be either %EXO_ICON_VIEW_LAYOUT_ROWS,
   * which is the default, where items are layed out horizontally in
   * rows from top to bottom, or %EXO_ICON_VIEW_LAYOUT_COLS, where items
   * are layed out vertically in columns from left to right.
   *
   * Since: 0.3.1.5
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_LAYOUT_MODE,
                                   g_param_spec_enum ("layout-mode",
                                                      _("Layout mode"),
                                                      _("The layout mode"),
                                                      EXO_TYPE_ICON_VIEW_LAYOUT_MODE,
                                                      EXO_ICON_VIEW_LAYOUT_ROWS,
                                                      EXO_PARAM_READWRITE));

  /**
   * ExoIconView:margin:
   *
   * The margin property specifies the space which is inserted
   * at the edges of the icon view.
   *
   * Since: 0.3.1
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MARGIN,
                                   g_param_spec_int ("margin",
                                                     _("Margin"),
                                                     _("Space which is inserted at the edges of the icon view"),
                                                     0, G_MAXINT, 6,
                                                     EXO_PARAM_READWRITE));

  /**
   * ExoIconView:markup-column:
   *
   * The markup-column property contains the number of the model column
   * containing markup information to be displayed. The markup column must be
   * of type #G_TYPE_STRING. If this property and the text-column property
   * are both set to column numbers, it overrides the text column.
   * If both are set to -1, no texts are displayed.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MARKUP_COLUMN,
                                   g_param_spec_int ("markup-column",
                                                     _("Markup column"),
                                                     _("Model column used to retrieve the text if using Pango markup"),
                                                     -1, G_MAXINT, -1,
                                                     EXO_PARAM_READWRITE));

  /**
   * ExoIconView:model:
   *
   * The model property contains the #GtkTreeModel, which should be
   * display by this icon view. Setting this property to %NULL turns
   * off the display of anything.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        _("Icon View Model"),
                                                        _("The model for the icon view"),
                                                        GTK_TYPE_TREE_MODEL,
                                                        EXO_PARAM_READWRITE));

  /**
   * ExoIconView:orientation:
   *
   * The orientation property specifies how the cells (i.e. the icon and
   * the text) of the item are positioned relative to each other.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation",
                                                      _("Orientation"),
                                                      _("How the text and icon of each item are positioned relative to each other"),
                                                      GTK_TYPE_ORIENTATION,
                                                      GTK_ORIENTATION_VERTICAL,
                                                      EXO_PARAM_READWRITE));

  /**
   * ExoIconView:pixbuf-column:
   *
   * The ::pixbuf-column property contains the number of the model column
   * containing the pixbufs which are displayed. The pixbuf column must be
   * of type #GDK_TYPE_PIXBUF. Setting this property to -1 turns off the
   * display of pixbufs.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_PIXBUF_COLUMN,
                                   g_param_spec_int ("pixbuf-column",
                                                     _("Pixbuf column"),
                                                     _("Model column used to retrieve the icon pixbuf from"),
                                                     -1, G_MAXINT, -1,
                                                     EXO_PARAM_READWRITE));

  /**
   * ExoIconView:reorderable:
   *
   * The reorderable property specifies if the items can be reordered
   * by Drag and Drop.
   *
   * Since: 0.3.1
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_REORDERABLE,
                                   g_param_spec_boolean ("reorderable",
                                                         _("Reorderable"),
                                                         _("View is reorderable"),
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  /**
   * ExoIconView:row-spacing:
   *
   * The row-spacing property specifies the space which is inserted between
   * the rows of the icon view.
   *
   * Since: 0.3.1
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ROW_SPACING,
                                   g_param_spec_int ("row-spacing",
                                                     _("Row Spacing"),
                                                     _("Space which is inserted between grid rows"),
                                                     0, G_MAXINT, 6,
                                                     EXO_PARAM_READWRITE));

  /**
   * ExoIconView:search-column:
   *
   * Model column to search through when searching through code.
   *
   * Since: 0.3.1.3
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SEARCH_COLUMN,
                                   g_param_spec_int ("search-column",
                                                     _("Search Column"),
                                                     _("Model column to search through when searching through item"),
                                                     -1, G_MAXINT, -1,
                                                     EXO_PARAM_READWRITE));

  /**
   * ExoIconView:selection-mode:
   *
   * The selection-mode property specifies the selection mode of
   * icon view. If the mode is #GTK_SELECTION_MULTIPLE, rubberband selection
   * is enabled, for the other modes, only keyboard selection is possible.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SELECTION_MODE,
                                   g_param_spec_enum ("selection-mode",
                                                      _("Selection mode"),
                                                      _("The selection mode"),
                                                      GTK_TYPE_SELECTION_MODE,
                                                      GTK_SELECTION_SINGLE,
                                                      EXO_PARAM_READWRITE));

  /**
   * ExoIconView:single-click:
   *
   * Determines whether items can be activated by single or double clicks.
   *
   * Since: 0.3.1.3
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SINGLE_CLICK,
                                   g_param_spec_boolean ("single-click",
                                                         _("Single Click"),
                                                         _("Whether the items in the view can be activated with single clicks"),
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  /**
   * ExoIconView:single-click-timeout:
   *
   * The amount of time in milliseconds after which a prelited item (an item
   * which is hovered by the mouse cursor) will be selected automatically in
   * single click mode. A value of %0 disables the automatic selection.
   *
   * Since: 0.3.1.5
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SINGLE_CLICK_TIMEOUT,
                                   g_param_spec_uint ("single-click-timeout",
                                                      _("Single Click Timeout"),
                                                      _("The amount of time after which the item under the mouse cursor will be selected automatically in single click mode"),
                                                      0, G_MAXUINT, 0,
                                                      EXO_PARAM_READWRITE));

  /**
   * ExoIconView:spacing:
   *
   * The spacing property specifies the space which is inserted between
   * the cells (i.e. the icon and the text) of an item.
   *
   * Since: 0.3.1
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SPACING,
                                   g_param_spec_int ("spacing",
                                                     _("Spacing"),
                                                     _("Space which is inserted between cells of an item"),
                                                     0, G_MAXINT, 0,
                                                     EXO_PARAM_READWRITE));

  /**
   * ExoIconView:text-column:
   *
   * The text-column property contains the number of the model column
   * containing the texts which are displayed. The text column must be
   * of type #G_TYPE_STRING. If this property and the markup-column
   * property are both set to -1, no texts are displayed.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TEXT_COLUMN,
                                   g_param_spec_int ("text-column",
                                                     _("Text column"),
                                                     _("Model column used to retrieve the text from"),
                                                     -1, G_MAXINT, -1,
                                                     EXO_PARAM_READWRITE));


#if GTK_CHECK_VERSION(3, 0, 0)
  /* Scrollable interface properties */
  g_object_class_override_property (gobject_class, PROP_HADJUSTMENT,    "hadjustment");
  g_object_class_override_property (gobject_class, PROP_VADJUSTMENT,    "vadjustment");
  g_object_class_override_property (gobject_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (gobject_class, PROP_VSCROLL_POLICY, "vscroll-policy");
#endif

  gtk_widget_class_install_style_property (gtkwidget_class,
                                           g_param_spec_boxed ("selection-box-color",
                                                               _("Selection Box Color"),
                                                               _("Color of the selection box"),
                                                               GDK_TYPE_COLOR,
                                                               EXO_PARAM_READABLE));

  gtk_widget_class_install_style_property (gtkwidget_class,
                                           g_param_spec_uchar ("selection-box-alpha",
                                                               _("Selection Box Alpha"),
                                                               _("Opacity of the selection box"),
                                                               0, 0xff,
                                                               0x40,
                                                               EXO_PARAM_READABLE));

  /**
   * ExoIconView::item-activated:
   * @icon_view : a #ExoIconView.
   * @path      :
   **/
  icon_view_signals[ITEM_ACTIVATED] =
    g_signal_new (I_("item-activated"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ExoIconViewClass, item_activated),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  GTK_TYPE_TREE_PATH);

  /**
   * ExoIconView::selection-changed:
   * @icon_view : a #ExoIconView.
   **/
  icon_view_signals[SELECTION_CHANGED] =
    g_signal_new (I_("selection-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ExoIconViewClass, selection_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

#if !GTK_CHECK_VERSION(3, 0, 0)
  /**
   * ExoIconView::set-scroll-adjustments:
   * @icon_view   : a #ExoIconView.
   * @hadjustment :
   * @vadjustment :
   **/
  gtkwidget_class->set_scroll_adjustments_signal =
    g_signal_new (I_("set-scroll-adjustments"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ExoIconViewClass, set_scroll_adjustments),
                  NULL, NULL,
                  _exo_marshal_VOID__OBJECT_OBJECT,
                  G_TYPE_NONE, 2,
                  GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
#endif

  /**
   * ExoIconView::select-all:
   * @icon_view : a #ExoIconView.
   **/
  icon_view_signals[SELECT_ALL] =
    g_signal_new (I_("select-all"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (ExoIconViewClass, select_all),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * ExoIconView::unselect-all:
   * @icon_view : a #ExoIconView.
   **/
  icon_view_signals[UNSELECT_ALL] =
    g_signal_new (I_("unselect-all"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (ExoIconViewClass, unselect_all),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * ExoIconView::select-cursor-item:
   * @icon_view : a #ExoIconView.
   **/
  icon_view_signals[SELECT_CURSOR_ITEM] =
    g_signal_new (I_("select-cursor-item"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (ExoIconViewClass, select_cursor_item),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * ExoIconView::toggle-cursor-item:
   * @icon_view : a #ExoIconView.
   **/
  icon_view_signals[TOGGLE_CURSOR_ITEM] =
    g_signal_new (I_("toggle-cursor-item"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (ExoIconViewClass, toggle_cursor_item),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * ExoIconView::activate-cursor-item:
   * @icon_view : a #ExoIconView.
   *
   * Return value:
   **/
  icon_view_signals[ACTIVATE_CURSOR_ITEM] =
    g_signal_new (I_("activate-cursor-item"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (ExoIconViewClass, activate_cursor_item),
                  NULL, NULL,
                  _exo_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);

  /**
   * ExoIconView::start-interactive-search:
   * @iconb_view : a #ExoIconView.
   *
   * Return value:
   **/
  icon_view_signals[START_INTERACTIVE_SEARCH] =
    g_signal_new (I_("start-interactive-search"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (ExoIconViewClass, start_interactive_search),
                  NULL, NULL,
                  _exo_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);

  /**
   * ExoIconView::move-cursor:
   * @icon_view : a #ExoIconView.
   * @step      :
   * @count     :
   *
   * Return value:
   **/
  icon_view_signals[MOVE_CURSOR] =
    g_signal_new (I_("move-cursor"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (ExoIconViewClass, move_cursor),
                  NULL, NULL,
                  _exo_marshal_BOOLEAN__ENUM_INT,
                  G_TYPE_BOOLEAN, 2,
                  GTK_TYPE_MOVEMENT_STEP,
                  G_TYPE_INT);

  /* Key bindings */
  gtkbinding_set = gtk_binding_set_by_class (klass);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_a, GDK_CONTROL_MASK, "select-all", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_a, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "unselect-all", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_space, GDK_CONTROL_MASK, "toggle-cursor-item", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_space, 0, "activate-cursor-item", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_Return, 0, "activate-cursor-item", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_ISO_Enter, 0, "activate-cursor-item", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_KP_Enter, 0, "activate-cursor-item", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_f, GDK_CONTROL_MASK, "start-interactive-search", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_F, GDK_CONTROL_MASK, "start-interactive-search", 0);

  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Up, 0, GTK_MOVEMENT_DISPLAY_LINES, -1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Up, 0, GTK_MOVEMENT_DISPLAY_LINES, -1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Down, 0, GTK_MOVEMENT_DISPLAY_LINES, 1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Down, 0, GTK_MOVEMENT_DISPLAY_LINES, 1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_p, GDK_CONTROL_MASK, GTK_MOVEMENT_DISPLAY_LINES, -1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_n, GDK_CONTROL_MASK, GTK_MOVEMENT_DISPLAY_LINES, 1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Home, 0, GTK_MOVEMENT_BUFFER_ENDS, -1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Home, 0, GTK_MOVEMENT_BUFFER_ENDS, -1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_End, 0, GTK_MOVEMENT_BUFFER_ENDS, 1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_End, 0, GTK_MOVEMENT_BUFFER_ENDS, 1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Page_Up, 0, GTK_MOVEMENT_PAGES, -1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Page_Up, 0, GTK_MOVEMENT_PAGES, -1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Page_Down, 0, GTK_MOVEMENT_PAGES, 1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Page_Down, 0, GTK_MOVEMENT_PAGES, 1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Right, 0, GTK_MOVEMENT_VISUAL_POSITIONS, 1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Left, 0, GTK_MOVEMENT_VISUAL_POSITIONS, -1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Right, 0, GTK_MOVEMENT_VISUAL_POSITIONS, 1);
  exo_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Left, 0, GTK_MOVEMENT_VISUAL_POSITIONS, -1);
}



static void
exo_icon_view_cell_layout_init (GtkCellLayoutIface *iface)
{
  iface->pack_start = exo_icon_view_cell_layout_pack_start;
  iface->pack_end = exo_icon_view_cell_layout_pack_end;
  iface->clear = exo_icon_view_cell_layout_clear;
  iface->add_attribute = exo_icon_view_cell_layout_add_attribute;
  iface->set_cell_data_func = exo_icon_view_cell_layout_set_cell_data_func;
  iface->clear_attributes = exo_icon_view_cell_layout_clear_attributes;
  iface->reorder = exo_icon_view_cell_layout_reorder;
}



static void
exo_icon_view_init (ExoIconView *icon_view)
{
  icon_view->priv = EXO_ICON_VIEW_GET_PRIVATE (icon_view);

  icon_view->priv->selection_mode = GTK_SELECTION_SINGLE;
  icon_view->priv->pressed_button = -1;
  icon_view->priv->press_start_x = -1;
  icon_view->priv->press_start_y = -1;
  icon_view->priv->text_column = -1;
  icon_view->priv->markup_column = -1;
  icon_view->priv->pixbuf_column = -1;
  icon_view->priv->text_cell = -1;
  icon_view->priv->pixbuf_cell = -1;

  gtk_widget_set_can_focus (GTK_WIDGET (icon_view), TRUE);

#if !GTK_CHECK_VERSION(3, 0, 0)
  exo_icon_view_set_adjustments (icon_view, NULL, NULL);
#endif

  icon_view->priv->cursor_cell = -1;

  icon_view->priv->orientation = GTK_ORIENTATION_VERTICAL;

  icon_view->priv->columns = -1;
  icon_view->priv->item_width = -1;
  icon_view->priv->row_spacing = 6;
  icon_view->priv->column_spacing = 6;
  icon_view->priv->margin = 6;

  icon_view->priv->enable_search = TRUE;
  icon_view->priv->search_column = -1;
  icon_view->priv->search_equal_func = exo_icon_view_search_equal_func;
  icon_view->priv->search_position_func = exo_icon_view_search_position_func;

  icon_view->priv->flags = EXO_ICON_VIEW_DRAW_KEYFOCUS;
}



static void
exo_icon_view_dispose (GObject *object)
{
  ExoIconView *icon_view = EXO_ICON_VIEW (object);

  /* cancel any pending search timeout */
  if (G_UNLIKELY (icon_view->priv->search_timeout_id != 0))
    g_source_remove (icon_view->priv->search_timeout_id);

  /* destroy the interactive search dialog */
  if (G_UNLIKELY (icon_view->priv->search_window != NULL))
    {
      gtk_widget_destroy (icon_view->priv->search_window);
      icon_view->priv->search_entry = NULL;
      icon_view->priv->search_window = NULL;
    }

  /* drop search equal and position functions (if any) */
  exo_icon_view_set_search_equal_func (icon_view, NULL, NULL, NULL);
  exo_icon_view_set_search_position_func (icon_view, NULL, NULL, NULL);

  /* reset the drag dest item */
  exo_icon_view_set_drag_dest_item (icon_view, NULL, EXO_ICON_VIEW_NO_DROP);

  /* drop the scroll to path (if any) */
  if (G_UNLIKELY (icon_view->priv->scroll_to_path != NULL))
    {
      gtk_tree_row_reference_free (icon_view->priv->scroll_to_path);
      icon_view->priv->scroll_to_path = NULL;
    }

  /* reset the model (also stops any active editing) */
  exo_icon_view_set_model (icon_view, NULL);

  /* drop the scroll timer */
  remove_scroll_timeout (icon_view);

  (*G_OBJECT_CLASS (exo_icon_view_parent_class)->dispose) (object);
}



static void
exo_icon_view_finalize (GObject *object)
{
  ExoIconView *icon_view = EXO_ICON_VIEW (object);

  /* drop the scroll adjustments */
  g_object_unref (G_OBJECT (icon_view->priv->hadjustment));
  g_object_unref (G_OBJECT (icon_view->priv->vadjustment));

  /* drop the cell renderers */
  exo_icon_view_cell_layout_clear (GTK_CELL_LAYOUT (icon_view));

  /* be sure to cancel the single click timeout */
  if (G_UNLIKELY (icon_view->priv->single_click_timeout_id != 0))
    g_source_remove (icon_view->priv->single_click_timeout_id);

  /* kill the layout idle source (it's important to have this last!) */
  if (G_UNLIKELY (icon_view->priv->layout_idle_id != 0))
    g_source_remove (icon_view->priv->layout_idle_id);

  (*G_OBJECT_CLASS (exo_icon_view_parent_class)->finalize) (object);
}


static void
exo_icon_view_get_property (GObject      *object,
                            guint         prop_id,
                            GValue       *value,
                            GParamSpec   *pspec)
{
  const ExoIconViewPrivate *priv = EXO_ICON_VIEW (object)->priv;

  switch (prop_id)
    {
    case PROP_COLUMN_SPACING:
      g_value_set_int (value, priv->column_spacing);
      break;

    case PROP_COLUMNS:
      g_value_set_int (value, priv->columns);
      break;

    case PROP_ENABLE_SEARCH:
      g_value_set_boolean (value, priv->enable_search);
      break;

    case PROP_ITEM_WIDTH:
      g_value_set_int (value, priv->item_width);
      break;

    case PROP_MARGIN:
      g_value_set_int (value, priv->margin);
      break;

    case PROP_MARKUP_COLUMN:
      g_value_set_int (value, priv->markup_column);
      break;

    case PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;

    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;

    case PROP_PIXBUF_COLUMN:
      g_value_set_int (value, priv->pixbuf_column);
      break;

    case PROP_REORDERABLE:
      g_value_set_boolean (value, priv->reorderable);
      break;

    case PROP_ROW_SPACING:
      g_value_set_int (value, priv->row_spacing);
      break;

    case PROP_SEARCH_COLUMN:
      g_value_set_int (value, priv->search_column);
      break;

    case PROP_SELECTION_MODE:
      g_value_set_enum (value, priv->selection_mode);
      break;

    case PROP_SINGLE_CLICK:
      g_value_set_boolean (value, priv->single_click);
      break;

    case PROP_SINGLE_CLICK_TIMEOUT:
      g_value_set_uint (value, priv->single_click_timeout);
      break;

    case PROP_SPACING:
      g_value_set_int (value, priv->spacing);
      break;

    case PROP_TEXT_COLUMN:
      g_value_set_int (value, priv->text_column);
      break;

    case PROP_LAYOUT_MODE:
      g_value_set_enum (value, priv->layout_mode);
      break;

#if GTK_CHECK_VERSION(3, 0, 0)
    case PROP_HADJUSTMENT:
      g_value_set_object (value, priv->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, priv->vadjustment);
      break;
    case PROP_HSCROLL_POLICY:
      g_value_set_enum (value, priv->hscroll_policy);
      break;
    case PROP_VSCROLL_POLICY:
      g_value_set_enum (value, priv->vscroll_policy);
      break;
#endif

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
exo_icon_view_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  ExoIconView *icon_view = EXO_ICON_VIEW (object);

  switch (prop_id)
    {
    case PROP_COLUMN_SPACING:
      exo_icon_view_set_column_spacing (icon_view, g_value_get_int (value));
      break;

    case PROP_COLUMNS:
      exo_icon_view_set_columns (icon_view, g_value_get_int (value));
      break;

    case PROP_ENABLE_SEARCH:
      exo_icon_view_set_enable_search (icon_view, g_value_get_boolean (value));
      break;

    case PROP_ITEM_WIDTH:
      exo_icon_view_set_item_width (icon_view, g_value_get_int (value));
      break;

    case PROP_MARGIN:
      exo_icon_view_set_margin (icon_view, g_value_get_int (value));
      break;

    case PROP_MODEL:
      exo_icon_view_set_model (icon_view, g_value_get_object (value));
      break;

    case PROP_ORIENTATION:
      exo_icon_view_set_orientation (icon_view, g_value_get_enum (value));
      break;

    case PROP_REORDERABLE:
      exo_icon_view_set_reorderable (icon_view, g_value_get_boolean (value));
      break;

    case PROP_ROW_SPACING:
      exo_icon_view_set_row_spacing (icon_view, g_value_get_int (value));
      break;

    case PROP_SEARCH_COLUMN:
      exo_icon_view_set_search_column (icon_view, g_value_get_int (value));
      break;

    case PROP_SELECTION_MODE:
      exo_icon_view_set_selection_mode (icon_view, g_value_get_enum (value));
      break;

    case PROP_SINGLE_CLICK:
      exo_icon_view_set_single_click (icon_view, g_value_get_boolean (value));
      break;

    case PROP_SINGLE_CLICK_TIMEOUT:
      exo_icon_view_set_single_click_timeout (icon_view, g_value_get_uint (value));
      break;

    case PROP_SPACING:
      exo_icon_view_set_spacing (icon_view, g_value_get_int (value));
      break;

    case PROP_LAYOUT_MODE:
      exo_icon_view_set_layout_mode (icon_view, g_value_get_enum (value));
      break;

#if GTK_CHECK_VERSION(3, 0, 0)
    case PROP_HADJUSTMENT:
      exo_icon_view_set_hadjustment (icon_view, g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      exo_icon_view_set_vadjustment (icon_view, g_value_get_object (value));
      break;
    case PROP_HSCROLL_POLICY:
      icon_view->priv->hscroll_policy = g_value_get_enum (value);
      gtk_widget_queue_resize (GTK_WIDGET (icon_view));
      break;
    case PROP_VSCROLL_POLICY:
      icon_view->priv->vscroll_policy = g_value_get_enum (value);
      gtk_widget_queue_resize (GTK_WIDGET (icon_view));
      break;
#endif

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
exo_icon_view_realize (GtkWidget *widget)
{
  ExoIconViewPrivate *priv = EXO_ICON_VIEW (widget)->priv;
  GdkWindow          *window;
#if GTK_CHECK_VERSION(3, 0, 0)
  GtkStyleContext    *style;
#else
  GtkStyle           *style;
#endif
  GdkWindowAttr       attributes;
  GtkAllocation       allocation;
  gint                attributes_mask;

#if GTK_CHECK_VERSION(2, 20, 0)
  gtk_widget_set_realized (widget, TRUE);
#else
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
#endif

  /* Allocate the clipping window */
  gtk_widget_get_allocation (widget, &allocation);
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK;
#if GTK_CHECK_VERSION(3, 0, 0)
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
#else
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
#endif
  window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gtk_widget_set_window (widget, window);
  gdk_window_set_user_data (window, widget);

  /* Allocate the icons window */
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = MAX (priv->width, allocation.width);
  attributes.height = MAX (priv->height, allocation.height);
  attributes.event_mask = GDK_EXPOSURE_MASK
                        | GDK_SCROLL_MASK
                        | GDK_POINTER_MOTION_MASK
                        | GDK_BUTTON_PRESS_MASK
                        | GDK_BUTTON_RELEASE_MASK
                        | GDK_KEY_PRESS_MASK
                        | GDK_KEY_RELEASE_MASK
                        | gtk_widget_get_events (widget);
  priv->bin_window = gdk_window_new (window, &attributes, attributes_mask);
  gdk_window_set_user_data (priv->bin_window, widget);

  /* Attach style/background */
#if GTK_CHECK_VERSION(3, 0, 0)
  style = gtk_widget_get_style_context (widget);
  gtk_style_context_set_background (style, priv->bin_window);
  gtk_style_context_set_background (style, window);
#else
  style = gtk_style_attach (gtk_widget_get_style (widget), window);
  gtk_widget_set_style (widget, style);
  gdk_window_set_background (priv->bin_window, &style->base[gtk_widget_get_state(widget)]);
  gdk_window_set_background (window, &style->base[gtk_widget_get_state(widget)]);
#endif

  /* map the icons window */
  gdk_window_show (priv->bin_window);
}



static void
exo_icon_view_unrealize (GtkWidget *widget)
{
  ExoIconViewPrivate *priv = EXO_ICON_VIEW (widget)->priv;

  /* drop the icons window */
  gdk_window_set_user_data (priv->bin_window, NULL);
  gdk_window_destroy (priv->bin_window);
  priv->bin_window = NULL;

  /* let GtkWidget destroy children and widget->window */
  if (GTK_WIDGET_CLASS (exo_icon_view_parent_class)->unrealize)
    (*GTK_WIDGET_CLASS (exo_icon_view_parent_class)->unrealize) (widget);
}


#if GTK_CHECK_VERSION(3, 0, 0)
static void                 exo_icon_view_state_flags_changed            (GtkWidget *widget,
                                                                          GtkStateFlags previous_state)
#else
static void                 exo_icon_view_state_changed                  (GtkWidget *widget,
                                                                          GtkStateType previous_state)
#endif
{
  ExoIconViewPrivate *priv = EXO_ICON_VIEW (widget)->priv;
  GdkWindow *window = gtk_widget_get_window (widget);

  if (gtk_widget_get_realized (widget))
    {
#if GTK_CHECK_VERSION(3, 0, 0)
      GtkStyleContext *style = gtk_widget_get_style_context (widget);
      gtk_style_context_save (style);
      gtk_style_context_add_class (style, GTK_STYLE_CLASS_VIEW);
      gtk_style_context_set_background (style, priv->bin_window);
      gtk_style_context_set_background (style, window);
      gtk_style_context_restore (style);
#else
      GtkStyle *style = gtk_widget_get_style (widget);
      gdk_window_set_background (priv->bin_window, &style->base[gtk_widget_get_state(widget)]);
      gdk_window_set_background (window, &style->base[gtk_widget_get_state(widget)]);
#endif
    }

  gtk_widget_queue_draw (widget);
}


static void
exo_icon_view_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
  const ExoIconViewPrivate *priv = EXO_ICON_VIEW (widget)->priv;
  ExoIconViewChild         *child;
  GtkRequisition            child_requisition;
  GList                    *lp;

  /* well, this is easy */
  requisition->width = priv->width;
  requisition->height = priv->height;

  /* handle the child widgets */
  for (lp = priv->children; lp != NULL; lp = lp->next)
    {
      child = lp->data;
      if (gtk_widget_get_visible (child->widget))
#if GTK_CHECK_VERSION(3, 0, 0)
        gtk_widget_get_preferred_size (child->widget, NULL, &child_requisition);
#else
        gtk_widget_size_request (child->widget, &child_requisition);
#endif
    }
}

#if GTK_CHECK_VERSION(3, 0, 0)
static void
exo_icon_view_get_preferred_width (GtkWidget *widget,
                                   gint *minimal_width,
                                   gint *natural_width)
{
  GtkRequisition requisition;

  exo_icon_view_size_request (widget, &requisition);

  *minimal_width = *natural_width = requisition.width;
}

static void
exo_icon_view_get_preferred_height (GtkWidget *widget,
                                    gint *minimal_height,
                                    gint *natural_height)
{
  GtkRequisition requisition;

  exo_icon_view_size_request (widget, &requisition);

  *minimal_height = *natural_height = requisition.height;
}
#endif


static void
exo_icon_view_allocate_children (ExoIconView *icon_view)
{
  const ExoIconViewPrivate *priv = icon_view->priv;
  const ExoIconViewChild   *child;
  GtkAllocation             allocation;
  const GList              *lp;
  gint                      focus_line_width;
  gint                      focus_padding;

  for (lp = priv->children; lp != NULL; lp = lp->next)
    {
      child = EXO_ICON_VIEW_CHILD (lp->data);

      /* totally ignore our child's requisition */
      if (child->cell < 0)
        allocation = child->item->area;
      else
        allocation = child->item->box[child->cell];

      /* increase the item area by focus width/padding */
      gtk_widget_style_get (GTK_WIDGET (icon_view), "focus-line-width", &focus_line_width, "focus-padding", &focus_padding, NULL);
      allocation.x = MAX (0, allocation.x - (focus_line_width + focus_padding));
      allocation.y = MAX (0, allocation.y - (focus_line_width + focus_padding));
      allocation.width = MIN (priv->width - allocation.x, allocation.width + 2 * (focus_line_width + focus_padding));
      allocation.height = MIN (priv->height - allocation.y, allocation.height + 2 * (focus_line_width + focus_padding));

      /* allocate the area to the child */
      gtk_widget_size_allocate (child->widget, &allocation);
    }
}



static void
exo_icon_view_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;
  ExoIconView   *icon_view = EXO_ICON_VIEW (widget);

  /* FIXME: gtk 3.4.2 calls this with weird values sometimes.
     Don't know if that is a bug of gtk or something else.
     Since that will break the view a workaround is required. */
  if (allocation->x < 0)
    allocation->x = 0;
  if (allocation->y < 0)
    allocation->y = 0;

  /* apply the new size allocation */
  gtk_widget_set_allocation (widget, allocation);

  /* move/resize the clipping window, the icons window
   * will be handled by exo_icon_view_layout().
   */
  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (gtk_widget_get_window(widget), allocation->x, allocation->y, allocation->width, allocation->height);

  /* layout the items */
  exo_icon_view_layout (icon_view);

  /* allocate space to the widgets (editing) */
  exo_icon_view_allocate_children (icon_view);

  /* update the horizontal scroll adjustment accordingly */
  hadjustment = icon_view->priv->hadjustment;
  gtk_adjustment_set_page_size(hadjustment, allocation->width);
  gtk_adjustment_set_page_increment(hadjustment, allocation->width * 0.9);
  gtk_adjustment_set_step_increment(hadjustment, allocation->width * 0.1);
  gtk_adjustment_set_lower(hadjustment, 0);
  gtk_adjustment_set_upper(hadjustment, MAX (allocation->width, icon_view->priv->width));
  if (gtk_adjustment_get_value(hadjustment) > gtk_adjustment_get_upper(hadjustment) - gtk_adjustment_get_page_size(hadjustment))
    gtk_adjustment_set_value (hadjustment, MAX (0, gtk_adjustment_get_upper(hadjustment) - gtk_adjustment_get_page_size(hadjustment)));

  /* update the vertical scroll adjustment accordingly */
  vadjustment = icon_view->priv->vadjustment;
  gtk_adjustment_set_page_size(vadjustment, allocation->height);
  gtk_adjustment_set_page_increment(vadjustment, allocation->height * 0.9);
  gtk_adjustment_set_step_increment(vadjustment, allocation->height * 0.1);
  gtk_adjustment_set_lower(vadjustment, 0);
  gtk_adjustment_set_upper(vadjustment, MAX (allocation->height, icon_view->priv->height));
  if (gtk_adjustment_get_value(vadjustment) > gtk_adjustment_get_upper(vadjustment) - gtk_adjustment_get_page_size(vadjustment))
    gtk_adjustment_set_value (vadjustment, MAX (0, gtk_adjustment_get_upper(vadjustment) - gtk_adjustment_get_page_size(vadjustment)));

  /* we need to emit "changed" ourselves */
  gtk_adjustment_changed (hadjustment);
  gtk_adjustment_changed (vadjustment);
#if GTK_CHECK_VERSION(3, 0, 0)
  g_object_notify (G_OBJECT (icon_view), "hadjustment");
  g_object_notify (G_OBJECT (icon_view), "vadjustment");
#endif
}



static void
exo_icon_view_style_set (GtkWidget *widget,
                         GtkStyle  *previous_style)
{
  ExoIconView *icon_view = EXO_ICON_VIEW (widget);
  GtkStyle *style;

  /* let GtkWidget do its work */
  (*GTK_WIDGET_CLASS (exo_icon_view_parent_class)->style_set) (widget, previous_style);

  /* apply the new style for the bin_window if we're realized */
  style = gtk_widget_get_style (widget);
  if (gtk_widget_get_realized (widget))
    gdk_window_set_background (icon_view->priv->bin_window, &style->base[gtk_widget_get_state(widget)]);
}



static gboolean
#if GTK_CHECK_VERSION(3, 0, 0)
exo_icon_view_draw (GtkWidget      *widget,
                    cairo_t        *cr)
#else
exo_icon_view_expose_event (GtkWidget      *widget,
                            GdkEventExpose *event)
#endif
{
  ExoIconViewDropPosition dest_pos;
  ExoIconViewPrivate     *priv = EXO_ICON_VIEW (widget)->priv;
  ExoIconViewItem        *dest_item = NULL;
  ExoIconViewItem        *item;
  ExoIconView            *icon_view = EXO_ICON_VIEW (widget);
  GtkTreePath            *path;
  GdkRectangle            rubber_rect;
  GdkRectangle            rect;
  const GList            *lp;
  gint                    dest_index = -1;
#if !GTK_CHECK_VERSION(3, 0, 0)
  GdkColor               *fill_color_gdk;
  guchar                  fill_color_alpha = 0;
  gboolean                rtl;
  gint                    event_area_last;
  GdkRectangle            event_area;
  cairo_t                *cr;
  GtkStyle               *style;

  /* verify that the expose happened on the icon window */
  if (G_UNLIKELY (event->window != priv->bin_window))
    return FALSE;
#else
  GtkStyleContext        *style;

  if (!gtk_cairo_should_draw_window (cr, priv->bin_window))
    return FALSE;
#endif

  /* don't handle expose if the layout isn't done yet; the layout
   * method will schedule a redraw when done.
   */
  if (G_UNLIKELY (priv->layout_idle_id != 0))
    return FALSE;

  /* scroll to the previously remembered path (if any) */
  if (G_UNLIKELY (priv->scroll_to_path != NULL))
    {
      /* grab the path from the reference and invalidate the reference */
      path = gtk_tree_row_reference_get_path (priv->scroll_to_path);
      gtk_tree_row_reference_free (priv->scroll_to_path);
      priv->scroll_to_path = NULL;

      /* check if the reference was still valid */
      if (G_LIKELY (path != NULL))
        {
          /* try to scroll again */
          exo_icon_view_scroll_to_path (icon_view, path,
                                        priv->scroll_to_use_align,
                                        priv->scroll_to_row_align,
                                        priv->scroll_to_col_align);

          /* release the path */
          gtk_tree_path_free (path);
        }
    }

  /* check if we need to draw a drag indicator */
  exo_icon_view_get_drag_dest_item (icon_view, &path, &dest_pos);
  if (G_UNLIKELY (path != NULL))
    {
      dest_index = gtk_tree_path_get_indices (path)[0];
      gtk_tree_path_free (path);
    }

#if GTK_CHECK_VERSION(3, 0, 0)
  cairo_save (cr);
  gtk_cairo_transform_to_window (cr, widget, priv->bin_window);
#else
  rtl = (gtk_widget_get_direction (GTK_WIDGET (icon_view)) == GTK_TEXT_DIR_RTL);
  event_area = event->area;

  /* determine the last interesting coordinate (depending on the layout mode) */
  event_area_last = (priv->layout_mode == EXO_ICON_VIEW_LAYOUT_ROWS)
                  ? event_area.y + event_area.height
                  : event_area.x + event_area.width;
#endif

  /* paint all items that are affected by the expose event */
  for (lp = priv->items; lp != NULL; lp = lp->next)
    {
      /* check if this item is in the visible area */
      item = EXO_ICON_VIEW_ITEM (lp->data);
#if !GTK_CHECK_VERSION(3, 0, 0)
      if (G_LIKELY (priv->layout_mode == EXO_ICON_VIEW_LAYOUT_ROWS))
        {
          if (item->area.y > event_area_last)
            break;
          else if (item->area.y + item->area.height < event_area.y)
            continue;
        }
      else if (rtl)
        {
          if (item->area.x > event_area_last)
            continue;
          else if (item->area.x + item->area.width < event_area.x)
            break;
        }
      else
        {
          if (item->area.x > event_area_last)
            break;
          else if (item->area.x + item->area.width < event_area.x)
            continue;
        }

      /* check if this item needs an update */
      if (G_LIKELY (gdk_region_rect_in (event->region, &item->area) != GDK_OVERLAP_RECTANGLE_OUT))
        {
          exo_icon_view_paint_item (icon_view, item, &event_area, event->window, item->area.x, item->area.y, TRUE);
#else
      cairo_save (cr);
      cairo_rectangle (cr, item->area.x, item->area.y, item->area.width, item->area.height);
      cairo_clip (cr);

      if (gdk_cairo_get_clip_rectangle (cr, NULL))
        {
          exo_icon_view_paint_item (icon_view, item, &item->area, cr, item->area.x, item->area.y, TRUE);
#endif
          if (G_UNLIKELY (dest_index == item->index))
            dest_item = item;
        }
#if GTK_CHECK_VERSION(3, 0, 0)
      cairo_restore (cr);
#endif
    }

  if (G_UNLIKELY (dest_item != NULL || priv->doing_rubberband))
#if GTK_CHECK_VERSION(3, 0, 0)
      style = gtk_widget_get_style_context (widget);
#else
      style = gtk_widget_get_style (widget);
#endif

  /* draw the drag indicator */
  if (G_UNLIKELY (dest_item != NULL))
    {
      switch (dest_pos)
        {
        case EXO_ICON_VIEW_DROP_INTO:
          rect = dest_item->area;
          break;
        case GTK_ICON_VIEW_DROP_ABOVE:
          rect.x = dest_item->area.x;
          rect.y = dest_item->area.y - 1;
          rect.width = dest_item->area.width;
          rect.height = 2;
          break;
        case GTK_ICON_VIEW_DROP_LEFT:
          rect.x = dest_item->area.x - 1;
          rect.y = dest_item->area.y;
          rect.width = 2;
          rect.height = dest_item->area.height;
          break;
        case GTK_ICON_VIEW_DROP_BELOW:
          rect.x = dest_item->area.x;
          rect.y = dest_item->area.y + dest_item->area.height - 1;
          rect.width = dest_item->area.width;
          rect.height = 2;
          break;
        case GTK_ICON_VIEW_DROP_RIGHT:
          rect.x = dest_item->area.x + dest_item->area.width - 1;
          rect.y = dest_item->area.y;
          rect.width = 2;
          rect.height = dest_item->area.height;
        case EXO_ICON_VIEW_NO_DROP:
          rect.x = rect.y = rect.width = rect.height = 0;
          break;

        default:
          g_assert_not_reached ();
        }
#if GTK_CHECK_VERSION(3, 0, 0)
      gtk_render_focus (style, cr,
#else
      gtk_paint_focus (style, priv->bin_window,
                       gtk_widget_get_state (widget), NULL, widget,
                       "iconview-drop-indicator",
#endif
                       rect.x, rect.y, rect.width, rect.height);
    }

  /* draw the rubberband border */
  if (G_UNLIKELY (priv->doing_rubberband))
    {
      /* calculate the rubberband area */
      rubber_rect.x = MIN (priv->rubberband_x_1, priv->rubberband_x2);
      rubber_rect.y = MIN (priv->rubberband_y_1, priv->rubberband_y2);
      rubber_rect.width = ABS (priv->rubberband_x_1 - priv->rubberband_x2) + 1;
      rubber_rect.height = ABS (priv->rubberband_y_1 - priv->rubberband_y2) + 1;

#if !GTK_CHECK_VERSION(3, 0, 0)
      if (gdk_rectangle_intersect (&rubber_rect, &event_area, &rect))
        {
          cr = gdk_cairo_create (event->window);
          gtk_widget_style_get (widget,
                                "selection-box-color", &fill_color_gdk,
                                "selection-box-alpha", &fill_color_alpha,
                                NULL);
          if (!fill_color_gdk)
            {
              style = gtk_widget_get_style (widget);
              fill_color_gdk = gdk_color_copy (&style->base[GTK_STATE_SELECTED]);
            }

          /* draw the area */
          cairo_set_source_rgba (cr,
                                 fill_color_gdk->red / 65535.,
                                 fill_color_gdk->green / 65535.,
                                 fill_color_gdk->blue / 65535.,
                                 fill_color_alpha / 255.);
          gdk_cairo_rectangle (cr, &rect);
          cairo_clip (cr);
          cairo_paint (cr);
          /* draw the border */
          cairo_set_source_rgb (cr,
                                fill_color_gdk->red / 65535.,
                                fill_color_gdk->green / 65535.,
                                fill_color_gdk->blue / 65535.);
          cairo_rectangle (cr, rubber_rect.x + 0.5, rubber_rect.y + 0.5, rubber_rect.width - 1, rubber_rect.height - 1);
          cairo_set_line_width (cr, 1);
          cairo_stroke (cr);
          gdk_color_free (fill_color_gdk);
          cairo_destroy (cr);
        }
#else
      gtk_style_context_save (style);
      gtk_style_context_add_class (style, GTK_STYLE_CLASS_RUBBERBAND);

      gdk_cairo_rectangle (cr, &rubber_rect);
      cairo_clip (cr);

      gtk_render_background (style, cr,
                             rubber_rect.x, rubber_rect.y,
                             rubber_rect.width, rubber_rect.height);
      gtk_render_frame (style, cr,
                        rubber_rect.x, rubber_rect.y,
                        rubber_rect.width, rubber_rect.height);

      gtk_style_context_restore (style);
#endif
    }

  /* let the GtkContainer forward the expose event to all children */
#if GTK_CHECK_VERSION(3, 0, 0)
  cairo_restore (cr);
  (*GTK_WIDGET_CLASS (exo_icon_view_parent_class)->draw) (widget, cr);
#else
  (*GTK_WIDGET_CLASS (exo_icon_view_parent_class)->expose_event) (widget, event);
#endif

  return FALSE;
}



static gboolean
rubberband_scroll_timeout (gpointer user_data)
{
  GtkAdjustment *adjustment;
  ExoIconView   *icon_view = EXO_ICON_VIEW (user_data);
  gdouble        value;

  /* ensure that source isn't removed yet */
  if(g_source_is_destroyed(g_main_current_source()))
    {
      return FALSE;
    }

  /* determine the adjustment for the scroll direction */
  adjustment = (icon_view->priv->layout_mode == EXO_ICON_VIEW_LAYOUT_ROWS)
             ? icon_view->priv->vadjustment
             : icon_view->priv->hadjustment;

  /* determine the new scroll value */
  value = MIN (gtk_adjustment_get_value(adjustment) + icon_view->priv->scroll_value_diff, gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment));

  /* apply the new value */
  gtk_adjustment_set_value (adjustment, value);

  /* update the rubberband */
  exo_icon_view_update_rubberband (icon_view);

  return TRUE;
}


static gboolean
exo_icon_view_motion_notify_event (GtkWidget      *widget,
                                   GdkEventMotion *event)
{
  ExoIconViewItem *item;
  ExoIconView     *icon_view = EXO_ICON_VIEW (widget);
  GdkCursor       *cursor;
  gint             size;
  gint             abso;
  GtkAllocation    allocation;

  exo_icon_view_maybe_begin_drag (icon_view, event);
  gtk_widget_get_allocation (widget, &allocation);

  if (icon_view->priv->doing_rubberband)
    {
      exo_icon_view_update_rubberband (widget);

      if (icon_view->priv->layout_mode == EXO_ICON_VIEW_LAYOUT_ROWS)
        {
          abso = event->y - icon_view->priv->height *
             (gtk_adjustment_get_value(icon_view->priv->vadjustment) /
             (gtk_adjustment_get_upper(icon_view->priv->vadjustment) -
              gtk_adjustment_get_lower(icon_view->priv->vadjustment)));

          size = allocation.height;
        }
      else
        {
          abso = event->x - icon_view->priv->width *
             (gtk_adjustment_get_value(icon_view->priv->hadjustment) /
             (gtk_adjustment_get_upper(icon_view->priv->hadjustment) -
              gtk_adjustment_get_lower(icon_view->priv->hadjustment)));

          size = allocation.width;
        }

      if (abso < 0 || abso > size)
        {
          if (abso < 0)
            icon_view->priv->scroll_value_diff = abso;
          else
            icon_view->priv->scroll_value_diff = abso - size;
          icon_view->priv->event_last_x = event->x;
          icon_view->priv->event_last_y = event->y;

          if (icon_view->priv->scroll_timeout_id == 0)
            icon_view->priv->scroll_timeout_id = gdk_threads_add_timeout (30, rubberband_scroll_timeout,
                                                                icon_view);
        }
      else
        {
          remove_scroll_timeout (icon_view);
        }
    }
  else
    {
      item = exo_icon_view_get_item_at_coords (icon_view, event->x, event->y, TRUE, NULL);
      if (item != icon_view->priv->prelit_item)
        {
          if (G_LIKELY (icon_view->priv->prelit_item != NULL))
            exo_icon_view_queue_draw_item (icon_view, icon_view->priv->prelit_item);
          icon_view->priv->prelit_item = item;
          if (G_LIKELY (item != NULL))
            exo_icon_view_queue_draw_item (icon_view, item);

          /* check if we are in single click mode right now */
          if (G_UNLIKELY (icon_view->priv->single_click))
            {
              /* display a hand cursor when pointer is above an item */
              if (G_LIKELY (item != NULL))
                {
                  /* hand2 seems to be what we should use */
                  cursor = gdk_cursor_new (GDK_HAND2);
                  gdk_window_set_cursor (event->window, cursor);
                  gdk_cursor_unref (cursor);
                }
              else
                {
                  /* reset the cursor */
                  gdk_window_set_cursor (event->window, NULL);
                }

              /* check if autoselection is enabled */
              if (G_LIKELY (icon_view->priv->single_click_timeout > 0))
                {
                  /* drop any running timeout */
                  if (G_LIKELY (icon_view->priv->single_click_timeout_id != 0))
                    g_source_remove (icon_view->priv->single_click_timeout_id);

                  /* remember the current event state */
                  icon_view->priv->single_click_timeout_state = event->state;

                  /* schedule a new timeout */
                  icon_view->priv->single_click_timeout_id = gdk_threads_add_timeout_full (G_PRIORITY_LOW, icon_view->priv->single_click_timeout,
                                                                                 exo_icon_view_single_click_timeout, icon_view,
                                                                                 exo_icon_view_single_click_timeout_destroy);
                }
            }
        }
    }

  return TRUE;
}



static void
exo_icon_view_remove (GtkContainer *container,
                      GtkWidget    *widget)
{
  ExoIconViewChild *child;
  ExoIconView      *icon_view = EXO_ICON_VIEW (container);
  GList            *lp;

  for (lp = icon_view->priv->children; lp != NULL; lp = lp->next)
    {
      child = lp->data;
      if (G_LIKELY (child->widget == widget))
        {
          icon_view->priv->children = g_list_delete_link (icon_view->priv->children, lp);
          gtk_widget_unparent (widget);
          g_slice_free (ExoIconViewChild, child);
          return;
        }
    }
}



static void
exo_icon_view_forall (GtkContainer *container,
                      gboolean      include_internals,
                      GtkCallback   callback,
                      gpointer      callback_data)
{
  ExoIconView *icon_view = EXO_ICON_VIEW (container);
  GList       *lp;

  for (lp = icon_view->priv->children; lp != NULL; lp = lp->next)
    (*callback) (((ExoIconViewChild *) lp->data)->widget, callback_data);
}

static void
exo_icon_view_item_selected_changed (ExoIconView      *icon_view,
                                     ExoIconViewItem  *item)
{
  AtkObject *obj;
  AtkObject *item_obj;

  obj = gtk_widget_get_accessible (GTK_WIDGET (icon_view));
  if (obj != NULL)
    {
      item_obj = atk_object_ref_accessible_child (obj, item->index);
      if (item_obj != NULL)
        {
          atk_object_notify_state_change (item_obj, ATK_STATE_SELECTED, item->selected);
          g_object_unref (item_obj);
        }
    }
}


static void
exo_icon_view_item_activate_cell (ExoIconView         *icon_view,
                                  ExoIconViewItem     *item,
                                  ExoIconViewCellInfo *info,
                                  GdkEvent            *event)
{
  GtkCellRendererMode mode;
  GdkRectangle        cell_area;
  GtkTreePath        *path;
  gboolean            visible;
  gchar              *path_string;

  exo_icon_view_set_cell_data (icon_view, item);

  g_object_get (G_OBJECT (info->cell), "visible", &visible, "mode", &mode, NULL);

  if (G_UNLIKELY (visible && mode == GTK_CELL_RENDERER_MODE_ACTIVATABLE))
    {
      exo_icon_view_get_cell_area (icon_view, item, info, &cell_area);

      path = gtk_tree_path_new_from_indices (item->index, -1);
      path_string = gtk_tree_path_to_string (path);
      gtk_tree_path_free (path);

      gtk_cell_renderer_activate (info->cell, event, GTK_WIDGET (icon_view), path_string, &cell_area, &cell_area, 0);

      g_free (path_string);
    }
}



static void
exo_icon_view_put (ExoIconView     *icon_view,
                   GtkWidget       *widget,
                   ExoIconViewItem *item,
                   gint             cell)
{
  ExoIconViewChild *child;

  /* allocate the new child */
  child = g_slice_new (ExoIconViewChild);
  child->widget = widget;
  child->item = item;
  child->cell = cell;

  /* hook up the child */
  icon_view->priv->children = g_list_append (icon_view->priv->children, child);

  /* setup the parent for the child */
  if (gtk_widget_get_realized (GTK_WIDGET (icon_view)))
    gtk_widget_set_parent_window (child->widget, icon_view->priv->bin_window);
  gtk_widget_set_parent (widget, GTK_WIDGET (icon_view));
}



static void
exo_icon_view_remove_widget (GtkCellEditable *editable,
                             ExoIconView     *icon_view)
{
  ExoIconViewItem *item;
  GList           *lp;

  if (G_LIKELY (icon_view->priv->edited_item != NULL))
    {
      item = icon_view->priv->edited_item;
      icon_view->priv->edited_item = NULL;
      icon_view->priv->editable = NULL;

      for (lp = icon_view->priv->cell_list; lp != NULL; lp = lp->next)
        ((ExoIconViewCellInfo *) lp->data)->editing = FALSE;

      if (gtk_widget_has_focus (GTK_WIDGET (editable)))
        gtk_widget_grab_focus (GTK_WIDGET (icon_view));

      g_signal_handlers_disconnect_by_func (editable, exo_icon_view_remove_widget, icon_view);
      gtk_container_remove (GTK_CONTAINER (icon_view), GTK_WIDGET (editable));

      exo_icon_view_queue_draw_item (icon_view, item);
    }
}



static void
exo_icon_view_start_editing (ExoIconView         *icon_view,
                             ExoIconViewItem     *item,
                             ExoIconViewCellInfo *info,
                             GdkEvent            *event)
{
  GtkCellRendererMode mode;
  GtkCellEditable    *editable;
  GdkRectangle        cell_area;
  GtkTreePath        *path;
  gboolean            visible;
  gchar              *path_string;

  /* setup cell data for the given item */
  exo_icon_view_set_cell_data (icon_view, item);

  /* check if the cell is visible and editable (given the updated cell data) */
  g_object_get (info->cell, "visible", &visible, "mode", &mode, NULL);
  if (G_LIKELY (visible && mode == GTK_CELL_RENDERER_MODE_EDITABLE))
    {
      /* draw keyboard focus while editing */
      EXO_ICON_VIEW_SET_FLAG (icon_view, EXO_ICON_VIEW_DRAW_KEYFOCUS);

      /* determine the cell area */
      exo_icon_view_get_cell_area (icon_view, item, info, &cell_area);

      /* determine the tree path */
      path = gtk_tree_path_new_from_indices (item->index, -1);
      path_string = gtk_tree_path_to_string (path);
      gtk_tree_path_free (path);

      /* allocate the editable from the cell renderer */
      editable = gtk_cell_renderer_start_editing (info->cell, event, GTK_WIDGET (icon_view), path_string, &cell_area, &cell_area, 0);

      /* ugly hack, but works */
      if (g_object_class_find_property (G_OBJECT_GET_CLASS (editable), "has-frame") != NULL)
        g_object_set (editable, "has-frame", TRUE, NULL);

      /* setup the editing widget */
      icon_view->priv->edited_item = item;
      icon_view->priv->editable = editable;
      info->editing = TRUE;

      exo_icon_view_put (icon_view, GTK_WIDGET (editable), item, info->position);
      gtk_cell_editable_start_editing (GTK_CELL_EDITABLE (editable), (GdkEvent *)event);
      gtk_widget_grab_focus (GTK_WIDGET (editable));
      g_signal_connect (G_OBJECT (editable), "remove-widget", G_CALLBACK (exo_icon_view_remove_widget), icon_view);

      /* cleanup */
      g_free (path_string);
    }
}



static void
exo_icon_view_stop_editing (ExoIconView *icon_view,
                            gboolean     cancel_editing)
{
  ExoIconViewItem *item;
  GtkCellRenderer *cell = NULL;
  GList           *lp;

  if (icon_view->priv->edited_item == NULL)
    return;

  /*
   * This is very evil. We need to do this, because
   * gtk_cell_editable_editing_done may trigger exo_icon_view_row_changed
   * later on. If exo_icon_view_row_changed notices
   * icon_view->priv->edited_item != NULL, it'll call
   * exo_icon_view_stop_editing again. Bad things will happen then.
   *
   * Please read that again if you intend to modify anything here.
   */

  item = icon_view->priv->edited_item;
  icon_view->priv->edited_item = NULL;

  for (lp = icon_view->priv->cell_list; lp != NULL; lp = lp->next)
    {
      ExoIconViewCellInfo *info = lp->data;
      if (info->editing)
        {
          cell = info->cell;
          break;
        }
    }

  if (G_UNLIKELY (cell == NULL))
    return;

  gtk_cell_renderer_stop_editing (cell, cancel_editing);
  if (G_LIKELY (!cancel_editing))
    gtk_cell_editable_editing_done (icon_view->priv->editable);

  icon_view->priv->edited_item = item;

  gtk_cell_editable_remove_widget (icon_view->priv->editable);
}



static gboolean
exo_icon_view_button_press_event (GtkWidget      *widget,
                                  GdkEventButton *event)
{
  ExoIconViewCellInfo *info = NULL;
  GtkCellRendererMode  mode;
  ExoIconViewItem     *item;
  ExoIconView         *icon_view;
  GtkTreePath         *path;
  gboolean             dirty = FALSE;
  gint                 cursor_cell;
  gpointer             drag_data;

  icon_view = EXO_ICON_VIEW (widget);

  if (event->window != icon_view->priv->bin_window)
    return FALSE;

  /* the widget can be destroyed by click so let keep reference on it */
  g_object_ref(widget);

  /* stop any pending "single-click-timeout" */
  if (G_UNLIKELY (icon_view->priv->single_click_timeout_id != 0))
    g_source_remove (icon_view->priv->single_click_timeout_id);

  if (G_UNLIKELY (!gtk_widget_has_focus (widget)))
    gtk_widget_grab_focus (widget);

  if (event->button == 1 && event->type == GDK_BUTTON_PRESS)
    {
      if (G_LIKELY (icon_view->priv->dnd_locked))
        {
          /* re-enable Gtk+ DnD callbacks if they were disabled by a
             double click before, otherwise DnD will not work */
          drag_data = g_object_get_data (G_OBJECT (icon_view), I_("gtk-site-data"));
          if (G_LIKELY (drag_data != NULL))
            {
              g_signal_handlers_unblock_matched (G_OBJECT (icon_view),
                                                 G_SIGNAL_MATCH_DATA,
                                                 0, 0, NULL, NULL,
                                                 drag_data);
            }
          icon_view->priv->dnd_locked = FALSE;
        }
      item = exo_icon_view_get_item_at_coords (icon_view,
                                               event->x, event->y,
                                               TRUE,
                                               &info);
      if (item != NULL)
        {
          g_object_get (info->cell, "mode", &mode, NULL);

          if (mode == GTK_CELL_RENDERER_MODE_ACTIVATABLE ||
              mode == GTK_CELL_RENDERER_MODE_EDITABLE)
            cursor_cell = g_list_index (icon_view->priv->cell_list, info);
          else
            cursor_cell = -1;

          exo_icon_view_scroll_to_item (icon_view, item);

          if (icon_view->priv->selection_mode == GTK_SELECTION_NONE)
            {
              exo_icon_view_set_cursor_item (icon_view, item, cursor_cell);
            }
          else if (icon_view->priv->selection_mode == GTK_SELECTION_MULTIPLE &&
                   (event->state & GDK_SHIFT_MASK))
            {
              if (!(event->state & GDK_CONTROL_MASK))
                exo_icon_view_unselect_all_internal (icon_view);

              exo_icon_view_set_cursor_item (icon_view, item, cursor_cell);
              if (!icon_view->priv->anchor_item)
                icon_view->priv->anchor_item = item;
              else
                exo_icon_view_select_all_between (icon_view,
                                                  icon_view->priv->anchor_item,
                                                  item);
              dirty = TRUE;
            }
          else
            {
              if ((icon_view->priv->selection_mode == GTK_SELECTION_MULTIPLE ||
                  ((icon_view->priv->selection_mode == GTK_SELECTION_SINGLE) && item->selected)) &&
                  (event->state & GDK_CONTROL_MASK))
                {
                  item->selected = !item->selected;
                  exo_icon_view_queue_draw_item (icon_view, item);
                  dirty = TRUE;
                }
              else
                {
                  if (!item->selected)
                    {
                      exo_icon_view_unselect_all_internal (icon_view);

                      item->selected = TRUE;
                      exo_icon_view_queue_draw_item (icon_view, item);
                      dirty = TRUE;
                    }
                }
              exo_icon_view_set_cursor_item (icon_view, item, cursor_cell);
              icon_view->priv->anchor_item = item;
            }

          /* Save press to possibly begin a drag */
          if (icon_view->priv->pressed_button < 0)
            {
              icon_view->priv->pressed_button = event->button;
              icon_view->priv->press_start_x = event->x;
              icon_view->priv->press_start_y = event->y;
            }

          if (G_LIKELY (icon_view->priv->last_single_clicked == NULL))
            icon_view->priv->last_single_clicked = item;

          /* cancel the current editing, if it exists */
          exo_icon_view_stop_editing (icon_view, TRUE);

          if (mode == GTK_CELL_RENDERER_MODE_ACTIVATABLE)
            exo_icon_view_item_activate_cell (icon_view, item, info,
                                              (GdkEvent *)event);
          else if (mode == GTK_CELL_RENDERER_MODE_EDITABLE)
            exo_icon_view_start_editing (icon_view, item, info,
                                         (GdkEvent *)event);
        }
      else
        {
          /* cancel the current editing, if it exists */
          exo_icon_view_stop_editing (icon_view, TRUE);

          if (icon_view->priv->selection_mode != GTK_SELECTION_BROWSE &&
              !(event->state & GDK_CONTROL_MASK))
            {
              dirty = exo_icon_view_unselect_all_internal (icon_view);
            }

          if (icon_view->priv->selection_mode == GTK_SELECTION_MULTIPLE)
            exo_icon_view_start_rubberbanding (icon_view, event->x, event->y);
        }
    }
  else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS)
    {
      /* ignore double-click events in single-click mode */
      if (G_LIKELY (!icon_view->priv->single_click))
        {
          item = exo_icon_view_get_item_at_coords (icon_view,
                                                   event->x, event->y,
                                                   TRUE,
                                                   NULL);
          if (G_LIKELY (item != NULL))
            {
              path = gtk_tree_path_new_from_indices (item->index, -1);
              exo_icon_view_item_activated (icon_view, path);
              gtk_tree_path_free (path);
              /* bug #3615031: don't start DnD by double click */
              if (icon_view->priv->selection_mode == GTK_SELECTION_MULTIPLE &&
                  gtk_widget_get_realized(widget))
                {
                  drag_data = g_object_get_data (G_OBJECT (icon_view), I_("gtk-site-data"));
                  if (G_LIKELY (drag_data != NULL))
                    {
                      g_signal_handlers_block_matched (G_OBJECT (icon_view),
                                                       G_SIGNAL_MATCH_DATA,
                                                       0, 0, NULL, NULL,
                                                       drag_data);
                      icon_view->priv->dnd_locked = TRUE;
                    }

                }
            }
        }

      icon_view->priv->last_single_clicked = NULL;
      icon_view->priv->pressed_button = -1;
    }

  /* grab focus and stop drawing the keyboard focus indicator on single clicks */
  if (G_LIKELY (event->type != GDK_2BUTTON_PRESS && event->type != GDK_3BUTTON_PRESS))
    {
      if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
        gtk_widget_grab_focus (GTK_WIDGET (icon_view));
      EXO_ICON_VIEW_UNSET_FLAG (icon_view, EXO_ICON_VIEW_DRAW_KEYFOCUS);
    }

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  /* SF bug #929: we have to drop prelit state to drop tooltip, see text renderer */
  icon_view->priv->prelit_item = NULL;

  /* release reference that was taken above */
  g_object_unref(widget);

  return event->button == 1;
}



static gboolean
exo_icon_view_button_release_event (GtkWidget      *widget,
                                    GdkEventButton *event)
{
  ExoIconViewItem *item;
  ExoIconView     *icon_view = EXO_ICON_VIEW (widget);
  GtkTreePath     *path;

  /* the widget can be destroyed by click so let keep reference on it */
  g_object_ref(widget);

  if (icon_view->priv->pressed_button == (gint) event->button)
    {
      /* check if we're in single click mode */
      if (G_UNLIKELY (icon_view->priv->single_click && (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == 0))
        {
          /* determine the item at the mouse coords and check if this is the last single clicked one */
          item = exo_icon_view_get_item_at_coords (icon_view, event->x, event->y, TRUE, NULL);
          if (G_LIKELY (item != NULL && item == icon_view->priv->last_single_clicked))
            {
              /* emit an "item-activated" signal for this item */
              path = gtk_tree_path_new_from_indices (item->index, -1);
              exo_icon_view_item_activated (icon_view, path);
              gtk_tree_path_free (path);
            }

          /* reset the last single clicked item */
          icon_view->priv->last_single_clicked = NULL;
        }

      /* reset the pressed_button state */
      icon_view->priv->pressed_button = -1;
    }

  exo_icon_view_stop_rubberbanding (icon_view);

  remove_scroll_timeout (icon_view);

  /* release reference that was taken above */
  g_object_unref(widget);

  return TRUE;
}



static gboolean
exo_icon_view_scroll_event (GtkWidget      *widget,
                            GdkEventScroll *event)
{
  GtkAdjustment *adjustment;
  ExoIconView   *icon_view = EXO_ICON_VIEW (widget);
  gdouble        delta;
  gdouble        value;

  /* we don't care for scroll events in "rows" layout mode, as
   * that's completely handled by GtkScrolledWindow.
   */
  if (icon_view->priv->layout_mode != EXO_ICON_VIEW_LAYOUT_COLS)
    return FALSE;

  /* also, we don't care for anything but Up/Down, as
   * everything else will be handled by GtkScrolledWindow.
   */
  if (event->direction != GDK_SCROLL_UP && event->direction != GDK_SCROLL_DOWN)
    return FALSE;

  /* we also don't care for scroll events with Shift/Ctrl/Alt pressed */
  if ((event->state & gtk_accelerator_get_default_mod_mask()) != 0)
    return FALSE;

  /* determine the horizontal adjustment */
  adjustment = icon_view->priv->hadjustment;

  /* determine the scroll delta */
  delta = pow (gtk_adjustment_get_page_size(adjustment), 2.0 / 3.0);
  delta = (event->direction == GDK_SCROLL_UP) ? -delta : delta;

  /* apply the new adjustment value */
  value = CLAMP (gtk_adjustment_get_value(adjustment) + delta, gtk_adjustment_get_lower(adjustment), gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment));
  gtk_adjustment_set_value (adjustment, value);

  return TRUE;
}



static gboolean
exo_icon_view_key_press_event (GtkWidget   *widget,
                               GdkEventKey *event)
{
  ExoIconView *icon_view = EXO_ICON_VIEW (widget);
  GdkScreen   *screen;
  GdkEvent    *new_event;
  gboolean     retval;
  gulong       popup_menu_id;
  gchar       *new_text;
  gchar       *old_text;

  /* let the parent class handle the key bindings and stuff */
  if ((*GTK_WIDGET_CLASS (exo_icon_view_parent_class)->key_press_event) (widget, event))
    return TRUE;

  /* 'space' keypress should not start search even if there is no selection */
  if (G_UNLIKELY (event->keyval == GDK_KEY_space))
    return FALSE;

  /* check if typeahead search is enabled */
  if (G_UNLIKELY (!icon_view->priv->enable_search))
    return FALSE;

  exo_icon_view_search_ensure_directory (icon_view);

  /* make sure the search window is realized */
  gtk_widget_realize (icon_view->priv->search_window);

  /* make a copy of the current text */
  old_text = gtk_editable_get_chars (GTK_EDITABLE (icon_view->priv->search_entry), 0, -1);

  /* make sure we don't accidently popup the context menu */
  popup_menu_id = g_signal_connect (G_OBJECT (icon_view->priv->search_entry), "popup-menu", G_CALLBACK (gtk_true), NULL);

  /* move the search window offscreen */
  screen = gtk_widget_get_screen (GTK_WIDGET (icon_view));
  gtk_window_move (GTK_WINDOW (icon_view->priv->search_window),
                   gdk_screen_get_width (screen) + 1,
                   gdk_screen_get_height (screen) + 1);
  gtk_widget_show (icon_view->priv->search_window);

  /* allocate a new event to forward */
  new_event = gdk_event_copy ((GdkEvent *) event);
  g_object_unref (G_OBJECT (new_event->key.window));
  new_event->key.window = g_object_ref (G_OBJECT (gtk_widget_get_window (icon_view->priv->search_entry)));

  /* send the event to the search entry. If the "preedit-changed" signal is
   * emitted during this event, priv->search_imcontext_changed will be set.
   */
  icon_view->priv->search_imcontext_changed = FALSE;
  retval = gtk_widget_event (icon_view->priv->search_entry, new_event);
  gtk_widget_hide (icon_view->priv->search_window);

  /* release the temporary event */
  gdk_event_free (new_event);

  /* disconnect the popup menu prevention */
  g_signal_handler_disconnect (G_OBJECT (icon_view->priv->search_entry), popup_menu_id);

  /* we check to make sure that the entry tried to handle the,
   * and that the text has actually changed.
   */
  new_text = gtk_editable_get_chars (GTK_EDITABLE (icon_view->priv->search_entry), 0, -1);
  retval = retval && (strcmp (new_text, old_text) != 0);
  g_free (old_text);
  g_free (new_text);

  /* if we're in a preedit or the text was modified */
  if (icon_view->priv->search_imcontext_changed || retval)
    {
      if (exo_icon_view_search_start (icon_view, FALSE))
        {
          gtk_widget_grab_focus (GTK_WIDGET (icon_view));
          return TRUE;
        }
      else
        {
          gtk_entry_set_text (GTK_ENTRY (icon_view->priv->search_entry), "");
          return FALSE;
        }
    }

  return FALSE;
}



static gboolean
exo_icon_view_focus_out_event (GtkWidget     *widget,
                               GdkEventFocus *event)
{
  ExoIconView *icon_view = EXO_ICON_VIEW (widget);

  /* be sure to cancel any single-click timeout */
  if (G_UNLIKELY (icon_view->priv->single_click_timeout_id != 0))
    g_source_remove (icon_view->priv->single_click_timeout_id);

  /* reset the cursor if we're still realized */
  if (G_LIKELY (icon_view->priv->bin_window != NULL))
    gdk_window_set_cursor (icon_view->priv->bin_window, NULL);

  /* destroy the interactive search dialog */
  if (G_UNLIKELY (icon_view->priv->search_window != NULL))
    exo_icon_view_search_dialog_hide (icon_view->priv->search_window, icon_view);

  /* schedule a redraw with the new focus state */
  gtk_widget_queue_draw (widget);

  return FALSE;
}



static gboolean
exo_icon_view_leave_notify_event (GtkWidget        *widget,
                                  GdkEventCrossing *event)
{
  /* reset cursor to default */
  if (gtk_widget_get_realized (widget))
    gdk_window_set_cursor (gtk_widget_get_window(widget), NULL);

  /* call the parent's leave_notify_event (if any) */
  if (GTK_WIDGET_CLASS (exo_icon_view_parent_class)->leave_notify_event != NULL)
    return (*GTK_WIDGET_CLASS (exo_icon_view_parent_class)->leave_notify_event) (widget, event);

  /* other signal handlers may be invoked */
  return FALSE;
}


static void
exo_icon_view_update_rubberband (gpointer data)
{
  ExoIconView *icon_view;
  gint x, y;
  GdkRectangle old_area;
  GdkRectangle new_area;
  GdkRectangle common;
#if GTK_CHECK_VERSION(3, 0, 0)
  cairo_region_t *invalid_region;
#else
  GdkRegion *invalid_region;
#endif

  icon_view = EXO_ICON_VIEW (data);

  gdk_window_get_device_position (icon_view->priv->bin_window,
                                  gtk_get_current_event_device(), &x, &y, NULL);

  x = MAX (x, 0);
  y = MAX (y, 0);

  old_area.x = MIN (icon_view->priv->rubberband_x_1,
                    icon_view->priv->rubberband_x2);
  old_area.y = MIN (icon_view->priv->rubberband_y_1,
                    icon_view->priv->rubberband_y2);
  old_area.width = ABS (icon_view->priv->rubberband_x2 -
                        icon_view->priv->rubberband_x_1) + 1;
  old_area.height = ABS (icon_view->priv->rubberband_y2 -
                         icon_view->priv->rubberband_y_1) + 1;

  new_area.x = MIN (icon_view->priv->rubberband_x_1, x);
  new_area.y = MIN (icon_view->priv->rubberband_y_1, y);
  new_area.width = ABS (x - icon_view->priv->rubberband_x_1) + 1;
  new_area.height = ABS (y - icon_view->priv->rubberband_y_1) + 1;

#if GTK_CHECK_VERSION(3, 0, 0)
  invalid_region = cairo_region_create_rectangle (&old_area);
  cairo_region_union_rectangle (invalid_region, &new_area);
#else
  invalid_region = gdk_region_rectangle (&old_area);
  gdk_region_union_with_rect (invalid_region, &new_area);
#endif

  gdk_rectangle_intersect (&old_area, &new_area, &common);
  if (common.width > 2 && common.height > 2)
    {
#if GTK_CHECK_VERSION(3, 0, 0)
      cairo_region_t *common_region;
#else
      GdkRegion *common_region;
#endif

       /* make sure the border is invalidated */
      common.x += 1;
      common.y += 1;
      common.width -= 2;
      common.height -= 2;

#if GTK_CHECK_VERSION(3, 0, 0)
      common_region = cairo_region_create_rectangle (&common);

      cairo_region_subtract (invalid_region, common_region);
      cairo_region_destroy (common_region);
#else
      common_region = gdk_region_rectangle (&common);

      gdk_region_subtract (invalid_region, common_region);
      gdk_region_destroy (common_region);
#endif
    }

  gdk_window_invalidate_region (icon_view->priv->bin_window, invalid_region, TRUE);

#if GTK_CHECK_VERSION(3, 0, 0)
  cairo_region_destroy (invalid_region);
#else
  gdk_region_destroy (invalid_region);
#endif

  icon_view->priv->rubberband_x2 = x;
  icon_view->priv->rubberband_y2 = y;

  exo_icon_view_update_rubberband_selection (icon_view);
}


static void
exo_icon_view_start_rubberbanding (ExoIconView  *icon_view,
                                   gint          x,
                                   gint          y)
{
  gpointer        drag_data;
  GList          *items;

  /* be sure to disable any previously active rubberband */
  exo_icon_view_stop_rubberbanding (icon_view);

  for (items = icon_view->priv->items; items; items = items->next)
    {
      ExoIconViewItem *item = items->data;
      item->selected_before_rubberbanding = item->selected;
    }

  icon_view->priv->rubberband_x_1 = x;
  icon_view->priv->rubberband_y_1 = y;
  icon_view->priv->rubberband_x2 = x;
  icon_view->priv->rubberband_y2 = y;

  icon_view->priv->doing_rubberband = TRUE;

  gtk_grab_add (GTK_WIDGET (icon_view));

  /* be sure to disable Gtk+ DnD callbacks, because else rubberbanding will be interrupted */
  drag_data = g_object_get_data (G_OBJECT (icon_view), I_("gtk-site-data"));
  if (G_LIKELY (drag_data != NULL))
    {
      g_signal_handlers_block_matched (G_OBJECT (icon_view),
                                       G_SIGNAL_MATCH_DATA,
                                       0, 0, NULL, NULL,
                                       drag_data);
      icon_view->priv->dnd_locked = TRUE;
    }
}



static void
exo_icon_view_stop_rubberbanding (ExoIconView *icon_view)
{
  gpointer drag_data;

  if (G_LIKELY (icon_view->priv->doing_rubberband))
    {
      icon_view->priv->doing_rubberband = FALSE;
      gtk_grab_remove (GTK_WIDGET (icon_view));
      gtk_widget_queue_draw (GTK_WIDGET (icon_view));
    }

  if (G_LIKELY (icon_view->priv->dnd_locked))
    {
      /* re-enable Gtk+ DnD callbacks again */
      drag_data = g_object_get_data (G_OBJECT (icon_view), I_("gtk-site-data"));
      if (G_LIKELY (drag_data != NULL))
        {
          g_signal_handlers_unblock_matched (G_OBJECT (icon_view),
                                             G_SIGNAL_MATCH_DATA,
                                             0, 0, NULL, NULL,
                                             drag_data);
        }
      icon_view->priv->dnd_locked = FALSE;
    }
}



static void
exo_icon_view_update_rubberband_selection (ExoIconView *icon_view)
{
  ExoIconViewItem *item;
  gboolean         selected;
  gboolean         changed = FALSE;
  gboolean         is_in;
  GList           *lp;
  gint             x, y;
  gint             width;
  gint             height;

  /* determine the new rubberband area */
  x = MIN (icon_view->priv->rubberband_x_1, icon_view->priv->rubberband_x2);
  y = MIN (icon_view->priv->rubberband_y_1, icon_view->priv->rubberband_y2);
  width = ABS (icon_view->priv->rubberband_x_1 - icon_view->priv->rubberband_x2);
  height = ABS (icon_view->priv->rubberband_y_1 - icon_view->priv->rubberband_y2);

  /* check all items */
  for (lp = icon_view->priv->items; lp != NULL; lp = lp->next)
    {
      item = EXO_ICON_VIEW_ITEM (lp->data);

      is_in = exo_icon_view_item_hit_test (icon_view, item, x, y, width, height);

      selected = is_in ^ item->selected_before_rubberbanding;

      if (G_UNLIKELY (item->selected != selected))
        {
          changed = TRUE;
          item->selected = selected;
          exo_icon_view_queue_draw_item (icon_view, item);
        }
    }

  if (G_LIKELY (changed))
    g_signal_emit (G_OBJECT (icon_view), icon_view_signals[SELECTION_CHANGED], 0);
}



static gboolean
exo_icon_view_item_hit_test (ExoIconView      *icon_view,
                             ExoIconViewItem  *item,
                             gint              x,
                             gint              y,
                             gint              width,
                             gint              height)
{
  GList *l;
  GdkRectangle box;

  for (l = icon_view->priv->cell_list; l; l = l->next)
    {
      ExoIconViewCellInfo *info = (ExoIconViewCellInfo *)l->data;

      if (!gtk_cell_renderer_get_visible(info->cell))
        continue;

      /* libfm: bug #3390778: item->box isn't allocated yet here! bad design! */
      if (!item->box)
        continue;

      box = item->box[info->position];

      if (MIN (x + width, box.x + box.width) - MAX (x, box.x) > 0 &&
        MIN (y + height, box.y + box.height) - MAX (y, box.y) > 0)
        return TRUE;
    }

  return FALSE;
}



static gboolean
exo_icon_view_unselect_all_internal (ExoIconView  *icon_view)
{
  ExoIconViewItem *item;
  gboolean         dirty = FALSE;
  GList           *lp;

  if (G_LIKELY (icon_view->priv->selection_mode != GTK_SELECTION_NONE))
    {
      for (lp = icon_view->priv->items; lp != NULL; lp = lp->next)
        {
          item = EXO_ICON_VIEW_ITEM (lp->data);
          if (item->selected)
            {
              dirty = TRUE;
              item->selected = FALSE;
              exo_icon_view_queue_draw_item (icon_view, item);
              exo_icon_view_item_selected_changed (icon_view, item);
            }
        }
    }

  return dirty;
}


#if !GTK_CHECK_VERSION(3, 0, 0)
static void
exo_icon_view_set_adjustments (ExoIconView   *icon_view,
                               GtkAdjustment *hadj,
                               GtkAdjustment *vadj)
{
  gboolean need_adjust = FALSE;

  if (hadj)
    _exo_return_if_fail (GTK_IS_ADJUSTMENT (hadj));
  else
    hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  if (vadj)
    _exo_return_if_fail (GTK_IS_ADJUSTMENT (vadj));
  else
    vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (icon_view->priv->hadjustment && (icon_view->priv->hadjustment != hadj))
    {
      g_signal_handlers_disconnect_matched (icon_view->priv->hadjustment, G_SIGNAL_MATCH_DATA,
                                           0, 0, NULL, NULL, icon_view);
      g_object_unref (icon_view->priv->hadjustment);
    }

  if (icon_view->priv->vadjustment && (icon_view->priv->vadjustment != vadj))
    {
      g_signal_handlers_disconnect_matched (icon_view->priv->vadjustment, G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, icon_view);
      g_object_unref (icon_view->priv->vadjustment);
    }

  if (icon_view->priv->hadjustment != hadj)
    {
      icon_view->priv->hadjustment = hadj;
      g_object_ref_sink (icon_view->priv->hadjustment);

      g_signal_connect (icon_view->priv->hadjustment, "value-changed",
                        G_CALLBACK (exo_icon_view_adjustment_changed),
                        icon_view);
      need_adjust = TRUE;
    }

  if (icon_view->priv->vadjustment != vadj)
    {
      icon_view->priv->vadjustment = vadj;
      g_object_ref_sink (icon_view->priv->vadjustment);

      g_signal_connect (icon_view->priv->vadjustment, "value-changed",
                        G_CALLBACK (exo_icon_view_adjustment_changed),
                        icon_view);
      need_adjust = TRUE;
    }

  if (need_adjust)
    exo_icon_view_adjustment_changed (NULL, icon_view);
}
#else
static void
exo_icon_view_set_hadjustment (ExoIconView *icon_view,
                               GtkAdjustment *hadj)
{
  gboolean need_adjust = FALSE;

  if (hadj)
    _exo_return_if_fail (GTK_IS_ADJUSTMENT (hadj));
  else
    hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (icon_view->priv->hadjustment && (icon_view->priv->hadjustment != hadj))
    {
      g_signal_handlers_disconnect_matched (icon_view->priv->hadjustment, G_SIGNAL_MATCH_DATA,
                                           0, 0, NULL, NULL, icon_view);
      g_object_unref (icon_view->priv->hadjustment);
    }

  if (icon_view->priv->hadjustment != hadj)
    {
      icon_view->priv->hadjustment = hadj;
      g_object_ref_sink (icon_view->priv->hadjustment);

      g_signal_connect (icon_view->priv->hadjustment, "value-changed",
                        G_CALLBACK (exo_icon_view_adjustment_changed),
                        icon_view);
      need_adjust = TRUE;
    }

  if (need_adjust)
    exo_icon_view_adjustment_changed (NULL, icon_view);
  g_object_notify (G_OBJECT (icon_view), "hadjustment");
}


static void
exo_icon_view_set_vadjustment (ExoIconView *icon_view,
                               GtkAdjustment *vadj)
{
  gboolean need_adjust = FALSE;

  if (vadj)
    _exo_return_if_fail (GTK_IS_ADJUSTMENT (vadj));
  else
    vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (icon_view->priv->vadjustment && (icon_view->priv->vadjustment != vadj))
    {
      g_signal_handlers_disconnect_matched (icon_view->priv->vadjustment, G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, icon_view);
      g_object_unref (icon_view->priv->vadjustment);
    }

  if (icon_view->priv->vadjustment != vadj)
    {
      icon_view->priv->vadjustment = vadj;
      g_object_ref_sink (icon_view->priv->vadjustment);

      g_signal_connect (icon_view->priv->vadjustment, "value-changed",
                        G_CALLBACK (exo_icon_view_adjustment_changed),
                        icon_view);
      need_adjust = TRUE;
    }

  if (need_adjust)
    exo_icon_view_adjustment_changed (NULL, icon_view);
  g_object_notify (G_OBJECT (icon_view), "vadjustment");
}
#endif


static void
exo_icon_view_real_select_all (ExoIconView *icon_view)
{
  exo_icon_view_select_all (icon_view);
}



static void
exo_icon_view_real_unselect_all (ExoIconView *icon_view)
{
  exo_icon_view_unselect_all (icon_view);
}



static void
exo_icon_view_real_select_cursor_item (ExoIconView *icon_view)
{
  exo_icon_view_unselect_all (icon_view);

  if (icon_view->priv->cursor_item != NULL)
    exo_icon_view_select_item (icon_view, icon_view->priv->cursor_item);
}



static gboolean
exo_icon_view_real_activate_cursor_item (ExoIconView *icon_view)
{
  GtkTreePath *path;
  GtkCellRendererMode mode;
  ExoIconViewCellInfo *info = NULL;

  if (!icon_view->priv->cursor_item)
    return FALSE;

  info = g_list_nth_data (icon_view->priv->cell_list,
                          icon_view->priv->cursor_cell);

  if (info)
    {
      g_object_get (info->cell, "mode", &mode, NULL);

      if (mode == GTK_CELL_RENDERER_MODE_ACTIVATABLE)
        {
          exo_icon_view_item_activate_cell (icon_view,
                                            icon_view->priv->cursor_item,
                                            info, NULL);
          return TRUE;
        }
      else if (mode == GTK_CELL_RENDERER_MODE_EDITABLE)
        {
          exo_icon_view_start_editing (icon_view,
                                       icon_view->priv->cursor_item,
                                       info, NULL);
          return TRUE;
        }
    }

  path = gtk_tree_path_new_from_indices (icon_view->priv->cursor_item->index, -1);
  exo_icon_view_item_activated (icon_view, path);
  gtk_tree_path_free (path);

  return TRUE;
}



static gboolean
exo_icon_view_real_start_interactive_search (ExoIconView *icon_view)
{
  return exo_icon_view_search_start (icon_view, TRUE);
}



static void
exo_icon_view_real_toggle_cursor_item (ExoIconView *icon_view)
{
  if (G_LIKELY (icon_view->priv->cursor_item != NULL))
    {
      switch (icon_view->priv->selection_mode)
        {
        case GTK_SELECTION_NONE:
          break;

        case GTK_SELECTION_BROWSE:
          exo_icon_view_select_item (icon_view, icon_view->priv->cursor_item);
          break;

        case GTK_SELECTION_SINGLE:
          if (icon_view->priv->cursor_item->selected)
            exo_icon_view_unselect_item (icon_view, icon_view->priv->cursor_item);
          else
            exo_icon_view_select_item (icon_view, icon_view->priv->cursor_item);
          break;

        case GTK_SELECTION_MULTIPLE:
          icon_view->priv->cursor_item->selected = !icon_view->priv->cursor_item->selected;
          g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
          exo_icon_view_item_selected_changed (icon_view, icon_view->priv->cursor_item);
          exo_icon_view_queue_draw_item (icon_view, icon_view->priv->cursor_item);
          break;

        default:
          g_assert_not_reached ();
        }
    }
}



static void
exo_icon_view_adjustment_changed (GtkAdjustment *adjustment,
                                  ExoIconView   *icon_view)
{
  if (gtk_widget_get_realized (GTK_WIDGET(icon_view)))
    {
      gdk_window_move (icon_view->priv->bin_window, -gtk_adjustment_get_value(icon_view->priv->hadjustment), -gtk_adjustment_get_value(icon_view->priv->vadjustment));

      if (G_UNLIKELY (icon_view->priv->doing_rubberband))
        exo_icon_view_update_rubberband (GTK_WIDGET (icon_view));

      gdk_window_process_updates (icon_view->priv->bin_window, TRUE);
    }
}


static GList*
exo_icon_view_layout_single_row (ExoIconView *icon_view,
                                 GList       *first_item,
                                 gint         item_width,
                                 gint         row,
                                 gint        *y,
                                 gint        *maximum_width,
                                 gint         max_cols)
{
  ExoIconViewPrivate *priv = icon_view->priv;
  ExoIconViewItem    *item;
  gboolean            rtl;
  GList              *last_item;
  GList              *items = first_item;
  gint               *max_width;
  gint               *max_height;
  gint                focus_width;
  gint                current_width;
  gint                colspan;
  gint                col = 0;
  gint                x;
  gint                i;
  GtkAllocation       allocation;

  rtl = (gtk_widget_get_direction (GTK_WIDGET (icon_view)) == GTK_TEXT_DIR_RTL);
  gtk_widget_get_allocation (GTK_WIDGET (icon_view), &allocation);

  max_width = g_newa (gint, priv->n_cells);
  max_height = g_newa (gint, priv->n_cells);
  for (i = priv->n_cells; --i >= 0; )
    {
      max_width[i] = 0;
      max_height[i] = 0;
    }

  gtk_widget_style_get (GTK_WIDGET (icon_view),
                        "focus-line-width", &focus_width,
                        NULL);

  x = priv->margin + focus_width;
  current_width = 2 * (priv->margin + focus_width);

  for (items = first_item; items != NULL; items = items->next)
    {
      item = EXO_ICON_VIEW_ITEM (items->data);

      exo_icon_view_calculate_item_size (icon_view, item);
      colspan = 1 + (item->area.width - 1) / (item_width + priv->column_spacing);

      item->area.width = colspan * item_width + (colspan - 1) * priv->column_spacing;

      current_width += item->area.width + priv->column_spacing + 2 * focus_width;

      if (G_LIKELY (items != first_item))
        {
          if ((priv->columns <= 0 && current_width > allocation.width) ||
              (priv->columns > 0 && col >= priv->columns) ||
              (max_cols > 0 && col >= max_cols))
            break;
        }

      item->area.y = *y + focus_width;
      item->area.x = rtl ? allocation.width - item->area.width - x : x;

      x = current_width - (priv->margin + focus_width);

      for (i = 0; i < priv->n_cells; i++)
        {
          max_width[i] = MAX (max_width[i], item->box[i].width);
          max_height[i] = MAX (max_height[i], item->box[i].height);
        }

      if (current_width > *maximum_width)
        *maximum_width = current_width;

      item->row = row;
      item->col = col;

      col += colspan;
    }

  last_item = items;

  /* Now go through the row again and align the icons */
  for (items = first_item; items != last_item; items = items->next)
    {
      item = EXO_ICON_VIEW_ITEM (items->data);

      exo_icon_view_calculate_item_size2 (icon_view, item, max_width, max_height);

      /* We may want to readjust the new y coordinate. */
      if (item->area.y + item->area.height + focus_width + priv->row_spacing > *y)
        *y = item->area.y + item->area.height + focus_width + priv->row_spacing;

      if (G_UNLIKELY (rtl))
        item->col = col - 1 - item->col;
    }

  return last_item;
}



static GList*
exo_icon_view_layout_single_col (ExoIconView *icon_view,
                                 GList       *first_item,
                                 gint         item_height,
                                 gint         col,
                                 gint        *x,
                                 gint        *maximum_height,
                                 gint         max_rows)
{
  ExoIconViewPrivate *priv = icon_view->priv;
  ExoIconViewItem    *item;
  GList              *items = first_item;
  GList              *last_item;
  gint               *max_width;
  gint               *max_height;
  gint                focus_width;
  gint                current_height;
  gint                rowspan;
  gint                row = 0;
  gint                y;
  gint                i;
  GtkAllocation       allocation;

  max_width = g_newa (gint, priv->n_cells);
  max_height = g_newa (gint, priv->n_cells);
  for (i = priv->n_cells; --i >= 0; )
    {
      max_width[i] = 0;
      max_height[i] = 0;
    }

  gtk_widget_style_get (GTK_WIDGET (icon_view),
                        "focus-line-width", &focus_width,
                        NULL);
  gtk_widget_get_allocation (GTK_WIDGET (icon_view), &allocation);

  y = priv->margin + focus_width;
  current_height = 2 * (priv->margin + focus_width);

  for (items = first_item; items != NULL; items = items->next)
    {
      item = EXO_ICON_VIEW_ITEM (items->data);

      exo_icon_view_calculate_item_size (icon_view, item);

      rowspan = 1 + (item->area.height - 1) / (item_height + priv->row_spacing);

      item->area.height = rowspan * item_height + (rowspan - 1) * priv->row_spacing;

      current_height += item->area.height + priv->row_spacing + 2 * focus_width;

      if (G_LIKELY (items != first_item))
        {
          if (current_height >= allocation.height ||
             (max_rows > 0 && row >= max_rows))
            break;
        }

      item->area.y = y + focus_width;
      item->area.x = *x;

      y = current_height - (priv->margin + focus_width);

      for (i = 0; i < priv->n_cells; i++)
        {
          max_width[i] = MAX (max_width[i], item->box[i].width);
          max_height[i] = MAX (max_height[i], item->box[i].height);
        }

      if (current_height > *maximum_height)
        *maximum_height = current_height;

      item->row = row;
      item->col = col;

      row += rowspan;
    }

  last_item = items;

  /* Now go through the column again and align the icons */
  for (items = first_item; items != last_item; items = items->next)
    {
      item = EXO_ICON_VIEW_ITEM (items->data);

      exo_icon_view_calculate_item_size2 (icon_view, item, max_width, max_height);

      /* We may want to readjust the new x coordinate. */
      if (item->area.x + item->area.width + focus_width + priv->column_spacing > *x)
        *x = item->area.x + item->area.width + focus_width + priv->column_spacing;
    }

  return last_item;
}



static void
exo_icon_view_set_adjustment_upper (GtkAdjustment *adj,
                                    gdouble        upper)
{
  if (upper != gtk_adjustment_get_upper(adj))
    {
      gdouble min = MAX (0.0, upper - gtk_adjustment_get_page_size(adj));
      gboolean value_changed = FALSE;

      gtk_adjustment_set_upper(adj, upper);

      if (gtk_adjustment_get_value(adj) > min)
        {
          gtk_adjustment_set_value(adj, min);
          value_changed = TRUE;
        }

      gtk_adjustment_changed (adj);

      if (value_changed)
        gtk_adjustment_value_changed (adj);
    }
}



static gint
exo_icon_view_layout_cols (ExoIconView *icon_view,
                           gint         item_height,
                           gint        *x,
                           gint        *maximum_height,
                           gint         max_rows)
{
  GList *icons = icon_view->priv->items;
  GList *items;
  gboolean rtl;
  gint   col = 0;
  gint   rows = 0;
  gint   shift;

  rtl = (gtk_widget_get_direction (GTK_WIDGET (icon_view)) == GTK_TEXT_DIR_RTL);

  shift = icon_view->priv->margin;
  *x = shift;

  do
    {
      items = icons; /* it will be used below */
      icons = exo_icon_view_layout_single_col (icon_view, icons,
                                               item_height, col,
                                               &shift, maximum_height, max_rows);

      if (rtl)
      {
          GList *items2;
          ExoIconViewItem *item;
          gint i;

          /* update width */
          shift -= icon_view->priv->margin; /* width of the new column */
          *x += shift;
          /* shift all previous items to right so new column will be left one */
          if (items) for (items2 = icon_view->priv->items; items2 != items; items2 = items2->next)
          {
              item = EXO_ICON_VIEW_ITEM (items2->data);

              item->area.x += shift;
              for (i = 0; i < icon_view->priv->n_cells; i++)
                  item->box[i].x += shift;
          }
          /* prepare for the next col */
          shift = icon_view->priv->margin;
      }
      else
          *x = shift;
      /* count the number of rows in the first column */
      if (G_UNLIKELY (col == 0))
        {
          for (items = icon_view->priv->items, rows = 0; items != icons; items = items->next, ++rows)
            ;
        }

      col++;
    }
  while (icons != NULL);

  *x += icon_view->priv->margin;
  icon_view->priv->cols = col;

  return rows;
}



static gint
exo_icon_view_layout_rows (ExoIconView *icon_view,
                           gint         item_width,
                           gint        *y,
                           gint        *maximum_width,
                           gint         max_cols)
{
  GList *icons = icon_view->priv->items;
  GList *items;
  gint   row = 0;
  gint   cols = 0;

  *y = icon_view->priv->margin;

  do
    {
      icons = exo_icon_view_layout_single_row (icon_view, icons,
                                               item_width, row,
                                               y, maximum_width, max_cols);

      /* count the number of columns in the first row */
      if (G_UNLIKELY (row == 0))
        {
          for (items = icon_view->priv->items, cols = 0; items != icons; items = items->next, ++cols)
            ;
        }

      row++;
    }
  while (icons != NULL);

  *y += icon_view->priv->margin;
  icon_view->priv->rows = row;

  return cols;
}



static void
exo_icon_view_layout (ExoIconView *icon_view)
{
  ExoIconViewPrivate *priv = icon_view->priv;
  ExoIconViewItem    *item;
  GList              *icons;
  gint                maximum_height = 0;
  gint                maximum_width = 0;
  gint                item_height;
  gint                item_width;
  gint                rows, cols;
  gint                x, y;
  GtkAllocation       allocation;
#if GTK_CHECK_VERSION(2, 20, 0)
  GtkRequisition      requisition;
#endif

  /* verify that we still have a valid model */
  if (G_UNLIKELY (priv->model == NULL))
    return;

  gtk_widget_get_allocation (GTK_WIDGET (icon_view), &allocation);

  /* determine the layout mode */
  if (G_LIKELY (priv->layout_mode == EXO_ICON_VIEW_LAYOUT_ROWS))
    {
      /* calculate item sizes on-demand */
      item_width = priv->item_width;
      if (item_width < 0)
        {
          for (icons = priv->items; icons != NULL; icons = icons->next)
            {
              item = icons->data;
              exo_icon_view_calculate_item_size (icon_view, item);
              item_width = MAX (item_width, item->area.width);
            }
        }

      cols = exo_icon_view_layout_rows (icon_view, item_width, &y, &maximum_width, 0);

      /* If, by adding another column, we increase the height of the icon view, thus forcing a
       * vertical scrollbar to appear that would prevent the last column from being able to fit,
       * we need to relayout the icons with one less column.
       */
      if (cols == priv->cols + 1 && y > allocation.height &&
          priv->height <= allocation.height)
        {
          cols = exo_icon_view_layout_rows (icon_view, item_width, &y, &maximum_width, priv->cols);
        }

      priv->width = maximum_width;
      priv->height = y;
      priv->cols = cols;
    }
  else
    {
      /* calculate item sizes on-demand */
      for (icons = priv->items, item_height = 0; icons != NULL; icons = icons->next)
        {
          item = icons->data;
          exo_icon_view_calculate_item_size (icon_view, item);
          item_height = MAX (item_height, item->area.height);
        }

      rows = exo_icon_view_layout_cols (icon_view, item_height, &x, &maximum_height, 0);

      /* If, by adding another row, we increase the width of the icon view, thus forcing a
       * horizontal scrollbar to appear that would prevent the last row from being able to fit,
       * we need to relayout the icons with one less row.
       */
      if (rows == priv->rows + 1 && x > allocation.width &&
          priv->width <= allocation.width)
        {
          rows = exo_icon_view_layout_cols (icon_view, item_height, &x, &maximum_height, priv->rows);
        }
      else if (x < allocation.width &&
               gtk_widget_get_direction (GTK_WIDGET (icon_view)) == GTK_TEXT_DIR_RTL)
        {
          /* shift items to align right border */
          gint shift = allocation.width - x, i;
          for (icons = priv->items; icons != NULL; icons = icons->next)
          {
              item = icons->data;
              item->area.x += shift;
              for (i = 0; i < icon_view->priv->n_cells; i++)
                  item->box[i].x += shift;
          }
        }

      priv->height = maximum_height;
      priv->width = x;
      priv->rows = rows;
    }

  exo_icon_view_set_adjustment_upper (priv->hadjustment, priv->width);
  exo_icon_view_set_adjustment_upper (priv->vadjustment, priv->height);

#if GTK_CHECK_VERSION(2, 20, 0)
  gtk_widget_get_requisition (GTK_WIDGET (icon_view), &requisition);
  if (priv->width != requisition.width
      || priv->height != requisition.height)
#else
  if (priv->width != GTK_WIDGET (icon_view)->requisition.width
      || priv->height != GTK_WIDGET (icon_view)->requisition.height)
#endif
    gtk_widget_queue_resize_no_redraw (GTK_WIDGET (icon_view));

  if (gtk_widget_get_realized (GTK_WIDGET(icon_view)))
    {
      gdk_window_resize (priv->bin_window,
                         MAX (priv->width, allocation.width),
                         MAX (priv->height, allocation.height));
    }

  /* drop any pending layout idle source */
  if (priv->layout_idle_id != 0)
    g_source_remove (priv->layout_idle_id);

  gtk_widget_queue_draw (GTK_WIDGET (icon_view));
}



static void
exo_icon_view_get_cell_area (ExoIconView         *icon_view,
                             ExoIconViewItem     *item,
                             ExoIconViewCellInfo *info,
                             GdkRectangle        *cell_area)
{
  if (icon_view->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      cell_area->x = item->box[info->position].x - item->before[info->position];
      cell_area->y = item->area.y;
      cell_area->width = item->box[info->position].width + item->before[info->position] + item->after[info->position];
      cell_area->height = item->area.height;
    }
  else
    {
      cell_area->x = item->area.x;
      cell_area->y = item->box[info->position].y - item->before[info->position];
      cell_area->width = item->area.width;
      cell_area->height = item->box[info->position].height + item->before[info->position] + item->after[info->position];
    }
}



static void
exo_icon_view_calculate_item_size (ExoIconView     *icon_view,
                                   ExoIconViewItem *item)
{
  ExoIconViewCellInfo *info;
  GList               *lp;
  gchar               *buffer;
#if GTK_CHECK_VERSION(3, 0, 0)
  GtkRequisition       requisition;
#endif

  if (G_LIKELY (item->area.width != -1))
    return;

  if (G_UNLIKELY (item->n_cells != icon_view->priv->n_cells))
    {
      /* apply the new cell size */
      item->n_cells = icon_view->priv->n_cells;

      /* release the memory chunk (if any) */
      g_free (item->box);

      /* allocate a single memory chunk for box, after and before */
      buffer = g_malloc0 (item->n_cells * (sizeof (GdkRectangle) + 2 * sizeof (gint)));

      /* assign the memory */
      item->box = (GdkRectangle *) buffer;
      item->after = (gint *) (buffer + item->n_cells * sizeof (GdkRectangle));
      item->before = item->after + item->n_cells;
    }

  exo_icon_view_set_cell_data (icon_view, item);

  item->area.width = 0;
  item->area.height = 0;
  for (lp = icon_view->priv->cell_list; lp != NULL; lp = lp->next)
    {
      info = EXO_ICON_VIEW_CELL_INFO (lp->data);
      if (G_UNLIKELY (!gtk_cell_renderer_get_visible(info->cell)))
        continue;

#if GTK_CHECK_VERSION(3, 0, 0)
      gtk_cell_renderer_get_preferred_size (info->cell, GTK_WIDGET (icon_view),
                                            NULL, &requisition);
      item->box[info->position].width = requisition.width;
      item->box[info->position].height = requisition.height;
#else
      gtk_cell_renderer_get_size (info->cell, GTK_WIDGET (icon_view),
                                  NULL, NULL, NULL,
                                  &item->box[info->position].width,
                                  &item->box[info->position].height);
#endif

      if (icon_view->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          item->area.width += item->box[info->position].width + (info->position > 0 ? icon_view->priv->spacing : 0);
          item->area.height = MAX (item->area.height, item->box[info->position].height);
        }
      else
        {
          item->area.width = MAX (item->area.width, item->box[info->position].width);
          item->area.height += item->box[info->position].height + (info->position > 0 ? icon_view->priv->spacing : 0);
        }
    }
}



static void
exo_icon_view_calculate_item_size2 (ExoIconView     *icon_view,
                                    ExoIconViewItem *item,
                                    gint            *max_width,
                                    gint            *max_height)
{
  ExoIconViewCellInfo *info;
  GdkRectangle        *box;
  GdkRectangle         cell_area;
  gboolean             rtl;
  GList               *lp;
  gint                 spacing;
  gint                 i, k;
  gint                 xpad, ypad;
  gfloat               xalign, yalign;

  rtl = (gtk_widget_get_direction (GTK_WIDGET (icon_view)) == GTK_TEXT_DIR_RTL);

  spacing = icon_view->priv->spacing;

  if (G_LIKELY (icon_view->priv->layout_mode == EXO_ICON_VIEW_LAYOUT_ROWS))
    {
      item->area.height = 0;
      for (i = 0; i < icon_view->priv->n_cells; ++i)
        {
          if (icon_view->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            item->area.height = MAX (item->area.height, max_height[i]);
          else
            item->area.height += max_height[i] + (i > 0 ? spacing : 0);
        }
    }
  else
    {
      item->area.width = 0;
      for (i = 0; i < icon_view->priv->n_cells; ++i)
        {
          if (icon_view->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            item->area.width += max_width[i] + (i > 0 ? spacing : 0);
          else
            item->area.width = MAX (item->area.width, max_width[i]);
        }
    }

  cell_area.x = item->area.x;
  cell_area.y = item->area.y;

  for (k = 0; k < 2; ++k)
    {
      for (lp = icon_view->priv->cell_list, i = 0; lp != NULL; lp = lp->next, ++i)
        {
          info = EXO_ICON_VIEW_CELL_INFO (lp->data);
          if (G_UNLIKELY (!gtk_cell_renderer_get_visible(info->cell) || info->pack == (k ? GTK_PACK_START : GTK_PACK_END)))
            continue;

          if (icon_view->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            {
              cell_area.width = item->box[info->position].width;
              cell_area.height = item->area.height;
            }
          else
            {
              cell_area.width = item->area.width;
              cell_area.height = max_height[i];
            }

          box = item->box + info->position;
          gtk_cell_renderer_get_alignment (info->cell, &xalign, &yalign);
          gtk_cell_renderer_get_padding (info->cell, &xpad, &ypad);
          box->x = cell_area.x + (rtl ? (1.0 - xalign) : xalign) * (cell_area.width - box->width - (2 * xpad));
          box->x = MAX (box->x, 0);
          box->y = cell_area.y + yalign * (cell_area.height - box->height - (2 * ypad));
          box->y = MAX (box->y, 0);

          if (icon_view->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            {
              item->before[info->position] = item->box[info->position].x - cell_area.x;
              item->after[info->position] = cell_area.width - item->box[info->position].width - item->before[info->position];
              cell_area.x += cell_area.width + spacing;
            }
          else
            {
              if (item->box[info->position].width > item->area.width)
                {
                  item->area.width = item->box[info->position].width;
                  cell_area.width = item->area.width;
                }
              item->before[info->position] = item->box[info->position].y - cell_area.y;
              item->after[info->position] = cell_area.height - item->box[info->position].height - item->before[info->position];
              cell_area.y += cell_area.height + spacing;
            }
        }
    }

  if (G_UNLIKELY (rtl && icon_view->priv->orientation == GTK_ORIENTATION_HORIZONTAL))
    {
      for (i = 0; i < icon_view->priv->n_cells; i++)
        item->box[i].x = item->area.x + item->area.width - (item->box[i].x + item->box[i].width - item->area.x);
    }
}



static void
exo_icon_view_invalidate_sizes (ExoIconView *icon_view)
{
  GList *lp;

  for (lp = icon_view->priv->items; lp != NULL; lp = lp->next)
    EXO_ICON_VIEW_ITEM (lp->data)->area.width = -1;
  exo_icon_view_queue_layout (icon_view);
}



static void
exo_icon_view_paint_item (ExoIconView     *icon_view,
                          ExoIconViewItem *item,
                          GdkRectangle    *area,
#if GTK_CHECK_VERSION(3, 0, 0)
                          cairo_t         *drawable,
#else
                          GdkDrawable     *drawable,
#endif
                          gint             x,
                          gint             y,
                          gboolean         draw_focus)
{
  GtkCellRendererState flags;
  ExoIconViewCellInfo *info;
  //GtkStateType         state;
  GdkRectangle         cell_area;
  //gboolean             rtl;
  GList               *lp;

  if (G_UNLIKELY (icon_view->priv->model == NULL))
    return;

  exo_icon_view_set_cell_data (icon_view, item);

  //rtl = gtk_widget_get_direction (GTK_WIDGET (icon_view)) == GTK_TEXT_DIR_RTL;

  if (item->selected)
    {
      flags = GTK_CELL_RENDERER_SELECTED;
      //state = gtk_widget_has_focus (icon_view) ? GTK_STATE_SELECTED : GTK_STATE_ACTIVE;
#if 0
      /* FIXME We hardwire background drawing behind text cell renderers
       * here. This is ugly, but it's done to be consistent with GtkIconView.
       * The additional info->is_text attribute is used for performance
       * optimization and should be removed alongside the following code. */

      cr = gdk_cairo_create (drawable);

      for (lp = icon_view->priv->cell_list; lp != NULL; lp = lp->next)
        {
          info = EXO_ICON_VIEW_CELL_INFO (lp->data);

          if (G_UNLIKELY (!gtk_cell_renderer_get_visible(info->cell)))
            continue;

          if (info->is_text)
            {
              exo_icon_view_get_cell_area (icon_view, item, info, &cell_area);

              x_0 = x - item->area.x + cell_area.x;
              y_0 = x - item->area.x + cell_area.y;
              x_1 = x_0 + cell_area.width;
              y_1 = y_0 + cell_area.height;

              cairo_move_to (cr, x_0 + 5, y_0);
              cairo_line_to (cr, x_1 - 5, y_0);
              cairo_curve_to (cr, x_1 - 5, y_0, x_1, y_0, x_1, y_0 + 5);
              cairo_line_to (cr, x_1, y_1 - 5);
              cairo_curve_to (cr, x_1, y_1 - 5, x_1, y_1, x_1 - 5, y_1);
              cairo_line_to (cr, x_0 + 5, y_1);
              cairo_curve_to (cr, x_0 + 5, y_1, x_0, y_1, x_0, y_1 - 5);
              cairo_line_to (cr, x_0, y_0 + 5);
              cairo_curve_to (cr, x_0, y_0 + 5, x_0, y_0, x_0 + 5, y_0);

              gdk_cairo_set_source_color (cr, &GTK_WIDGET (icon_view)->style->base[state]);

              cairo_fill (cr);
            }
        }

      cairo_destroy (cr);

      /* FIXME Ugly code ends here */
#endif
    }
  else
    {
      flags = 0;
      //state = GTK_STATE_NORMAL;
    }

  if (G_UNLIKELY (icon_view->priv->prelit_item == item))
    flags |= GTK_CELL_RENDERER_PRELIT;
  if (G_UNLIKELY (EXO_ICON_VIEW_FLAG_SET (icon_view, EXO_ICON_VIEW_DRAW_KEYFOCUS) && icon_view->priv->cursor_item == item))
    flags |= GTK_CELL_RENDERER_FOCUSED;

#ifdef DEBUG_ICON_VIEW
  gdk_draw_rectangle (drawable,
                      GTK_WIDGET (icon_view)->style->black_gc,
                      FALSE,
                      x, y,
                      item->area.width, item->area.height);
#endif

  for (lp = icon_view->priv->cell_list; lp != NULL; lp = lp->next)
    {
      info = EXO_ICON_VIEW_CELL_INFO (lp->data);

      if (G_UNLIKELY (!gtk_cell_renderer_get_visible(info->cell)))
        continue;

      exo_icon_view_get_cell_area (icon_view, item, info, &cell_area);

#ifdef DEBUG_ICON_VIEW
      gdk_draw_rectangle (drawable,
                          GTK_WIDGET (icon_view)->style->black_gc,
                          FALSE,
                          x - item->area.x + cell_area.x,
                          y - item->area.y + cell_area.y,
                          cell_area.width, cell_area.height);

      gdk_draw_rectangle (drawable,
                          GTK_WIDGET (icon_view)->style->black_gc,
                          FALSE,
                          x - item->area.x + item->box[info->position].x,
                          y - item->area.y + item->box[info->position].y,
                          item->box[info->position].width, item->box[info->position].height);
#endif

      cell_area.x = x - item->area.x + cell_area.x;
      cell_area.y = y - item->area.y + cell_area.y;

      gtk_cell_renderer_render (info->cell,
                                drawable,
                                GTK_WIDGET (icon_view),
                                &cell_area, &cell_area,
#if !GTK_CHECK_VERSION(3, 0, 0)
                                area,
#endif
                                flags);

    }
}



static void
exo_icon_view_queue_draw_item (ExoIconView     *icon_view,
                               ExoIconViewItem *item)
{
  GdkRectangle rect;
  gint         focus_width;

  gtk_widget_style_get (GTK_WIDGET (icon_view),
                        "focus-line-width", &focus_width,
                        NULL);

  rect.x = item->area.x - focus_width;
  rect.y = item->area.y - focus_width;
  rect.width = item->area.width + 2 * focus_width;
  rect.height = item->area.height + 2 * focus_width;

  if (icon_view->priv->bin_window)
    gdk_window_invalidate_rect (icon_view->priv->bin_window, &rect, TRUE);
}



static gboolean
layout_callback (gpointer user_data)
{
  ExoIconView *icon_view = EXO_ICON_VIEW (user_data);

  if(!g_source_is_destroyed(g_main_current_source()))
    exo_icon_view_layout (icon_view);

  return FALSE;
}



static void
layout_destroy (gpointer user_data)
{
  EXO_ICON_VIEW (user_data)->priv->layout_idle_id = 0;
}



static void
exo_icon_view_queue_layout (ExoIconView *icon_view)
{
  if (G_UNLIKELY (icon_view->priv->layout_idle_id == 0))
    icon_view->priv->layout_idle_id = gdk_threads_add_idle_full (G_PRIORITY_DEFAULT_IDLE, layout_callback, icon_view, layout_destroy);
}



static void
exo_icon_view_set_cursor_item (ExoIconView     *icon_view,
                               ExoIconViewItem *item,
                               gint             cursor_cell)
{
  AtkObject *obj;
  AtkObject *item_obj;
  AtkObject *cursor_item_obj;

  /* When hitting this path from keynav, the focus cell is
   * already set, we dont need to notify the atk object
   * but we still need to queue the draw here (in the case
   * that the focus cell changes but not the cursor item).
   */
  exo_icon_view_queue_draw_item (icon_view, item);

  if (icon_view->priv->cursor_item == item &&
      (cursor_cell < 0 || cursor_cell == icon_view->priv->cursor_cell))
    return;

  obj = gtk_widget_get_accessible (GTK_WIDGET (icon_view));
  if (icon_view->priv->cursor_item != NULL)
    {
      exo_icon_view_queue_draw_item (icon_view, icon_view->priv->cursor_item);
      if (obj != NULL)
        {
          cursor_item_obj = atk_object_ref_accessible_child (obj, icon_view->priv->cursor_item->index);
          if (cursor_item_obj != NULL)
            atk_object_notify_state_change (cursor_item_obj, ATK_STATE_FOCUSED, FALSE);
        }
    }
  icon_view->priv->cursor_item = item;
  if (cursor_cell >= 0)
    icon_view->priv->cursor_cell = cursor_cell;

  /* Notify that accessible focus object has changed */
  item_obj = atk_object_ref_accessible_child (obj, item->index);

  if (item_obj != NULL)
    {
#if !ATK_CHECK_VERSION(2, 9, 4)
      atk_focus_tracker_notify (item_obj);
#endif
      atk_object_notify_state_change (item_obj, ATK_STATE_FOCUSED, TRUE);
      g_object_unref (item_obj);
    }
}



static ExoIconViewItem*
exo_icon_view_get_item_at_coords (const ExoIconView    *icon_view,
                                  gint                  x,
                                  gint                  y,
                                  gboolean              only_in_cell,
                                  ExoIconViewCellInfo **cell_at_pos)
{
  const ExoIconViewPrivate *priv = icon_view->priv;
  ExoIconViewCellInfo      *info;
  ExoIconViewItem          *item;
  GdkRectangle              box;
  const GList              *items;
  const GList              *lp;

  for (items = priv->items; items != NULL; items = items->next)
    {
      item = items->data;
      if (x >= item->area.x - priv->row_spacing / 2 && x <= item->area.x + item->area.width + priv->row_spacing / 2 &&
          y >= item->area.y - priv->column_spacing / 2 && y <= item->area.y + item->area.height + priv->column_spacing / 2)
        {
          if (only_in_cell || cell_at_pos)
            {
              exo_icon_view_set_cell_data (icon_view, item);
              for (lp = priv->cell_list; lp != NULL; lp = lp->next)
                {
                  /* check if the cell is visible */
                  info = (ExoIconViewCellInfo *) lp->data;
                  if (!gtk_cell_renderer_get_visible(info->cell))
                    continue;

                  box = item->box[info->position];
                  if ((x >= box.x && x <= box.x + box.width &&
                       y >= box.y && y <= box.y + box.height) ||
                      (x >= box.x  &&
                       x <= box.x + box.width &&
                       y >= box.y &&
                       y <= box.y + box.height))
                    {
                      if (cell_at_pos != NULL)
                        *cell_at_pos = info;

                      return item;
                    }
                }

              if (only_in_cell)
                return NULL;

              if (cell_at_pos != NULL)
                *cell_at_pos = NULL;
            }

          return item;
        }
    }

  return NULL;
}



static void
exo_icon_view_select_item (ExoIconView      *icon_view,
                           ExoIconViewItem  *item)
{
  if (item->selected || icon_view->priv->selection_mode == GTK_SELECTION_NONE)
    return;
  else if (icon_view->priv->selection_mode != GTK_SELECTION_MULTIPLE)
    exo_icon_view_unselect_all_internal (icon_view);

  item->selected = TRUE;

  exo_icon_view_queue_draw_item (icon_view, item);

  exo_icon_view_item_selected_changed (icon_view, item);
  g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



static void
exo_icon_view_unselect_item (ExoIconView      *icon_view,
                             ExoIconViewItem  *item)
{
  if (!item->selected)
    return;

  if (icon_view->priv->selection_mode == GTK_SELECTION_NONE ||
      icon_view->priv->selection_mode == GTK_SELECTION_BROWSE)
    return;

  item->selected = FALSE;

  exo_icon_view_item_selected_changed (icon_view, item);
  g_signal_emit (G_OBJECT (icon_view), icon_view_signals[SELECTION_CHANGED], 0);

  exo_icon_view_queue_draw_item (icon_view, item);
}


static void
verify_items (ExoIconView *icon_view)
{
  GList *items;
  int i = 0;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      ExoIconViewItem *item = items->data;

      if (item->index != i)
        g_error ("List item does not match its index: "
                 "item index %d and list index %d\n", item->index, i);

      i++;
    }
}


static void
exo_icon_view_row_changed (GtkTreeModel *model,
                           GtkTreePath  *path,
                           GtkTreeIter  *iter,
                           ExoIconView  *icon_view)
{
  ExoIconViewItem *item;

  item = g_list_nth_data (icon_view->priv->items, gtk_tree_path_get_indices(path)[0]);

  /* stop editing this item */
  if (G_UNLIKELY (item == icon_view->priv->edited_item))
    exo_icon_view_stop_editing (icon_view, TRUE);

  /* emit "selection-changed" if the item is selected */
  if (G_UNLIKELY (item->selected))
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  /* recalculate layout (a value of -1 for width
   * indicates that the item needs to be layouted).
   */
  item->area.width = -1;
  exo_icon_view_queue_layout (icon_view);
  verify_items (icon_view);
}



static void
exo_icon_view_row_inserted (GtkTreeModel *model,
                            GtkTreePath  *path,
                            GtkTreeIter  *iter,
                            ExoIconView  *icon_view)
{
  ExoIconViewItem *item;
  GList           *list;
  gint             idx;

  idx = gtk_tree_path_get_indices (path)[0];

  /* allocate the new item */
  item = g_slice_new0 (ExoIconViewItem);
  item->iter = *iter;
  item->area.width = -1;
  item->index = idx;
  icon_view->priv->items = g_list_insert (icon_view->priv->items, item, idx);

  list = g_list_nth (icon_view->priv->items, idx + 1);
  for (; list; list = list->next)
    {
      item = list->data;

      item->index++;
    }
  verify_items (icon_view);

  /* recalculate the layout */
  exo_icon_view_queue_layout (icon_view);
}



static void
exo_icon_view_row_deleted (GtkTreeModel *model,
                           GtkTreePath  *path,
                           ExoIconView  *icon_view)
{
  ExoIconViewItem *item;
  gboolean         changed = FALSE;
  GList           *list, *next;

  /* determine the position and the item for the path */
  list = g_list_nth (icon_view->priv->items, gtk_tree_path_get_indices (path)[0]);
  next = list->next;
  item = list->data;

  if (G_UNLIKELY (item == icon_view->priv->edited_item))
    exo_icon_view_stop_editing (icon_view, TRUE);

  /* use the next item (if any) as anchor, else use prev, otherwise reset anchor */
  if (G_UNLIKELY (item == icon_view->priv->anchor_item))
    icon_view->priv->anchor_item = (list->next != NULL) ? list->next->data : ((list->prev != NULL) ? list->prev->data : NULL);

  /* use the next item (if any) as cursor, else use prev, otherwise reset cursor */
  if (G_UNLIKELY (item == icon_view->priv->cursor_item))
    icon_view->priv->cursor_item = (list->next != NULL) ? list->next->data : ((list->prev != NULL) ? list->prev->data : NULL);

  if (G_UNLIKELY (item == icon_view->priv->prelit_item))
    {
      /* reset the prelit item */
      icon_view->priv->prelit_item = NULL;

      /* cancel any pending single click timer */
      if (G_UNLIKELY (icon_view->priv->single_click_timeout_id != 0))
        g_source_remove (icon_view->priv->single_click_timeout_id);

      /* in single click mode, we also reset the cursor when realized */
      if (G_UNLIKELY (icon_view->priv->single_click && gtk_widget_get_realized (GTK_WIDGET(icon_view))))
        gdk_window_set_cursor (icon_view->priv->bin_window, NULL);
    }

  /* check if the selection changed */
  if (G_UNLIKELY (item->selected))
    changed = TRUE;

  /* release the item resources */
  g_free (item->box);

  /* drop the item from the list */
  icon_view->priv->items = g_list_delete_link (icon_view->priv->items, list);

  /* release the item */
  g_slice_free (ExoIconViewItem, item);

  /* update indices */
  for (; next; next = next->next)
    {
      item = next->data;

      item->index--;
    }
  verify_items (icon_view);

  /* recalculate the layout */
  exo_icon_view_queue_layout (icon_view);

  /* if we removed a previous selected item, we need
   * to tell others that we have a new selection.
   */
  if (G_UNLIKELY (changed))
    g_signal_emit (G_OBJECT (icon_view), icon_view_signals[SELECTION_CHANGED], 0);
}



static void
exo_icon_view_rows_reordered (GtkTreeModel *model,
                              GtkTreePath  *parent,
                              GtkTreeIter  *iter,
                              gint         *new_order,
                              ExoIconView  *icon_view)
{
  GList **list_array;
  GList  *list;
  gint   *order;
  gint     length;
  gint     i;

  /* cancel any editing attempt */
  exo_icon_view_stop_editing (icon_view, TRUE);

  /* determine the number of items to reorder */
  length = gtk_tree_model_iter_n_children (model, NULL);
  if (G_UNLIKELY (length == 0))
    return;

  list_array = g_newa (GList *, length);
  order = g_newa (gint, length);

  for (i = 0; i < length; i++)
    order[new_order[i]] = i;

  for (i = 0, list = icon_view->priv->items; list != NULL; list = list->next, i++)
    list_array[order[i]] = list;

  /* hook up the first item */
  icon_view->priv->items = list_array[0];
  list_array[0]->prev = NULL;
  ((ExoIconViewItem*)list_array[0]->data)->index = 0;

  /* hook up the remaining items */
  for (i = 1; i < length; ++i)
    {
      ((ExoIconViewItem*)list_array[i]->data)->index = i;
      list_array[i - 1]->next = list_array[i];
      list_array[i]->prev = list_array[i - 1];
    }

  /* hook up the last item */
  list_array[length - 1]->next = NULL;

  exo_icon_view_queue_layout (icon_view);
  verify_items (icon_view);
}



static void
exo_icon_view_add_move_binding (GtkBindingSet  *binding_set,
                                guint           keyval,
                                guint           modmask,
                                GtkMovementStep step,
                                gint            count)
{

  gtk_binding_entry_add_signal (binding_set, keyval, modmask, "move-cursor", 2, G_TYPE_ENUM, step, G_TYPE_INT, count);

  /* skip shift+n and shift+p because this blocks type-ahead search.
   * see http://bugzilla.xfce.org/show_bug.cgi?id=4633
   */
  if (G_LIKELY (keyval != GDK_KEY_p && keyval != GDK_KEY_n))
    gtk_binding_entry_add_signal (binding_set, keyval, GDK_SHIFT_MASK, "move-cursor", 2, G_TYPE_ENUM, step, G_TYPE_INT, count);

  if ((modmask & GDK_CONTROL_MASK) != GDK_CONTROL_MASK)
    {
      gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "move-cursor", 2, G_TYPE_ENUM, step, G_TYPE_INT, count);
      gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK, "move-cursor", 2, G_TYPE_ENUM, step, G_TYPE_INT, count);
    }
}



static gboolean
exo_icon_view_real_move_cursor (ExoIconView     *icon_view,
                                GtkMovementStep  step,
                                gint             count)
{
  GdkModifierType state;

  _exo_return_val_if_fail (EXO_ICON_VIEW (icon_view), FALSE);
  _exo_return_val_if_fail (step == GTK_MOVEMENT_LOGICAL_POSITIONS ||
                           step == GTK_MOVEMENT_VISUAL_POSITIONS ||
                           step == GTK_MOVEMENT_DISPLAY_LINES ||
                           step == GTK_MOVEMENT_PAGES ||
                           step == GTK_MOVEMENT_BUFFER_ENDS, FALSE);

  if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
    return FALSE;

  exo_icon_view_stop_editing (icon_view, FALSE);
  EXO_ICON_VIEW_SET_FLAG (icon_view, EXO_ICON_VIEW_DRAW_KEYFOCUS);
  gtk_widget_grab_focus (GTK_WIDGET (icon_view));

  if (gtk_get_current_event_state (&state))
    {
      if ((state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
        icon_view->priv->ctrl_pressed = TRUE;
      if ((state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK)
        icon_view->priv->shift_pressed = TRUE;
    }
  /* else we assume not pressed */

  switch (step)
    {
    case GTK_MOVEMENT_LOGICAL_POSITIONS:
    case GTK_MOVEMENT_VISUAL_POSITIONS:
      exo_icon_view_move_cursor_left_right (icon_view, count);
      break;
    case GTK_MOVEMENT_DISPLAY_LINES:
      exo_icon_view_move_cursor_up_down (icon_view, count);
      break;
    case GTK_MOVEMENT_PAGES:
      exo_icon_view_move_cursor_page_up_down (icon_view, count);
      break;
    case GTK_MOVEMENT_BUFFER_ENDS:
      exo_icon_view_move_cursor_start_end (icon_view, count);
      break;
    default:
      g_assert_not_reached ();
    }

  icon_view->priv->ctrl_pressed = FALSE;
  icon_view->priv->shift_pressed = FALSE;

  return TRUE;
}



static gint
find_cell (ExoIconView     *icon_view,
           ExoIconViewItem *item,
           gint             cell,
           GtkOrientation   orientation,
           gint             step,
           gint            *count)
{
  gint n_focusable;
  gint *focusable;
  gint first_text;
  gint current;
  gint i, k;
  GList *l;

  if (icon_view->priv->orientation != orientation)
    return cell;

  exo_icon_view_set_cell_data (icon_view, item);

  focusable = g_new0 (gint, icon_view->priv->n_cells);
  n_focusable = 0;

  first_text = 0;
  current = 0;
  for (k = 0; k < 2; k++)
    for (l = icon_view->priv->cell_list, i = 0; l; l = l->next, i++)
      {
        ExoIconViewCellInfo *info = (ExoIconViewCellInfo *)l->data;
        GtkCellRendererMode mode;

        if (info->pack == (k ? GTK_PACK_START : GTK_PACK_END))
          continue;

        if (!gtk_cell_renderer_get_visible(info->cell))
          continue;

        if (GTK_IS_CELL_RENDERER_TEXT (info->cell))
          first_text = i;

        g_object_get(info->cell, "mode", &mode, NULL);
        if (mode != GTK_CELL_RENDERER_MODE_INERT)
          {
            if (cell == i)
              current = n_focusable;

            focusable[n_focusable] = i;

            n_focusable++;
          }
      }

  if (n_focusable == 0)
    focusable[n_focusable++] = first_text;

  if (cell < 0)
    {
      current = step > 0 ? 0 : n_focusable - 1;
      cell = focusable[current];
    }

  if (current + *count < 0)
    {
      cell = -1;
      *count = current + *count;
    }
  else if (current + *count > n_focusable - 1)
    {
      cell = -1;
      *count = current + *count - (n_focusable - 1);
    }
  else
    {
      cell = focusable[current + *count];
      *count = 0;
    }

  g_free (focusable);

  return cell;
}



static ExoIconViewItem *
find_item_page_up_down (ExoIconView     *icon_view,
                        ExoIconViewItem *current,
                        gint             count)
{
  GList *item = g_list_find (icon_view->priv->items, current);
  GList *next;
  gint   col = current->col;
  gint   y = current->area.y + count * gtk_adjustment_get_page_size(icon_view->priv->vadjustment);

  if (count > 0)
    {
      for (; item != NULL; item = item->next)
        {
          for (next = item->next; next; next = next->next)
            if (EXO_ICON_VIEW_ITEM (next->data)->col == col)
              break;

          if (next == NULL || EXO_ICON_VIEW_ITEM (next->data)->area.y > y)
            break;
        }
    }
  else
    {
      for (; item != NULL; item = item->prev)
        {
          for (next = item->prev; next; next = next->prev)
            if (EXO_ICON_VIEW_ITEM (next->data)->col == col)
              break;

          if (next == NULL || EXO_ICON_VIEW_ITEM (next->data)->area.y < y)
            break;
        }
    }

  return (item != NULL) ? item->data : NULL;
}



static gboolean
exo_icon_view_select_all_between (ExoIconView     *icon_view,
                                  ExoIconViewItem *anchor,
                                  ExoIconViewItem *cursor)
{
  GList *items;
  ExoIconViewItem *item, *last;
  gboolean dirty = FALSE;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      item = items->data;

      if (item == anchor)
        {
          last = cursor;
          break;
        }
      else if (item == cursor)
        {
          last = anchor;
          break;
        }
    }

  for (; items; items = items->next)
    {
      item = items->data;

      if (!item->selected)
      {
        dirty = TRUE;
        item->selected = TRUE;
        exo_icon_view_item_selected_changed (icon_view, item);
      }

      exo_icon_view_queue_draw_item (icon_view, item);

      if (item == last)
        break;
    }

  return dirty;
}



static void
exo_icon_view_move_cursor_up_down (ExoIconView *icon_view,
                                   gint         count)
{
  ExoIconViewItem *item;
  gboolean         dirty = FALSE;
  GList           *list;
  gint             cell = -1;
  gint             step;

  if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
    return;

  if (!icon_view->priv->cursor_item)
    {
      if (count > 0)
        list = icon_view->priv->items;
      else
        list = g_list_last (icon_view->priv->items);

      item = list ? list->data : NULL;
    }
  else
    {
      item = icon_view->priv->cursor_item;
      cell = icon_view->priv->cursor_cell;
      step = count > 0 ? 1 : -1;
      while (item)
        {
          cell = find_cell (icon_view, item, cell,
                            GTK_ORIENTATION_VERTICAL,
                            step, &count);
          if (count == 0)
            break;

          /* determine the list position for the item */
          list = g_list_find (icon_view->priv->items, item);

          if (G_LIKELY (icon_view->priv->layout_mode == EXO_ICON_VIEW_LAYOUT_ROWS))
            {
              /* determine the item in the next/prev row */
              if (step > 0)
                {
                  for (list = list->next; list != NULL; list = list->next)
                    if (EXO_ICON_VIEW_ITEM (list->data)->row == item->row + step
                        && EXO_ICON_VIEW_ITEM (list->data)->col == item->col)
                      break;
                 }
              else
                {
                  for (list = list->prev; list != NULL; list = list->prev)
                    if (EXO_ICON_VIEW_ITEM (list->data)->row == item->row + step
                        && EXO_ICON_VIEW_ITEM (list->data)->col == item->col)
                      break;
                }
            }
          else
            {
              list = (step > 0) ? list->next : list->prev;
            }

          /* check if we found a matching item */
          item = (list != NULL) ? list->data : NULL;

          count = count - step;
        }
    }

  if (item == icon_view->priv->cursor_item)
    gtk_widget_error_bell (GTK_WIDGET (icon_view));

  if (!item)
    return;

  if (icon_view->priv->ctrl_pressed ||
      !icon_view->priv->shift_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != GTK_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  exo_icon_view_set_cursor_item (icon_view, item, cell);

  if (!icon_view->priv->ctrl_pressed &&
      icon_view->priv->selection_mode != GTK_SELECTION_NONE)
    {
      dirty = exo_icon_view_unselect_all_internal (icon_view);
      dirty = exo_icon_view_select_all_between (icon_view,
                                                icon_view->priv->anchor_item,
                                                item) || dirty;
    }

  exo_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



static void
exo_icon_view_move_cursor_page_up_down (ExoIconView *icon_view,
                                        gint         count)
{
  ExoIconViewItem *item;
  gboolean dirty = FALSE;

  if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
    return;

  if (!icon_view->priv->cursor_item)
    {
      GList *list;

      if (count > 0)
        list = icon_view->priv->items;
      else
        list = g_list_last (icon_view->priv->items);

      item = list ? list->data : NULL;
    }
  else
    item = find_item_page_up_down (icon_view,
                                   icon_view->priv->cursor_item,
                                   count);

  if (item == icon_view->priv->cursor_item)
    gtk_widget_error_bell (GTK_WIDGET (icon_view));

  if (!item)
    return;

  if (icon_view->priv->ctrl_pressed ||
      !icon_view->priv->shift_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != GTK_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  exo_icon_view_set_cursor_item (icon_view, item, -1);

  if (!icon_view->priv->ctrl_pressed &&
      icon_view->priv->selection_mode != GTK_SELECTION_NONE)
    {
      dirty = exo_icon_view_unselect_all_internal (icon_view);
      dirty = exo_icon_view_select_all_between (icon_view,
                                                icon_view->priv->anchor_item,
                                                item) || dirty;
    }

  exo_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



static void
exo_icon_view_move_cursor_left_right (ExoIconView *icon_view,
                                      gint         count)
{
  ExoIconViewItem *item;
  gboolean         dirty = FALSE;
  GList           *list;
  gint             cell = -1;
  gint             step;

  if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
    return;

  if (gtk_widget_get_direction (GTK_WIDGET (icon_view)) == GTK_TEXT_DIR_RTL)
    count *= -1;

  if (!icon_view->priv->cursor_item)
    {
      if (count > 0)
        list = icon_view->priv->items;
      else
        list = g_list_last (icon_view->priv->items);

      item = list ? list->data : NULL;
    }
  else
    {
      item = icon_view->priv->cursor_item;
      cell = icon_view->priv->cursor_cell;
      step = count > 0 ? 1 : -1;
      while (item)
        {
          cell = find_cell (icon_view, item, cell,
                            GTK_ORIENTATION_HORIZONTAL,
                            step, &count);
          if (count == 0)
            break;

          /* lookup the item in the list */
          list = g_list_find (icon_view->priv->items, item);

          if (G_LIKELY (icon_view->priv->layout_mode == EXO_ICON_VIEW_LAYOUT_ROWS))
            {
              /* determine the next/prev list item depending on step,
               * support wrapping around on the edges, as requested
               * in http://bugzilla.xfce.org/show_bug.cgi?id=1623.
               */
              list = (step > 0) ? list->next : list->prev;
            }
          else
            {
              /* determine the item in the next/prev row */
              if (step > 0)
                {
                  for (list = list->next; list != NULL; list = list->next)
                    if (EXO_ICON_VIEW_ITEM (list->data)->col == item->col + step
                        && EXO_ICON_VIEW_ITEM (list->data)->row == item->row)
                      break;
                 }
              else
                {
                  for (list = list->prev; list != NULL; list = list->prev)
                    if (EXO_ICON_VIEW_ITEM (list->data)->col == item->col + step
                        && EXO_ICON_VIEW_ITEM (list->data)->row == item->row)
                      break;
                }
            }

          /* determine the item for the list position (if any) */
          item = (list != NULL) ? list->data : NULL;

          count = count - step;
        }
    }

  if (item == icon_view->priv->cursor_item)
    gtk_widget_error_bell (GTK_WIDGET (icon_view));

  if (!item)
    return;

  if (icon_view->priv->ctrl_pressed ||
      !icon_view->priv->shift_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != GTK_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  exo_icon_view_set_cursor_item (icon_view, item, cell);

  if (!icon_view->priv->ctrl_pressed &&
      icon_view->priv->selection_mode != GTK_SELECTION_NONE)
    {
      dirty = exo_icon_view_unselect_all_internal (icon_view);
      dirty = exo_icon_view_select_all_between (icon_view,
                                                icon_view->priv->anchor_item,
                                                item) || dirty;
    }

  exo_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



static void
exo_icon_view_move_cursor_start_end (ExoIconView *icon_view,
                                     gint         count)
{
  ExoIconViewItem *item;
  gboolean         dirty = FALSE;
  GList           *lp;

  if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
    return;

  lp = (count < 0) ? icon_view->priv->items : g_list_last (icon_view->priv->items);
  item = lp ? EXO_ICON_VIEW_ITEM (lp->data) : NULL;

  if (item == icon_view->priv->cursor_item)
    gtk_widget_error_bell (GTK_WIDGET (icon_view));

  if (G_UNLIKELY (item == NULL))
    return;

  if (icon_view->priv->ctrl_pressed ||
      !icon_view->priv->shift_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != GTK_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  exo_icon_view_set_cursor_item (icon_view, item, -1);

  if (!icon_view->priv->ctrl_pressed &&
      icon_view->priv->selection_mode != GTK_SELECTION_NONE)
    {
      dirty = exo_icon_view_unselect_all_internal (icon_view);
      dirty = exo_icon_view_select_all_between (icon_view,
                                                icon_view->priv->anchor_item,
                                                item) || dirty;
    }

  exo_icon_view_scroll_to_item (icon_view, item);

  if (G_UNLIKELY (dirty))
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



static void
exo_icon_view_scroll_to_item (ExoIconView     *icon_view,
                              ExoIconViewItem *item)
{
  gint x, y;
  gint focus_width;
  GtkAllocation allocation;
  GList *lp;
  GdkRectangle rect;

  gtk_widget_style_get (GTK_WIDGET (icon_view),
                        "focus-line-width", &focus_width,
                        NULL);
  gtk_widget_get_allocation (GTK_WIDGET (icon_view), &allocation);

  gdk_window_get_position (icon_view->priv->bin_window, &x, &y);

  rect.x = item->area.x;
  rect.y = item->area.y;
  rect.width = rect.height = 0;
  for (lp = icon_view->priv->cell_list; lp != NULL; lp = lp->next)
    {
      ExoIconViewCellInfo *info = EXO_ICON_VIEW_CELL_INFO (lp->data);
      if (G_UNLIKELY (!gtk_cell_renderer_get_visible(info->cell)))
        continue;

      if (icon_view->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          rect.width += item->box[info->position].width + (info->position > 0 ? icon_view->priv->spacing : 0);
          rect.height = MAX (rect.height, item->box[info->position].height);
        }
      else
        {
          rect.width = MAX (rect.width, item->box[info->position].width);
          rect.height += item->box[info->position].height + (info->position > 0 ? icon_view->priv->spacing : 0);
        }
    }

  if (y + rect.y - focus_width < 0)
    gtk_adjustment_set_value (icon_view->priv->vadjustment,
                              gtk_adjustment_get_value(icon_view->priv->vadjustment) + y + rect.y - focus_width);
  else if (y + rect.y + rect.height + focus_width > allocation.height)
    gtk_adjustment_set_value (icon_view->priv->vadjustment,
                              gtk_adjustment_get_value(icon_view->priv->vadjustment) + y + rect.y + rect.height
                              + focus_width - allocation.height);

  if (x + rect.x - focus_width < 0)
    {
      gtk_adjustment_set_value (icon_view->priv->hadjustment,
                                gtk_adjustment_get_value(icon_view->priv->hadjustment) + x + rect.x - focus_width);
    }
  else if (x + rect.x + rect.width + focus_width > allocation.width
        && rect.width < allocation.width)
    {
      /* the second condition above is to make sure that we don't scroll horizontally if the item
       * width is larger than the allocation width. Fixes a weird scrolling bug in the compact view.
       * See http://bugzilla.xfce.org/show_bug.cgi?id=1683 for details.
       */

      gtk_adjustment_set_value (icon_view->priv->hadjustment,
                                gtk_adjustment_get_value(icon_view->priv->hadjustment) + x + rect.x + rect.width
                                + focus_width - allocation.width);
    }

  gtk_adjustment_changed (icon_view->priv->hadjustment);
  gtk_adjustment_changed (icon_view->priv->vadjustment);
}



static ExoIconViewCellInfo *
exo_icon_view_get_cell_info (ExoIconView     *icon_view,
                             GtkCellRenderer *renderer)
{
  GList *lp;

  for (lp = icon_view->priv->cell_list; lp != NULL; lp = lp->next)
    if (EXO_ICON_VIEW_CELL_INFO (lp->data)->cell == renderer)
      return lp->data;

  return NULL;
}



static void
exo_icon_view_set_cell_data (const ExoIconView *icon_view,
                             ExoIconViewItem   *item)
{
  ExoIconViewCellInfo *info;
  GtkTreePath         *path;
  GtkTreeIter          iter;
  GValue               value = {0, };
  GSList              *slp;
  GList               *lp;

  if (G_UNLIKELY (!EXO_ICON_VIEW_FLAG_SET (icon_view, EXO_ICON_VIEW_ITERS_PERSIST)))
    {
      path = gtk_tree_path_new_from_indices (item->index, -1);
      gtk_tree_model_get_iter (icon_view->priv->model, &iter, path);
      gtk_tree_path_free (path);
    }
  else
    {
      iter = item->iter;
    }

  for (lp = icon_view->priv->cell_list; lp != NULL; lp = lp->next)
    {
      info = EXO_ICON_VIEW_CELL_INFO (lp->data);

      for (slp = info->attributes; slp != NULL && slp->next != NULL; slp = slp->next->next)
        {
          gtk_tree_model_get_value (icon_view->priv->model, &iter, GPOINTER_TO_INT (slp->next->data), &value);
          g_object_set_property (G_OBJECT (info->cell), slp->data, &value);
          g_value_unset (&value);
        }

      if (G_UNLIKELY (info->func != NULL))
        (*info->func) (GTK_CELL_LAYOUT (icon_view), info->cell, icon_view->priv->model, &iter, info->func_data);
    }
}



static void
free_cell_attributes (ExoIconViewCellInfo *info)
{
  GSList *lp;

  for (lp = info->attributes; lp != NULL && lp->next != NULL; lp = lp->next->next)
    g_free (lp->data);
  g_slist_free (info->attributes);
  info->attributes = NULL;
}



static void
free_cell_info (ExoIconViewCellInfo *info)
{
  if (G_UNLIKELY (info->destroy != NULL))
    (*info->destroy) (info->func_data);

  free_cell_attributes (info);
  g_object_unref (G_OBJECT (info->cell));
  g_slice_free (ExoIconViewCellInfo, info);
}



static void
exo_icon_view_cell_layout_pack_start (GtkCellLayout   *layout,
                                      GtkCellRenderer *renderer,
                                      gboolean         expand)
{
  ExoIconViewCellInfo *info;
  ExoIconView         *icon_view = EXO_ICON_VIEW (layout);

  _exo_return_if_fail (GTK_IS_CELL_RENDERER (renderer));
  _exo_return_if_fail (exo_icon_view_get_cell_info (icon_view, renderer) == NULL);

  g_object_ref_sink (renderer);

  info = g_slice_new0 (ExoIconViewCellInfo);
  info->cell = renderer;
  info->expand = expand ? TRUE : FALSE;
  info->pack = GTK_PACK_START;
  info->position = icon_view->priv->n_cells;
  info->is_text = GTK_IS_CELL_RENDERER_TEXT (renderer);

  icon_view->priv->cell_list = g_list_append (icon_view->priv->cell_list, info);
  icon_view->priv->n_cells++;

  exo_icon_view_invalidate_sizes (icon_view);
}



static void
exo_icon_view_cell_layout_pack_end (GtkCellLayout   *layout,
                                    GtkCellRenderer *renderer,
                                    gboolean         expand)
{
  ExoIconViewCellInfo *info;
  ExoIconView         *icon_view = EXO_ICON_VIEW (layout);

  _exo_return_if_fail (GTK_IS_CELL_RENDERER (renderer));
  _exo_return_if_fail (exo_icon_view_get_cell_info (icon_view, renderer) == NULL);

  g_object_ref_sink (renderer);

  info = g_slice_new0 (ExoIconViewCellInfo);
  info->cell = renderer;
  info->expand = expand ? TRUE : FALSE;
  info->pack = GTK_PACK_END;
  info->position = icon_view->priv->n_cells;
  info->is_text = GTK_IS_CELL_RENDERER_TEXT (renderer);

  icon_view->priv->cell_list = g_list_append (icon_view->priv->cell_list, info);
  icon_view->priv->n_cells++;

  exo_icon_view_invalidate_sizes (icon_view);
}



static void
exo_icon_view_cell_layout_add_attribute (GtkCellLayout   *layout,
                                         GtkCellRenderer *renderer,
                                         const gchar     *attribute,
                                         gint             column)
{
  ExoIconViewCellInfo *info;

  info = exo_icon_view_get_cell_info (EXO_ICON_VIEW (layout), renderer);
  if (G_LIKELY (info != NULL))
    {
      info->attributes = g_slist_prepend (info->attributes, GINT_TO_POINTER (column));
      info->attributes = g_slist_prepend (info->attributes, g_strdup (attribute));

      exo_icon_view_invalidate_sizes (EXO_ICON_VIEW (layout));
    }
}



static void
exo_icon_view_cell_layout_clear (GtkCellLayout *layout)
{
  ExoIconView *icon_view = EXO_ICON_VIEW (layout);

  g_list_foreach (icon_view->priv->cell_list, (GFunc) free_cell_info, NULL);
  g_list_free (icon_view->priv->cell_list);
  icon_view->priv->cell_list = NULL;
  icon_view->priv->n_cells = 0;

  exo_icon_view_invalidate_sizes (icon_view);
}



static void
exo_icon_view_cell_layout_set_cell_data_func (GtkCellLayout         *layout,
                                              GtkCellRenderer       *cell,
                                              GtkCellLayoutDataFunc  func,
                                              gpointer               func_data,
                                              GDestroyNotify         destroy)
{
  ExoIconViewCellInfo *info;
  GDestroyNotify       notify;

  info = exo_icon_view_get_cell_info (EXO_ICON_VIEW (layout), cell);
  if (G_LIKELY (info != NULL))
    {
      if (G_UNLIKELY (info->destroy != NULL))
        {
          notify = info->destroy;
          info->destroy = NULL;
          (*notify) (info->func_data);
        }

      info->func = func;
      info->func_data = func_data;
      info->destroy = destroy;

      exo_icon_view_invalidate_sizes (EXO_ICON_VIEW (layout));
    }
}



static void
exo_icon_view_cell_layout_clear_attributes (GtkCellLayout   *layout,
                                            GtkCellRenderer *renderer)
{
  ExoIconViewCellInfo *info;

  info = exo_icon_view_get_cell_info (EXO_ICON_VIEW (layout), renderer);
  if (G_LIKELY (info != NULL))
    {
      free_cell_attributes (info);

      exo_icon_view_invalidate_sizes (EXO_ICON_VIEW (layout));
    }
}



static void
exo_icon_view_cell_layout_reorder (GtkCellLayout   *layout,
                                   GtkCellRenderer *cell,
                                   gint             position)
{
  ExoIconViewCellInfo *info;
  ExoIconView         *icon_view = EXO_ICON_VIEW (layout);
  GList               *lp;
  gint                 n;

  icon_view = EXO_ICON_VIEW (layout);

  info = exo_icon_view_get_cell_info (icon_view, cell);
  if (G_LIKELY (info != NULL))
    {
      lp = g_list_find (icon_view->priv->cell_list, info);

      icon_view->priv->cell_list = g_list_remove_link (icon_view->priv->cell_list, lp);
      icon_view->priv->cell_list = g_list_insert (icon_view->priv->cell_list, info, position);

      for (lp = icon_view->priv->cell_list, n = 0; lp != NULL; lp = lp->next, ++n)
        EXO_ICON_VIEW_CELL_INFO (lp->data)->position = n;

      exo_icon_view_invalidate_sizes (icon_view);
    }
}



/**
 * exo_icon_view_new:
 *
 * Creates a new #ExoIconView widget
 *
 * Return value: A newly created #ExoIconView widget
 **/
GtkWidget*
exo_icon_view_new (void)
{
  return g_object_new (EXO_TYPE_ICON_VIEW, NULL);
}



/**
 * exo_icon_view_new_with_model:
 * @model: The model.
 *
 * Creates a new #ExoIconView widget with the model @model.
 *
 * Return value: A newly created #ExoIconView widget.
 **/
GtkWidget*
exo_icon_view_new_with_model (GtkTreeModel *model)
{
  g_return_val_if_fail (model == NULL || GTK_IS_TREE_MODEL (model), NULL);

  return g_object_new (EXO_TYPE_ICON_VIEW,
                       "model", model,
                       NULL);
}



/**
 * exo_icon_view_widget_to_icon_coords:
 * @icon_view : a #ExoIconView.
 * @wx        : widget x coordinate.
 * @wy        : widget y coordinate.
 * @ix        : return location for icon x coordinate or %NULL.
 * @iy        : return location for icon y coordinate or %NULL.
 *
 * Converts widget coordinates to coordinates for the icon window
 * (the full scrollable area of the icon view).
 **/
void
exo_icon_view_widget_to_icon_coords (const ExoIconView *icon_view,
                                     gint               wx,
                                     gint               wy,
                                     gint              *ix,
                                     gint              *iy)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (ix != NULL))
    *ix = wx + gtk_adjustment_get_value(icon_view->priv->hadjustment);
  if (G_LIKELY (iy != NULL))
    *iy = wy + gtk_adjustment_get_value(icon_view->priv->vadjustment);
}



/**
 * exo_icon_view_icon_to_widget_coords:
 * @icon_view : a #ExoIconView.
 * @ix        : icon x coordinate.
 * @iy        : icon y coordinate.
 * @wx        : return location for widget x coordinate or %NULL.
 * @wy        : return location for widget y coordinate or %NULL.
 *
 * Converts icon view coordinates (coordinates in full scrollable
 * area of the icon view) to widget coordinates.
 **/
void
exo_icon_view_icon_to_widget_coords (const ExoIconView *icon_view,
                                     gint               ix,
                                     gint               iy,
                                     gint              *wx,
                                     gint              *wy)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (wx != NULL))
    *wx = ix - gtk_adjustment_get_value(icon_view->priv->hadjustment);
  if (G_LIKELY (wy != NULL))
    *wy = iy - gtk_adjustment_get_value(icon_view->priv->vadjustment);
}



/**
 * exo_icon_view_get_path_at_pos:
 * @icon_view : A #ExoIconView.
 * @x         : The x position to be identified
 * @y         : The y position to be identified
 *
 * Finds the path at the point (@x, @y), relative to widget coordinates.
 * See exo_icon_view_get_item_at_pos(), if you are also interested in
 * the cell at the specified position.
 *
 * Return value: The #GtkTreePath corresponding to the icon or %NULL
 *               if no icon exists at that position.
 **/
GtkTreePath*
exo_icon_view_get_path_at_pos (const ExoIconView *icon_view,
                               gint               x,
                               gint               y)
{
  ExoIconViewItem *item;

  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), NULL);

  /* translate the widget coordinates to icon window coordinates */
  x += gtk_adjustment_get_value(icon_view->priv->hadjustment);
  y += gtk_adjustment_get_value(icon_view->priv->vadjustment);

  item = exo_icon_view_get_item_at_coords (icon_view, x, y, TRUE, NULL);

  return (item != NULL) ? gtk_tree_path_new_from_indices (item->index, -1) : NULL;
}



/**
 * exo_icon_view_get_item_at_pos:
 * @icon_view: A #ExoIconView.
 * @x: The x position to be identified
 * @y: The y position to be identified
 * @path: Return location for the path, or %NULL
 * @cell: Return location for the renderer responsible for the cell
 *   at (@x, @y), or %NULL
 *
 * Finds the path at the point (@x, @y), relative to widget coordinates.
 * In contrast to exo_icon_view_get_path_at_pos(), this function also
 * obtains the cell at the specified position. The returned path should
 * be freed with gtk_tree_path_free().
 *
 * Return value: %TRUE if an item exists at the specified position
 *
 * Since: 0.3.1
 **/
gboolean
exo_icon_view_get_item_at_pos (const ExoIconView *icon_view,
                               gint               x,
                               gint               y,
                               GtkTreePath      **path,
                               GtkCellRenderer  **cell)
{
  ExoIconViewCellInfo *info = NULL;
  ExoIconViewItem     *item;

  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), FALSE);

  item = exo_icon_view_get_item_at_coords (icon_view, x, y, TRUE, &info);

  if (G_LIKELY (path != NULL))
    *path = (item != NULL) ? gtk_tree_path_new_from_indices (item->index, -1) : NULL;

  if (G_LIKELY (cell != NULL))
    *cell = (info != NULL) ? info->cell : NULL;

  return (item != NULL);
}



/**
 * exo_icon_view_get_visible_range:
 * @icon_view  : A #ExoIconView
 * @start_path : Return location for start of region, or %NULL
 * @end_path   : Return location for end of region, or %NULL
 *
 * Sets @start_path and @end_path to be the first and last visible path.
 * Note that there may be invisible paths in between.
 *
 * Both paths should be freed with gtk_tree_path_free() after use.
 *
 * Return value: %TRUE, if valid paths were placed in @start_path and @end_path
 *
 * Since: 0.3.1
 **/
gboolean
exo_icon_view_get_visible_range (const ExoIconView *icon_view,
                                 GtkTreePath      **start_path,
                                 GtkTreePath      **end_path)
{
  const ExoIconViewPrivate *priv = icon_view->priv;
  const ExoIconViewItem    *item;
  const GList              *lp;
  gint                      start_index = -1;
  gint                      end_index = -1;
  gint                      i;

  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), FALSE);

  if (priv->hadjustment == NULL || priv->vadjustment == NULL)
    return FALSE;

  if (start_path == NULL && end_path == NULL)
    return FALSE;

  for (i = 0, lp = priv->items; lp != NULL; ++i, lp = lp->next)
    {
      item = (const ExoIconViewItem *) lp->data;
      if ((item->area.x + item->area.width >= (gint) gtk_adjustment_get_value(priv->hadjustment)) &&
          (item->area.y + item->area.height >= (gint) gtk_adjustment_get_value(priv->vadjustment)) &&
          (item->area.x <= (gint) (gtk_adjustment_get_value(priv->hadjustment) + gtk_adjustment_get_page_size(priv->hadjustment))) &&
          (item->area.y <= (gint) (gtk_adjustment_get_value(priv->vadjustment) + gtk_adjustment_get_page_size(priv->vadjustment))))
        {
          if (start_index == -1)
            start_index = i;
          end_index = i;
        }
    }

  if (start_path != NULL && start_index != -1)
    *start_path = gtk_tree_path_new_from_indices (start_index, -1);
  if (end_path != NULL && end_index != -1)
    *end_path = gtk_tree_path_new_from_indices (end_index, -1);

  return (start_index != -1);
}



/**
 * exo_icon_view_selected_foreach:
 * @icon_view : A #ExoIconView.
 * @func      : The funcion to call for each selected icon.
 * @data      : User data to pass to the function.
 *
 * Calls a function for each selected icon. Note that the model or
 * selection cannot be modified from within this function.
 **/
void
exo_icon_view_selected_foreach (ExoIconView           *icon_view,
                                ExoIconViewForeachFunc func,
                                gpointer               data)
{
  GtkTreePath *path;
  GList       *lp;

  path = gtk_tree_path_new_first ();
  for (lp = icon_view->priv->items; lp != NULL; lp = lp->next)
    {
      if (EXO_ICON_VIEW_ITEM (lp->data)->selected)
        (*func) (icon_view, path, data);
      gtk_tree_path_next (path);
    }
  gtk_tree_path_free (path);
}



/**
 * exo_icon_view_get_selection_mode:
 * @icon_view : A #ExoIconView.
 *
 * Gets the selection mode of the @icon_view.
 *
 * Return value: the current selection mode
 **/
GtkSelectionMode
exo_icon_view_get_selection_mode (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), GTK_SELECTION_SINGLE);
  return icon_view->priv->selection_mode;
}



/**
 * exo_icon_view_set_selection_mode:
 * @icon_view : A #ExoIconView.
 * @mode      : The selection mode
 *
 * Sets the selection mode of the @icon_view.
 **/
void
exo_icon_view_set_selection_mode (ExoIconView      *icon_view,
                                  GtkSelectionMode  mode)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (mode != icon_view->priv->selection_mode))
    {
      if (mode == GTK_SELECTION_NONE || icon_view->priv->selection_mode == GTK_SELECTION_MULTIPLE)
        exo_icon_view_unselect_all (icon_view);

      icon_view->priv->selection_mode = mode;

      g_object_notify (G_OBJECT (icon_view), "selection-mode");
    }
}



/**
 * exo_icon_view_get_layout_mode:
 * @icon_view : A #ExoIconView.
 *
 * Returns the #ExoIconViewLayoutMode used to layout the
 * items in the @icon_view.
 *
 * Return value: the layout mode of @icon_view.
 *
 * Since: 0.3.1.5
 **/
ExoIconViewLayoutMode
exo_icon_view_get_layout_mode (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), EXO_ICON_VIEW_LAYOUT_ROWS);
  return icon_view->priv->layout_mode;
}



/**
 * exo_icon_view_set_layout_mode:
 * @icon_view   : a #ExoIconView.
 * @layout_mode : the new #ExoIconViewLayoutMode for @icon_view.
 *
 * Sets the layout mode of @icon_view to @layout_mode.
 *
 * Since: 0.3.1.5
 **/
void
exo_icon_view_set_layout_mode (ExoIconView          *icon_view,
                               ExoIconViewLayoutMode layout_mode)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  /* check if we have a new setting */
  if (G_LIKELY (icon_view->priv->layout_mode != layout_mode))
    {
      /* apply the new setting */
      icon_view->priv->layout_mode = layout_mode;

      /* cancel any active cell editor */
      exo_icon_view_stop_editing (icon_view, TRUE);

      /* invalidate the current item sizes */
      exo_icon_view_invalidate_sizes (icon_view);
      exo_icon_view_queue_layout (icon_view);

      /* notify listeners */
      g_object_notify (G_OBJECT (icon_view), "layout-mode");
    }
}



/**
 * exo_icon_view_get_model:
 * @icon_view : a #ExoIconView
 *
 * Returns the model the #ExoIconView is based on. Returns %NULL if the
 * model is unset.
 *
 * Return value: A #GtkTreeModel, or %NULL if none is currently being used.
 **/
GtkTreeModel*
exo_icon_view_get_model (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), NULL);
  return icon_view->priv->model;
}



/**
 * exo_icon_view_set_model:
 * @icon_view : A #ExoIconView.
 * @model     : The model.
 *
 * Sets the model for a #ExoIconView.
 * If the @icon_view already has a model set, it will remove
 * it before setting the new model.  If @model is %NULL, then
 * it will unset the old model.
 **/
void
exo_icon_view_set_model (ExoIconView  *icon_view,
                         GtkTreeModel *model)
{
  ExoIconViewItem *item;
  GtkTreeIter      iter;
  GList           *items = NULL;
  GList           *lp;
  gint             n;

  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));
  g_return_if_fail (model == NULL || GTK_IS_TREE_MODEL (model));

  /* verify that we don't already use that model */
  if (G_UNLIKELY (icon_view->priv->model == model))
    return;

  /* verify the new model */
  if (G_LIKELY (model != NULL))
    {
      g_return_if_fail (gtk_tree_model_get_flags (model) & GTK_TREE_MODEL_LIST_ONLY);

      if (G_UNLIKELY (icon_view->priv->pixbuf_column != -1))
        g_return_if_fail (gtk_tree_model_get_column_type (model, icon_view->priv->pixbuf_column) == GDK_TYPE_PIXBUF);

      if (G_UNLIKELY (icon_view->priv->text_column != -1))
        g_return_if_fail (gtk_tree_model_get_column_type (model, icon_view->priv->text_column) == G_TYPE_STRING);

      if (G_UNLIKELY (icon_view->priv->markup_column != -1))
        g_return_if_fail (gtk_tree_model_get_column_type (model, icon_view->priv->markup_column) == G_TYPE_STRING);
    }

  /* be sure to cancel any pending editor */
  exo_icon_view_stop_editing (icon_view, TRUE);

  /* disconnect from the previous model */
  if (G_LIKELY (icon_view->priv->model != NULL))
    {
      /* disconnect signals handlers from the previous model */
      g_signal_handlers_disconnect_by_func (G_OBJECT (icon_view->priv->model), exo_icon_view_row_changed, icon_view);
      g_signal_handlers_disconnect_by_func (G_OBJECT (icon_view->priv->model), exo_icon_view_row_inserted, icon_view);
      g_signal_handlers_disconnect_by_func (G_OBJECT (icon_view->priv->model), exo_icon_view_row_deleted, icon_view);
      g_signal_handlers_disconnect_by_func (G_OBJECT (icon_view->priv->model), exo_icon_view_rows_reordered, icon_view);

      /* release our reference on the model */
      g_object_unref (G_OBJECT (icon_view->priv->model));

      /* drop all items belonging to the previous model */
      for (lp = icon_view->priv->items; lp != NULL; lp = lp->next)
        {
          g_free (EXO_ICON_VIEW_ITEM (lp->data)->box);
          g_slice_free (ExoIconViewItem, lp->data);
        }
      g_list_free (icon_view->priv->items);
      icon_view->priv->items = NULL;

      /* reset statistics */
      icon_view->priv->search_column = -1;
      icon_view->priv->anchor_item = NULL;
      icon_view->priv->cursor_item = NULL;
      icon_view->priv->prelit_item = NULL;
      icon_view->priv->last_single_clicked = NULL;
      icon_view->priv->width = 0;
      icon_view->priv->height = 0;

      /* cancel any pending single click timer */
      if (G_UNLIKELY (icon_view->priv->single_click_timeout_id != 0))
        g_source_remove (icon_view->priv->single_click_timeout_id);

      /* reset cursor when in single click mode and realized */
      if (G_UNLIKELY (icon_view->priv->single_click && gtk_widget_get_realized (GTK_WIDGET(icon_view))))        gdk_window_set_cursor (icon_view->priv->bin_window, NULL);
    }

  /* be sure to drop any previous scroll_to_path reference,
   * as it points to the old (no longer valid) model.
   */
  if (G_UNLIKELY (icon_view->priv->scroll_to_path != NULL))
    {
      gtk_tree_row_reference_free (icon_view->priv->scroll_to_path);
      icon_view->priv->scroll_to_path = NULL;
    }

  /* activate the new model */
  icon_view->priv->model = model;

  /* connect to the new model */
  if (G_LIKELY (model != NULL))
    {
      /* take a reference on the model */
      g_object_ref (G_OBJECT (model));

      /* connect signals */
      g_signal_connect (G_OBJECT (model), "row-changed", G_CALLBACK (exo_icon_view_row_changed), icon_view);
      g_signal_connect (G_OBJECT (model), "row-inserted", G_CALLBACK (exo_icon_view_row_inserted), icon_view);
      g_signal_connect (G_OBJECT (model), "row-deleted", G_CALLBACK (exo_icon_view_row_deleted), icon_view);
      g_signal_connect (G_OBJECT (model), "rows-reordered", G_CALLBACK (exo_icon_view_rows_reordered), icon_view);

      /* check if the new model supports persistent iterators */
      if (gtk_tree_model_get_flags (model) & GTK_TREE_MODEL_ITERS_PERSIST)
        EXO_ICON_VIEW_SET_FLAG (icon_view, EXO_ICON_VIEW_ITERS_PERSIST);
      else
        EXO_ICON_VIEW_UNSET_FLAG (icon_view, EXO_ICON_VIEW_ITERS_PERSIST);

      /* determine an appropriate search column */
      if (icon_view->priv->search_column <= 0)
        {
          /* we simply use the first string column */
          for (n = 0; n < gtk_tree_model_get_n_columns (model); ++n)
            if (g_value_type_transformable (gtk_tree_model_get_column_type (model, n), G_TYPE_STRING))
              {
                icon_view->priv->search_column = n;
                break;
              }
        }

      /* build up the initial items list */
      if (gtk_tree_model_get_iter_first (model, &iter))
        {
          n = 0;
          do
            {
              item = g_slice_new0 (ExoIconViewItem);
              item->iter = iter;
              item->area.width = -1;
              item->index = n++;
              items = g_list_prepend (items, item);
            }
          while (gtk_tree_model_iter_next (model, &iter));
        }
      icon_view->priv->items = g_list_reverse (items);

      /* layout the new items */
      exo_icon_view_queue_layout (icon_view);
    }

  /* hide the interactive search dialog (if any) */
  if (G_LIKELY (icon_view->priv->search_window != NULL))
    exo_icon_view_search_dialog_hide (icon_view->priv->search_window, icon_view);

  /* notify listeners */
  g_object_notify (G_OBJECT (icon_view), "model");

  if (gtk_widget_get_realized (GTK_WIDGET (icon_view)))
    gtk_widget_queue_resize (GTK_WIDGET (icon_view));
}



static void
update_text_cell (ExoIconView *icon_view)
{
  ExoIconViewCellInfo *info;
  GList *l;
  gint i;

  if (icon_view->priv->text_column == -1 &&
      icon_view->priv->markup_column == -1)
    {
      if (icon_view->priv->text_cell != -1)
        {
          info = g_list_nth_data (icon_view->priv->cell_list,
                                  icon_view->priv->text_cell);

          icon_view->priv->cell_list = g_list_remove (icon_view->priv->cell_list, info);

          free_cell_info (info);

          icon_view->priv->n_cells--;
          icon_view->priv->text_cell = -1;
        }
    }
  else
    {
      if (icon_view->priv->text_cell == -1)
        {
          GtkCellRenderer *cell = gtk_cell_renderer_text_new ();
          gtk_cell_layout_pack_end (GTK_CELL_LAYOUT (icon_view), cell, FALSE);
          for (l = icon_view->priv->cell_list, i = 0; l; l = l->next, i++)
            {
              info = l->data;
              if (info->cell == cell)
                {
                  icon_view->priv->text_cell = i;
                  break;
                }
            }
        }

      info = g_list_nth_data (icon_view->priv->cell_list,
                              icon_view->priv->text_cell);

      if (icon_view->priv->markup_column != -1)
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view),
                                        info->cell,
                                        "markup", icon_view->priv->markup_column,
                                        NULL);
      else
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view),
                                        info->cell,
                                        "text", icon_view->priv->text_column,
                                        NULL);
    }
}

static void
update_pixbuf_cell (ExoIconView *icon_view)
{
  ExoIconViewCellInfo *info;
  GList *l;
  gint i;

  if (icon_view->priv->pixbuf_column == -1)
    {
      if (icon_view->priv->pixbuf_cell != -1)
        {
          info = g_list_nth_data (icon_view->priv->cell_list,
                                  icon_view->priv->pixbuf_cell);

          icon_view->priv->cell_list = g_list_remove (icon_view->priv->cell_list, info);

          free_cell_info (info);

          icon_view->priv->n_cells--;
          icon_view->priv->pixbuf_cell = -1;
        }
    }
  else
    {
      if (icon_view->priv->pixbuf_cell == -1)
        {
          GtkCellRenderer *cell = gtk_cell_renderer_pixbuf_new ();

          gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view), cell, FALSE);
          for (l = icon_view->priv->cell_list, i = 0; l; l = l->next, i++)
            {
              info = l->data;
              if (info->cell == cell)
                {
                  icon_view->priv->pixbuf_cell = i;
                  break;
                }
            }
        }

        info = g_list_nth_data (icon_view->priv->cell_list,
                                icon_view->priv->pixbuf_cell);

        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view),
                                        info->cell,
                                        "pixbuf", icon_view->priv->pixbuf_column,
                                        NULL);
    }
}



/**
 * exo_icon_view_select_path:
 * @icon_view : A #ExoIconView.
 * @path      : The #GtkTreePath to be selected.
 *
 * Selects the row at @path.
 **/
void
exo_icon_view_select_path (ExoIconView *icon_view,
                           GtkTreePath *path)
{
  ExoIconViewItem *item;

  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));
  g_return_if_fail (icon_view->priv->model != NULL);
  g_return_if_fail (gtk_tree_path_get_depth (path) > 0);

  item = g_list_nth_data (icon_view->priv->items, gtk_tree_path_get_indices(path)[0]);
  if (G_LIKELY (item != NULL))
    exo_icon_view_select_item (icon_view, item);
}



/**
 * exo_icon_view_unselect_path:
 * @icon_view : A #ExoIconView.
 * @path      : The #GtkTreePath to be unselected.
 *
 * Unselects the row at @path.
 **/
void
exo_icon_view_unselect_path (ExoIconView *icon_view,
                             GtkTreePath *path)
{
  ExoIconViewItem *item;

  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));
  g_return_if_fail (icon_view->priv->model != NULL);
  g_return_if_fail (gtk_tree_path_get_depth (path) > 0);

  item = g_list_nth_data (icon_view->priv->items, gtk_tree_path_get_indices(path)[0]);
  if (G_LIKELY (item != NULL))
    exo_icon_view_unselect_item (icon_view, item);
}



/**
 * exo_icon_view_get_selected_items:
 * @icon_view: A #ExoIconView.
 *
 * Creates a list of paths of all selected items. Additionally, if you are
 * planning on modifying the model after calling this function, you may
 * want to convert the returned list into a list of #GtkTreeRowReference<!-- -->s.
 * To do this, you can use gtk_tree_row_reference_new().
 *
 * To free the return value, use:
 * <informalexample><programlisting>
 * g_list_foreach (list, gtk_tree_path_free, NULL);
 * g_list_free (list);
 * </programlisting></informalexample>
 *
 * Return value: A #GList containing a #GtkTreePath for each selected row.
 **/
GList*
exo_icon_view_get_selected_items (const ExoIconView *icon_view)
{
  GList *selected = NULL;
  GList *lp;
  gint   i;

  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), NULL);

  for (i = 0, lp = icon_view->priv->items; lp != NULL; ++i, lp = lp->next)
    {
      if (EXO_ICON_VIEW_ITEM (lp->data)->selected)
        selected = g_list_append (selected, gtk_tree_path_new_from_indices (i, -1));
    }

  return selected;
}



gint exo_icon_view_count_selected_items (const ExoIconView *icon_view)
{
  GList *lp;
  gint   i = 0;

  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), 0);

  for (lp = icon_view->priv->items; lp != NULL; lp = lp->next)
    {
      if (EXO_ICON_VIEW_ITEM (lp->data)->selected)
        i++;
    }

  return i;
}



/**
 * exo_icon_view_select_all:
 * @icon_view : A #ExoIconView.
 *
 * Selects all the icons. @icon_view must has its selection mode set
 * to #GTK_SELECTION_MULTIPLE.
 **/
void
exo_icon_view_select_all (ExoIconView *icon_view)
{
  GList *items;
  gboolean dirty = FALSE;

  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->selection_mode != GTK_SELECTION_MULTIPLE)
    return;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      ExoIconViewItem *item = items->data;

      if (!item->selected)
        {
          dirty = TRUE;
          item->selected = TRUE;
          exo_icon_view_queue_draw_item (icon_view, item);
        }
    }

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



/**
 * exo_icon_view_unselect_all:
 * @icon_view : A #ExoIconView.
 *
 * Unselects all the icons.
 **/
void
exo_icon_view_unselect_all (ExoIconView *icon_view)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (G_UNLIKELY (icon_view->priv->selection_mode == GTK_SELECTION_BROWSE))
    return;

  if (exo_icon_view_unselect_all_internal (icon_view))
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



/**
 * exo_icon_view_path_is_selected:
 * @icon_view: A #ExoIconView.
 * @path: A #GtkTreePath to check selection on.
 *
 * Returns %TRUE if the icon pointed to by @path is currently
 * selected. If @icon does not point to a valid location, %FALSE is returned.
 *
 * Return value: %TRUE if @path is selected.
 **/
gboolean
exo_icon_view_path_is_selected (const ExoIconView *icon_view,
                                GtkTreePath       *path)
{
  ExoIconViewItem *item;

  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (icon_view->priv->model != NULL, FALSE);
  g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, FALSE);

  item = g_list_nth_data (icon_view->priv->items, gtk_tree_path_get_indices(path)[0]);

  return (item != NULL && item->selected);
}



/**
 * exo_icon_view_item_activated:
 * @icon_view : a #ExoIconView
 * @path      : the #GtkTreePath to be activated
 *
 * Activates the item determined by @path.
 **/
void
exo_icon_view_item_activated (ExoIconView *icon_view,
                              GtkTreePath *path)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));
  g_return_if_fail (gtk_tree_path_get_depth (path) > 0);

  g_signal_emit (icon_view, icon_view_signals[ITEM_ACTIVATED], 0, path);
}



/**
 * exo_icon_view_get_cursor:
 * @icon_view : A #ExoIconView
 * @path      : Return location for the current cursor path, or %NULL
 * @cell      : Return location the current focus cell, or %NULL
 *
 * Fills in @path and @cell with the current cursor path and cell.
 * If the cursor isn't currently set, then *@path will be %NULL.
 * If no cell currently has focus, then *@cell will be %NULL.
 *
 * The returned #GtkTreePath must be freed with gtk_tree_path_free().
 *
 * Return value: %TRUE if the cursor is set.
 *
 * Since: 0.3.1
 **/
gboolean
exo_icon_view_get_cursor (const ExoIconView *icon_view,
                          GtkTreePath      **path,
                          GtkCellRenderer  **cell)
{
  ExoIconViewCellInfo *info;
  ExoIconViewItem     *item;

  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), FALSE);

  item = icon_view->priv->cursor_item;
  info = (icon_view->priv->cursor_cell < 0) ? NULL : g_list_nth_data (icon_view->priv->cell_list, icon_view->priv->cursor_cell);

  if (G_LIKELY (path != NULL))
    *path = (item != NULL) ? gtk_tree_path_new_from_indices (item->index, -1) : NULL;

  if (G_LIKELY (cell != NULL))
    *cell = (info != NULL) ? info->cell : NULL;

  return (item != NULL);
}



/**
 * exo_icon_view_set_cursor:
 * @icon_view     : a #ExoIconView
 * @path          : a #GtkTreePath
 * @cell          : a #GtkCellRenderer or %NULL
 * @start_editing : %TRUE if the specified cell should start being edited.
 *
 * Sets the current keyboard focus to be at @path, and selects it.  This is
 * useful when you want to focus the user's attention on a particular item.
 * If @cell is not %NULL, then focus is given to the cell specified by
 * it. Additionally, if @start_editing is %TRUE, then editing should be
 * started in the specified cell.
 *
 * This function is often followed by <literal>gtk_widget_grab_focus
 * (icon_view)</literal> in order to give keyboard focus to the widget.
 * Please note that editing can only happen when the widget is realized.
 *
 * Since: 0.3.1
 **/
void
exo_icon_view_set_cursor (ExoIconView     *icon_view,
                          GtkTreePath     *path,
                          GtkCellRenderer *cell,
                          gboolean         start_editing)
{
  ExoIconViewItem *item;
  ExoIconViewCellInfo *info =  NULL;
  GList *l;
  gint i, cell_pos;

  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));
  g_return_if_fail (path != NULL);
  g_return_if_fail (cell == NULL || GTK_IS_CELL_RENDERER (cell));

  exo_icon_view_stop_editing (icon_view, TRUE);

  item = g_list_nth_data (icon_view->priv->items, gtk_tree_path_get_indices(path)[0]);
  if (G_UNLIKELY (item == NULL))
    return;

  cell_pos = -1;
  for (l = icon_view->priv->cell_list, i = 0; l; l = l->next, i++)
    {
      info = l->data;

      if (info->cell == cell)
        {
          cell_pos = i;
          break;
        }

      info = NULL;
    }

  /* place the cursor on the item */
  exo_icon_view_set_cursor_item (icon_view, item, cell_pos);
  icon_view->priv->anchor_item = item;

  /* scroll to the item (maybe delayed) */
  exo_icon_view_scroll_to_path (icon_view, path, FALSE, 0.0f, 0.0f);

  if (start_editing)
    exo_icon_view_start_editing (icon_view, item, info, NULL);
}



/**
 * exo_icon_view_scroll_to_path:
 * @icon_view: A #ExoIconView.
 * @path: The path of the item to move to.
 * @use_align: whether to use alignment arguments, or %FALSE.
 * @row_align: The vertical alignment of the item specified by @path.
 * @col_align: The horizontal alignment of the item specified by @column.
 *
 * Moves the alignments of @icon_view to the position specified by @path.
 * @row_align determines where the row is placed, and @col_align determines where
 * @column is placed.  Both are expected to be between 0.0 and 1.0.
 * 0.0 means left/top alignment, 1.0 means right/bottom alignment, 0.5 means center.
 *
 * If @use_align is %FALSE, then the alignment arguments are ignored, and the
 * tree does the minimum amount of work to scroll the item onto the screen.
 * This means that the item will be scrolled to the edge closest to its current
 * position.  If the item is currently visible on the screen, nothing is done.
 *
 * This function only works if the model is set, and @path is a valid row on the
 * model.  If the model changes before the @tree_view is realized, the centered
 * path will be modified to reflect this change.
 *
 * Since: 0.3.1
 **/
void
exo_icon_view_scroll_to_path (ExoIconView *icon_view,
                              GtkTreePath *path,
                              gboolean     use_align,
                              gfloat       row_align,
                              gfloat       col_align)
{
  ExoIconViewItem *item;

  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));
  g_return_if_fail (gtk_tree_path_get_depth (path) > 0);
  g_return_if_fail (row_align >= 0.0 && row_align <= 1.0);
  g_return_if_fail (col_align >= 0.0 && col_align <= 1.0);

  /* Delay scrolling if either not realized or pending layout() */
  if (!gtk_widget_get_realized (GTK_WIDGET (icon_view)) || icon_view->priv->layout_idle_id != 0)
    {
      /* release the previous scroll_to_path reference */
      if (G_UNLIKELY (icon_view->priv->scroll_to_path != NULL))
        gtk_tree_row_reference_free (icon_view->priv->scroll_to_path);

      /* remember a reference for the new path and settings */
      icon_view->priv->scroll_to_path = gtk_tree_row_reference_new_proxy (G_OBJECT (icon_view), icon_view->priv->model, path);
      icon_view->priv->scroll_to_use_align = use_align;
      icon_view->priv->scroll_to_row_align = row_align;
      icon_view->priv->scroll_to_col_align = col_align;
    }
  else
    {
      item = g_list_nth_data (icon_view->priv->items, gtk_tree_path_get_indices(path)[0]);
      if (G_UNLIKELY (item == NULL))
        return;

      if (use_align)
        {
          gint x, y;
          gint focus_width;
          gfloat offset, value;
          GtkAllocation allocation;

          gtk_widget_style_get (GTK_WIDGET (icon_view),
                                "focus-line-width", &focus_width,
                                NULL);
          gtk_widget_get_allocation (GTK_WIDGET (icon_view), &allocation);

          gdk_window_get_position (icon_view->priv->bin_window, &x, &y);

          offset =  y + item->area.y - focus_width -
            row_align * (allocation.height - item->area.height);
          value = CLAMP (gtk_adjustment_get_value(icon_view->priv->vadjustment) + offset,
                         gtk_adjustment_get_lower(icon_view->priv->vadjustment),
                         gtk_adjustment_get_upper(icon_view->priv->vadjustment) - gtk_adjustment_get_page_size(icon_view->priv->vadjustment));
          gtk_adjustment_set_value (icon_view->priv->vadjustment, value);

          offset = x + item->area.x - focus_width -
            col_align * (allocation.width - item->area.width);
          value = CLAMP (gtk_adjustment_get_value(icon_view->priv->hadjustment) + offset,
                         gtk_adjustment_get_lower(icon_view->priv->hadjustment),
                         gtk_adjustment_get_upper(icon_view->priv->hadjustment) - gtk_adjustment_get_page_size(icon_view->priv->hadjustment));
          gtk_adjustment_set_value (icon_view->priv->hadjustment, value);

          gtk_adjustment_changed (icon_view->priv->hadjustment);
          gtk_adjustment_changed (icon_view->priv->vadjustment);
        }
      else
        {
          exo_icon_view_scroll_to_item (icon_view, item);
        }
    }
}



/**
 * exo_icon_view_get_orientation:
 * @icon_view : a #ExoIconView
 *
 * Returns the value of the ::orientation property which determines
 * whether the labels are drawn beside the icons instead of below.
 *
 * Return value: the relative position of texts and icons
 *
 * Since: 0.3.1
 **/
GtkOrientation
exo_icon_view_get_orientation (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), GTK_ORIENTATION_VERTICAL);
  return icon_view->priv->orientation;
}



/**
 * exo_icon_view_set_orientation:
 * @icon_view   : a #ExoIconView
 * @orientation : the relative position of texts and icons
 *
 * Sets the ::orientation property which determines whether the labels
 * are drawn beside the icons instead of below.
 *
 * Since: 0.3.1
 **/
void
exo_icon_view_set_orientation (ExoIconView   *icon_view,
                               GtkOrientation orientation)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (icon_view->priv->orientation != orientation))
    {
      icon_view->priv->orientation = orientation;

      exo_icon_view_stop_editing (icon_view, TRUE);
      exo_icon_view_invalidate_sizes (icon_view);

      update_text_cell (icon_view);
      update_pixbuf_cell (icon_view);

      g_object_notify (G_OBJECT (icon_view), "orientation");
    }
}



/**
 * exo_icon_view_get_columns:
 * @icon_view: a #ExoIconView
 *
 * Returns the value of the ::columns property.
 *
 * Return value: the number of columns, or -1
 */
gint
exo_icon_view_get_columns (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), -1);
  return icon_view->priv->columns;
}



/**
 * exo_icon_view_set_columns:
 * @icon_view : a #ExoIconView
 * @columns   : the number of columns
 *
 * Sets the ::columns property which determines in how
 * many columns the icons are arranged. If @columns is
 * -1, the number of columns will be chosen automatically
 * to fill the available area.
 *
 * Since: 0.3.1
 */
void
exo_icon_view_set_columns (ExoIconView *icon_view,
                           gint         columns)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (icon_view->priv->columns != columns))
    {
      icon_view->priv->columns = columns;

      exo_icon_view_stop_editing (icon_view, TRUE);
      exo_icon_view_queue_layout (icon_view);

      g_object_notify (G_OBJECT (icon_view), "columns");
    }
}



/**
 * exo_icon_view_get_item_width:
 * @icon_view: a #ExoIconView
 *
 * Returns the value of the ::item-width property.
 *
 * Return value: the width of a single item, or -1
 *
 * Since: 0.3.1
 */
gint
exo_icon_view_get_item_width (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), -1);
  return icon_view->priv->item_width;
}



/**
 * exo_icon_view_set_item_width:
 * @icon_view  : a #ExoIconView
 * @item_width : the width for each item
 *
 * Sets the ::item-width property which specifies the width
 * to use for each item. If it is set to -1, the icon view will
 * automatically determine a suitable item size.
 *
 * Since: 0.3.1
 */
void
exo_icon_view_set_item_width (ExoIconView *icon_view,
                              gint         item_width)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->item_width != item_width)
    {
      icon_view->priv->item_width = item_width;

      exo_icon_view_stop_editing (icon_view, TRUE);
      exo_icon_view_invalidate_sizes (icon_view);

      update_text_cell (icon_view);

      g_object_notify (G_OBJECT (icon_view), "item-width");
    }
}



/**
 * exo_icon_view_get_spacing:
 * @icon_view: a #ExoIconView
 *
 * Returns the value of the ::spacing property.
 *
 * Return value: the space between cells
 *
 * Since: 0.3.1
 */
gint
exo_icon_view_get_spacing (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), -1);
  return icon_view->priv->spacing;
}



/**
 * exo_icon_view_set_spacing:
 * @icon_view : a #ExoIconView
 * @spacing   : the spacing
 *
 * Sets the ::spacing property which specifies the space
 * which is inserted between the cells (i.e. the icon and
 * the text) of an item.
 *
 * Since: 0.3.1
 */
void
exo_icon_view_set_spacing (ExoIconView *icon_view,
                           gint         spacing)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (icon_view->priv->spacing != spacing))
    {
      icon_view->priv->spacing = spacing;

      exo_icon_view_stop_editing (icon_view, TRUE);
      exo_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "spacing");
    }
}



/**
 * exo_icon_view_get_row_spacing:
 * @icon_view: a #ExoIconView
 *
 * Returns the value of the ::row-spacing property.
 *
 * Return value: the space between rows
 *
 * Since: 0.3.1
 */
gint
exo_icon_view_get_row_spacing (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), -1);
  return icon_view->priv->row_spacing;
}



/**
 * exo_icon_view_set_row_spacing:
 * @icon_view   : a #ExoIconView
 * @row_spacing : the row spacing
 *
 * Sets the ::row-spacing property which specifies the space
 * which is inserted between the rows of the icon view.
 *
 * Since: 0.3.1
 */
void
exo_icon_view_set_row_spacing (ExoIconView *icon_view,
                               gint         row_spacing)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (icon_view->priv->row_spacing != row_spacing))
    {
      icon_view->priv->row_spacing = row_spacing;

      exo_icon_view_stop_editing (icon_view, TRUE);
      exo_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "row-spacing");
    }
}



/**
 * exo_icon_view_get_column_spacing:
 * @icon_view: a #ExoIconView
 *
 * Returns the value of the ::column-spacing property.
 *
 * Return value: the space between columns
 *
 * Since: 0.3.1
 **/
gint
exo_icon_view_get_column_spacing (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), -1);
  return icon_view->priv->column_spacing;
}



/**
 * exo_icon_view_set_column_spacing:
 * @icon_view      : a #ExoIconView
 * @column_spacing : the column spacing
 *
 * Sets the ::column-spacing property which specifies the space
 * which is inserted between the columns of the icon view.
 *
 * Since: 0.3.1
 **/
void
exo_icon_view_set_column_spacing (ExoIconView *icon_view,
                                  gint         column_spacing)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (icon_view->priv->column_spacing != column_spacing))
    {
      icon_view->priv->column_spacing = column_spacing;

      exo_icon_view_stop_editing (icon_view, TRUE);
      exo_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "column-spacing");
    }
}



/**
 * exo_icon_view_get_margin:
 * @icon_view : a #ExoIconView
 *
 * Returns the value of the ::margin property.
 *
 * Return value: the space at the borders
 *
 * Since: 0.3.1
 **/
gint
exo_icon_view_get_margin (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), -1);
  return icon_view->priv->margin;
}



/**
 * exo_icon_view_set_margin:
 * @icon_view : a #ExoIconView
 * @margin    : the margin
 *
 * Sets the ::margin property which specifies the space
 * which is inserted at the top, bottom, left and right
 * of the icon view.
 *
 * Since: 0.3.1
 **/
void
exo_icon_view_set_margin (ExoIconView *icon_view,
                          gint         margin)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (icon_view->priv->margin != margin))
    {
      icon_view->priv->margin = margin;

      exo_icon_view_stop_editing (icon_view, TRUE);
      exo_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "margin");
    }
}



/* Get/set whether drag_motion requested the drag data and
 * drag_data_received should thus not actually insert the data,
 * since the data doesn't result from a drop.
 */
static void
set_status_pending (GdkDragContext *context,
                    GdkDragAction   suggested_action)
{
  g_object_set_data (G_OBJECT (context),
                     I_("exo-icon-view-status-pending"),
                     GINT_TO_POINTER (suggested_action));
}

static GdkDragAction
get_status_pending (GdkDragContext *context)
{
  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (context), I_("exo-icon-view-status-pending")));
}

static void
unset_reorderable (ExoIconView *icon_view)
{
  if (icon_view->priv->reorderable)
    {
      icon_view->priv->reorderable = FALSE;
      g_object_notify (G_OBJECT (icon_view), "reorderable");
    }
}

static void
clear_source_info (ExoIconView *icon_view)
{
  if (icon_view->priv->source_targets)
    gtk_target_list_unref (icon_view->priv->source_targets);
  icon_view->priv->source_targets = NULL;

  icon_view->priv->source_set = FALSE;
}

static void
clear_dest_info (ExoIconView *icon_view)
{
  if (icon_view->priv->dest_targets)
    gtk_target_list_unref (icon_view->priv->dest_targets);
  icon_view->priv->dest_targets = NULL;

  icon_view->priv->dest_set = FALSE;
}

static void
set_source_row (GdkDragContext *context,
                GtkTreeModel   *model,
                GtkTreePath    *source_row)
{
  if (source_row)
    g_object_set_data_full (G_OBJECT (context),
                            I_("exo-icon-view-source-row"),
                            gtk_tree_row_reference_new (model, source_row),
                            (GDestroyNotify) gtk_tree_row_reference_free);
  else
    g_object_set_data_full (G_OBJECT (context),
                            I_("exo-icon-view-source-row"),
                            NULL, NULL);
}

static GtkTreePath*
get_source_row (GdkDragContext *context)
{
  GtkTreeRowReference *ref;

  ref = g_object_get_data (G_OBJECT (context), I_("exo-icon-view-source-row"));

  if (ref)
    return gtk_tree_row_reference_get_path (ref);
  else
    return NULL;
}

typedef struct
{
  GtkTreeRowReference *dest_row;
  gboolean             empty_view_drop;
  gboolean             drop_append_mode;
} DestRow;

static void
dest_row_free (gpointer data)
{
  DestRow *dr = (DestRow *)data;

  gtk_tree_row_reference_free (dr->dest_row);
  g_slice_free (DestRow, dr);
}

static void
set_dest_row (GdkDragContext *context,
              GtkTreeModel   *model,
              GtkTreePath    *dest_row,
              gboolean        empty_view_drop,
              gboolean        drop_append_mode)
{
  DestRow *dr;

  if (!dest_row)
    {
      g_object_set_data_full (G_OBJECT (context),
                              I_("exo-icon-view-dest-row"),
                              NULL, NULL);
      return;
    }

  dr = g_slice_new0 (DestRow);

  dr->dest_row = gtk_tree_row_reference_new (model, dest_row);
  dr->empty_view_drop = empty_view_drop;
  dr->drop_append_mode = drop_append_mode;
  g_object_set_data_full (G_OBJECT (context),
                          I_("exo-icon-view-dest-row"),
                          dr, (GDestroyNotify) dest_row_free);
}



static GtkTreePath*
get_dest_row (GdkDragContext *context)
{
  DestRow *dr;

  dr = g_object_get_data (G_OBJECT (context), I_("exo-icon-view-dest-row"));

  if (dr)
    {
      GtkTreePath *path = NULL;

      if (dr->dest_row)
        path = gtk_tree_row_reference_get_path (dr->dest_row);
      else if (dr->empty_view_drop)
        path = gtk_tree_path_new_from_indices (0, -1);
      else
        path = NULL;

      if (path && dr->drop_append_mode)
        gtk_tree_path_next (path);

      return path;
    }
  else
    return NULL;
}



static gboolean
check_model_dnd (GtkTreeModel *model,
                 GType         required_iface,
                 const gchar  *_signal)
{
  if (model == NULL || !G_TYPE_CHECK_INSTANCE_TYPE ((model), required_iface))
    {
      g_warning ("You must override the default '%s' handler "
                 "on ExoIconView when using models that don't support "
                 "the %s interface and enabling drag-and-drop. The simplest way to do this "
                 "is to connect to '%s' and call "
                 "g_signal_stop_emission_by_name() in your signal handler to prevent "
                 "the default handler from running. Look at the source code "
                 "for the default handler in gtkiconview.c to get an idea what "
                 "your handler should do. (gtkiconview.c is in the GTK+ source "
                 "code.) If you're using GTK+ from a language other than C, "
                 "there may be a more natural way to override default handlers, e.g. via derivation.",
                 _signal, g_type_name (required_iface), _signal);
      return FALSE;
    }
  else
    return TRUE;
}



static void
remove_scroll_timeout (ExoIconView *icon_view)
{
  if (icon_view->priv->scroll_timeout_id != 0)
    {
      g_source_remove (icon_view->priv->scroll_timeout_id);

      icon_view->priv->scroll_timeout_id = 0;
    }
}



static void
exo_icon_view_autoscroll (ExoIconView *icon_view)
{
  gint px, py, x, y, width, height;
  gint hoffset, voffset;
  gfloat value;
  GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (icon_view));

  gdk_window_get_device_position (window,
                                  gdk_device_manager_get_client_pointer(
                                        gdk_display_get_device_manager(
                                            gdk_window_get_display(window))),
                                  &px, &py, NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
  gdk_window_get_geometry (window, &x, &y, &width, &height);
#else
  gdk_window_get_geometry (window, &x, &y, &width, &height, NULL);
#endif

  /* see if we are near the edge. */
  voffset = py - (y + 2 * SCROLL_EDGE_SIZE);
  if (voffset > 0)
    voffset = MAX (py - (y + height - 2 * SCROLL_EDGE_SIZE), 0);

  hoffset = px - (x + 2 * SCROLL_EDGE_SIZE);
  if (hoffset > 0)
    hoffset = MAX (px - (x + width - 2 * SCROLL_EDGE_SIZE), 0);

  if (voffset != 0)
    {
      value = CLAMP (gtk_adjustment_get_value(icon_view->priv->vadjustment) + voffset,
                     gtk_adjustment_get_lower(icon_view->priv->vadjustment),
                     gtk_adjustment_get_upper(icon_view->priv->vadjustment) - gtk_adjustment_get_page_size(icon_view->priv->vadjustment));
      gtk_adjustment_set_value (icon_view->priv->vadjustment, value);
    }
  if (hoffset != 0)
    {
      value = CLAMP (gtk_adjustment_get_value(icon_view->priv->hadjustment) + hoffset,
                     gtk_adjustment_get_lower(icon_view->priv->hadjustment),
                     gtk_adjustment_get_upper(icon_view->priv->hadjustment) - gtk_adjustment_get_page_size(icon_view->priv->hadjustment));
      gtk_adjustment_set_value (icon_view->priv->hadjustment, value);
    }
}


static gboolean
drag_scroll_timeout (gpointer data)
{
  ExoIconView *icon_view = EXO_ICON_VIEW (data);

  if(!g_source_is_destroyed(g_main_current_source()))
    exo_icon_view_autoscroll (icon_view);

  return TRUE;
}


static gboolean
set_destination (ExoIconView    *icon_view,
                 GdkDragContext *context,
                 gint            x,
                 gint            y,
                 GdkDragAction  *suggested_action,
                 GdkAtom        *target)
{
  GtkWidget *widget;
  GtkTreePath *path = NULL;
  ExoIconViewDropPosition pos;
  ExoIconViewDropPosition old_pos;
  GtkTreePath *old_dest_path = NULL;
  gboolean can_drop = FALSE;

  widget = GTK_WIDGET (icon_view);

  *suggested_action = 0;
  *target = GDK_NONE;

  if (!icon_view->priv->dest_set)
    {
      /* someone unset us as a drag dest, note that if
       * we return FALSE drag_leave isn't called
       */

      exo_icon_view_set_drag_dest_item (icon_view,
                                        NULL,
                                        EXO_ICON_VIEW_DROP_LEFT);

      remove_scroll_timeout (EXO_ICON_VIEW (widget));

      return FALSE; /* no longer a drop site */
    }

  *target = gtk_drag_dest_find_target (widget, context, icon_view->priv->dest_targets);
  if (*target == GDK_NONE)
    return FALSE;

  if (!exo_icon_view_get_dest_item_at_pos (icon_view, x, y, &path, &pos))
    {
      gint n_children;
      GtkTreeModel *model;

      /* the row got dropped on empty space, let's setup a special case
       */

      if (path)
        gtk_tree_path_free (path);

      model = exo_icon_view_get_model (icon_view);

      n_children = gtk_tree_model_iter_n_children (model, NULL);
      if (n_children)
        {
          pos = EXO_ICON_VIEW_DROP_BELOW;
          path = gtk_tree_path_new_from_indices (n_children - 1, -1);
        }
      else
        {
          pos = EXO_ICON_VIEW_DROP_ABOVE;
          path = gtk_tree_path_new_from_indices (0, -1);
        }

      can_drop = TRUE;

      goto out;
    }

  g_assert (path);

  exo_icon_view_get_drag_dest_item (icon_view,
                                    &old_dest_path,
                                    &old_pos);

  if (old_dest_path)
    gtk_tree_path_free (old_dest_path);

  if (TRUE /* FIXME if the location droppable predicate */)
    {
      can_drop = TRUE;
    }

out:
  if (can_drop)
    {
      GtkWidget *source_widget;

      *suggested_action = gdk_drag_context_get_suggested_action (context);
      source_widget = gtk_drag_get_source_widget (context);

      if (source_widget == widget)
        {
          /* Default to MOVE, unless the user has
           * pressed ctrl or shift to affect available actions
           */
          if ((gdk_drag_context_get_actions (context) & GDK_ACTION_MOVE) != 0)
            *suggested_action = GDK_ACTION_MOVE;
        }

      exo_icon_view_set_drag_dest_item (EXO_ICON_VIEW (widget),
                                        path, pos);
    }
  else
    {
      /* can't drop here */
      exo_icon_view_set_drag_dest_item (EXO_ICON_VIEW (widget),
                                        NULL,
                                        EXO_ICON_VIEW_DROP_LEFT);
    }

  if (path)
    gtk_tree_path_free (path);

  return TRUE;
}

static GtkTreePath*
get_logical_destination (ExoIconView *icon_view,
                         gboolean    *drop_append_mode)
{
  /* adjust path to point to the row the drop goes in front of */
  GtkTreePath *path = NULL;
  ExoIconViewDropPosition pos;

  *drop_append_mode = FALSE;

  exo_icon_view_get_drag_dest_item (icon_view, &path, &pos);

  if (path == NULL)
    return NULL;

  if (pos == EXO_ICON_VIEW_DROP_RIGHT ||
      pos == EXO_ICON_VIEW_DROP_BELOW)
    {
      GtkTreeIter iter;
      GtkTreeModel *model = icon_view->priv->model;

      if (!gtk_tree_model_get_iter (model, &iter, path) ||
          !gtk_tree_model_iter_next (model, &iter))
        *drop_append_mode = TRUE;
      else
        {
          *drop_append_mode = FALSE;
          gtk_tree_path_next (path);
        }
    }

  return path;
}

static gboolean
exo_icon_view_maybe_begin_drag (ExoIconView    *icon_view,
                                GdkEventMotion *event)
{
  GdkDragContext *context;
  GtkTreePath *path = NULL;
  gint button;
  GtkTreeModel *model;
  gboolean retval = FALSE;

  if (!icon_view->priv->source_set)
    goto out;

  if (icon_view->priv->pressed_button < 0)
    goto out;

  if (!gtk_drag_check_threshold (GTK_WIDGET (icon_view),
                                 icon_view->priv->press_start_x,
                                 icon_view->priv->press_start_y,
                                 event->x, event->y))
    goto out;

  model = exo_icon_view_get_model (icon_view);

  if (model == NULL)
    goto out;

  button = icon_view->priv->pressed_button;
  icon_view->priv->pressed_button = -1;

  path = exo_icon_view_get_path_at_pos (icon_view,
                                        icon_view->priv->press_start_x,
                                        icon_view->priv->press_start_y);

  if (path == NULL)
    goto out;

  if (!GTK_IS_TREE_DRAG_SOURCE (model) ||
      !gtk_tree_drag_source_row_draggable (GTK_TREE_DRAG_SOURCE (model),
                                           path))
    goto out;

  /* FIXME Check whether we're a start button, if not return FALSE and
   * free path
   */

  /* Now we can begin the drag */

  retval = TRUE;

  context = gtk_drag_begin (GTK_WIDGET (icon_view),
                            icon_view->priv->source_targets,
                            icon_view->priv->source_actions,
                            button,
                            (GdkEvent*)event);

  set_source_row (context, model, path);

 out:
  if (path)
    gtk_tree_path_free (path);

  return retval;
}

/* Source side drag signals */
static void
exo_icon_view_drag_begin (GtkWidget      *widget,
                          GdkDragContext *context)
{
  ExoIconView *icon_view;
  ExoIconViewItem *item;
  GdkPixbuf *icon;
  gint x, y;
  GtkTreePath *path;

  icon_view = EXO_ICON_VIEW (widget);

  /* if the user uses a custom DnD impl, we don't set the icon here */
  if (!icon_view->priv->dest_set && !icon_view->priv->source_set)
    return;

  item = exo_icon_view_get_item_at_coords (icon_view,
                                           icon_view->priv->press_start_x,
                                           icon_view->priv->press_start_y,
                                           TRUE,
                                           NULL);

  _exo_return_if_fail (item != NULL);

  x = icon_view->priv->press_start_x - item->area.x + 1;
  y = icon_view->priv->press_start_y - item->area.y + 1;

  path = gtk_tree_path_new_from_indices (item->index, -1);
  icon = exo_icon_view_create_drag_icon (icon_view, path);
  gtk_tree_path_free (path);

  gtk_drag_set_icon_pixbuf (context,
                            icon,
                            x, y);

  g_object_unref (icon);
}

static void
exo_icon_view_drag_end (GtkWidget      *widget,
                        GdkDragContext *context)
{
  /* do nothing */
}

static void
exo_icon_view_drag_data_get (GtkWidget        *widget,
                             GdkDragContext   *context,
                             GtkSelectionData *selection_data,
                             guint             info,
                             guint             drag_time)
{
  ExoIconView *icon_view;
  GtkTreeModel *model;
  GtkTreePath *source_row;

  icon_view = EXO_ICON_VIEW (widget);
  model = exo_icon_view_get_model (icon_view);

  if (model == NULL)
    return;

  if (!icon_view->priv->dest_set)
    return;

  source_row = get_source_row (context);

  if (source_row == NULL)
    return;

  /* We can implement the GTK_TREE_MODEL_ROW target generically for
   * any model; for DragSource models there are some other targets
   * we also support.
   */

  if (GTK_IS_TREE_DRAG_SOURCE (model) &&
      gtk_tree_drag_source_drag_data_get (GTK_TREE_DRAG_SOURCE (model),
                                          source_row,
                                          selection_data))
    goto done;

  /* If drag_data_get does nothing, try providing row data. */
  if (gtk_selection_data_get_target(selection_data) == gdk_atom_intern ("GTK_TREE_MODEL_ROW", FALSE))
    gtk_tree_set_row_drag_data (selection_data,
                                model,
                                source_row);

 done:
  gtk_tree_path_free (source_row);
}

static void
exo_icon_view_drag_data_delete (GtkWidget      *widget,
                                GdkDragContext *context)
{
  GtkTreeModel *model;
  ExoIconView *icon_view;
  GtkTreePath *source_row;

  icon_view = EXO_ICON_VIEW (widget);
  model = exo_icon_view_get_model (icon_view);

  if (!check_model_dnd (model, GTK_TYPE_TREE_DRAG_SOURCE, "drag_data_delete"))
    return;

  if (!icon_view->priv->dest_set)
    return;

  source_row = get_source_row (context);

  if (source_row == NULL)
    return;

  gtk_tree_drag_source_drag_data_delete (GTK_TREE_DRAG_SOURCE (model),
                                         source_row);

  gtk_tree_path_free (source_row);

  set_source_row (context, NULL, NULL);
}

/* Target side drag signals */
static void
exo_icon_view_drag_leave (GtkWidget      *widget,
                          GdkDragContext *context,
                          guint           drag_time)
{
  ExoIconView *icon_view;

  icon_view = EXO_ICON_VIEW (widget);

  /* unset any highlight row */
  exo_icon_view_set_drag_dest_item (icon_view,
                                    NULL,
                                    EXO_ICON_VIEW_DROP_LEFT);

  remove_scroll_timeout (icon_view);
}

static gboolean
exo_icon_view_drag_motion (GtkWidget      *widget,
                           GdkDragContext *context,
                           gint            x,
                           gint            y,
                           guint           drag_time)
{
  ExoIconViewDropPosition pos;
  GdkDragAction           suggested_action = 0;
  GtkTreePath            *path = NULL;
  ExoIconView            *icon_view = EXO_ICON_VIEW (widget);
  gboolean                empty;
  GdkAtom                 target;

  if (!set_destination (icon_view, context, x, y, &suggested_action, &target))
    return FALSE;

  exo_icon_view_get_drag_dest_item (icon_view, &path, &pos);

  /* we only know this *after* set_desination_row */
  empty = icon_view->priv->empty_view_drop;

  if (path == NULL && !empty)
    {
      /* Can't drop here. */
      gdk_drag_status (context, 0, drag_time);
    }
  else
    {
      if (icon_view->priv->scroll_timeout_id == 0)
        icon_view->priv->scroll_timeout_id = gdk_threads_add_timeout (50, drag_scroll_timeout, icon_view);

      if (target == gdk_atom_intern ("GTK_TREE_MODEL_ROW", FALSE))
        {
          /* Request data so we can use the source row when
           * determining whether to accept the drop
           */
          set_status_pending (context, suggested_action);
          gtk_drag_get_data (widget, context, target, drag_time);
        }
      else
        {
          set_status_pending (context, 0);
          gdk_drag_status (context, suggested_action, drag_time);
        }
    }

  if (path != NULL)
    gtk_tree_path_free (path);

  return TRUE;
}

static gboolean
exo_icon_view_drag_drop (GtkWidget      *widget,
                         GdkDragContext *context,
                         gint            x,
                         gint            y,
                         guint           drag_time)
{
  ExoIconView *icon_view;
  GtkTreePath *path;
  GdkDragAction suggested_action = 0;
  GdkAtom target = GDK_NONE;
  GtkTreeModel *model;
  gboolean drop_append_mode;

  icon_view = EXO_ICON_VIEW (widget);
  model = exo_icon_view_get_model (icon_view);

  remove_scroll_timeout (EXO_ICON_VIEW (widget));

  if (!icon_view->priv->dest_set)
    return FALSE;

  if (!check_model_dnd (model, GTK_TYPE_TREE_DRAG_DEST, "drag_drop"))
    return FALSE;

  if (!set_destination (icon_view, context, x, y, &suggested_action, &target))
    return FALSE;

  path = get_logical_destination (icon_view, &drop_append_mode);

  if (target != GDK_NONE && path != NULL)
    {
      /* in case a motion had requested drag data, change things so we
       * treat drag data receives as a drop.
       */
      set_status_pending (context, 0);
      set_dest_row (context, model, path,
                    icon_view->priv->empty_view_drop, drop_append_mode);
    }

  if (path)
    gtk_tree_path_free (path);

  /* Unset this thing */
  exo_icon_view_set_drag_dest_item (icon_view, NULL, EXO_ICON_VIEW_DROP_LEFT);

  if (target != GDK_NONE)
    {
      gtk_drag_get_data (widget, context, target, drag_time);
      return TRUE;
    }
  else
    return FALSE;
}

static void
exo_icon_view_drag_data_received (GtkWidget        *widget,
                                  GdkDragContext   *context,
                                  gint              x,
                                  gint              y,
                                  GtkSelectionData *selection_data,
                                  guint             info,
                                  guint             drag_time)
{
  GtkTreePath *path;
  gboolean accepted = FALSE;
  GtkTreeModel *model;
  ExoIconView *icon_view;
  GtkTreePath *dest_row;
  GdkDragAction suggested_action;
  gboolean drop_append_mode;

  icon_view = EXO_ICON_VIEW (widget);
  model = exo_icon_view_get_model (icon_view);

  if (!check_model_dnd (model, GTK_TYPE_TREE_DRAG_DEST, "drag_data_received"))
    return;

  if (!icon_view->priv->dest_set)
    return;

  suggested_action = get_status_pending (context);

  if (suggested_action)
    {
      /* We are getting this data due to a request in drag_motion,
       * rather than due to a request in drag_drop, so we are just
       * supposed to call drag_status, not actually paste in the
       * data.
       */
      path = get_logical_destination (icon_view, &drop_append_mode);

      if (path == NULL)
        suggested_action = 0;

      if (suggested_action)
        {
          if (!gtk_tree_drag_dest_row_drop_possible (GTK_TREE_DRAG_DEST (model),
                                                     path,
                                                     selection_data))
            suggested_action = 0;
        }

      gdk_drag_status (context, suggested_action, drag_time);

      if (path)
        gtk_tree_path_free (path);

      /* If you can't drop, remove user drop indicator until the next motion */
      if (suggested_action == 0)
        exo_icon_view_set_drag_dest_item (icon_view,
                                          NULL,
                                          EXO_ICON_VIEW_DROP_LEFT);
      return;
    }


  dest_row = get_dest_row (context);

  if (dest_row == NULL)
    return;

  if (gtk_selection_data_get_length(selection_data) >= 0)
    {
      if (gtk_tree_drag_dest_drag_data_received (GTK_TREE_DRAG_DEST (model),
                                                 dest_row,
                                                 selection_data))
        accepted = TRUE;
    }

  gtk_drag_finish (context,
                   accepted,
                   (gdk_drag_context_get_selected_action (context) == GDK_ACTION_MOVE),
                   drag_time);

  gtk_tree_path_free (dest_row);

  /* drop dest_row */
  set_dest_row (context, NULL, NULL, FALSE, FALSE);
}



/**
 * exo_icon_view_enable_model_drag_source:
 * @icon_view         : a #GtkIconTreeView
 * @start_button_mask : Mask of allowed buttons to start drag
 * @targets           : the table of targets that the drag will support
 * @n_targets         : the number of items in @targets
 * @actions           : the bitmask of possible actions for a drag from this widget
 *
 * Turns @icon_view into a drag source for automatic DND.
 *
 * Since: 0.3.1
 **/
void
exo_icon_view_enable_model_drag_source (ExoIconView              *icon_view,
                                        GdkModifierType           start_button_mask,
                                        const GtkTargetEntry     *targets,
                                        gint                      n_targets,
                                        GdkDragAction             actions)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  gtk_drag_source_set (GTK_WIDGET (icon_view), 0, NULL, 0, actions);

  clear_source_info (icon_view);
  icon_view->priv->start_button_mask = start_button_mask;
  icon_view->priv->source_targets = gtk_target_list_new (targets, n_targets);
  icon_view->priv->source_actions = actions;

  icon_view->priv->source_set = TRUE;

  unset_reorderable (icon_view);
}



/**
 * exo_icon_view_enable_model_drag_dest:
 * @icon_view : a #ExoIconView
 * @targets   : the table of targets that the drag will support
 * @n_targets : the number of items in @targets
 * @actions   : the bitmask of possible actions for a drag from this widget
 *
 * Turns @icon_view into a drop destination for automatic DND.
 *
 * Since: 0.3.1
 **/
void
exo_icon_view_enable_model_drag_dest (ExoIconView          *icon_view,
                                      const GtkTargetEntry *targets,
                                      gint                  n_targets,
                                      GdkDragAction         actions)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  gtk_drag_dest_set (GTK_WIDGET (icon_view), 0, NULL, 0, actions);

  clear_dest_info (icon_view);

  icon_view->priv->dest_targets = gtk_target_list_new (targets, n_targets);
  icon_view->priv->dest_actions = actions;

  icon_view->priv->dest_set = TRUE;

  unset_reorderable (icon_view);
}



/**
 * exo_icon_view_unset_model_drag_source:
 * @icon_view : a #ExoIconView
 *
 * Undoes the effect of #exo_icon_view_enable_model_drag_source().
 *
 * Since: 0.3.1
 **/
void
exo_icon_view_unset_model_drag_source (ExoIconView *icon_view)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->source_set)
    {
      gtk_drag_source_unset (GTK_WIDGET (icon_view));
      clear_source_info (icon_view);
    }

  unset_reorderable (icon_view);
}



/**
 * exo_icon_view_unset_model_drag_dest:
 * @icon_view : a #ExoIconView
 *
 * Undoes the effect of #exo_icon_view_enable_model_drag_dest().
 *
 * Since: 0.3.1
 **/
void
exo_icon_view_unset_model_drag_dest (ExoIconView *icon_view)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->dest_set)
    {
      gtk_drag_dest_unset (GTK_WIDGET (icon_view));
      clear_dest_info (icon_view);
    }

  unset_reorderable (icon_view);
}



/**
 * exo_icon_view_set_drag_dest_item:
 * @icon_view : a #ExoIconView
 * @path      : The path of the item to highlight, or %NULL.
 * @pos       : Specifies whether to drop, relative to the item
 *
 * Sets the item that is highlighted for feedback.
 *
 * Since: 0.3.1
 */
void
exo_icon_view_set_drag_dest_item (ExoIconView            *icon_view,
                                  GtkTreePath            *path,
                                  ExoIconViewDropPosition pos)
{
  ExoIconViewItem *item;
  GtkTreePath     *previous_path;

  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  if (icon_view->priv->dest_item != NULL)
    {
      /* determine and reset the previous path */
      previous_path = gtk_tree_row_reference_get_path (icon_view->priv->dest_item);
      gtk_tree_row_reference_free (icon_view->priv->dest_item);
      icon_view->priv->dest_item = NULL;

      /* check if the path is still valid */
      if (G_LIKELY (previous_path != NULL))
        {
          /* schedule a redraw for the previous path */
          item = g_list_nth_data (icon_view->priv->items, gtk_tree_path_get_indices (previous_path)[0]);
          if (G_LIKELY (item != NULL))
            exo_icon_view_queue_draw_item (icon_view, item);
          gtk_tree_path_free (previous_path);
        }
    }

  /* special case a drop on an empty model */
  icon_view->priv->empty_view_drop = FALSE;
  if (pos == EXO_ICON_VIEW_NO_DROP && path
      && gtk_tree_path_get_depth (path) == 1
      && gtk_tree_path_get_indices (path)[0] == 0)
    {
      gint n_children;

      n_children = gtk_tree_model_iter_n_children (icon_view->priv->model,
                                                   NULL);

      if (n_children == 0)
        icon_view->priv->empty_view_drop = TRUE;
    }

  icon_view->priv->dest_pos = pos;

  if (G_LIKELY (path != NULL))
    {
      /* take a row reference for the new item path */
      icon_view->priv->dest_item = gtk_tree_row_reference_new_proxy (G_OBJECT (icon_view), icon_view->priv->model, path);

      /* schedule a redraw on the new path */
      item = g_list_nth_data (icon_view->priv->items, gtk_tree_path_get_indices (path)[0]);
      if (G_LIKELY (item != NULL))
        exo_icon_view_queue_draw_item (icon_view, item);
    }
}



/**
 * exo_icon_view_get_drag_dest_item:
 * @icon_view : a #ExoIconView
 * @path      : Return location for the path of the highlighted item, or %NULL.
 * @pos       : Return location for the drop position, or %NULL
 *
 * Gets information about the item that is highlighted for feedback.
 *
 * Since: 0.3.1
 **/
void
exo_icon_view_get_drag_dest_item (ExoIconView              *icon_view,
                                  GtkTreePath             **path,
                                  ExoIconViewDropPosition  *pos)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (path)
    {
      if (icon_view->priv->dest_item)
        *path = gtk_tree_row_reference_get_path (icon_view->priv->dest_item);
      else
        *path = NULL;
    }

  if (pos)
    *pos = icon_view->priv->dest_pos;
}



/**
 * exo_icon_view_get_dest_item_at_pos:
 * @icon_view : a #ExoIconView
 * @drag_x    : the position to determine the destination item for
 * @drag_y    : the position to determine the destination item for
 * @path      : Return location for the path of the highlighted item, or %NULL.
 * @pos       : Return location for the drop position, or %NULL
 *
 * Determines the destination item for a given position.
 *
 * Both @drag_x and @drag_y are given in icon window coordinates. Use
 * #exo_icon_view_widget_to_icon_coords() if you need to translate
 * widget coordinates first.
 *
 * Return value: whether there is an item at the given position.
 *
 * Since: 0.3.1
 **/
gboolean
exo_icon_view_get_dest_item_at_pos (ExoIconView              *icon_view,
                                    gint                      drag_x,
                                    gint                      drag_y,
                                    GtkTreePath             **path,
                                    ExoIconViewDropPosition  *pos)
{
  ExoIconViewItem *item;

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (drag_x >= 0, FALSE);
  g_return_val_if_fail (drag_y >= 0, FALSE);
  g_return_val_if_fail (icon_view->priv->bin_window != NULL, FALSE);

  if (G_LIKELY (path != NULL))
    *path = NULL;

  item = exo_icon_view_get_item_at_coords (icon_view, drag_x, drag_y, FALSE, NULL);

  if (G_UNLIKELY (item == NULL))
    return FALSE;

  if (G_LIKELY (path != NULL))
    *path = gtk_tree_path_new_from_indices (item->index, -1);

  if (G_LIKELY (pos != NULL))
    {
      if (drag_x < item->area.x + item->area.width / 4)
        *pos = EXO_ICON_VIEW_DROP_LEFT;
      else if (drag_x > item->area.x + item->area.width * 3 / 4)
        *pos = EXO_ICON_VIEW_DROP_RIGHT;
      else if (drag_y < item->area.y + item->area.height / 4)
        *pos = EXO_ICON_VIEW_DROP_ABOVE;
      else if (drag_y > item->area.y + item->area.height * 3 / 4)
        *pos = EXO_ICON_VIEW_DROP_BELOW;
      else
        *pos = EXO_ICON_VIEW_DROP_INTO;
    }

  return TRUE;
}



/**
 * exo_icon_view_create_drag_icon:
 * @icon_view : a #ExoIconView
 * @path      : a #GtkTreePath in @icon_view
 *
 * Creates a #GdkPixbuf representation of the item at @path.
 * This image is used for a drag icon.
 *
 * Return value: a newly-allocated pixmap of the drag icon.
 *
 * Since: 0.3.1
 **/
GdkPixbuf*
exo_icon_view_create_drag_icon (ExoIconView *icon_view,
                                GtkTreePath *path)
{
  GdkRectangle area;
  GtkWidget   *widget = GTK_WIDGET (icon_view);
#if GTK_CHECK_VERSION(3, 0, 0)
  cairo_surface_t *s;
#else
  GdkPixmap   *drawable;
#endif
  GdkPixbuf   *pixbuf;
  cairo_t     *cr;
  GList       *lp;
  GtkStyle    *style;
  gint         idx;

  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), NULL);
  g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, NULL);

  /* verify that the widget is realized */
  if (G_UNLIKELY (!gtk_widget_get_realized (GTK_WIDGET (icon_view))))
    return NULL;

  idx = gtk_tree_path_get_indices (path)[0];
  style = gtk_widget_get_style (widget);

  for (lp = icon_view->priv->items; lp != NULL; lp = lp->next)
    {
      ExoIconViewItem *item = lp->data;
      if (G_UNLIKELY (idx == item->index))
        {
#if GTK_CHECK_VERSION(3, 0, 0)
          s = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
                                          item->area.width + 2,
                                          item->area.height + 2);

          cr = cairo_create (s);
#else
          drawable = gdk_pixmap_new (icon_view->priv->bin_window,
                                     item->area.width + 2,
                                     item->area.height + 2,
                                     -1);

          cr = gdk_cairo_create (drawable);
#endif
          gdk_cairo_set_source_color (cr, &style->base[gtk_widget_get_state (widget)]);
          cairo_rectangle (cr, 0, 0, item->area.width + 2, item->area.height + 2);
          cairo_fill (cr);

          area.x = 0;
          area.y = 0;
          area.width = item->area.width;
          area.height = item->area.height;

#if GTK_CHECK_VERSION(3, 0, 0)
          exo_icon_view_paint_item (icon_view, item, &area, cr, 1, 1, FALSE);
#else
          /* NOTE: this is inefficient but Gtk+2 uses GtkWindow for render */
          exo_icon_view_paint_item (icon_view, item, &area, drawable, 1, 1, FALSE);
#endif

          gdk_cairo_set_source_color (cr, &style->black);
          cairo_rectangle (cr, 1, 1, item->area.width + 1, item->area.height + 1);
          cairo_stroke (cr);

          cairo_destroy (cr);

#if GTK_CHECK_VERSION(3, 0, 0)
          pixbuf = gdk_pixbuf_get_from_surface (s, 0, 0,
                                                item->area.width + 2,
                                                item->area.height + 2);
          cairo_surface_destroy (s);

          return pixbuf;
#else
          pixbuf = gdk_pixbuf_get_from_drawable (NULL, drawable,
                                                 gdk_drawable_get_colormap (drawable),
                                                 0, 0, 0, 0,
                                                 item->area.width + 2,
                                                 item->area.height + 2);
          g_object_unref (drawable);
          return pixbuf;
#endif
        }
    }

  return NULL;
}



/**
 * exo_icon_view_get_reorderable:
 * @icon_view : a #ExoIconView
 *
 * Retrieves whether the user can reorder the list via drag-and-drop.
 * See exo_icon_view_set_reorderable().
 *
 * Return value: %TRUE if the list can be reordered.
 *
 * Since: 0.3.1
 **/
gboolean
exo_icon_view_get_reorderable (ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), FALSE);

  return icon_view->priv->reorderable;
}



/**
 * exo_icon_view_set_reorderable:
 * @icon_view   : A #ExoIconView.
 * @reorderable : %TRUE, if the list of items can be reordered.
 *
 * This function is a convenience function to allow you to reorder models that
 * support the #GtkTreeDragSourceIface and the #GtkTreeDragDestIface.  Both
 * #GtkTreeStore and #GtkListStore support these.  If @reorderable is %TRUE, then
 * the user can reorder the model by dragging and dropping rows.  The
 * developer can listen to these changes by connecting to the model's
 * ::row-inserted and ::row-deleted signals.
 *
 * This function does not give you any degree of control over the order -- any
 * reordering is allowed.  If more control is needed, you should probably
 * handle drag and drop manually.
 *
 * Since: 0.3.1
 **/
void
exo_icon_view_set_reorderable (ExoIconView *icon_view,
                               gboolean     reorderable)
{
  static const GtkTargetEntry item_targets[] =
  {
    { "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, 0, },
  };

  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  reorderable = (reorderable != FALSE);

  if (G_UNLIKELY (icon_view->priv->reorderable == reorderable))
    return;

  if (G_LIKELY (reorderable))
    {
      exo_icon_view_enable_model_drag_source (icon_view, GDK_BUTTON1_MASK, item_targets, G_N_ELEMENTS (item_targets), GDK_ACTION_MOVE);
      exo_icon_view_enable_model_drag_dest (icon_view, item_targets, G_N_ELEMENTS (item_targets), GDK_ACTION_MOVE);
    }
  else
    {
      exo_icon_view_unset_model_drag_source (icon_view);
      exo_icon_view_unset_model_drag_dest (icon_view);
    }

  icon_view->priv->reorderable = reorderable;

  g_object_notify (G_OBJECT (icon_view), "reorderable");
}



/*----------------------*
 * Single-click support *
 *----------------------*/

/**
 * exo_icon_view_get_single_click:
 * @icon_view : a #ExoIconView.
 *
 * Returns %TRUE if @icon_view is currently in single click mode,
 * else %FALSE will be returned.
 *
 * Return value: whether @icon_view is currently in single click mode.
 *
 * Since: 0.3.1.3
 **/
gboolean
exo_icon_view_get_single_click (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), FALSE);
  return icon_view->priv->single_click;
}



/**
 * exo_icon_view_set_single_click:
 * @icon_view    : a #ExoIconView.
 * @single_click : %TRUE for single click, %FALSE for double click mode.
 *
 * If @single_click is %TRUE, @icon_view will be in single click mode
 * afterwards, else @icon_view will be in double click mode.
 *
 * Since: 0.3.1.3
 **/
void
exo_icon_view_set_single_click (ExoIconView *icon_view,
                                gboolean     single_click)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  /* normalize the value */
  single_click = !!single_click;

  /* check if we have a new setting here */
  if (icon_view->priv->single_click != single_click)
    {
      icon_view->priv->single_click = single_click;
      g_object_notify (G_OBJECT (icon_view), "single-click");
    }
}



/**
 * exo_icon_view_get_single_click_timeout:
 * @icon_view : a #ExoIconView.
 *
 * Returns the amount of time in milliseconds after which the
 * item under the mouse cursor will be selected automatically
 * in single click mode. A value of %0 means that the behavior
 * is disabled and the user must alter the selection manually.
 *
 * Return value: the single click autoselect timeout or %0 if
 *               the behavior is disabled.
 *
 * Since: 0.3.1.5
 **/
guint
exo_icon_view_get_single_click_timeout (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), 0u);
  return icon_view->priv->single_click_timeout;
}



/**
 * exo_icon_view_set_single_click_timeout:
 * @icon_view            : a #ExoIconView.
 * @single_click_timeout : the new timeout or %0 to disable.
 *
 * If @single_click_timeout is a value greater than zero, it specifies
 * the amount of time in milliseconds after which the item under the
 * mouse cursor will be selected automatically in single click mode.
 * A value of %0 for @single_click_timeout disables the autoselection
 * for @icon_view.
 *
 * This setting does not have any effect unless the @icon_view is in
 * single-click mode, see exo_icon_view_set_single_click().
 *
 * Since: 0.3.1.5
 **/
void
exo_icon_view_set_single_click_timeout (ExoIconView *icon_view,
                                        guint        single_click_timeout)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  /* check if we have a new setting */
  if (icon_view->priv->single_click_timeout != single_click_timeout)
    {
      /* apply the new setting */
      icon_view->priv->single_click_timeout = single_click_timeout;

      /* be sure to cancel any pending single click timeout */
      if (G_UNLIKELY (icon_view->priv->single_click_timeout_id != 0))
        g_source_remove (icon_view->priv->single_click_timeout_id);

      /* notify listeners */
      g_object_notify (G_OBJECT (icon_view), "single-click-timeout");
    }
}



static gboolean
exo_icon_view_single_click_timeout (gpointer user_data)
{
  ExoIconViewItem *item;
  gboolean         dirty = FALSE;
  ExoIconView     *icon_view = EXO_ICON_VIEW (user_data);

  /* ensure that source isn't removed yet */
  if(g_source_is_destroyed(g_main_current_source()))
      return FALSE;

  /* verify that we are in single-click mode, have focus and a prelit item */
  if (gtk_widget_has_focus (GTK_WIDGET (icon_view)) && icon_view->priv->single_click && icon_view->priv->prelit_item != NULL)
    {
      /* work on the prelit item */
      item = icon_view->priv->prelit_item;

      /* be sure the item is fully visible */
      exo_icon_view_scroll_to_item (icon_view, item);

      /* change the selection appropriately */
      if (G_UNLIKELY (icon_view->priv->selection_mode == GTK_SELECTION_NONE))
        {
          exo_icon_view_set_cursor_item (icon_view, item, -1);
        }
      else if ((icon_view->priv->single_click_timeout_state & GDK_SHIFT_MASK) != 0
            && icon_view->priv->selection_mode == GTK_SELECTION_MULTIPLE)
        {
          if (!(icon_view->priv->single_click_timeout_state & GDK_CONTROL_MASK))
            /* unselect all previously selected items */
            exo_icon_view_unselect_all_internal (icon_view);

          /* select all items between the anchor and the prelit item */
          exo_icon_view_set_cursor_item (icon_view, item, -1);
          if (icon_view->priv->anchor_item == NULL)
            icon_view->priv->anchor_item = item;
          else
            exo_icon_view_select_all_between (icon_view, icon_view->priv->anchor_item, item);

          /* selection was changed */
          dirty = TRUE;
        }
      else
        {
          if ((icon_view->priv->selection_mode == GTK_SELECTION_MULTIPLE ||
              ((icon_view->priv->selection_mode == GTK_SELECTION_SINGLE) && item->selected)) &&
              (icon_view->priv->single_click_timeout_state & GDK_CONTROL_MASK) != 0)
            {
              item->selected = !item->selected;
              exo_icon_view_queue_draw_item (icon_view, item);
              dirty = TRUE;
            }
          else if (!item->selected)
            {
              exo_icon_view_unselect_all_internal (icon_view);
              exo_icon_view_queue_draw_item (icon_view, item);
              item->selected = TRUE;
              dirty = TRUE;
            }
          exo_icon_view_set_cursor_item (icon_view, item, -1);
          icon_view->priv->anchor_item = item;
        }
    }

  /* emit "selection-changed" and stop drawing keyboard
   * focus indicator if the selection was altered
   */
  if (G_LIKELY (dirty))
    {
      /* reset "draw keyfocus" flag */
      EXO_ICON_VIEW_UNSET_FLAG (icon_view, EXO_ICON_VIEW_DRAW_KEYFOCUS);

      /* emit "selection-changed" */
      g_signal_emit (G_OBJECT (icon_view), icon_view_signals[SELECTION_CHANGED], 0);
    }

  return FALSE;
}



static void
exo_icon_view_single_click_timeout_destroy (gpointer user_data)
{
  EXO_ICON_VIEW (user_data)->priv->single_click_timeout_id = 0;
}



/*----------------------------*
 * Interactive search support *
 *----------------------------*/

/**
 * exo_icon_view_get_enable_search:
 * @icon_view : an #ExoIconView.
 *
 * Returns whether or not the @icon_view allows to start
 * interactive searching by typing in text.
 *
 * Return value: whether or not to let the user search
 *               interactively.
 *
 * Since: 0.3.1.3
 **/
gboolean
exo_icon_view_get_enable_search (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), FALSE);
  return icon_view->priv->enable_search;
}



/**
 * exo_icon_view_set_enable_search:
 * @icon_view     : an #ExoIconView.
 * @enable_search : %TRUE if the user can search interactively.
 *
 * If @enable_search is set, then the user can type in text to search through
 * the @icon_view interactively (this is sometimes called "typeahead find").
 *
 * Note that even if this is %FALSE, the user can still initiate a search
 * using the "start-interactive-search" key binding.
 *
 * Since: 0.3.1.3
 **/
void
exo_icon_view_set_enable_search (ExoIconView *icon_view,
                                 gboolean     enable_search)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  enable_search = !!enable_search;

  if (G_LIKELY (icon_view->priv->enable_search != enable_search))
    {
      icon_view->priv->enable_search = enable_search;
      g_object_notify (G_OBJECT (icon_view), "enable-search");
    }
}



/**
 * exo_icon_view_get_search_column:
 * @icon_view : an #ExoIconView.
 *
 * Returns the column searched on by the interactive search code.
 *
 * Return value: the column the interactive search code searches in.
 *
 * Since: 0.3.1.3
 **/
gint
exo_icon_view_get_search_column (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), -1);
  return icon_view->priv->search_column;
}



/**
 * exo_icon_view_set_search_column:
 * @icon_view     : an #ExoIconView.
 * @search_column : the column of the model to search in, or -1 to disable searching.
 *
 * Sets @search_column as the column where the interactive search code should search in.
 *
 * If the search column is set, user can use the "start-interactive-search" key
 * binding to bring up search popup. The "enable-search" property controls
 * whether simply typing text will also start an interactive search.
 *
 * Note that @search_column refers to a column of the model.
 *
 * Since: 0.3.1.3
 **/
void
exo_icon_view_set_search_column (ExoIconView *icon_view,
                                 gint         search_column)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));
  g_return_if_fail (search_column >= -1);

  if (G_LIKELY (icon_view->priv->search_column != search_column))
    {
      icon_view->priv->search_column = search_column;
      g_object_notify (G_OBJECT (icon_view), "search-column");
    }
}



/**
 * exo_icon_view_get_search_equal_func:
 * @icon_view : an #ExoIconView.
 *
 * Returns the compare function currently in use.
 *
 * Return value: the currently used compare function for the search code.
 *
 * Since: 0.3.1.3
 **/
ExoIconViewSearchEqualFunc
exo_icon_view_get_search_equal_func (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), NULL);
  return icon_view->priv->search_equal_func;
}



/**
 * exo_icon_view_set_search_equal_func:
 * @icon_view            : an #ExoIconView.
 * @search_equal_func    : the compare function to use during the search, or %NULL.
 * @search_equal_data    : user data to pass to @search_equal_func, or %NULL.
 * @search_equal_destroy : destroy notifier for @search_equal_data, or %NULL.
 *
 * Sets the compare function for the interactive search capabilities;
 * note that some like strcmp() returning 0 for equality
 * #ExoIconViewSearchEqualFunc returns %FALSE on matches.
 *
 * Specifying %NULL for @search_equal_func will reset @icon_view to use the default
 * search equal function.
 *
 * Since: 0.3.1.3
 **/
void
exo_icon_view_set_search_equal_func (ExoIconView               *icon_view,
                                     ExoIconViewSearchEqualFunc search_equal_func,
                                     gpointer                   search_equal_data,
                                     GDestroyNotify             search_equal_destroy)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));
  g_return_if_fail (search_equal_func != NULL || (search_equal_data == NULL && search_equal_destroy == NULL));

  /* destroy the previous data (if any) */
  if (G_UNLIKELY (icon_view->priv->search_equal_destroy != NULL))
    (*icon_view->priv->search_equal_destroy) (icon_view->priv->search_equal_data);

  icon_view->priv->search_equal_func = (search_equal_func != NULL) ? search_equal_func : exo_icon_view_search_equal_func;
  icon_view->priv->search_equal_data = search_equal_data;
  icon_view->priv->search_equal_destroy = search_equal_destroy;
}



/**
 * exo_icon_view_get_search_position_func:
 * @icon_view : an #ExoIconView.
 *
 * Returns the search dialog positioning function currently in use.
 *
 * Return value: the currently used function for positioning the search dialog.
 *
 * Since: 0.3.1.3
 **/
ExoIconViewSearchPositionFunc
exo_icon_view_get_search_position_func (const ExoIconView *icon_view)
{
  g_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), NULL);
  return icon_view->priv->search_position_func;
}



/**
 * exo_icon_view_set_search_position_func:
 * @icon_view               : an #ExoIconView.
 * @search_position_func    : the function to use to position the search dialog, or %NULL.
 * @search_position_data    : user data to pass to @search_position_func, or %NULL.
 * @search_position_destroy : destroy notifier for @search_position_data, or %NULL.
 *
 * Sets the function to use when positioning the seach dialog.
 *
 * Specifying %NULL for @search_position_func will reset @icon_view to use the default
 * search position function.
 *
 * Since: 0.3.1.3
 **/
void
exo_icon_view_set_search_position_func (ExoIconView                  *icon_view,
                                        ExoIconViewSearchPositionFunc search_position_func,
                                        gpointer                      search_position_data,
                                        GDestroyNotify                search_position_destroy)
{
  g_return_if_fail (EXO_IS_ICON_VIEW (icon_view));
  g_return_if_fail (search_position_func != NULL || (search_position_data == NULL && search_position_destroy == NULL));

  /* destroy the previous data (if any) */
  if (icon_view->priv->search_position_destroy != NULL)
    (*icon_view->priv->search_position_destroy) (icon_view->priv->search_position_data);

  icon_view->priv->search_position_func = (search_position_func != NULL) ? search_position_func : exo_icon_view_search_position_func;
  icon_view->priv->search_position_data = search_position_data;
  icon_view->priv->search_position_destroy = search_position_destroy;
}



static void
exo_icon_view_search_activate (GtkEntry    *entry,
                               ExoIconView *icon_view)
{
  GtkTreePath *path;

  /* hide the interactive search dialog */
  exo_icon_view_search_dialog_hide (icon_view->priv->search_window, icon_view);

  /* check if we have a cursor item, and if so, activate it */
  if (exo_icon_view_get_cursor (icon_view, &path, NULL))
    {
      /* only activate the cursor item if it's selected */
      if (exo_icon_view_path_is_selected (icon_view, path))
        exo_icon_view_item_activated (icon_view, path);
      gtk_tree_path_free (path);
    }
}



static void
exo_icon_view_search_dialog_hide (GtkWidget   *search_dialog,
                                  ExoIconView *icon_view)
{
  _exo_return_if_fail (GTK_IS_WIDGET (search_dialog));
  _exo_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->search_disable_popdown)
    return;

  /* disconnect the "changed" signal handler */
  if (icon_view->priv->search_entry_changed_id != 0)
    {
      g_signal_handler_disconnect (G_OBJECT (icon_view->priv->search_entry), icon_view->priv->search_entry_changed_id);
      icon_view->priv->search_entry_changed_id = 0;
    }

  /* disable the flush timeout */
  if (icon_view->priv->search_timeout_id != 0)
    g_source_remove (icon_view->priv->search_timeout_id);

  /* send focus-out event */
  _exo_gtk_widget_send_focus_change (icon_view->priv->search_entry, FALSE);
  gtk_widget_hide (search_dialog);
  gtk_entry_set_text (GTK_ENTRY (icon_view->priv->search_entry), "");
}



static void
exo_icon_view_search_ensure_directory (ExoIconView *icon_view)
{
  GtkWidget *toplevel;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWindowGroup *group;

  /* determine the toplevel window */
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (icon_view));

  /* check if we already have a search window */
  if (G_LIKELY (icon_view->priv->search_window != NULL))
    {
      if ((group = gtk_window_get_group (GTK_WINDOW (toplevel))) != NULL)
        gtk_window_group_add_window (group, GTK_WINDOW (icon_view->priv->search_window));
      else if ((group = gtk_window_get_group (GTK_WINDOW (icon_view->priv->search_window))) != NULL)
        gtk_window_group_remove_window (group, GTK_WINDOW (icon_view->priv->search_window));
      return;
    }

  /* allocate a new search window */
  icon_view->priv->search_window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_type_hint (GTK_WINDOW (icon_view->priv->search_window),
                            GDK_WINDOW_TYPE_HINT_UTILITY);
  if ((group = gtk_window_get_group (GTK_WINDOW (toplevel))) != NULL)
    gtk_window_group_add_window (group, GTK_WINDOW (icon_view->priv->search_window));
  gtk_window_set_modal (GTK_WINDOW (icon_view->priv->search_window), TRUE);
  gtk_window_set_screen (GTK_WINDOW (icon_view->priv->search_window), gtk_widget_get_screen (GTK_WIDGET (icon_view)));

  /* connect signal handlers */
  g_signal_connect (G_OBJECT (icon_view->priv->search_window), "delete-event", G_CALLBACK (exo_icon_view_search_delete_event), icon_view);
  g_signal_connect (G_OBJECT (icon_view->priv->search_window), "scroll-event", G_CALLBACK (exo_icon_view_search_scroll_event), icon_view);
  g_signal_connect (G_OBJECT (icon_view->priv->search_window), "key-press-event", G_CALLBACK (exo_icon_view_search_key_press_event), icon_view);
  g_signal_connect (G_OBJECT (icon_view->priv->search_window), "button-press-event", G_CALLBACK (exo_icon_view_search_button_press_event), icon_view);

  /* allocate the frame widget */
  frame = g_object_new (GTK_TYPE_FRAME, "shadow-type", GTK_SHADOW_ETCHED_IN, NULL);
  gtk_container_add (GTK_CONTAINER (icon_view->priv->search_window), frame);
  gtk_widget_show (frame);

  /* allocate the vertical box */
  vbox = g_object_new (GTK_TYPE_VBOX, "border-width", 3, NULL);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* allocate the search entry widget */
  icon_view->priv->search_entry = gtk_entry_new ();
  g_signal_connect (G_OBJECT (icon_view->priv->search_entry), "activate", G_CALLBACK (exo_icon_view_search_activate), icon_view);
#if GTK_CHECK_VERSION(2, 20, 0)
  g_signal_connect (G_OBJECT (icon_view->priv->search_entry), "preedit-changed",
                    G_CALLBACK (exo_icon_view_search_preedit_changed), icon_view);
#else
  g_signal_connect (G_OBJECT (GTK_ENTRY (icon_view->priv->search_entry)->im_context), "preedit-changed",
                    G_CALLBACK (exo_icon_view_search_preedit_changed), icon_view);
#endif
  gtk_box_pack_start (GTK_BOX (vbox), icon_view->priv->search_entry, TRUE, TRUE, 0);
  gtk_widget_realize (icon_view->priv->search_entry);
  gtk_widget_show (icon_view->priv->search_entry);
}



static void
exo_icon_view_search_init (GtkWidget   *search_entry,
                           ExoIconView *icon_view)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  const gchar  *text;
  gint          length;
  gint          count = 0;

  _exo_return_if_fail (GTK_IS_ENTRY (search_entry));
  _exo_return_if_fail (EXO_IS_ICON_VIEW (icon_view));

  /* determine the current text for the search entry */
  text = gtk_entry_get_text (GTK_ENTRY (search_entry));
  if (G_UNLIKELY (text == NULL))
    return;

  /* unselect all items */
  exo_icon_view_unselect_all (icon_view);

  /* renew the flush timeout */
  if ((icon_view->priv->search_timeout_id != 0))
    {
      /* drop the previous timeout */
      g_source_remove (icon_view->priv->search_timeout_id);

      /* schedule a new timeout */
      icon_view->priv->search_timeout_id = gdk_threads_add_timeout_full (G_PRIORITY_LOW, EXO_ICON_VIEW_SEARCH_DIALOG_TIMEOUT,
                                                               exo_icon_view_search_timeout, icon_view,
                                                               exo_icon_view_search_timeout_destroy);
    }

  /* verify that we have a search text */
  length = strlen (text);
  if (length < 1)
    return;

  /* verify that we have a valid model */
  model = exo_icon_view_get_model (icon_view);
  if (G_UNLIKELY (model == NULL))
    return;

  /* start the interactive search */
  if (gtk_tree_model_get_iter_first (model, &iter))
    {
      /* let's see if we have a match */
      if (exo_icon_view_search_iter (icon_view, model, &iter, text, &count, 1))
        icon_view->priv->search_selected_iter = 1;
    }
}



static gboolean
exo_icon_view_search_iter (ExoIconView  *icon_view,
                           GtkTreeModel *model,
                           GtkTreeIter  *iter,
                           const gchar  *text,
                           gint         *count,
                           gint          n)
{
  GtkTreePath *path;

  _exo_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), FALSE);
  _exo_return_val_if_fail (GTK_IS_TREE_MODEL (model), FALSE);
  _exo_return_val_if_fail (count != NULL, FALSE);

  /* search for a matching item */
  do
    {
      if (!(*icon_view->priv->search_equal_func) (model, icon_view->priv->search_column, text, iter, icon_view->priv->search_equal_data))
        {
          (*count) += 1;
          if (*count == n)
            {
              /* place cursor on the item and select it */
              path = gtk_tree_model_get_path (model, iter);
              exo_icon_view_select_path (icon_view, path);
              exo_icon_view_set_cursor (icon_view, path, NULL, FALSE);
              gtk_tree_path_free (path);
              return TRUE;
            }
        }
    }
  while (gtk_tree_model_iter_next (model, iter));

  /* no match */
  return FALSE;
}



static void
exo_icon_view_search_move (GtkWidget   *widget,
                           ExoIconView *icon_view,
                           gboolean     move_up)
{
  GtkTreeModel *model;
  const gchar  *text;
  GtkTreeIter   iter;
  gboolean      retval;
  gint          length;
  gint          count = 0;

  /* determine the current text for the search entry */
  text = gtk_entry_get_text (GTK_ENTRY (icon_view->priv->search_entry));
  if (G_UNLIKELY (text == NULL))
    return;

  /* if we already selected the first item, we cannot go up */
  if (move_up && icon_view->priv->search_selected_iter == 1)
    return;

  /* determine the length of the search text */
  length = strlen (text);
  if (G_UNLIKELY (length < 1))
    return;

  /* unselect all items */
  exo_icon_view_unselect_all (icon_view);

  /* verify that we have a valid model */
  model = exo_icon_view_get_model (icon_view);
  if (G_UNLIKELY (model == NULL))
    return;

  /* determine the iterator to the first item */
  if (!gtk_tree_model_get_iter_first (model, &iter))
    return;

  /* first attempt to search */
  retval = exo_icon_view_search_iter (icon_view, model, &iter, text, &count, move_up
                                      ? (icon_view->priv->search_selected_iter - 1)
                                      : (icon_view->priv->search_selected_iter + 1));

  /* check if we found something */
  if (G_LIKELY (retval))
    {
      /* match found */
      icon_view->priv->search_selected_iter += move_up ? -1 : 1;
    }
  else
    {
      /* return to old iter */
      if (gtk_tree_model_get_iter_first (model, &iter))
        {
          count = 0;
          exo_icon_view_search_iter (icon_view, model, &iter, text, &count,
                                     icon_view->priv->search_selected_iter);
        }
    }
}



static void
#if GTK_CHECK_VERSION(2, 20, 0)
exo_icon_view_search_preedit_changed (GtkEntry     *entry,
                                      gchar        *preedit,
                                      ExoIconView  *icon_view)
#else
exo_icon_view_search_preedit_changed (GtkIMContext *im_context,
                                      ExoIconView  *icon_view)
#endif
{
  icon_view->priv->search_imcontext_changed = TRUE;

  /* re-register the search timeout */
  if (G_LIKELY (icon_view->priv->search_timeout_id != 0))
    {
      g_source_remove (icon_view->priv->search_timeout_id);
      icon_view->priv->search_timeout_id = gdk_threads_add_timeout_full (G_PRIORITY_LOW, EXO_ICON_VIEW_SEARCH_DIALOG_TIMEOUT,
                                                               exo_icon_view_search_timeout, icon_view,
                                                               exo_icon_view_search_timeout_destroy);
    }
}



static gboolean
exo_icon_view_search_start (ExoIconView *icon_view,
                            gboolean     keybinding)
{
  GTypeClass *klass;

  /* check if typeahead is enabled */
  if (G_UNLIKELY (!icon_view->priv->enable_search && !keybinding))
    return FALSE;

  /* check if we already display the search window */
  if (icon_view->priv->search_window != NULL && gtk_widget_get_visible (icon_view->priv->search_window))
    return TRUE;

  /* we only start interactive search if we have focus,
   * we don't want to start interactive search if one of
   * our children has the focus.
   */
  if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
    return FALSE;

  /* verify that we have a search column */
  if (G_UNLIKELY (icon_view->priv->search_column < 0))
    return FALSE;

  exo_icon_view_search_ensure_directory (icon_view);

  /* clear search entry if we were started by a keybinding */
  if (G_UNLIKELY (keybinding))
    gtk_entry_set_text (GTK_ENTRY (icon_view->priv->search_entry), "");

  /* determine the position for the search dialog */
  (*icon_view->priv->search_position_func) (icon_view, icon_view->priv->search_window, icon_view->priv->search_position_data);

  /* display the search dialog */
  gtk_widget_show (icon_view->priv->search_window);

  /* connect "changed" signal for the entry */
  if (G_UNLIKELY (icon_view->priv->search_entry_changed_id == 0))
    {
      icon_view->priv->search_entry_changed_id = g_signal_connect (G_OBJECT (icon_view->priv->search_entry), "changed",
                                                                   G_CALLBACK (exo_icon_view_search_init), icon_view);
    }

  /* start the search timeout */
  icon_view->priv->search_timeout_id = gdk_threads_add_timeout_full (G_PRIORITY_LOW, EXO_ICON_VIEW_SEARCH_DIALOG_TIMEOUT,
                                                           exo_icon_view_search_timeout, icon_view,
                                                           exo_icon_view_search_timeout_destroy);

  /* grab focus will select all the text, we don't want that to happen, so we
   * call the parent instance and bypass the selection change. This is probably
   * really hackish, but GtkTreeView does it as well *hrhr*
   */
  klass = g_type_class_peek_parent (GTK_ENTRY_GET_CLASS (icon_view->priv->search_entry));
  (*GTK_WIDGET_CLASS (klass)->grab_focus) (icon_view->priv->search_entry);

  /* send focus-in event */
  _exo_gtk_widget_send_focus_change (icon_view->priv->search_entry, TRUE);

  /* search first matching iter */
  exo_icon_view_search_init (icon_view->priv->search_entry, icon_view);

  return TRUE;
}



static gboolean
exo_icon_view_search_equal_func (GtkTreeModel *model,
                                 gint          column,
                                 const gchar  *key,
                                 GtkTreeIter  *iter,
                                 gpointer      user_data)
{
  const gchar *str;
  gboolean     retval = TRUE;
  GValue       transformed = { 0, };
  GValue       value = { 0, };
  gchar       *case_normalized_string = NULL;
  gchar       *case_normalized_key = NULL;
  gchar       *normalized_string;
  gchar       *normalized_key;

  /* determine the value for the column/iter */
  gtk_tree_model_get_value (model, iter, column, &value);

  /* try to transform the value to a string */
  g_value_init (&transformed, G_TYPE_STRING);
  if (!g_value_transform (&value, &transformed))
    {
      g_value_unset (&value);
      return TRUE;
    }
  g_value_unset (&value);

  /* check if we have a string value */
  str = g_value_get_string (&transformed);
  if (G_UNLIKELY (str == NULL))
    {
      g_value_unset (&transformed);
      return TRUE;
    }

  /* normalize the string and the key */
  normalized_string = g_utf8_normalize (str, -1, G_NORMALIZE_ALL);
  normalized_key = g_utf8_normalize (key, -1, G_NORMALIZE_ALL);

  /* check if we have normalized both string */
  if (G_LIKELY (normalized_string != NULL && normalized_key != NULL))
    {
      case_normalized_string = g_utf8_casefold (normalized_string, -1);
      case_normalized_key = g_utf8_casefold (normalized_key, -1);

      /* compare the casefolded strings */
      if (strncmp (case_normalized_key, case_normalized_string, strlen (case_normalized_key)) == 0)
        retval = FALSE;
    }

  /* cleanup */
  g_free (case_normalized_string);
  g_free (case_normalized_key);
  g_value_unset (&transformed);
  g_free (normalized_string);
  g_free (normalized_key);

  return retval;
}



static void
exo_icon_view_search_position_func (ExoIconView *icon_view,
                                    GtkWidget   *search_dialog,
                                    gpointer     user_data)
{
  GtkRequisition requisition;
  GdkRectangle   monitor;
  GdkWindow     *view_window = gtk_widget_get_window (GTK_WIDGET (icon_view));
  GdkScreen     *screen = gdk_window_get_screen (view_window);
  gint           view_width, view_height;
  gint           view_x, view_y;
  gint           monitor_num;
  gint           x, y;

  /* determine the monitor geometry */
  monitor_num = gdk_screen_get_monitor_at_window (screen, view_window);
  gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  /* make sure the search dialog is realized */
  gtk_widget_realize (search_dialog);

  gdk_window_get_origin (view_window, &view_x, &view_y);
#if GTK_CHECK_VERSION(2, 24, 0)
  view_width = gdk_window_get_width (view_window);
  view_height = gdk_window_get_height (view_window);
#else
  gdk_drawable_get_size (view_window, &view_width, &view_height);
#endif
#if GTK_CHECK_VERSION(3, 0, 0)
  gtk_widget_get_preferred_size (search_dialog, NULL, &requisition);
#else
  gtk_widget_size_request (search_dialog, &requisition);
#endif

  if (view_x + view_width > gdk_screen_get_width (screen))
    x = gdk_screen_get_width (screen) - requisition.width;
  else if (view_x + view_width - requisition.width < 0)
    x = 0;
  else
    x = view_x + view_width - requisition.width;

  if (view_y + view_height + requisition.height > gdk_screen_get_height (screen))
    y = gdk_screen_get_height (screen) - requisition.height;
  else if (view_y + view_height < 0) /* isn't really possible ... */
    y = 0;
  else
    y = view_y + view_height;

  gtk_window_move (GTK_WINDOW (search_dialog), x, y);
}



static gboolean
exo_icon_view_search_button_press_event (GtkWidget      *widget,
                                         GdkEventButton *event,
                                         ExoIconView    *icon_view)
{
  _exo_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  _exo_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), FALSE);

  /* hide the search dialog */
  exo_icon_view_search_dialog_hide (widget, icon_view);

  if (event->window == icon_view->priv->bin_window)
    exo_icon_view_button_press_event (GTK_WIDGET (icon_view), event);

  return TRUE;
}



static gboolean
exo_icon_view_search_delete_event (GtkWidget   *widget,
                                   GdkEventAny *event,
                                   ExoIconView *icon_view)
{
  _exo_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  _exo_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), FALSE);

  /* hide the search dialog */
  exo_icon_view_search_dialog_hide (widget, icon_view);

  return TRUE;
}



static gboolean
exo_icon_view_search_key_press_event (GtkWidget   *widget,
                                      GdkEventKey *event,
                                      ExoIconView *icon_view)
{
  gboolean retval = FALSE;

  _exo_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  _exo_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), FALSE);


  /* close window and cancel the search */
  if (event->keyval == GDK_KEY_Escape || event->keyval == GDK_KEY_Tab)
    {
      exo_icon_view_search_dialog_hide (widget, icon_view);
      return TRUE;
    }

  /* select previous matching iter */
  if (event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up)
    {
      exo_icon_view_search_move (widget, icon_view, TRUE);
      retval = TRUE;
    }

  if (((event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
      && (event->keyval == GDK_KEY_g || event->keyval == GDK_KEY_G))
    {
      exo_icon_view_search_move (widget, icon_view, TRUE);
      retval = TRUE;
    }

  /* select next matching iter */
  if (event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down)
    {
      exo_icon_view_search_move (widget, icon_view, FALSE);
      retval = TRUE;
    }

  if (((event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
      && (event->keyval == GDK_KEY_g || event->keyval == GDK_KEY_G))
    {
      exo_icon_view_search_move (widget, icon_view, FALSE);
      retval = TRUE;
    }

  /* renew the flush timeout */
  if (retval && (icon_view->priv->search_timeout_id != 0))
    {
      /* drop the previous timeout */
      g_source_remove (icon_view->priv->search_timeout_id);

      /* schedule a new timeout */
      icon_view->priv->search_timeout_id = gdk_threads_add_timeout_full (G_PRIORITY_LOW, EXO_ICON_VIEW_SEARCH_DIALOG_TIMEOUT,
                                                               exo_icon_view_search_timeout, icon_view,
                                                               exo_icon_view_search_timeout_destroy);
    }

  return retval;
}



static gboolean
exo_icon_view_search_scroll_event (GtkWidget      *widget,
                                   GdkEventScroll *event,
                                   ExoIconView    *icon_view)
{
  gboolean retval = TRUE;

  _exo_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  _exo_return_val_if_fail (EXO_IS_ICON_VIEW (icon_view), FALSE);

  if (event->direction == GDK_SCROLL_UP)
    exo_icon_view_search_move (widget, icon_view, TRUE);
  else if (event->direction == GDK_SCROLL_DOWN)
    exo_icon_view_search_move (widget, icon_view, FALSE);
  else
    retval = FALSE;

  return retval;
}



static gboolean
exo_icon_view_search_timeout (gpointer user_data)
{
  ExoIconView *icon_view = EXO_ICON_VIEW (user_data);

  if(!g_source_is_destroyed(g_main_current_source()))
    exo_icon_view_search_dialog_hide (icon_view->priv->search_window, icon_view);

  return FALSE;
}



static void
exo_icon_view_search_timeout_destroy (gpointer user_data)
{
  EXO_ICON_VIEW (user_data)->priv->search_timeout_id = 0;
}


/* Accessibility Support */

static gpointer accessible_parent_class;
static gpointer accessible_item_parent_class;
static GQuark accessible_private_data_quark = 0;

#define EXO_TYPE_ICON_VIEW_ITEM_ACCESSIBLE      (exo_icon_view_item_accessible_get_type ())
#define EXO_ICON_VIEW_ITEM_ACCESSIBLE(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXO_TYPE_ICON_VIEW_ITEM_ACCESSIBLE, ExoIconViewItemAccessible))
#define EXO_IS_ICON_VIEW_ITEM_ACCESSIBLE(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXO_TYPE_ICON_VIEW_ITEM_ACCESSIBLE))

static GType exo_icon_view_item_accessible_get_type (void);

enum {
    ACTION_ACTIVATE,
    LAST_ACTION
};

typedef struct
{
  AtkObject parent;
  ExoIconViewItem *item;
  GtkWidget *widget;
  AtkStateSet *state_set;
  gchar *text;
  GtkTextBuffer *text_buffer;
  gchar *action_descriptions[LAST_ACTION];
  gchar *image_description;
  guint action_idle_handler;
} ExoIconViewItemAccessible;

static const gchar *const exo_icon_view_item_accessible_action_names[] =
{
  "activate",
  NULL
};

static const gchar *const exo_icon_view_item_accessible_action_descriptions[] =
{
  "Activate item",
  NULL
};
typedef struct _ExoIconViewItemAccessibleClass
{
  AtkObjectClass parent_class;

} ExoIconViewItemAccessibleClass;

static gboolean exo_icon_view_item_accessible_is_showing (ExoIconViewItemAccessible *item);

static gboolean
exo_icon_view_item_accessible_idle_do_action (gpointer data)
{
  ExoIconViewItemAccessible *item;
  ExoIconView *icon_view;
  GtkTreePath *path;

  if(g_source_is_destroyed(g_main_current_source()))
    return FALSE;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (data);
  item->action_idle_handler = 0;

  if (item->widget != NULL)
    {
      icon_view = EXO_ICON_VIEW (item->widget);
      path = gtk_tree_path_new_from_indices (item->item->index, -1);
      exo_icon_view_item_activated (icon_view, path);
      gtk_tree_path_free (path);
    }

  return FALSE;
}

static gboolean
exo_icon_view_item_accessible_action_do_action (AtkAction *action,
                                                gint       i)
{
  ExoIconViewItemAccessible *item;

  if (i < 0 || i >= LAST_ACTION)
    return FALSE;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (action);

  if (!EXO_IS_ICON_VIEW (item->widget))
    return FALSE;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return FALSE;

  switch (i)
    {
    case ACTION_ACTIVATE:
      if (!item->action_idle_handler)
        item->action_idle_handler = gdk_threads_add_idle (exo_icon_view_item_accessible_idle_do_action, item);
      break;
    default:
      g_assert_not_reached ();
      return FALSE;

    }
  return TRUE;
}

static gint
exo_icon_view_item_accessible_action_get_n_actions (AtkAction *action)
{
        return LAST_ACTION;
}

static const gchar *
exo_icon_view_item_accessible_action_get_description (AtkAction *action,
                                                      gint       i)
{
  ExoIconViewItemAccessible *item;

  if (i < 0 || i >= LAST_ACTION)
    return NULL;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (action);

  if (item->action_descriptions[i])
    return item->action_descriptions[i];
  else
    return exo_icon_view_item_accessible_action_descriptions[i];
}

static const gchar *
exo_icon_view_item_accessible_action_get_name (AtkAction *action,
                                               gint       i)
{
  if (i < 0 || i >= LAST_ACTION)
    return NULL;

  return exo_icon_view_item_accessible_action_names[i];
}

static gboolean
exo_icon_view_item_accessible_action_set_description (AtkAction   *action,
                                                      gint         i,
                                                      const gchar *description)
{
  ExoIconViewItemAccessible *item;

  if (i < 0 || i >= LAST_ACTION)
    return FALSE;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (action);

  g_free (item->action_descriptions[i]);

  item->action_descriptions[i] = g_strdup (description);

  return TRUE;
}

static void
atk_action_item_interface_init (AtkActionIface *iface)
{
  iface->do_action = exo_icon_view_item_accessible_action_do_action;
  iface->get_n_actions = exo_icon_view_item_accessible_action_get_n_actions;
  iface->get_description = exo_icon_view_item_accessible_action_get_description;
  iface->get_name = exo_icon_view_item_accessible_action_get_name;
  iface->set_description = exo_icon_view_item_accessible_action_set_description;
}

static const gchar *
exo_icon_view_item_accessible_image_get_image_description (AtkImage *image)
{
  ExoIconViewItemAccessible *item;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (image);

  return item->image_description;
}

static gboolean
exo_icon_view_item_accessible_image_set_image_description (AtkImage    *image,
                                                           const gchar *description)
{
  ExoIconViewItemAccessible *item;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (image);

  g_free (item->image_description);
  item->image_description = g_strdup (description);

  return TRUE;
}

static gboolean
get_pixbuf_box (ExoIconView     *icon_view,
		ExoIconViewItem *item,
		GdkRectangle    *box)
{
  GList *l;

  for (l = icon_view->priv->cell_list; l; l = l->next)
    {
      ExoIconViewCellInfo *info = l->data;

      if (GTK_IS_CELL_RENDERER_PIXBUF (info->cell))
	{
	  if (info->position < item->n_cells)
	    *box = item->box[info->position];

	  return TRUE;
	}
    }

  return FALSE;
}

static gchar *
get_text (ExoIconView     *icon_view,
	  ExoIconViewItem *item)
{
  GList *l;
  gchar *text;

  for (l = icon_view->priv->cell_list; l; l = l->next)
    {
      ExoIconViewCellInfo *info = l->data;

      if (GTK_IS_CELL_RENDERER_TEXT (info->cell))
	{
	  g_object_get (info->cell, "text", &text, NULL);

	  return text;
	}
    }

  return NULL;
}

static void
exo_icon_view_item_accessible_image_get_image_size (AtkImage *image,
                                                    gint     *width,
                                                    gint     *height)
{
  ExoIconViewItemAccessible *item;
  GdkRectangle box;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (image);

  if (!EXO_IS_ICON_VIEW (item->widget))
    return;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return;

  if (get_pixbuf_box (EXO_ICON_VIEW (item->widget), item->item, &box))
    {
      *width = box.width;
      *height = box.height;
    }
}

static void
exo_icon_view_item_accessible_image_get_image_position (AtkImage    *image,
                                                        gint        *x,
                                                        gint        *y,
                                                        AtkCoordType coord_type)
{
  ExoIconViewItemAccessible *item;
  AtkObject *parent_obj;
  GdkRectangle box;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (image);

  if (!EXO_IS_ICON_VIEW (item->widget))
    return;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return;

  parent_obj = gtk_widget_get_accessible (item->widget);
  atk_component_get_extents (ATK_COMPONENT (parent_obj), x, y, NULL, NULL, coord_type);

  if (get_pixbuf_box (EXO_ICON_VIEW (item->widget), item->item, &box))
    {
      *x+= box.x - item->item->area.x;
      *y+= box.y - item->item->area.y;
    }

}

static void
atk_image_item_interface_init (AtkImageIface *iface)
{
  iface->get_image_description = exo_icon_view_item_accessible_image_get_image_description;
  iface->set_image_description = exo_icon_view_item_accessible_image_set_image_description;
  iface->get_image_size = exo_icon_view_item_accessible_image_get_image_size;
  iface->get_image_position = exo_icon_view_item_accessible_image_get_image_position;
}

static gchar *
exo_icon_view_item_accessible_text_get_text (AtkText *text,
                                             gint     start_pos,
                                             gint     end_pos)
{
  ExoIconViewItemAccessible *item;
  GtkTextIter start, end;
  GtkTextBuffer *buffer;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!EXO_IS_ICON_VIEW (item->widget))
    return NULL;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return NULL;

  buffer = item->text_buffer;
  gtk_text_buffer_get_iter_at_offset (buffer, &start, start_pos);
  if (end_pos < 0)
    gtk_text_buffer_get_end_iter (buffer, &end);
  else
    gtk_text_buffer_get_iter_at_offset (buffer, &end, end_pos);

  return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static gunichar
exo_icon_view_item_accessible_text_get_character_at_offset (AtkText *text,
                                                            gint     offset)
{
  ExoIconViewItemAccessible *item;
  GtkTextIter start, end;
  GtkTextBuffer *buffer;
  gchar *string;
  gunichar unichar;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!EXO_IS_ICON_VIEW (item->widget))
    return '\0';

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return '\0';

  buffer = item->text_buffer;
  if (offset >= gtk_text_buffer_get_char_count (buffer))
    return '\0';

  gtk_text_buffer_get_iter_at_offset (buffer, &start, offset);
  end = start;
  gtk_text_iter_forward_char (&end);
  string = gtk_text_buffer_get_slice (buffer, &start, &end, FALSE);
  unichar = g_utf8_get_char (string);
  g_free(string);

  return unichar;
}

static gchar*
exo_icon_view_item_accessible_text_get_text_before_offset (AtkText         *text,
                                                           gint            offset,
                                                           AtkTextBoundary boundary_type,
                                                           gint            *start_offset,
                                                           gint            *end_offset)
{
  ExoIconViewItemAccessible *item;
  GtkTextIter start, end;
  GtkTextBuffer *buffer;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!EXO_IS_ICON_VIEW (item->widget))
    return NULL;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return NULL;

  buffer = item->text_buffer;

  if (!gtk_text_buffer_get_char_count (buffer))
    {
      *start_offset = 0;
      *end_offset = 0;
      return g_strdup ("");
    }
  gtk_text_buffer_get_iter_at_offset (buffer, &start, offset);

  end = start;

  switch (boundary_type)
    {
    case ATK_TEXT_BOUNDARY_CHAR:
      gtk_text_iter_backward_char(&start);
      break;
    case ATK_TEXT_BOUNDARY_WORD_START:
      if (!gtk_text_iter_starts_word (&start))
        gtk_text_iter_backward_word_start (&start);
      end = start;
      gtk_text_iter_backward_word_start(&start);
      break;
    case ATK_TEXT_BOUNDARY_WORD_END:
      if (gtk_text_iter_inside_word (&start) &&
          !gtk_text_iter_starts_word (&start))
        gtk_text_iter_backward_word_start (&start);
      while (!gtk_text_iter_ends_word (&start))
        {
          if (!gtk_text_iter_backward_char (&start))
            break;
        }
      end = start;
      gtk_text_iter_backward_word_start(&start);
      while (!gtk_text_iter_ends_word (&start))
        {
          if (!gtk_text_iter_backward_char (&start))
            break;
        }
      break;
    case ATK_TEXT_BOUNDARY_SENTENCE_START:
      if (!gtk_text_iter_starts_sentence (&start))
        gtk_text_iter_backward_sentence_start (&start);
      end = start;
      gtk_text_iter_backward_sentence_start (&start);
      break;
    case ATK_TEXT_BOUNDARY_SENTENCE_END:
      if (gtk_text_iter_inside_sentence (&start) &&
          !gtk_text_iter_starts_sentence (&start))
        gtk_text_iter_backward_sentence_start (&start);
      while (!gtk_text_iter_ends_sentence (&start))
        {
          if (!gtk_text_iter_backward_char (&start))
            break;
        }
      end = start;
      gtk_text_iter_backward_sentence_start (&start);
      while (!gtk_text_iter_ends_sentence (&start))
        {
          if (!gtk_text_iter_backward_char (&start))
            break;
        }
      break;
   case ATK_TEXT_BOUNDARY_LINE_START:
   case ATK_TEXT_BOUNDARY_LINE_END:
      break;
    }

  *start_offset = gtk_text_iter_get_offset (&start);
  *end_offset = gtk_text_iter_get_offset (&end);

  return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static gchar*
exo_icon_view_item_accessible_text_get_text_at_offset (AtkText         *text,
                                                       gint            offset,
                                                       AtkTextBoundary boundary_type,
                                                       gint            *start_offset,
                                                       gint            *end_offset)
{
  ExoIconViewItemAccessible *item;
  GtkTextIter start, end;
  GtkTextBuffer *buffer;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!EXO_IS_ICON_VIEW (item->widget))
    return NULL;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return NULL;

  buffer = item->text_buffer;

  if (!gtk_text_buffer_get_char_count (buffer))
    {
      *start_offset = 0;
      *end_offset = 0;
      return g_strdup ("");
    }
  gtk_text_buffer_get_iter_at_offset (buffer, &start, offset);

  end = start;

  switch (boundary_type)
    {
    case ATK_TEXT_BOUNDARY_CHAR:
      gtk_text_iter_forward_char (&end);
      break;
    case ATK_TEXT_BOUNDARY_WORD_START:
      if (!gtk_text_iter_starts_word (&start))
        gtk_text_iter_backward_word_start (&start);
      if (gtk_text_iter_inside_word (&end))
        gtk_text_iter_forward_word_end (&end);
      while (!gtk_text_iter_starts_word (&end))
        {
          if (!gtk_text_iter_forward_char (&end))
            break;
        }
      break;
    case ATK_TEXT_BOUNDARY_WORD_END:
      if (gtk_text_iter_inside_word (&start) &&
          !gtk_text_iter_starts_word (&start))
        gtk_text_iter_backward_word_start (&start);
      while (!gtk_text_iter_ends_word (&start))
        {
          if (!gtk_text_iter_backward_char (&start))
            break;
        }
      gtk_text_iter_forward_word_end (&end);
      break;
    case ATK_TEXT_BOUNDARY_SENTENCE_START:
      if (!gtk_text_iter_starts_sentence (&start))
        gtk_text_iter_backward_sentence_start (&start);
      if (gtk_text_iter_inside_sentence (&end))
        gtk_text_iter_forward_sentence_end (&end);
      while (!gtk_text_iter_starts_sentence (&end))
        {
          if (!gtk_text_iter_forward_char (&end))
            break;
        }
      break;
    case ATK_TEXT_BOUNDARY_SENTENCE_END:
      if (gtk_text_iter_inside_sentence (&start) &&
          !gtk_text_iter_starts_sentence (&start))
        gtk_text_iter_backward_sentence_start (&start);
      while (!gtk_text_iter_ends_sentence (&start))
        {
          if (!gtk_text_iter_backward_char (&start))
            break;
        }
      gtk_text_iter_forward_sentence_end (&end);
      break;
   case ATK_TEXT_BOUNDARY_LINE_START:
   case ATK_TEXT_BOUNDARY_LINE_END:
      break;
    }


  *start_offset = gtk_text_iter_get_offset (&start);
  *end_offset = gtk_text_iter_get_offset (&end);

  return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static gchar*
exo_icon_view_item_accessible_text_get_text_after_offset (AtkText         *text,
                                                          gint            offset,
                                                          AtkTextBoundary boundary_type,
                                                          gint            *start_offset,
                                                          gint            *end_offset)
{
  ExoIconViewItemAccessible *item;
  GtkTextIter start, end;
  GtkTextBuffer *buffer;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!EXO_IS_ICON_VIEW (item->widget))
    return NULL;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return NULL;

  buffer = item->text_buffer;

  if (!gtk_text_buffer_get_char_count (buffer))
    {
      *start_offset = 0;
      *end_offset = 0;
      return g_strdup ("");
    }
  gtk_text_buffer_get_iter_at_offset (buffer, &start, offset);

  end = start;

  switch (boundary_type)
    {
    case ATK_TEXT_BOUNDARY_CHAR:
      gtk_text_iter_forward_char(&start);
      gtk_text_iter_forward_chars(&end, 2);
      break;
    case ATK_TEXT_BOUNDARY_WORD_START:
      if (gtk_text_iter_inside_word (&end))
        gtk_text_iter_forward_word_end (&end);
      while (!gtk_text_iter_starts_word (&end))
        {
          if (!gtk_text_iter_forward_char (&end))
            break;
        }
      start = end;
      if (!gtk_text_iter_is_end (&end))
        {
          gtk_text_iter_forward_word_end (&end);
          while (!gtk_text_iter_starts_word (&end))
            {
              if (!gtk_text_iter_forward_char (&end))
                break;
            }
        }
      break;
    case ATK_TEXT_BOUNDARY_WORD_END:
      gtk_text_iter_forward_word_end (&end);
      start = end;
      if (!gtk_text_iter_is_end (&end))
        gtk_text_iter_forward_word_end (&end);
      break;
    case ATK_TEXT_BOUNDARY_SENTENCE_START:
      if (gtk_text_iter_inside_sentence (&end))
        gtk_text_iter_forward_sentence_end (&end);
      while (!gtk_text_iter_starts_sentence (&end))
        {
          if (!gtk_text_iter_forward_char (&end))
            break;
        }
      start = end;
      if (!gtk_text_iter_is_end (&end))
        {
          gtk_text_iter_forward_sentence_end (&end);
          while (!gtk_text_iter_starts_sentence (&end))
            {
              if (!gtk_text_iter_forward_char (&end))
                break;
            }
        }
      break;
    case ATK_TEXT_BOUNDARY_SENTENCE_END:
      gtk_text_iter_forward_sentence_end (&end);
      start = end;
      if (!gtk_text_iter_is_end (&end))
        gtk_text_iter_forward_sentence_end (&end);
      break;
   case ATK_TEXT_BOUNDARY_LINE_START:
   case ATK_TEXT_BOUNDARY_LINE_END:
      break;
    }
  *start_offset = gtk_text_iter_get_offset (&start);
  *end_offset = gtk_text_iter_get_offset (&end);

  return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static gint
exo_icon_view_item_accessible_text_get_character_count (AtkText *text)
{
  ExoIconViewItemAccessible *item;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!EXO_IS_ICON_VIEW (item->widget))
    return 0;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return 0;

  return gtk_text_buffer_get_char_count (item->text_buffer);
}

static void
exo_icon_view_item_accessible_text_get_character_extents (AtkText      *text,
                                                          gint         offset,
                                                          gint         *x,
                                                          gint         *y,
                                                          gint         *width,
                                                          gint         *height,
                                                          AtkCoordType coord_type)
{
  ExoIconViewItemAccessible *item;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!EXO_IS_ICON_VIEW (item->widget))
    return;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return;

}

static gint
exo_icon_view_item_accessible_text_get_offset_at_point (AtkText      *text,
                                                        gint          x,
                                                        gint          y,
                                                        AtkCoordType coord_type)
{
  ExoIconViewItemAccessible *item;
  gint offset = 0;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!EXO_IS_ICON_VIEW (item->widget))
    return -1;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return -1;

  return offset;
}

static void
atk_text_item_interface_init (AtkTextIface *iface)
{
  iface->get_text = exo_icon_view_item_accessible_text_get_text;
  iface->get_character_at_offset = exo_icon_view_item_accessible_text_get_character_at_offset;
  iface->get_text_before_offset = exo_icon_view_item_accessible_text_get_text_before_offset;
  iface->get_text_at_offset = exo_icon_view_item_accessible_text_get_text_at_offset;
  iface->get_text_after_offset = exo_icon_view_item_accessible_text_get_text_after_offset;
  iface->get_character_count = exo_icon_view_item_accessible_text_get_character_count;
  iface->get_character_extents = exo_icon_view_item_accessible_text_get_character_extents;
  iface->get_offset_at_point = exo_icon_view_item_accessible_text_get_offset_at_point;
}

static void
exo_icon_view_item_accessible_get_extents (AtkComponent *component,
                                           gint         *x,
                                           gint         *y,
                                           gint         *width,
                                           gint         *height,
                                           AtkCoordType  coord_type)
{
  ExoIconViewItemAccessible *item;
  AtkObject *parent_obj;
  gint l_x, l_y;

  g_return_if_fail (EXO_IS_ICON_VIEW_ITEM_ACCESSIBLE (component));

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (component);
  if (!GTK_IS_WIDGET (item->widget))
    return;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return;

  *width = item->item->area.width;
  *height = item->item->area.height;
  if (exo_icon_view_item_accessible_is_showing (item))
    {
      parent_obj = gtk_widget_get_accessible (item->widget);
      atk_component_get_extents (ATK_COMPONENT (parent_obj), &l_x, &l_y, NULL,
                                 NULL, coord_type);
      *x = l_x + item->item->area.x;
      *y = l_y + item->item->area.y;
    }
  else
    {
      *x = G_MININT;
      *y = G_MININT;
    }
}

static gboolean
exo_icon_view_item_accessible_grab_focus (AtkComponent *component)
{
  ExoIconViewItemAccessible *item;
  GtkWidget *toplevel;

  g_return_val_if_fail (EXO_IS_ICON_VIEW_ITEM_ACCESSIBLE (component), FALSE);

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (component);
  if (!GTK_IS_WIDGET (item->widget))
    return FALSE;

  gtk_widget_grab_focus (item->widget);
  exo_icon_view_set_cursor_item (EXO_ICON_VIEW (item->widget), item->item, -1);
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (item->widget));
  if (gtk_widget_is_toplevel (toplevel))
    gtk_window_present (GTK_WINDOW (toplevel));

  return TRUE;
}

static void
atk_component_item_interface_init (AtkComponentIface *iface)
{
  iface->get_extents = exo_icon_view_item_accessible_get_extents;
  iface->grab_focus = exo_icon_view_item_accessible_grab_focus;
}

static gboolean
exo_icon_view_item_accessible_add_state (ExoIconViewItemAccessible *item,
                                         AtkStateType               state_type,
                                         gboolean                   emit_signal)
{
  gboolean rc;

  rc = atk_state_set_add_state (item->state_set, state_type);
  /*
   * The signal should only be generated if the value changed,
   * not when the item is set up.  So states that are set
   * initially should pass FALSE as the emit_signal argument.
   */

  if (emit_signal)
    {
      atk_object_notify_state_change (ATK_OBJECT (item), state_type, TRUE);
      /* If state_type is ATK_STATE_VISIBLE, additional notification */
      if (state_type == ATK_STATE_VISIBLE)
        g_signal_emit_by_name (item, "visible-data-changed");
    }

  return rc;
}

static gboolean
exo_icon_view_item_accessible_remove_state (ExoIconViewItemAccessible *item,
                                            AtkStateType               state_type,
                                            gboolean                   emit_signal)
{
  if (atk_state_set_contains_state (item->state_set, state_type))
    {
      gboolean rc;

      rc = atk_state_set_remove_state (item->state_set, state_type);
      /*
       * The signal should only be generated if the value changed,
       * not when the item is set up.  So states that are set
       * initially should pass FALSE as the emit_signal argument.
       */

      if (emit_signal)
        {
          atk_object_notify_state_change (ATK_OBJECT (item), state_type, FALSE);
          /* If state_type is ATK_STATE_VISIBLE, additional notification */
          if (state_type == ATK_STATE_VISIBLE)
            g_signal_emit_by_name (item, "visible-data-changed");
        }

      return rc;
    }
  else
    return FALSE;
}

static gboolean
exo_icon_view_item_accessible_is_showing (ExoIconViewItemAccessible *item)
{
  ExoIconView *icon_view;
  GtkAllocation allocation;
  GdkRectangle visible_rect;
  gboolean is_showing;

  /*
   * An item is considered "SHOWING" if any part of the item is in the
   * visible rectangle.
   */

  if (!EXO_IS_ICON_VIEW (item->widget))
    return FALSE;

  if (item->item == NULL)
    return FALSE;

  gtk_widget_get_allocation (item->widget, &allocation);

  icon_view = EXO_ICON_VIEW (item->widget);
  visible_rect.x = 0;
  if (icon_view->priv->hadjustment)
    visible_rect.x += gtk_adjustment_get_value (icon_view->priv->hadjustment);
  visible_rect.y = 0;
  if (icon_view->priv->hadjustment)
    visible_rect.y += gtk_adjustment_get_value (icon_view->priv->vadjustment);
  visible_rect.width = allocation.width;
  visible_rect.height = allocation.height;

  if (((item->item->area.x + item->item->area.width) < visible_rect.x) ||
     ((item->item->area.y + item->item->area.height) < (visible_rect.y)) ||
     (item->item->area.x > (visible_rect.x + visible_rect.width)) ||
     (item->item->area.y > (visible_rect.y + visible_rect.height)))
    is_showing =  FALSE;
  else
    is_showing = TRUE;

  return is_showing;
}

static gboolean
exo_icon_view_item_accessible_set_visibility (ExoIconViewItemAccessible *item,
                                              gboolean                   emit_signal)
{
  if (exo_icon_view_item_accessible_is_showing (item))
    return exo_icon_view_item_accessible_add_state (item, ATK_STATE_SHOWING,
						    emit_signal);
  else
    return exo_icon_view_item_accessible_remove_state (item, ATK_STATE_SHOWING,
						       emit_signal);
}

static void
exo_icon_view_item_accessible_object_init (ExoIconViewItemAccessible *item)
{
  gint i;

  item->state_set = atk_state_set_new ();

  atk_state_set_add_state (item->state_set, ATK_STATE_ENABLED);
  atk_state_set_add_state (item->state_set, ATK_STATE_FOCUSABLE);
  atk_state_set_add_state (item->state_set, ATK_STATE_SENSITIVE);
  atk_state_set_add_state (item->state_set, ATK_STATE_SELECTABLE);
  atk_state_set_add_state (item->state_set, ATK_STATE_VISIBLE);

  for (i = 0; i < LAST_ACTION; i++)
    item->action_descriptions[i] = NULL;

  item->image_description = NULL;

  item->action_idle_handler = 0;
}

static void
exo_icon_view_item_accessible_finalize (GObject *object)
{
  ExoIconViewItemAccessible *item;
  gint i;

  g_return_if_fail (EXO_IS_ICON_VIEW_ITEM_ACCESSIBLE (object));

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (object);

  if (item->widget)
    g_object_remove_weak_pointer (G_OBJECT (item->widget), (gpointer) &item->widget);

  if (item->state_set)
    g_object_unref (item->state_set);

  if (item->text_buffer)
     g_object_unref (item->text_buffer);

  for (i = 0; i < LAST_ACTION; i++)
    g_free (item->action_descriptions[i]);

  g_free (item->image_description);

  if (item->action_idle_handler)
    {
      g_source_remove (item->action_idle_handler);
      item->action_idle_handler = 0;
    }

  G_OBJECT_CLASS (accessible_item_parent_class)->finalize (object);
}

static AtkObject*
exo_icon_view_item_accessible_get_parent (AtkObject *obj)
{
  ExoIconViewItemAccessible *item;

  g_return_val_if_fail (EXO_IS_ICON_VIEW_ITEM_ACCESSIBLE (obj), NULL);
  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (obj);

  if (item->widget)
    return gtk_widget_get_accessible (item->widget);
  else
    return NULL;
}

static gint
exo_icon_view_item_accessible_get_index_in_parent (AtkObject *obj)
{
  ExoIconViewItemAccessible *item;

  g_return_val_if_fail (EXO_IS_ICON_VIEW_ITEM_ACCESSIBLE (obj), 0);
  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (obj);

  return item->item->index;
}

static AtkStateSet *
exo_icon_view_item_accessible_ref_state_set (AtkObject *obj)
{
  ExoIconViewItemAccessible *item;
  ExoIconView *icon_view;

  item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (obj);
  g_return_val_if_fail (item->state_set, NULL);

  if (!item->widget)
    return NULL;

  icon_view = EXO_ICON_VIEW (item->widget);
  if (icon_view->priv->cursor_item == item->item)
    atk_state_set_add_state (item->state_set, ATK_STATE_FOCUSED);
  else
    atk_state_set_remove_state (item->state_set, ATK_STATE_FOCUSED);
  if (item->item->selected)
    atk_state_set_add_state (item->state_set, ATK_STATE_SELECTED);
  else
    atk_state_set_remove_state (item->state_set, ATK_STATE_SELECTED);

  return g_object_ref (item->state_set);
}

static void
exo_icon_view_item_accessible_class_init (AtkObjectClass *klass)
{
  GObjectClass *gobject_class;

  accessible_item_parent_class = g_type_class_peek_parent (klass);

  gobject_class = (GObjectClass *)klass;

  gobject_class->finalize = exo_icon_view_item_accessible_finalize;

  klass->get_index_in_parent = exo_icon_view_item_accessible_get_index_in_parent;
  klass->get_parent = exo_icon_view_item_accessible_get_parent;
  klass->ref_state_set = exo_icon_view_item_accessible_ref_state_set;
}

static GType
exo_icon_view_item_accessible_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo tinfo =
      {
        sizeof (ExoIconViewItemAccessibleClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) exo_icon_view_item_accessible_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        sizeof (ExoIconViewItemAccessible), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) exo_icon_view_item_accessible_object_init, /* instance init */
        NULL /* value table */
      };

      const GInterfaceInfo atk_component_info =
      {
        (GInterfaceInitFunc) atk_component_item_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };
      const GInterfaceInfo atk_action_info =
      {
        (GInterfaceInitFunc) atk_action_item_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };
      const GInterfaceInfo atk_image_info =
      {
        (GInterfaceInitFunc) atk_image_item_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };
      const GInterfaceInfo atk_text_info =
      {
        (GInterfaceInitFunc) atk_text_item_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      type = g_type_register_static (ATK_TYPE_OBJECT,
                                     I_("ExoIconViewItemAccessible"), &tinfo, 0);
      g_type_add_interface_static (type, ATK_TYPE_COMPONENT,
                                   &atk_component_info);
      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);
      g_type_add_interface_static (type, ATK_TYPE_IMAGE,
                                   &atk_image_info);
      g_type_add_interface_static (type, ATK_TYPE_TEXT,
                                   &atk_text_info);
    }

  return type;
}

#define EXO_TYPE_ICON_VIEW_ACCESSIBLE      (exo_icon_view_accessible_get_type ())
#define EXO_ICON_VIEW_ACCESSIBLE(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXO_TYPE_ICON_VIEW_ACCESSIBLE, ExoIconViewAccessible))
#define EXO_IS_ICON_VIEW_ACCESSIBLE(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXO_TYPE_ICON_VIEW_ACCESSIBLE))

static GType exo_icon_view_accessible_get_type (void);

typedef struct
{
   AtkObject parent;
} ExoIconViewAccessible;

typedef struct
{
  AtkObject *item;
  gint       index;
} ExoIconViewItemAccessibleInfo;

typedef struct
{
  GList *items;

  GtkAdjustment *old_hadj;
  GtkAdjustment *old_vadj;

  GtkTreeModel *model;

} ExoIconViewAccessiblePrivate;

static ExoIconViewAccessiblePrivate *
exo_icon_view_accessible_get_priv (AtkObject *accessible)
{
  return g_object_get_qdata (G_OBJECT (accessible),
                             accessible_private_data_quark);
}

static void
exo_icon_view_item_accessible_info_new (AtkObject *accessible,
                                        AtkObject *item,
                                        gint       index)
{
  ExoIconViewItemAccessibleInfo *info;
  ExoIconViewItemAccessibleInfo *tmp_info;
  ExoIconViewAccessiblePrivate *priv;
  GList *items;

  info = g_new (ExoIconViewItemAccessibleInfo, 1);
  info->item = item;
  info->index = index;

  priv = exo_icon_view_accessible_get_priv (accessible);
  items = priv->items;
  while (items)
    {
      tmp_info = items->data;
      if (tmp_info->index > index)
        break;
      items = items->next;
    }
  priv->items = g_list_insert_before (priv->items, items, info);
  priv->old_hadj = NULL;
  priv->old_vadj = NULL;
}

static gint
exo_icon_view_accessible_get_n_children (AtkObject *accessible)
{
  ExoIconView *icon_view;
  GtkWidget *widget;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
  if (!widget)
      return 0;

  icon_view = EXO_ICON_VIEW (widget);

  return g_list_length (icon_view->priv->items);
}

static AtkObject *
exo_icon_view_accessible_find_child (AtkObject *accessible,
                                     gint       index)
{
  ExoIconViewAccessiblePrivate *priv;
  ExoIconViewItemAccessibleInfo *info;
  GList *items;

  priv = exo_icon_view_accessible_get_priv (accessible);
  items = priv->items;

  while (items)
    {
      info = items->data;
      if (info->index == index)
        return info->item;
      items = items->next;
    }
  return NULL;
}

static AtkObject *
exo_icon_view_accessible_ref_child (AtkObject *accessible,
                                    gint       index)
{
  ExoIconView *icon_view;
  GtkWidget *widget;
  GList *icons;
  AtkObject *obj;
  ExoIconViewItemAccessible *a11y_item;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
  if (!widget)
    return NULL;

  icon_view = EXO_ICON_VIEW (widget);
  icons = g_list_nth (icon_view->priv->items, index);
  obj = NULL;
  if (icons)
    {
      ExoIconViewItem *item = icons->data;

      g_return_val_if_fail (item->index == index, NULL);
      obj = exo_icon_view_accessible_find_child (accessible, index);
      if (!obj)
        {
          gchar *text;

          obj = g_object_new (exo_icon_view_item_accessible_get_type (), NULL);
          exo_icon_view_item_accessible_info_new (accessible,
                                                  obj,
                                                  index);
          obj->role = ATK_ROLE_ICON;
          a11y_item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (obj);
          a11y_item->item = item;
          a11y_item->widget = widget;
          a11y_item->text_buffer = gtk_text_buffer_new (NULL);

	  exo_icon_view_set_cell_data (icon_view, item);
          text = get_text (icon_view, item);
          if (text)
            {
              gtk_text_buffer_set_text (a11y_item->text_buffer, text, -1);
              g_free (text);
            }

          exo_icon_view_item_accessible_set_visibility (a11y_item, FALSE);
          g_object_add_weak_pointer (G_OBJECT (widget), (gpointer) &(a11y_item->widget));
       }
      g_object_ref (obj);
    }
  return obj;
}

static void
exo_icon_view_accessible_traverse_items (ExoIconViewAccessible *view,
                                         GList                 *list)
{
  ExoIconViewAccessiblePrivate *priv;
  ExoIconViewItemAccessibleInfo *info;
  ExoIconViewItemAccessible *item;
  GList *items;

  priv =  exo_icon_view_accessible_get_priv (ATK_OBJECT (view));
  if (priv->items)
    {
      GtkWidget *widget;
      gboolean act_on_item;

      widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (view));
      if (widget == NULL)
        return;

      items = priv->items;

      act_on_item = (list == NULL);

      while (items)
        {

          info = (ExoIconViewItemAccessibleInfo *)items->data;
          item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (info->item);

          if (act_on_item == FALSE && list == items)
            act_on_item = TRUE;

          if (act_on_item)
	    exo_icon_view_item_accessible_set_visibility (item, TRUE);

          items = items->next;
       }
   }
}

static void
exo_icon_view_accessible_adjustment_changed (GtkAdjustment *adjustment,
                                             ExoIconView   *icon_view)
{
  AtkObject *obj;
  ExoIconViewAccessible *view;

  /*
   * The scrollbars have changed
   */
  obj = gtk_widget_get_accessible (GTK_WIDGET (icon_view));
  view = EXO_ICON_VIEW_ACCESSIBLE (obj);

  exo_icon_view_accessible_traverse_items (view, NULL);
}

static void
exo_icon_view_accessible_set_scroll_adjustments (GtkWidget      *widget,
                                                 GtkAdjustment *hadj,
                                                 GtkAdjustment *vadj)
{
  AtkObject *atk_obj;
  ExoIconViewAccessiblePrivate *priv;

  atk_obj = gtk_widget_get_accessible (widget);
  priv = exo_icon_view_accessible_get_priv (atk_obj);

  if (priv->old_hadj != hadj)
    {
      if (priv->old_hadj)
        {
          g_object_remove_weak_pointer (G_OBJECT (priv->old_hadj),
                                        (gpointer *)&priv->old_hadj);

          g_signal_handlers_disconnect_by_func (priv->old_hadj,
                                                (gpointer) exo_icon_view_accessible_adjustment_changed,
                                                widget);
        }
      priv->old_hadj = hadj;
      if (priv->old_hadj)
        {
          g_object_add_weak_pointer (G_OBJECT (priv->old_hadj),
                                     (gpointer *)&priv->old_hadj);
          g_signal_connect (hadj,
                            "value-changed",
                            G_CALLBACK (exo_icon_view_accessible_adjustment_changed),
                            widget);
        }
    }
  if (priv->old_vadj != vadj)
    {
      if (priv->old_vadj)
        {
          g_object_remove_weak_pointer (G_OBJECT (priv->old_vadj),
                                        (gpointer *)&priv->old_vadj);

          g_signal_handlers_disconnect_by_func (priv->old_vadj,
                                                (gpointer) exo_icon_view_accessible_adjustment_changed,
                                                widget);
        }
      priv->old_vadj = vadj;
      if (priv->old_vadj)
        {
          g_object_add_weak_pointer (G_OBJECT (priv->old_vadj),
                                     (gpointer *)&priv->old_vadj);
          g_signal_connect (vadj,
                            "value-changed",
                            G_CALLBACK (exo_icon_view_accessible_adjustment_changed),
                            widget);
        }
    }
}

static void
exo_icon_view_accessible_model_row_changed (GtkTreeModel *tree_model,
                                            GtkTreePath  *path,
                                            GtkTreeIter  *iter,
                                            gpointer      user_data)
{
  AtkObject *atk_obj;
  gint index;
  GtkWidget *widget;
  ExoIconView *icon_view;
  ExoIconViewItem *item;
  //ExoIconViewAccessible *a11y_view;
  ExoIconViewItemAccessible *a11y_item;
  const gchar *name;
  gchar *text;

  atk_obj = gtk_widget_get_accessible (GTK_WIDGET (user_data));
  //a11y_view = EXO_ICON_VIEW_ACCESSIBLE (atk_obj);
  index = gtk_tree_path_get_indices(path)[0];
  a11y_item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (
      exo_icon_view_accessible_find_child (atk_obj, index));

  if (a11y_item)
    {
      widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (atk_obj));
      icon_view = EXO_ICON_VIEW (widget);
      item = a11y_item->item;

      name = atk_object_get_name (ATK_OBJECT (a11y_item));

      if (!name || strcmp (name, "") == 0)
        {
          exo_icon_view_set_cell_data (icon_view, item);
          text = get_text (icon_view, item);
          if (text)
            {
              gtk_text_buffer_set_text (a11y_item->text_buffer, text, -1);
              g_free (text);
            }
        }
    }

  g_signal_emit_by_name (atk_obj, "visible-data-changed");

  return;
}

static void
exo_icon_view_accessible_model_row_inserted (GtkTreeModel *tree_model,
                                             GtkTreePath  *path,
                                             GtkTreeIter  *iter,
                                             gpointer     user_data)
{
  ExoIconViewAccessiblePrivate *priv;
  ExoIconViewItemAccessibleInfo *info;
  ExoIconViewAccessible *view;
  ExoIconViewItemAccessible *item;
  GList *items;
  GList *tmp_list;
  AtkObject *atk_obj;
  gint index;

  index = gtk_tree_path_get_indices(path)[0];
  atk_obj = gtk_widget_get_accessible (GTK_WIDGET (user_data));
  view = EXO_ICON_VIEW_ACCESSIBLE (atk_obj);
  priv = exo_icon_view_accessible_get_priv (atk_obj);

  items = priv->items;
  tmp_list = NULL;
  while (items)
    {
      info = items->data;
      item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (info->item);
      if (info->index != item->item->index)
        {
          if (info->index < index)
            g_warning ("Unexpected index value on insertion %d %d", index, info->index);

          if (tmp_list == NULL)
            tmp_list = items;

          info->index = item->item->index;
        }

      items = items->next;
    }
  exo_icon_view_accessible_traverse_items (view, tmp_list);
  g_signal_emit_by_name (atk_obj, "children-changed::add",
                         index, NULL, NULL);
  return;
}

static void
exo_icon_view_accessible_model_row_deleted (GtkTreeModel *tree_model,
                                            GtkTreePath  *path,
                                            gpointer     user_data)
{
  ExoIconViewAccessiblePrivate *priv;
  ExoIconViewItemAccessibleInfo *info;
  ExoIconViewAccessible *view;
  ExoIconViewItemAccessible *item;
  GList *items;
  GList *tmp_list;
  GList *deleted_item;
  AtkObject *atk_obj;
  gint index;

  index = gtk_tree_path_get_indices(path)[0];
  atk_obj = gtk_widget_get_accessible (GTK_WIDGET (user_data));
  view = EXO_ICON_VIEW_ACCESSIBLE (atk_obj);
  priv = exo_icon_view_accessible_get_priv (atk_obj);

  items = priv->items;
  tmp_list = NULL;
  deleted_item = NULL;
  info = NULL;
  while (items)
    {
      info = items->data;
      item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (info->item);
      if (info->index == index)
        {
          deleted_item = items;
        }
      if (info->index != item->item->index)
        {
          if (tmp_list == NULL)
            tmp_list = items;

          info->index = item->item->index;
        }

      items = items->next;
    }
  exo_icon_view_accessible_traverse_items (view, tmp_list);
  if (deleted_item)
    {
      info = deleted_item->data;
      exo_icon_view_item_accessible_add_state (EXO_ICON_VIEW_ITEM_ACCESSIBLE (info->item), ATK_STATE_DEFUNCT, TRUE);
      g_signal_emit_by_name (atk_obj, "children-changed::remove",
                             index, NULL, NULL);
      priv->items = g_list_remove_link (priv->items, deleted_item);
      g_free (info);
    }

  return;
}

static gint
exo_icon_view_accessible_item_compare (ExoIconViewItemAccessibleInfo *i1,
                                       ExoIconViewItemAccessibleInfo *i2)
{
  return i1->index - i2->index;
}

static void
exo_icon_view_accessible_model_rows_reordered (GtkTreeModel *tree_model,
                                               GtkTreePath  *path,
                                               GtkTreeIter  *iter,
                                               gint         *new_order,
                                               gpointer     user_data)
{
  ExoIconViewAccessiblePrivate *priv;
  ExoIconViewItemAccessibleInfo *info;
  ExoIconView *icon_view;
  ExoIconViewItemAccessible *item;
  GList *items;
  AtkObject *atk_obj;
  gint *order;
  gint length, i;

  atk_obj = gtk_widget_get_accessible (GTK_WIDGET (user_data));
  icon_view = EXO_ICON_VIEW (user_data);
  priv = exo_icon_view_accessible_get_priv (atk_obj);

  length = gtk_tree_model_iter_n_children (tree_model, NULL);

  order = g_new (gint, length);
  for (i = 0; i < length; i++)
    order [new_order[i]] = i;

  items = priv->items;
  while (items)
    {
      info = items->data;
      item = EXO_ICON_VIEW_ITEM_ACCESSIBLE (info->item);
      info->index = order[info->index];
      item->item = g_list_nth_data (icon_view->priv->items, info->index);
      items = items->next;
    }
  g_free (order);
  priv->items = g_list_sort (priv->items,
                             (GCompareFunc)exo_icon_view_accessible_item_compare);

  return;
}

static void
exo_icon_view_accessible_disconnect_model_signals (GtkTreeModel *model,
                                                   GtkWidget *widget)
{
  GObject *obj;

  obj = G_OBJECT (model);
  g_signal_handlers_disconnect_by_func (obj, (gpointer) exo_icon_view_accessible_model_row_changed, widget);
  g_signal_handlers_disconnect_by_func (obj, (gpointer) exo_icon_view_accessible_model_row_inserted, widget);
  g_signal_handlers_disconnect_by_func (obj, (gpointer) exo_icon_view_accessible_model_row_deleted, widget);
  g_signal_handlers_disconnect_by_func (obj, (gpointer) exo_icon_view_accessible_model_rows_reordered, widget);
}

static void
exo_icon_view_accessible_connect_model_signals (ExoIconView *icon_view)
{
  GObject *obj;

  obj = G_OBJECT (icon_view->priv->model);
  g_signal_connect_data (obj, "row-changed",
                         (GCallback) exo_icon_view_accessible_model_row_changed,
                         icon_view, NULL, 0);
  g_signal_connect_data (obj, "row-inserted",
                         (GCallback) exo_icon_view_accessible_model_row_inserted,
                         icon_view, NULL, G_CONNECT_AFTER);
  g_signal_connect_data (obj, "row-deleted",
                         (GCallback) exo_icon_view_accessible_model_row_deleted,
                         icon_view, NULL, G_CONNECT_AFTER);
  g_signal_connect_data (obj, "rows-reordered",
                         (GCallback) exo_icon_view_accessible_model_rows_reordered,
                         icon_view, NULL, G_CONNECT_AFTER);
}

static void
exo_icon_view_accessible_clear_cache (ExoIconViewAccessiblePrivate *priv)
{
  ExoIconViewItemAccessibleInfo *info;
  GList *items;

  items = priv->items;
  while (items)
    {
      info = (ExoIconViewItemAccessibleInfo *) items->data;
      g_object_unref (info->item);
      g_free (items->data);
      items = items->next;
    }
  g_list_free (priv->items);
  priv->items = NULL;
}

static void
exo_icon_view_accessible_notify_gtk (GObject *obj,
                                     GParamSpec *pspec)
{
  ExoIconView *icon_view;
  GtkWidget *widget;
  AtkObject *atk_obj;
  ExoIconViewAccessiblePrivate *priv;

  if (strcmp (pspec->name, "model") == 0)
    {
      widget = GTK_WIDGET (obj);
      atk_obj = gtk_widget_get_accessible (widget);
      priv = exo_icon_view_accessible_get_priv (atk_obj);
      if (priv->model)
        {
          g_object_remove_weak_pointer (G_OBJECT (priv->model),
                                        (gpointer *)&priv->model);
          exo_icon_view_accessible_disconnect_model_signals (priv->model, widget);
        }
      exo_icon_view_accessible_clear_cache (priv);

      icon_view = EXO_ICON_VIEW (obj);
      priv->model = icon_view->priv->model;
      /* If there is no model the ExoIconView is probably being destroyed */
      if (priv->model)
        {
          g_object_add_weak_pointer (G_OBJECT (priv->model), (gpointer *)&priv->model);
          exo_icon_view_accessible_connect_model_signals (icon_view);
        }
    }

  return;
}

static void
exo_icon_view_accessible_initialize (AtkObject *accessible,
                                     gpointer   data)
{
  ExoIconViewAccessiblePrivate *priv;
  ExoIconView *icon_view;

  if (ATK_OBJECT_CLASS (accessible_parent_class)->initialize)
    ATK_OBJECT_CLASS (accessible_parent_class)->initialize (accessible, data);

  priv = g_new0 (ExoIconViewAccessiblePrivate, 1);
  g_object_set_qdata (G_OBJECT (accessible),
                      accessible_private_data_quark,
                      priv);

  icon_view = EXO_ICON_VIEW (data);
  if (icon_view->priv->hadjustment)
    {
      priv->old_hadj = icon_view->priv->hadjustment;
      g_object_add_weak_pointer (G_OBJECT (priv->old_hadj), (gpointer *)&priv->old_hadj);
      g_signal_connect (icon_view->priv->hadjustment,
                        "value-changed",
                        G_CALLBACK (exo_icon_view_accessible_adjustment_changed),
                        icon_view);
    }
  if (icon_view->priv->vadjustment)
    {
      priv->old_vadj = icon_view->priv->vadjustment;
      g_object_add_weak_pointer (G_OBJECT (priv->old_vadj), (gpointer *)&priv->old_vadj);
      g_signal_connect (icon_view->priv->vadjustment,
                        "value-changed",
                        G_CALLBACK (exo_icon_view_accessible_adjustment_changed),
                        icon_view);
    }
  g_signal_connect_after (data,
                          "set-scroll-adjustments",
                          G_CALLBACK (exo_icon_view_accessible_set_scroll_adjustments),
                          NULL);
  g_signal_connect (data,
                    "notify",
                    G_CALLBACK (exo_icon_view_accessible_notify_gtk),
                    NULL);

  priv->model = icon_view->priv->model;
  if (priv->model)
    {
      g_object_add_weak_pointer (G_OBJECT (priv->model), (gpointer *)&priv->model);
      exo_icon_view_accessible_connect_model_signals (icon_view);
    }

  accessible->role = ATK_ROLE_LAYERED_PANE;
}

static void
exo_icon_view_accessible_finalize (GObject *object)
{
  ExoIconViewAccessiblePrivate *priv;

  priv = exo_icon_view_accessible_get_priv (ATK_OBJECT (object));
  exo_icon_view_accessible_clear_cache (priv);

  g_free (priv);

  G_OBJECT_CLASS (accessible_parent_class)->finalize (object);
}

#if !GTK_CHECK_VERSION(3, 0, 0)
static void
exo_icon_view_accessible_destroyed (GtkWidget *widget,
                                    GtkAccessible *accessible)
{
  AtkObject *atk_obj;
  ExoIconViewAccessiblePrivate *priv;

  atk_obj = ATK_OBJECT (accessible);
  priv = exo_icon_view_accessible_get_priv (atk_obj);
  if (priv->old_hadj)
    {
      g_object_remove_weak_pointer (G_OBJECT (priv->old_hadj),
                                    (gpointer *)&priv->old_hadj);

      g_signal_handlers_disconnect_by_func (priv->old_hadj,
                                            (gpointer) exo_icon_view_accessible_adjustment_changed,
                                            widget);
      priv->old_hadj = NULL;
    }
  if (priv->old_vadj)
    {
      g_object_remove_weak_pointer (G_OBJECT (priv->old_vadj),
                                    (gpointer *)&priv->old_vadj);

      g_signal_handlers_disconnect_by_func (priv->old_vadj,
                                            (gpointer) exo_icon_view_accessible_adjustment_changed,
                                            widget);
      priv->old_vadj = NULL;
    }
}

static void
exo_icon_view_accessible_connect_widget_destroyed (GtkAccessible *accessible)
{
  if (gtk_accessible_get_widget (accessible))
    {
      g_signal_connect_after (accessible->widget,
                              "destroy",
                              G_CALLBACK (exo_icon_view_accessible_destroyed),
                              accessible);
    }
  GTK_ACCESSIBLE_CLASS (accessible_parent_class)->connect_widget_destroyed (accessible);
}
#endif

static void
exo_icon_view_accessible_class_init (AtkObjectClass *klass)
{
  GObjectClass *gobject_class;
#if !GTK_CHECK_VERSION(3, 0, 0)
  GtkAccessibleClass *accessible_class;
#endif

  accessible_parent_class = g_type_class_peek_parent (klass);

  gobject_class = (GObjectClass *)klass;

  gobject_class->finalize = exo_icon_view_accessible_finalize;

  klass->get_n_children = exo_icon_view_accessible_get_n_children;
  klass->ref_child = exo_icon_view_accessible_ref_child;
  klass->initialize = exo_icon_view_accessible_initialize;

#if !GTK_CHECK_VERSION(3, 0, 0)
  accessible_class = (GtkAccessibleClass *)klass;
  accessible_class->connect_widget_destroyed = exo_icon_view_accessible_connect_widget_destroyed;
#endif

  accessible_private_data_quark = g_quark_from_static_string ("icon_view-accessible-private-data");
}

static AtkObject*
exo_icon_view_accessible_ref_accessible_at_point (AtkComponent *component,
                                                  gint          x,
                                                  gint          y,
                                                  AtkCoordType  coord_type)
{
  GtkWidget *widget;
  ExoIconView *icon_view;
  ExoIconViewItem *item;
  gint x_pos, y_pos;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (component));
  if (widget == NULL)
  /* State is defunct */
    return NULL;

  icon_view = EXO_ICON_VIEW (widget);
  atk_component_get_extents (component, &x_pos, &y_pos, NULL, NULL, coord_type);
  item = exo_icon_view_get_item_at_coords (icon_view, x - x_pos, y - y_pos, TRUE, NULL);
  if (item)
    return exo_icon_view_accessible_ref_child (ATK_OBJECT (component), item->index);

  return NULL;
}

static void
atk_component_interface_init (AtkComponentIface *iface)
{
  iface->ref_accessible_at_point = exo_icon_view_accessible_ref_accessible_at_point;
}

static gboolean
exo_icon_view_accessible_add_selection (AtkSelection *selection,
                                        gint i)
{
  GtkWidget *widget;
  ExoIconView *icon_view;
  ExoIconViewItem *item;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  icon_view = EXO_ICON_VIEW (widget);

  item = g_list_nth_data (icon_view->priv->items, i);

  if (!item)
    return FALSE;

  exo_icon_view_select_item (icon_view, item);

  return TRUE;
}

static gboolean
exo_icon_view_accessible_clear_selection (AtkSelection *selection)
{
  GtkWidget *widget;
  ExoIconView *icon_view;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  icon_view = EXO_ICON_VIEW (widget);
  exo_icon_view_unselect_all (icon_view);

  return TRUE;
}

static AtkObject*
exo_icon_view_accessible_ref_selection (AtkSelection *selection,
                                        gint          i)
{
  GList *l;
  GtkWidget *widget;
  ExoIconView *icon_view;
  ExoIconViewItem *item;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return NULL;

  icon_view = EXO_ICON_VIEW (widget);

  l = icon_view->priv->items;
  while (l)
    {
      item = l->data;
      if (item->selected)
        {
          if (i == 0)
	    return atk_object_ref_accessible_child (gtk_widget_get_accessible (widget), item->index);
          else
            i--;
        }
      l = l->next;
    }

  return NULL;
}

static gint
exo_icon_view_accessible_get_selection_count (AtkSelection *selection)
{
  GtkWidget *widget;
  ExoIconView *icon_view;
  ExoIconViewItem *item;
  GList *l;
  gint count;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return 0;

  icon_view = EXO_ICON_VIEW (widget);

  l = icon_view->priv->items;
  count = 0;
  while (l)
    {
      item = l->data;

      if (item->selected)
	count++;

      l = l->next;
    }

  return count;
}

static gboolean
exo_icon_view_accessible_is_child_selected (AtkSelection *selection,
                                            gint          i)
{
  GtkWidget *widget;
  ExoIconView *icon_view;
  ExoIconViewItem *item;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  icon_view = EXO_ICON_VIEW (widget);

  item = g_list_nth_data (icon_view->priv->items, i);
  if (!item)
    return FALSE;

  return item->selected;
}

static gboolean
exo_icon_view_accessible_remove_selection (AtkSelection *selection,
                                           gint          i)
{
  GtkWidget *widget;
  ExoIconView *icon_view;
  ExoIconViewItem *item;
  GList *l;
  gint count;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  icon_view = EXO_ICON_VIEW (widget);
  l = icon_view->priv->items;
  count = 0;
  while (l)
    {
      item = l->data;
      if (item->selected)
        {
          if (count == i)
            {
              exo_icon_view_unselect_item (icon_view, item);
              return TRUE;
            }
          count++;
        }
      l = l->next;
    }

  return FALSE;
}

static gboolean
exo_icon_view_accessible_select_all_selection (AtkSelection *selection)
{
  GtkWidget *widget;
  ExoIconView *icon_view;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  icon_view = EXO_ICON_VIEW (widget);
  exo_icon_view_select_all (icon_view);
  return TRUE;
}

static void
exo_icon_view_accessible_selection_interface_init (AtkSelectionIface *iface)
{
  iface->add_selection = exo_icon_view_accessible_add_selection;
  iface->clear_selection = exo_icon_view_accessible_clear_selection;
  iface->ref_selection = exo_icon_view_accessible_ref_selection;
  iface->get_selection_count = exo_icon_view_accessible_get_selection_count;
  iface->is_child_selected = exo_icon_view_accessible_is_child_selected;
  iface->remove_selection = exo_icon_view_accessible_remove_selection;
  iface->select_all_selection = exo_icon_view_accessible_select_all_selection;
}

static GType exo_icon_view_accessible_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      GTypeInfo tinfo =
      {
        0, /* class size */
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) exo_icon_view_accessible_class_init,
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        0, /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) NULL, /* instance init */
        NULL /* value table */
      };
      const GInterfaceInfo atk_component_info =
      {
        (GInterfaceInitFunc) atk_component_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };
      const GInterfaceInfo atk_selection_info =
      {
        (GInterfaceInitFunc) exo_icon_view_accessible_selection_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      /*
       * Figure out the size of the class and instance
       * we are deriving from
       */
      AtkObjectFactory *factory;
      GType derived_type;
      GTypeQuery query;
      GType derived_atk_type;

      derived_type = g_type_parent (EXO_TYPE_ICON_VIEW);
      factory = atk_registry_get_factory (atk_get_default_registry (),
                                          derived_type);
      derived_atk_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (derived_atk_type, &query);
      tinfo.class_size = query.class_size;
      tinfo.instance_size = query.instance_size;

      type = g_type_register_static (derived_atk_type,
                                     I_("ExoIconViewAccessible"),
                                     &tinfo, 0);
      g_type_add_interface_static (type, ATK_TYPE_COMPONENT,
                                   &atk_component_info);
      g_type_add_interface_static (type, ATK_TYPE_SELECTION,
                                   &atk_selection_info);
    }
  return type;
}

static AtkObject *exo_icon_view_accessible_new (GObject *obj)
{
  AtkObject *accessible;

  g_return_val_if_fail (GTK_IS_WIDGET (obj), NULL);

  accessible = g_object_new (exo_icon_view_accessible_get_type (), NULL);
  atk_object_initialize (accessible, obj);

  return accessible;
}

static GType exo_icon_view_accessible_factory_get_accessible_type (void)
{
  return exo_icon_view_accessible_get_type ();
}

static AtkObject* exo_icon_view_accessible_factory_create_accessible (GObject *obj)
{
  return exo_icon_view_accessible_new (obj);
}

static void exo_icon_view_accessible_factory_class_init (AtkObjectFactoryClass *klass)
{
  klass->create_accessible = exo_icon_view_accessible_factory_create_accessible;
  klass->get_accessible_type = exo_icon_view_accessible_factory_get_accessible_type;
}

static GType exo_icon_view_accessible_factory_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo tinfo =
      {
        sizeof (AtkObjectFactoryClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) exo_icon_view_accessible_factory_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (AtkObjectFactory),
        0,             /* n_preallocs */
        NULL, NULL
      };

      type = g_type_register_static (ATK_TYPE_OBJECT_FACTORY,
                                    I_("ExoIconViewAccessibleFactory"),
                                    &tinfo, 0);
    }
  return type;
}

static AtkObject *exo_icon_view_get_accessible (GtkWidget *widget)
{
  static gboolean first_time = TRUE;

  if (first_time)
    {
      AtkObjectFactory *factory;
      AtkRegistry *registry;
      GType derived_type;
      GType derived_atk_type;

      /*
       * Figure out whether accessibility is enabled by looking at the
       * type of the accessible object which would be created for
       * the parent type of ExoIconView.
       */
      derived_type = g_type_parent (EXO_TYPE_ICON_VIEW);

      registry = atk_get_default_registry ();
      factory = atk_registry_get_factory (registry,
                                          derived_type);
      derived_atk_type = atk_object_factory_get_accessible_type (factory);
      if (g_type_is_a (derived_atk_type, GTK_TYPE_ACCESSIBLE))
        atk_registry_set_factory_type (registry,
                                       EXO_TYPE_ICON_VIEW,
                                       exo_icon_view_accessible_factory_get_type ());
      first_time = FALSE;
    }
  return GTK_WIDGET_CLASS (exo_icon_view_parent_class)->get_accessible (widget);
}

/*
#define __EXO_ICON_VIEW_C__
#include <exo/exo-aliasdef.c>
*/
