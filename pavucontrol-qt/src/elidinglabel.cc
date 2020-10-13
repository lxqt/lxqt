/***
  This file is part of pavucontrol-qt.

  pavucontrol-qt is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  pavucontrol-qt is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with pavucontrol-qt. If not, see <https://www.gnu.org/licenses/>.
***/

#include "elidinglabel.h"
#include <QPainter>
#include <QStyleOption>

ElidingLabel::ElidingLabel(QWidget *parent, Qt::WindowFlags f):
    QLabel(parent, f),
    lastWidth_(0) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    // set a min width to prevent the window from widening with long texts
    setMinimumWidth(fontMetrics().averageCharWidth() * 10);
}

// A simplified version of QLabel::paintEvent() without pixmap or shortcut but with eliding
void ElidingLabel::paintEvent(QPaintEvent */*event*/) {
    QRect cr = contentsRect().adjusted(margin(), margin(), -margin(), -margin());
    QString txt = text();
    // if the text is changed or its rect is resized (due to window resizing),
    // find whether it needs to be elided...
    if (txt != lastText_ || cr.width() != lastWidth_) {
        lastText_ = txt;
        lastWidth_ = cr.width();
        elidedText_ = fontMetrics().elidedText(txt, Qt::ElideMiddle, cr.width());
    }
    // ... then, draw the (elided) text */
    QPainter painter(this);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawItemText(&painter, cr, alignment(), opt.palette, isEnabled(), elidedText_, foregroundRole());
}
