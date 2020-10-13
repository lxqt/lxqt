/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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

#include "widgets.h"

#include <QMouseEvent>
#include <QDebug>


#include <QTextDocument>
#include <QPainter>
#include <QAbstractTextDocumentLayout>


/************************************************

 ************************************************/
HorizSizeGrip::HorizSizeGrip(QWidget *parent):
    QWidget(parent)
{
}


/************************************************

 ************************************************/
void HorizSizeGrip::mouseMoveEvent(QMouseEvent *event)
{
    if (!parentWidget())
        return;

    QWidget *parent = parentWidget();
    while (parent->parentWidget() && !parent->isWindow())
    {
        parent = parent->parentWidget();
    }

    QRect rect = parent->geometry();
    int delta = event->globalPos().x() - rect.center().x();

    bool isLeft = pos().x() < parent->size().width() - geometry().right();
    if (isLeft)
    {
        rect.setLeft(event->globalPos().x());
        rect.setRight(parent->geometry().center().x() - delta);
    }
    else
    {
        rect.setLeft(parent->geometry().center().x() - delta);
        rect.setRight(event->globalPos().x());
    }

    if (rect.width() < parent->minimumWidth() ||
        rect.width() > parent->maximumWidth())
        return;

    parent->setGeometry(rect);
}


/************************************************

 ************************************************/
CommandComboBox::CommandComboBox(QWidget *parent) :
    QComboBox(parent)
{

}


/************************************************

 ************************************************/
CommandListView::CommandListView(QWidget *parent) :
    QListView(parent)
{

}
