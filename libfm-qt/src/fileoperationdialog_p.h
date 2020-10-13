/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#ifndef FM_FILEOPERATIONDIALOG_P_H
#define FM_FILEOPERATIONDIALOG_P_H

#include <QPainter>
#include <QStyleOption>
#include <QLabel>

namespace Fm {

class ElidedLabel : public QLabel {
Q_OBJECT

public:
    explicit ElidedLabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags()):
        QLabel(parent, f),
        lastWidth_(0) {
            setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            // set a min width to prevent the window from widening with long texts
            setMinimumWidth(fontMetrics().averageCharWidth() * 10);
        }

protected:
    // A simplified version of QLabel::paintEvent() without pixmap or shortcut but with eliding.
    void paintEvent(QPaintEvent* /*event*/) override {
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

private:
    QString elidedText_;
    QString lastText_;
    int lastWidth_;
};

}

#endif // FM_FILEOPERATIONDIALOG_P_H
