/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  <copyright holder> <email>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef OBCONF_MAINDIALOG_H
#define OBCONF_MAINDIALOG_H

#include <QDialog>
#include <glib.h>

// We include a fixed version rather than the originally generated one.
#include "ui_obconf_fixed.h"

class QStandardItemModel;
class QItemSelection;

namespace Obconf {

class MainDialog : public QDialog {
  Q_OBJECT

public:
  MainDialog();
  virtual ~MainDialog();

  virtual void accept();
  virtual void reject();

  void theme_install(const char* path);
  void theme_load_all();

private:
  void theme_setup_tab();
  void appearance_setup_tab();
  void windows_setup_tab();
  void mouse_setup_tab();
  void moveresize_setup_tab();
  void margins_setup_tab();
  void desktops_setup_tab();
  void dock_setup_tab();

  // theme
  void add_theme_dir(const char* dirname);

  // windows
  void windows_enable_stuff();
  
  // move & resize
  void moveresize_enable_stuff();
  void write_fixed_position(const char* coord);
  
  // mouse
  void mouse_enable_stuff();
  int read_doubleclick_action();
  void write_doubleclick_action(int a);

  // desktops
  void desktops_read_names();
  void desktops_write_names();
  void desktops_write_number();

private Q_SLOTS:

  void on_about_clicked();

  // theme
  void onThemeNamesSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
  void on_install_theme_clicked();
  void on_theme_archive_clicked();

  //apearance
  void on_window_border_toggled(bool checked);
  void on_animate_iconify_toggled(bool checked);
  void on_title_layout_textChanged(const QString& text);

  // font
  void on_font_active_changed();
  void on_font_inactive_changed();
  void on_font_menu_header_changed();
  void on_font_menu_item_changed();
  void on_font_active_display_changed();
  void on_font_inactive_display_changed();

  // windows
  void on_fixed_monitor_valueChanged(int newValue);
  void on_focus_new_toggled(bool checked);
  void on_place_mouse_toggled(bool checked);

  void on_place_active_popup_currentIndexChanged(int index);
  void on_primary_monitor_popup_currentIndexChanged(int index);

  // move & resize
  void on_resist_window_valueChanged(int newValue);
  void on_resist_edge_valueChanged(int newValue);
  void on_resize_contents_toggled(bool checked);
  
  void on_resize_popup_currentIndexChanged(int index);
  void on_resize_position_currentIndexChanged(int index);
  void on_fixed_x_popup_currentIndexChanged(int index);
  void on_fixed_y_popup_currentIndexChanged(int index);

  void on_drag_threshold_valueChanged(int newValue);

  void on_fixed_x_pos_valueChanged(int newValue);
  void on_fixed_y_pos_valueChanged(int newValue);
  void on_warp_edge_toggled(bool checked);
  void on_warp_edge_time_valueChanged(int newValue);

  // margins
  void on_margins_left_valueChanged(int newValue);
  void on_margins_right_valueChanged(int newValue);
  void on_margins_top_valueChanged(int newValue);
  void on_margins_bottom_valueChanged(int newValue);

  // mouse
  void on_focus_mouse_toggled(bool checked);
  void on_focus_delay_valueChanged(int newValue);
  void on_focus_raise_toggled(bool checked);
  void on_focus_notlast_toggled(bool checked);
  void on_focus_under_mouse_toggled(bool checked);
  void on_doubleclick_time_valueChanged(int newValue);
  void on_titlebar_doubleclick_currentIndexChanged(int index);

  // desktop
  void on_desktop_num_valueChanged(int newValue);
  void on_desktop_popup_toggled(bool checked);
  void on_desktop_popup_time_valueChanged(int newValue);
  void on_desktop_names_itemChanged(QListWidgetItem * item);

  // docks
  void on_dock_float_x_valueChanged(int newValue);
  void on_dock_float_y_valueChanged(int newValue);
  void on_dock_stack_top_toggled(bool checked);
  void on_dock_stack_normal_toggled(bool checked);
  void on_dock_stack_bottom_toggled(bool checked);
  void on_dock_nostrut_toggled(bool checked);
  void on_dock_hide_toggled(bool checked);
  void on_dock_hide_delay_valueChanged(int newValue);
  void on_dock_show_delay_valueChanged(int newValue);
  void on_dock_position_currentIndexChanged(int index);
  void on_dock_direction_currentIndexChanged(int index);

private:
  Ui::MainDialog ui;
  GList* themes;
  QStandardItemModel* themes_model;
};

}

#endif // OBCONF_MAINDIALOG_H
