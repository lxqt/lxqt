/*
 * LXImage-Qt - a simple and fast image viewer
 * Copyright (C) 2013  PCMan <pcman.tw@gmail.com>
 *
 * Resize feature inspired by Gwenview's one
 * Copyright 2010 Aurélien Gâteau <agateau@kde.org>
 * adjam refactored
 * Copyright 2020 Andrea Diamantini <adjam@protonmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef LXIMAGE_RESIZEIMAGEDIALOG_H
#define LXIMAGE_RESIZEIMAGEDIALOG_H

#include <QDialog>
#include "ui_resizeimagedialog.h"

namespace LxImage {


class ResizeImageDialog : public QDialog {
  Q_OBJECT
public:
  explicit ResizeImageDialog(QWidget* parent = nullptr);
  virtual ~ResizeImageDialog();

  void setOriginalSize(const QSize&);
  QSize scaledSize() const;

private Q_SLOTS:
  void onWidthChanged(int);
  void onHeightChanged(int);
  void onWidthPercentChanged(double);
  void onHeightPercentChanged(double);
  void onKeepAspectChanged(bool);

private:
  Ui::ResizeImageDialog ui;
  bool updateFromRatio_;
  bool updateFromSizeOrPercentage_;
  QSize originalSize_;
};

}

#endif // LXIMAGE_RESIZEIMAGEDIALOG_H
