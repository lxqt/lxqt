/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef SELECTKEYBOARDLAYOUTDIALOG_H
#define SELECTKEYBOARDLAYOUTDIALOG_H

#include <QDialog>
#include "ui_selectkeyboardlayoutdialog.h"
#include "keyboardlayoutinfo.h"

class SelectKeyboardLayoutDialog : public QDialog {
  Q_OBJECT
public:
  SelectKeyboardLayoutDialog(QMap< QString, KeyboardLayoutInfo >& knownLayouts, QWidget* parent = 0);
  virtual ~SelectKeyboardLayoutDialog();

  QString selectedLayout();
  QString selectedVariant();

private Q_SLOTS:
  void onLayoutChanged();

private:
  Ui::SelectKeyboardLayoutDialog ui;
  QMap<QString, KeyboardLayoutInfo>& knownLayouts_;
};

#endif // SELECTKEYBOARDLAYOUTDIALOG_H
