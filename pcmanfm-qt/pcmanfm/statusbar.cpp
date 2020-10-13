/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "statusbar.h"
#include <QPainter>
#include <QStyleOption>

#define MESSAGE_DELAY 250

namespace PCManFM {

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
    if (txt != lastText_ || cr.width() != lastWidth_) {
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

StatusBar::StatusBar(QWidget *parent):
  QStatusBar(parent),
  lastTimeOut_(0) {
    statusLabel_ = new Label();
    statusLabel_->setFrameShape(QFrame::NoFrame);
    // 4px space on both sides (not to be mixed with the permanent widget)
    statusLabel_->setContentsMargins(4, 0, 4, 0);
    addWidget(statusLabel_);

    messageTimer_ = new QTimer (this);
    messageTimer_->setSingleShot(true);
    messageTimer_->setInterval(MESSAGE_DELAY);
    connect(messageTimer_, &QTimer::timeout, this, &StatusBar::reallyShowMessage);
}

StatusBar::~StatusBar() {
    if(messageTimer_) {
        messageTimer_->stop();
        delete messageTimer_;
    }
}

void StatusBar::showMessage(const QString &message, int timeout) {
    // don't show the message immediately
    lastMessage_ = message;
    lastTimeOut_ = timeout;
    if(!messageTimer_->isActive()) {
        messageTimer_->start();
    }
}

void StatusBar::reallyShowMessage() {
    if(lastTimeOut_ == 0) {
        // set the text on the label to prevent its disappearance on focusing menubar items
        // and also ensure that it contsains no newline (because file names may contain it)
        statusLabel_->setText(lastMessage_.replace(QLatin1Char('\n'), QLatin1Char(' ')));
    }
    else {
        QStatusBar::showMessage(lastMessage_, lastTimeOut_);
    }
}

}
