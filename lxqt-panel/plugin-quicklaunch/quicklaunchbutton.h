/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2012 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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


#ifndef LXQTQUICKLAUNCHBUTTON_H
#define LXQTQUICKLAUNCHBUTTON_H

#include "quicklaunchaction.h"
#include <QMimeData>
#include <QToolButton>

class ILXQtPanelPlugin;

class QuickLaunchButton : public QToolButton
{
    Q_OBJECT

public:
    QuickLaunchButton(QuickLaunchAction * act, ILXQtPanelPlugin * plugin, QWidget* parent = nullptr);
    ~QuickLaunchButton();

    QHash<QString,QString> settingsMap();

signals:
    void buttonDeleted();
    void switchButtons(QuickLaunchButton *from, QuickLaunchButton *to);
    void movedLeft();
    void movedRight();

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void dragEnterEvent(QDragEnterEvent *e);
    void dragMoveEvent(QDragMoveEvent * e);

private:
    QuickLaunchAction *mAct;
    ILXQtPanelPlugin * mPlugin;
    QAction *mDeleteAct;
    QAction *mMoveLeftAct;
    QAction *mMoveRightAct;
    QMenu *mMenu;
    QPoint mDragStart;

private slots:
    void this_customContextMenuRequested(const QPoint & pos);
    void selfRemove();
};


class ButtonMimeData: public QMimeData
{
    Q_OBJECT
public:
    ButtonMimeData():
        QMimeData(),
        mButton(0)
    {
    }

    QuickLaunchButton *button() const { return mButton; }
    void setButton(QuickLaunchButton *button) { mButton = button; }
private:
    QuickLaunchButton *mButton;
};

#endif
