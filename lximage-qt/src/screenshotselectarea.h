/*
 * LXImage-Qt - a simple and fast image viewer
 * Copyright (C) 2013  PCMan <pcman.tw@gmail.com>
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

#ifndef LXIMAGE_SCREENSHOTDIALOG_SELECT_AREA_H
#define LXIMAGE_SCREENSHOTDIALOG_SELECT_AREA_H

#include "screenshotselectareagraphicsview.h"
#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>


namespace LxImage {

class ScreenshotSelectArea : public QDialog {
  Q_OBJECT
public:
  ScreenshotSelectArea(const QImage & image, QWidget* parent = nullptr);
  QRect selectedArea();

private Q_SLOTS:
  void areaSelected(QRect rect);
  
private:
  QGraphicsScene *scene_;
  ScreenshotSelectAreaGraphicsView *view_;
  QRect selectedRect_;
};

}

#endif // LXIMAGE_SCREENSHOTDIALOG_SELECT_AREA_H
