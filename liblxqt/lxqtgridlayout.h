/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
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


#ifndef LXQTGRIDLAYOUT_H
#define LXQTGRIDLAYOUT_H

#include <QList>
#include "lxqtglobals.h"
#include <QLayout>


namespace LXQt
{

class GridLayoutPrivate;

/**
 The GridLayout class lays out widgets in a grid.
 **/
class LXQT_API GridLayout: public QLayout
{
    Q_OBJECT
public:
    /**
      This enum type is used to describe direction for this grid.
     **/
    enum Direction
    {
        LeftToRight,    ///< The items are first laid out horizontally and then vertically.
        TopToBottom     ///< The items are first laid out vertically and then horizontally.
    };

    /**
    This enum type is used to describe stretch. It contains one horizontal
    and one vertical flags that can be combined to produce the required effect.
    */
    enum StretchFlag
    {
        NoStretch         = 0,   ///< No justifies items
        StretchHorizontal = 1,   ///< Justifies items in the available horizontal space
        StretchVertical   = 2    ///< Justifies items in the available vertical space
    };
    Q_DECLARE_FLAGS(Stretch, StretchFlag)



    /**
    Constructs a new GridLayout with parent widget, parent.
    The layout has one row and zero column initially, and will
    expand to left when new items are inserted.
    **/
    explicit GridLayout(QWidget *parent = nullptr);

    /**
    Destroys the grid layout. The layout's widgets aren't destroyed.
     **/
    ~GridLayout() override;

    void addItem(QLayoutItem *item) override;
    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;
    int count() const override;
    void invalidate() override;

    QSize sizeHint() const override;
    void setGeometry(const QRect &geometry) override;
    QRect occupiedGeometry() const;


    /**
    Returns the number of rows in this grid.
    **/
    int rowCount() const;

    /**
     Sets the number of rows in this grid. If value is 0, then rows
     count will calculated automatically when new items are inserted.

     In the most cases you should to set fixed number for one thing,
     or for rows, or for columns.

     \sa GridLayout::setColumnCount
     **/
    void setRowCount(int value);


    /**
    Returns the number of columns in this grid.
    **/
    int columnCount() const;

    /**
     Sets the number of columns in this grid. If value is 0, then columns
     count will calculated automatically when new items are inserted.

     In the most cases you should to set fixed number for one thing,
     or for rows, or for columns.

     \sa GridLayout::setRowCount
     **/
    void setColumnCount(int value);


    /**
    Returns the alignment of this grid.

    \sa  GridLayout::Direction
    **/
    Direction direction() const;

    /**
     Sets the direction for this grid.

    \sa  GridLayout::Direction
    **/
    void setDirection(Direction value);

    /**
    Returns the stretch flags of this grid.

    \sa  GridLayout::StretchFlag
    **/
    Stretch stretch() const;

    /**
     Sets the stretch flags for this grid.

     \sa  GridLayout::StretchFlag
     **/
    void setStretch(Stretch value);

    /**
      Moves the item at index position \param from to index position \param to.
      If \param withAnimation set the reordering will be animated
     **/
    void moveItem(int from, int to, bool withAnimation = false);

    /**
      Checks if layout is currently animated after the \sa moveItem()
      invocation.
     **/
    bool animatedMoveInProgress() const;

    /**
     Returns the cells' minimum size.
     By default, this property contains a size with zero width and height.
     **/
    QSize cellMinimumSize() const;

    /**
      Sets the minimum size of all cells to minSize pixels.
     **/
    void setCellMinimumSize(QSize minSize);

    /**
     Sets the minimum height of the cells to value without
     changing the width. Provided for convenience.
     **/
    void setCellMinimumHeight(int value);

    /**
     Sets the minimum width of the cells to value without
     changing the heights. Provided for convenience.
     **/
    void setCellMinimumWidth(int value);



    /**
     Returns the cells' maximum size.
     By default, this property contains a size with zero width and height.
     **/
    QSize cellMaximumSize() const;

    /**
     Sets the maximum size of all cells to maxSize pixels.
     **/
    void setCellMaximumSize(QSize maxSize);

    /**
     Sets the maximum height of the cells to value without
     changing the width. Provided for convenience.
     **/
    void setCellMaximumHeight(int value);

    /**
     Sets the maximum width of the cells to value without
     changing the heights. Provided for convenience.
     **/
    void setCellMaximumWidth(int value);



    /**
     Sets both the minimum and maximum sizes of the cells to size,
     thereby preventing it from ever growing or shrinking.
     **/
    void setCellFixedSize(QSize size);

    /**
     Sets both the minimum and maximum height of the cells to value without
     changing the width. Provided for convenience.
     **/
    void setCellFixedHeight(int value);

    /**
     Sets both the minimum and maximum width of the cells to value without
     changing the heights. Provided for convenience.
     **/
    void setCellFixedWidth(int value);

private:
    GridLayoutPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(GridLayout)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(GridLayout::Stretch)

} // namespace LXQt
#endif // LXQTGRIDLAYOUT_H
