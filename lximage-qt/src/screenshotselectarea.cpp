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

#include "screenshotselectarea.h"
#include <QMouseEvent>

using namespace LxImage;

ScreenshotSelectArea::ScreenshotSelectArea(const QImage & image, QWidget* parent) : QDialog(parent)
{
  scene_ = new QGraphicsScene(this);
  scene_->addPixmap(QPixmap::fromImage(image));
  
  view_ = new ScreenshotSelectAreaGraphicsView(scene_, this);
  view_->setRenderHints( QPainter::Antialiasing );
  view_->setHorizontalScrollBarPolicy ( Qt::ScrollBarAlwaysOff );
  view_->setVerticalScrollBarPolicy ( Qt::ScrollBarAlwaysOff );
  view_->show();
  view_->move(0,0);
  view_->resize(image.width(), image.height());
  setWindowState(windowState() | Qt::WindowFullScreen);
  connect(view_, &ScreenshotSelectAreaGraphicsView::selectedArea, this, &ScreenshotSelectArea::areaSelected);
}

QRect ScreenshotSelectArea::selectedArea()
{
  return selectedRect_;
}

void ScreenshotSelectArea::areaSelected(QRect rect)
{
  this->selectedRect_ = rect;
  accept();
}
