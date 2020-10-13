/*

    Copyright (C) 2014  Kuzma Shapran <kuzma.shapran@gmail.com>

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


#ifndef FM_TABBAR_H
#define FM_TABBAR_H

#include <QTabBar>

class QMouseEvent;

namespace PCManFM {

class TabBar : public QTabBar {
Q_OBJECT

public:
    explicit TabBar(QWidget *parent = 0);
    void finishMouseMoveEvent();
    void releaseMouse();

    void setDetachable(bool detachable) {
        detachable_ = detachable;
    }

Q_SIGNALS:
    void tabDetached();

protected:
    void mouseReleaseEvent(QMouseEvent *event);
    // from qtabbar.cpp
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *event);

private:
    QPoint dragStartPosition_;
    bool dragStarted_;
    bool detachable_;
};

}

#endif // FM_TABBAR_H
