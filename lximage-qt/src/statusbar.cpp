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

#include "statusbar.h"
#include <QPainter>
#include <QStyleOption>

#define MESSAGE_DELAY 250

namespace LxImage {

Label::Label(QWidget* parent, Qt::WindowFlags f):
  QLabel(parent, f),
  lastWidth_(0) {
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  // set a min width to prevent the window from widening with long texts
  setMinimumWidth(fontMetrics().averageCharWidth() * 10);
}

// A simplified version of QLabel::paintEvent()
// without pixmap or shortcut but with eliding.
void Label::paintEvent(QPaintEvent* /*event*/) {
  QRect cr = contentsRect().adjusted(margin(), margin(), -margin(), -margin());
  QString txt = text();
  // if the text is changed or its rect is resized (due to window resizing),
  // find whether it needs to be elided...
  if(txt != lastText_ || cr.width() != lastWidth_) {
    lastText_ = txt;
    lastWidth_ = cr.width();
    elidedText_ = fontMetrics().elidedText(txt, Qt::ElideMiddle, cr.width());
  }
  // ... then, draw the (elided) text
  if(!elidedText_.isEmpty()) {
    QPainter painter(this);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawItemText(&painter, cr, alignment(), opt.palette, isEnabled(), elidedText_, foregroundRole());
  }
}

StatusBar::StatusBar(QWidget* parent)
  : QStatusBar(parent) {
  // NOTE: The spacing is set to 6 in Qt → qstatusbar.cpp.
  sizeLabel0_ = new QLabel(QStringLiteral("<b>") + tr("Size:") + QStringLiteral("</b>"));
  sizeLabel0_->setContentsMargins(2, 0, 0, 0);
  sizeLabel0_->hide();
  addWidget(sizeLabel0_);

  sizeLabel_ = new QLabel();
  sizeLabel_->hide();
  sizeLabel_->setContentsMargins(0, 0, 2, 0);
  addWidget(sizeLabel_);

  pathLabel0_ = new QLabel(QStringLiteral("<b>") + tr("Path:") + QStringLiteral("</b>"));
  pathLabel0_->hide();
  addWidget(pathLabel0_);

  pathLabel_ = new Label();
  pathLabel_->hide();
  pathLabel_->setFrameShape(QFrame::NoFrame);
  addWidget(pathLabel_);
}

StatusBar::~StatusBar() {
}

void StatusBar::setText(const QString& sizeTxt, const QString& pathTxt) {
  sizeLabel_->setText(sizeTxt);
  sizeLabel0_->setVisible(!sizeTxt.isEmpty());
  sizeLabel_->setVisible(!sizeTxt.isEmpty());

  pathLabel_->setText(pathTxt);
  pathLabel0_->setVisible(!pathTxt.isEmpty());
  pathLabel_->setVisible(!pathTxt.isEmpty());
}

}
