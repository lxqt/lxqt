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

#ifndef LXIMAGE_SCREENSHOTDIALOG_H
#define LXIMAGE_SCREENSHOTDIALOG_H

#include <QDialog>
#include "ui_screenshotdialog.h"

namespace LxImage {

class ScreenshotDialog : public QDialog {
  Q_OBJECT
 public:
  explicit ScreenshotDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  virtual ~ScreenshotDialog();

  virtual void done(int r);
  static void cmdTopShotToDir(QString path);
 private Q_SLOTS:
  void doScreenshot();

 private:
  Ui::ScreenshotDialog ui;
  const bool hasXfixes_;

  QImage static takeScreenshot(const WId &wid, const QRect &rect, bool takeCursor = false);
  static WId activeWindowId();
  static QRect windowFrame(WId wid);
};

}

#endif // LXIMAGE_SCREENSHOTDIALOG_H
