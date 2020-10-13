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

#ifndef LXIMAGE_IMAGEVIEW_H
#define LXIMAGE_IMAGEVIEW_H

#include "graphicsscene.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QImage>
#include <QPixmap>
#include <QMovie>
#include <QRect>

class QTimer;

namespace LxImage {

class ImageView : public QGraphicsView {
  Q_OBJECT

public:
  ImageView(QWidget* parent = nullptr);
  virtual ~ImageView();

  QGraphicsItem* imageGraphicsItem() const;
  QGraphicsItem* outlineGraphicsItem() const;

  void setImage(const QImage& image, bool show = true);
  void setGifAnimation(const QString& fileName);
  void setSVG(const QString& fileName);

  QImage image() const {
    return image_;
  }

  void setScaleFactor(double scale);

  double scaleFactor() const {
    return scaleFactor_;
  }

  void zoomIn();
  void zoomOut();
  void zoomFit();
  void zoomOriginal();

  bool autoZoomFit() const {
    return autoZoomFit_;
  }

  // if set to true, zoomFit() is done automatically when the size of the window is changed.
  void setAutoZoomFit(bool value) {
    autoZoomFit_ = value;
  }

  // if set to true, hides the cursor after 3s of inactivity
  void hideCursor(bool enable);

  // Annotation tools
  enum Tool {
    ToolNone,
    ToolArrow,
    ToolRectangle,
    ToolCircle,
    ToolNumber
  };
  void activateTool(Tool tool);
  void showOutline(bool show);
  void updateOutline();

Q_SIGNALS:
  void fileDropped(const QString file);
  void zooming();

protected:
  virtual void wheelEvent(QWheelEvent* event);
  virtual void mouseDoubleClickEvent(QMouseEvent* event);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseReleaseEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);
  virtual void focusInEvent(QFocusEvent* event);
  virtual void resizeEvent(QResizeEvent* event);
  virtual void paintEvent(QPaintEvent* event);

private:
  void queueGenerateCache();
  QRect viewportToScene(const QRect& rect);
  QRect sceneToViewport(const QRectF& rect);

  void drawOutline();

  void drawArrow(QPainter &painter,
                 const QPoint &start,
                 const QPoint &end,
                 qreal tipAngle,
                 int tipLen);

  void removeAnnotations();

private Q_SLOTS:
  void onFileDropped(const QString file);
  void generateCache();
  void blankCursor();

private:
  GraphicsScene* scene_; // the topmost container of all graphic items
  QGraphicsRectItem* imageItem_; // the rect item used to draw the image
  QGraphicsRectItem* outlineItem_; // the rect to draw the image outline
  QImage image_; // image to show
  QMovie *gifMovie_; // gif animation to show (should be deleted explicitly)
  QPixmap cachedPixmap_; // caching of current viewport content (high quality scaled image)
  QRect cachedRect_; // rectangle containing the cached region (in viewport coordinate)
  QRect cachedSceneRect_; // rectangle containing the cached region (in scene/original image coordinate)
  QTimer* cacheTimer_;
  QTimer *cursorTimer_; // for hiding cursor in fullscreen mode
  double scaleFactor_;
  bool autoZoomFit_;
  bool isSVG; // is the image an SVG file?
  Tool currentTool; // currently selected tool
  QPoint startPoint; // starting point for the tool
  QList<QGraphicsItem *> annotations;	//annotation items which have been drawn in the scene
  int nextNumber;
  bool showOutline_;
};

}

#endif // LXIMAGE_IMAGEVIEW_H
