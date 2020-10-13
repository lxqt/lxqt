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

#include "graphicsscene.h"
#include <QMimeData>
#include <QUrl>

namespace LxImage {

GraphicsScene::GraphicsScene(QObject *parent):
  QGraphicsScene (parent) {
}

void GraphicsScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event) {
  if(event->mimeData()->hasUrls())
    event->acceptProposedAction();
}

void GraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event) {
  if(event->mimeData()->hasUrls())
    event->acceptProposedAction();
}

void GraphicsScene::dropEvent(QGraphicsSceneDragDropEvent* event) {
  QList<QUrl> urlList = event->mimeData()->urls();
  if(!urlList.isEmpty())
    Q_EMIT fileDropped(urlList.first().toLocalFile());
  event->acceptProposedAction();
}

}
