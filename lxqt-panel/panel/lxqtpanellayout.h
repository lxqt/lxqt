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


#ifndef LXQTPANELLAYOUT_H
#define LXQTPANELLAYOUT_H

#include <QLayout>
#include <QList>
#include <QWidget>
#include <QLayoutItem>
#include "ilxqtpanel.h"
#include "lxqtpanelglobals.h"

class MoveInfo;
class QMouseEvent;
class QEvent;

class Plugin;
class LayoutItemGrid;

class LXQT_PANEL_API LXQtPanelLayout : public QLayout
{
    Q_OBJECT
public:
    explicit LXQtPanelLayout(QWidget *parent);
    ~LXQtPanelLayout();

    void addItem(QLayoutItem *item);
    QLayoutItem *itemAt(int index) const;
    QLayoutItem *takeAt(int index);
    int count() const;
    void moveItem(int from, int to, bool withAnimation=false);

    QSize sizeHint() const;
    //QSize minimumSize() const;
    void setGeometry(const QRect &geometry);

    bool isHorizontal() const;

    void invalidate();

    int lineCount() const;
    void setLineCount(int value);

    int lineSize() const;
    void setLineSize(int value);

    ILXQtPanel::Position position() const { return mPosition; }
    void setPosition(ILXQtPanel::Position value);

    /*! \brief Force the layout to re-read items/plugins "static" configuration
     */
    void rebuild();

    static bool itemIsSeparate(QLayoutItem *item);
signals:
    void pluginMoved(Plugin * plugin);

public slots:
    void startMovePlugin();
    void finishMovePlugin();
    void moveUpPlugin(Plugin * plugin);
    void addPlugin(Plugin * plugin);

private:
    mutable QSize mMinPluginSize;
    LayoutItemGrid *mLeftGrid;
    LayoutItemGrid *mRightGrid;
    ILXQtPanel::Position mPosition;
    bool mAnimate;


    void setGeometryHoriz(const QRect &geometry);
    void setGeometryVert(const QRect &geometry);
    void globalIndexToLocal(int index, LayoutItemGrid **grid, int *gridIndex);
    void globalIndexToLocal(int index, LayoutItemGrid **grid, int *gridIndex) const;

    void setItemGeometry(QLayoutItem *item, const QRect &geometry, bool withAnimation);
};

#endif // LXQTPANELLAYOUT_H
