/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
 *            2014 LXQt team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *   Maciej Płaza <plaza.maciej@gmail.com>
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef LXQTTASKPOPUP_H
#define LXQTTASKPOPUP_H

#include <QHash>
#include <QFrame>
#include <QLayout>
#include <QTimer>
#include <QEvent>

#include "lxqttaskbutton.h"
#include "lxqttaskgroup.h"
#include "lxqttaskbar.h"

class LXQtGroupPopup: public QFrame
{
    Q_OBJECT

public:
    LXQtGroupPopup(LXQtTaskGroup *group);
    ~LXQtGroupPopup();

    void hide(bool fast = false);
    void show();

    // Layout
    int indexOf(LXQtTaskButton *button) { return layout()->indexOf(button); }
    int count() { return layout()->count(); }
    QLayoutItem * itemAt(int i) { return layout()->itemAt(i); }
    int spacing() { return layout()->spacing(); }
    void addButton(LXQtTaskButton* button) { layout()->addWidget(button); }
    void removeWidget(QWidget *button) { layout()->removeWidget(button); }

protected:
    void dragEnterEvent(QDragEnterEvent * event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent * event);
    void leaveEvent(QEvent * event);
    void enterEvent(QEvent * event);
    void paintEvent(QPaintEvent * event);

    void closeTimerSlot();

private:
    LXQtTaskGroup *mGroup;
    QTimer mCloseTimer;
};

#endif // LXQTTASKPOPUP_H
