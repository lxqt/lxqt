/*
 * LXImage-Qt - a simple and fast image viewer
 * Copyright (C) 2013  PCMan <pcman.tw@gmail.com>
 *
 * StatusBar by tsujan <tsujan2000@gmail.com>
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

#ifndef LXIMAGE_STATUSBAR_H
#define LXIMAGE_STATUSBAR_H

#include <QStatusBar>
#include <QLabel>

namespace LxImage {

class Label : public QLabel {
  Q_OBJECT

public:
  explicit Label(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  QString elidedText_;
  QString lastText_;
  int lastWidth_;
};

class StatusBar : public QStatusBar {
Q_OBJECT

public:
  explicit StatusBar(QWidget* parent = nullptr);
  ~StatusBar();

  void setText(const QString& sizeTxt = QString(), const QString& pathTxt = QString());

private:
  QLabel *sizeLabel0_, *sizeLabel_, *pathLabel0_;
  Label *pathLabel_; // an elided path
};

}

#endif // LXIMAGE_STATUSBAR_H
