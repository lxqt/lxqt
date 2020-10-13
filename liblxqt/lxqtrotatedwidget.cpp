/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2013 Razor team
 * Authors:
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


#include "lxqtrotatedwidget.h"

#include <QPainter>
#include <QImage>

#include <QApplication>

#include <QMouseEvent>

#ifndef QT_NO_WHEELEVENT
#include <QWheelEvent>
#endif

#include <QResizeEvent>

using namespace LXQt;

RotatedWidget::RotatedWidget(QWidget &content, QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , mContent(&content)
    , mOrigin(Qt::TopLeftCorner)
    , mTransferMousePressEvent(false)
    , mTransferMouseReleaseEvent(false)
    , mTransferMouseDoubleClickEvent(false)
    , mTransferMouseMoveEvent(false)
#ifndef QT_NO_WHEELEVENT
    , mTransferWheelEvent(false)
#endif
    , mTransferEnterEvent(false)
    , mTransferLeaveEvent(false)
{
    mContent->setParent(this);
}

Qt::Corner RotatedWidget::origin() const
{
    return mOrigin;
}

void RotatedWidget::setOrigin(Qt::Corner newOrigin)
{
    if (mOrigin != newOrigin)
    {
        if (mOrigin == Qt::TopLeftCorner)
            mContent->hide();

        mOrigin = newOrigin;
        adjustContentSize();
        update();

        if (mOrigin == Qt::TopLeftCorner)
            mContent->show();
    }
}

QWidget * RotatedWidget::content() const
{
    return mContent;
}

void RotatedWidget::adjustContentSize()
{
    mContent->adjustSize();

    const QSize before = size();

    adjustSize();

    if (before != size())
        updateGeometry();
}

QSize RotatedWidget::adjustedSize(QSize size) const
{
    switch (mOrigin)
    {
    case Qt::TopLeftCorner:
    case Qt::BottomRightCorner:
        break;

    case Qt::TopRightCorner:
    case Qt::BottomLeftCorner:
        size.transpose();
        break;
    }

    return size;
}

QPoint RotatedWidget::adjustedPoint(QPoint point) const
{
    switch (mOrigin)
    {
    case Qt::TopLeftCorner:
    case Qt::BottomRightCorner:
        break;

    case Qt::TopRightCorner:
    case Qt::BottomLeftCorner:
        point = QPoint(point.y(), point.x());
        break;
    }

    return point;
}

QSize RotatedWidget::minimumSizeHint() const
{
    return adjustedSize(mContent->minimumSizeHint());
}

QSize RotatedWidget::sizeHint() const
{
    return adjustedSize(mContent->sizeHint());
}

void RotatedWidget::paintEvent(QPaintEvent */*event*/)
{
    if (mOrigin == Qt::TopLeftCorner)
        return;

    const QSize sz = mContent->size();

    QPainter painter(this);

    QTransform transform;
    QPoint originPoint;
    switch (mOrigin)
    {
    case Qt::TopLeftCorner:
//        transform.rotate(0.0);
//        originPoint = QPoint(0.0, 0.0);
        break;

    case Qt::TopRightCorner:
        transform.rotate(90.0);
        originPoint = QPoint(0.0, -sz.height());
        break;

    case Qt::BottomRightCorner:
        transform.rotate(180.0);
        originPoint = QPoint(-sz.width(), -sz.height());
        break;

    case Qt::BottomLeftCorner:
        transform.rotate(270.0);
        originPoint = QPoint(-sz.width(), 0.0);
        break;
    }
    painter.setTransform(transform);
    mContent->render(&painter, originPoint, QRegion(), RenderFlags(DrawChildren));
}

void RotatedWidget::mousePressEvent(QMouseEvent *event)
{
    if (!mTransferMousePressEvent)
    {
        event->ignore();
        return;
    }

    static bool cascadeCall = false;
    if (cascadeCall)
        return;
    cascadeCall = true;

    QMouseEvent contentEvent(event->type(), adjustedPoint(event->pos()), event->globalPos(), event->button(), event->buttons(), event->modifiers());
    QApplication::sendEvent(mContent, &contentEvent);

    cascadeCall = false;
}

void RotatedWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!mTransferMouseReleaseEvent)
    {
        event->ignore();
        return;
    }

    static bool cascadeCall = false;
    if (cascadeCall)
        return;
    cascadeCall = true;

    QMouseEvent contentEvent(event->type(), adjustedPoint(event->pos()), event->globalPos(), event->button(), event->buttons(), event->modifiers());
    QApplication::sendEvent(mContent, &contentEvent);

    cascadeCall = false;
}

void RotatedWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!mTransferMouseDoubleClickEvent)
    {
        event->ignore();
        return;
    }

    static bool cascadeCall = false;
    if (cascadeCall)
        return;
    cascadeCall = true;

    QMouseEvent contentEvent(event->type(), adjustedPoint(event->pos()), event->globalPos(), event->button(), event->buttons(), event->modifiers());
    QApplication::sendEvent(mContent, &contentEvent);

    cascadeCall = false;
}

void RotatedWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!mTransferMouseMoveEvent)
    {
        event->ignore();
        return;
    }

    static bool cascadeCall = false;
    if (cascadeCall)
        return;
    cascadeCall = true;

    QMouseEvent contentEvent(event->type(), adjustedPoint(event->pos()), event->globalPos(), event->button(), event->buttons(), event->modifiers());
    QApplication::sendEvent(mContent, &contentEvent);

    cascadeCall = false;
}

#ifndef QT_NO_WHEELEVENT
void RotatedWidget::wheelEvent(QWheelEvent *event)
{
    if (!mTransferWheelEvent)
    {
        event->ignore();
        return;
    }

    static bool cascadeCall = false;
    if (cascadeCall)
        return;
    cascadeCall = true;

    QWheelEvent contentEvent(adjustedPoint(event->pos()), event->globalPos(), event->delta(), event->buttons(), event->modifiers(), event->orientation());
    QApplication::sendEvent(mContent, &contentEvent);

    cascadeCall = false;
}
#endif

void RotatedWidget::enterEvent(QEvent *event)
{
    if (!mTransferEnterEvent)
    {
        event->ignore();
        return;
    }

    static bool cascadeCall = false;
    if (cascadeCall)
        return;
    cascadeCall = true;

    QApplication::sendEvent(mContent, event);

    cascadeCall = false;
}

void RotatedWidget::leaveEvent(QEvent *event)
{
    if (!mTransferLeaveEvent)
    {
        event->ignore();
        return;
    }

    static bool cascadeCall = false;
    if (cascadeCall)
        return;
    cascadeCall = true;

    QApplication::sendEvent(mContent, event);

    cascadeCall = false;
}

void RotatedWidget::resizeEvent(QResizeEvent *event)
{
    static bool cascadeCall = false;
    if (cascadeCall)
        return;
    cascadeCall = true;

    QResizeEvent contentEvent(adjustedSize(event->size()), adjustedSize(event->oldSize()));
    QApplication::sendEvent(mContent, &contentEvent);

    cascadeCall = false;
}
