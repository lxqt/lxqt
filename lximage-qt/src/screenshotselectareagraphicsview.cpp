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

#include "screenshotselectareagraphicsview.h"
#include <QMouseEvent>

using namespace LxImage;

ScreenshotSelectAreaGraphicsView::ScreenshotSelectAreaGraphicsView(QGraphicsScene* scene, QWidget* parent) : QGraphicsView(scene, parent)
{
  p0_ = QPointF(-1.0, -1.0);
  selectedAreaRect_ = nullptr;
  setCursor(Qt::CrossCursor);
}

void ScreenshotSelectAreaGraphicsView::mousePressEvent(QMouseEvent *event)
{
  if(p0_.x() < 0) {
    p0_ = QPointF(event->pos());
  } else {
    if(selectedAreaRect_ == nullptr) {
      QColor highlight = palette().color(QPalette::Active,QPalette::Highlight);
      QPen pen(highlight, 3, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
      QColor color(highlight);
      color.setAlpha(128);
      QBrush brush(color);
      selectedAreaRect_ = scene()->addRect(QRectF(), pen, brush);
    } 
    selectedAreaRect_->setRect(QRectF(p0_,QPointF(event->pos())).normalized());
  }
}

void ScreenshotSelectAreaGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
  mousePressEvent(event);
}

void ScreenshotSelectAreaGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
  Q_EMIT selectedArea(QRectF(p0_,QPointF(event->pos())).normalized().toRect());
}
