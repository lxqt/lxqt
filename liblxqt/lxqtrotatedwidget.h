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

#ifndef LXQTROTATED_WIDGET_H
#define LXQTROTATED_WIDGET_H

#include <QWidget>
#include "lxqtglobals.h"

namespace LXQt
{

class LXQT_API RotatedWidget: public QWidget
{
    Q_OBJECT

    Q_PROPERTY(Qt::Corner origin READ origin WRITE setOrigin)

    Q_PROPERTY(bool transferMousePressEvent READ transferMousePressEvent WRITE setTransferMousePressEvent)
    Q_PROPERTY(bool transferMouseReleaseEvent READ transferMouseReleaseEvent WRITE setTransferMouseReleaseEvent)
    Q_PROPERTY(bool transferMouseDoubleClickEvent READ transferMouseDoubleClickEvent WRITE setTransferMouseDoubleClickEvent)
    Q_PROPERTY(bool transferMouseMoveEvent READ transferMouseMoveEvent WRITE setTransferMouseMoveEvent)
#ifndef QT_NO_WHEELEVENT
    Q_PROPERTY(bool transferWheelEvent READ transferWheelEvent WRITE setTransferWheelEvent)
#endif

    Q_PROPERTY(bool transferEnterEvent READ transferEnterEvent WRITE setTransferEnterEvent)
    Q_PROPERTY(bool transferLeaveEvent READ transferLeaveEvent WRITE setTransferLeaveEvent)

public:
    explicit RotatedWidget(QWidget &content, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    Qt::Corner origin() const;
    void setOrigin(Qt::Corner);

    QWidget * content() const;

    void adjustContentSize();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    QSize adjustedSize(QSize) const;
    QPoint adjustedPoint(QPoint) const;


    bool transferMousePressEvent() const { return mTransferMousePressEvent; }
    void setTransferMousePressEvent(bool value) { mTransferMousePressEvent = value; }

    bool transferMouseReleaseEvent() const { return mTransferMouseReleaseEvent; }
    void setTransferMouseReleaseEvent(bool value) { mTransferMouseReleaseEvent = value; }

    bool transferMouseDoubleClickEvent() const { return mTransferMouseDoubleClickEvent; }
    void setTransferMouseDoubleClickEvent(bool value) { mTransferMouseDoubleClickEvent = value; }

    bool transferMouseMoveEvent() const { return mTransferMouseMoveEvent; }
    void setTransferMouseMoveEvent(bool value) { mTransferMouseMoveEvent = value; }

#ifndef QT_NO_WHEELEVENT
    bool transferWheelEvent() const { return mTransferWheelEvent; }
    void setTransferWheelEvent(bool value) { mTransferWheelEvent = value; }
#endif

    bool transferEnterEvent() const { return mTransferEnterEvent; }
    void setTransferEnterEvent(bool value) { mTransferEnterEvent = value; }

    bool transferLeaveEvent() const { return mTransferLeaveEvent; }
    void setTransferLeaveEvent(bool value) { mTransferLeaveEvent = value; }

protected:
    void paintEvent(QPaintEvent *) override;

    // Transition event handlers
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *) override;
#endif
    void enterEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;

    void resizeEvent(QResizeEvent *) override;

private:
    QWidget *mContent;
    Qt::Corner mOrigin;

    bool mTransferMousePressEvent;
    bool mTransferMouseReleaseEvent;
    bool mTransferMouseDoubleClickEvent;
    bool mTransferMouseMoveEvent;
#ifndef QT_NO_WHEELEVENT
    bool mTransferWheelEvent;
#endif
    bool mTransferEnterEvent;
    bool mTransferLeaveEvent;
};

} // namespace LXQt
#endif // LXQTROTATEDWIDGET_H
