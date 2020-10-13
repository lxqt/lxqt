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


#include "lxqtgridlayout.h"
#include <QDebug>
#include <math.h>
#include <QWidget>
#include <QVariantAnimation>

using namespace LXQt;

class LXQt::GridLayoutPrivate
{
public:
    GridLayoutPrivate();
    ~GridLayoutPrivate();

    QList<QLayoutItem*> mItems;
    int mRowCount;
    int mColumnCount;
    GridLayout::Direction mDirection;

    bool mIsValid;
    QSize mCellSizeHint;
    QSize mCellMaxSize;
    int mVisibleCount;
    GridLayout::Stretch mStretch;
    bool mAnimate;
    int mAnimatedItems; //!< counter of currently animated items


    void updateCache();
    int rows() const;
    int cols() const;
    void setItemGeometry(QLayoutItem * item, QRect const & geometry);
    QSize mPrefCellMinSize;
    QSize mPrefCellMaxSize;
    QRect mOccupiedGeometry;
};

namespace
{
    class ItemMoveAnimation : public QVariantAnimation
    {
    public:
        static void animate(QLayoutItem * item, QRect const & geometry, LXQt::GridLayoutPrivate * layout)
        {
            ItemMoveAnimation* animation = new ItemMoveAnimation(item);
            animation->setStartValue(item->geometry());
            animation->setEndValue(geometry);
            ++layout->mAnimatedItems;
            connect(animation, &QAbstractAnimation::finished, [layout] { --layout->mAnimatedItems; Q_ASSERT(0 <= layout->mAnimatedItems); });
            animation->start(DeleteWhenStopped);
        }

        ItemMoveAnimation(QLayoutItem *item)
            : mItem(item)
        {
            setDuration(150);
        }

        void updateCurrentValue(const QVariant &current) override
        {
            mItem->setGeometry(current.toRect());
        }

    private:
        QLayoutItem* mItem;

    };
}


/************************************************

 ************************************************/
GridLayoutPrivate::GridLayoutPrivate()
{
    mColumnCount = 0;
    mRowCount = 0;
    mDirection = GridLayout::LeftToRight;
    mIsValid = false;
    mVisibleCount = 0;
    mStretch = GridLayout::StretchHorizontal | GridLayout::StretchVertical;
    mAnimate = false;
    mAnimatedItems = 0;
    mPrefCellMinSize = QSize(0,0);
    mPrefCellMaxSize = QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

/************************************************

 ************************************************/
GridLayoutPrivate::~GridLayoutPrivate()
{
    qDeleteAll(mItems);
}


/************************************************

 ************************************************/
void GridLayoutPrivate::updateCache()
{
    mCellSizeHint = QSize(0, 0);
    mCellMaxSize = QSize(0, 0);
    mVisibleCount = 0;

    const int N = mItems.count();
    for (int i=0; i < N; ++i)
    {
        QLayoutItem *item = mItems.at(i);
        if (!item->widget() || item->widget()->isHidden())
            continue;

        int h = qBound(item->minimumSize().height(),
                       item->sizeHint().height(),
                       item->maximumSize().height());

        int w = qBound(item->minimumSize().width(),
                       item->sizeHint().width(),
                       item->maximumSize().width());

        mCellSizeHint.rheight() = qMax(mCellSizeHint.height(), h);
        mCellSizeHint.rwidth()  = qMax(mCellSizeHint.width(), w);

        mCellMaxSize.rheight() = qMax(mCellMaxSize.height(), item->maximumSize().height());
        mCellMaxSize.rwidth()  = qMax(mCellMaxSize.width(), item->maximumSize().width());
        mVisibleCount++;

#if 0
        qDebug() << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-";
        qDebug() << "item.min" << item->minimumSize().width();
        qDebug() << "item.sz " << item->sizeHint().width();
        qDebug() << "item.max" << item->maximumSize().width();
        qDebug() << "w h" << w << h;
        qDebug() << "wid.sizeHint" << item->widget()->sizeHint();
        qDebug() << "mCellSizeHint:" << mCellSizeHint;
        qDebug() << "mCellMaxSize: " << mCellMaxSize;
        qDebug() << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-";
#endif

    }
    mCellSizeHint.rwidth() = qBound(mPrefCellMinSize.width(),  mCellSizeHint.width(),  mPrefCellMaxSize.width());
    mCellSizeHint.rheight()= qBound(mPrefCellMinSize.height(), mCellSizeHint.height(), mPrefCellMaxSize.height());
    mIsValid = !mCellSizeHint.isEmpty();
}


/************************************************

 ************************************************/
int GridLayoutPrivate::rows() const
{
    if (mRowCount)
        return mRowCount;

    if (!mColumnCount)
        return 1;

    return ceil(mVisibleCount * 1.0 / mColumnCount);
}


/************************************************

 ************************************************/
int GridLayoutPrivate::cols() const
{
    if (mColumnCount)
        return mColumnCount;

    int rows = mRowCount;
    if (!rows)
        rows = 1;

    return ceil(mVisibleCount * 1.0 / rows);
}

void GridLayoutPrivate::setItemGeometry(QLayoutItem * item, QRect const & geometry)
{
    mOccupiedGeometry |= geometry;
    if (mAnimate)
    {
        ItemMoveAnimation::animate(item, geometry, this);
    } else
    {
        item->setGeometry(geometry);
    }
}


/************************************************

 ************************************************/
GridLayout::GridLayout(QWidget *parent):
    QLayout(parent),
    d_ptr(new GridLayoutPrivate())
{
    // no space between items by default
    setSpacing(0);
}


/************************************************

 ************************************************/
GridLayout::~GridLayout()
{
    delete d_ptr;
}


/************************************************

 ************************************************/
void GridLayout::addItem(QLayoutItem *item)
{
    d_ptr->mItems.append(item);
}


/************************************************

 ************************************************/
QLayoutItem *GridLayout::itemAt(int index) const
{
    Q_D(const GridLayout);
    if (index < 0 || index >= d->mItems.count())
        return nullptr;

    return d->mItems.at(index);
}


/************************************************

 ************************************************/
QLayoutItem *GridLayout::takeAt(int index)
{
    Q_D(GridLayout);
    if (index < 0 || index >= d->mItems.count())
        return nullptr;

    QLayoutItem *item = d->mItems.takeAt(index);
    return item;
}


/************************************************

 ************************************************/
int GridLayout::count() const
{
    Q_D(const GridLayout);
    return d->mItems.count();
}


/************************************************

 ************************************************/
void GridLayout::invalidate()
{
    Q_D(GridLayout);
    d->mIsValid = false;
    QLayout::invalidate();
}


/************************************************

 ************************************************/
int GridLayout::rowCount() const
{
    Q_D(const GridLayout);
    return d->mRowCount;
}


/************************************************

 ************************************************/
void GridLayout::setRowCount(int value)
{
    Q_D(GridLayout);
    if (d->mRowCount != value)
    {
        d->mRowCount = value;
        invalidate();
    }
}


/************************************************

 ************************************************/
int GridLayout::columnCount() const
{
    Q_D(const GridLayout);
    return d->mColumnCount;
}


/************************************************

 ************************************************/
void GridLayout::setColumnCount(int value)
{
    Q_D(GridLayout);
    if (d->mColumnCount != value)
    {
        d->mColumnCount = value;
        invalidate();
    }
}


/************************************************

 ************************************************/
GridLayout::Direction GridLayout::direction() const
{
    Q_D(const GridLayout);
    return d->mDirection;
}


/************************************************

 ************************************************/
void GridLayout::setDirection(GridLayout::Direction value)
{
    Q_D(GridLayout);
    if (d->mDirection != value)
    {
        d->mDirection = value;
        invalidate();
    }
}

/************************************************

 ************************************************/
GridLayout::Stretch GridLayout::stretch() const
{
    Q_D(const GridLayout);
    return d->mStretch;
}

/************************************************

 ************************************************/
void GridLayout::setStretch(Stretch value)
{
    Q_D(GridLayout);
    if (d->mStretch != value)
    {
        d->mStretch = value;
        invalidate();
    }
}


/************************************************

 ************************************************/
void GridLayout::moveItem(int from, int to, bool withAnimation /*= false*/)
{
    Q_D(GridLayout);
    d->mAnimate = withAnimation;
    d->mItems.move(from, to);
    invalidate();
}


/************************************************

 ************************************************/
bool GridLayout::animatedMoveInProgress() const
{
    Q_D(const GridLayout);
    return 0 < d->mAnimatedItems;
}


/************************************************

 ************************************************/
QSize GridLayout::cellMinimumSize() const
{
    Q_D(const GridLayout);
    return d->mPrefCellMinSize;
}


/************************************************

 ************************************************/
void GridLayout::setCellMinimumSize(QSize minSize)
{
    Q_D(GridLayout);
    if (d->mPrefCellMinSize != minSize)
    {
        d->mPrefCellMinSize = minSize;
        invalidate();
    }
}


/************************************************

 ************************************************/
void GridLayout::setCellMinimumHeight(int value)
{
    Q_D(GridLayout);
    if (d->mPrefCellMinSize.height() != value)
    {
        d->mPrefCellMinSize.setHeight(value);
        invalidate();
    }
}


/************************************************

 ************************************************/
void GridLayout::setCellMinimumWidth(int value)
{
    Q_D(GridLayout);
    if (d->mPrefCellMinSize.width() != value)
    {
        d->mPrefCellMinSize.setWidth(value);
        invalidate();
    }
}


/************************************************

 ************************************************/
QSize GridLayout::cellMaximumSize() const
{
    Q_D(const GridLayout);
    return d->mPrefCellMaxSize;
}


/************************************************

 ************************************************/
void GridLayout::setCellMaximumSize(QSize maxSize)
{
    Q_D(GridLayout);
    if (d->mPrefCellMaxSize != maxSize)
    {
        d->mPrefCellMaxSize = maxSize;
        invalidate();
    }
}


/************************************************

 ************************************************/
void GridLayout::setCellMaximumHeight(int value)
{
    Q_D(GridLayout);
    if (d->mPrefCellMaxSize.height() != value)
    {
        d->mPrefCellMaxSize.setHeight(value);
        invalidate();
    }
}


/************************************************

 ************************************************/
void GridLayout::setCellMaximumWidth(int value)
{
    Q_D(GridLayout);
    if (d->mPrefCellMaxSize.width() != value)
    {
        d->mPrefCellMaxSize.setWidth(value);
        invalidate();
    }
}


/************************************************

 ************************************************/
void GridLayout::setCellFixedSize(QSize size)
{
    Q_D(GridLayout);
    if (d->mPrefCellMinSize != size ||
        d->mPrefCellMaxSize != size)
    {
        d->mPrefCellMinSize = size;
        d->mPrefCellMaxSize = size;
        invalidate();
    }
}


/************************************************

 ************************************************/
void GridLayout::setCellFixedHeight(int value)
{
    Q_D(GridLayout);
    if (d->mPrefCellMinSize.height() != value ||
        d->mPrefCellMaxSize.height() != value)
    {
        d->mPrefCellMinSize.setHeight(value);
        d->mPrefCellMaxSize.setHeight(value);
        invalidate();
    }
}


/************************************************

 ************************************************/
void GridLayout::setCellFixedWidth(int value)
{
    Q_D(GridLayout);
    if (d->mPrefCellMinSize.width() != value ||
        d->mPrefCellMaxSize.width() != value)
    {
        d->mPrefCellMinSize.setWidth(value);
        d->mPrefCellMaxSize.setWidth(value);
        invalidate();
    }
}


/************************************************

 ************************************************/
QSize GridLayout::sizeHint() const
{
    Q_D(const GridLayout);

    if (!d->mIsValid)
        const_cast<GridLayoutPrivate*>(d)->updateCache();

    if (d->mVisibleCount == 0)
        return {0, 0};

    const int sp = spacing();
    return {d->cols() * (d->mCellSizeHint.width() + sp) - sp,
            d->rows() * (d->mCellSizeHint.height() + sp) - sp};
}


/************************************************

 ************************************************/
void GridLayout::setGeometry(const QRect &geometry)
{
    Q_D(GridLayout);

    const bool visual_h_reversed = parentWidget() && parentWidget()->isRightToLeft();

    QLayout::setGeometry(geometry);
    const QPoint occupied_start = visual_h_reversed ? geometry.topRight() : geometry.topLeft();
    d->mOccupiedGeometry.setTopLeft(occupied_start);
    d->mOccupiedGeometry.setBottomRight(occupied_start);

    if (!d->mIsValid)
        d->updateCache();

    int y = geometry.top();
    int x = geometry.left();

    // For historical reasons QRect::right returns left() + width() - 1
    // and QRect::bottom() returns top() + height() - 1;
    // So we use left() + height() and top() + height()
    //
    // http://qt-project.org/doc/qt-4.8/qrect.html

    const int maxX = geometry.left() + geometry.width();
    const int maxY = geometry.top() + geometry.height();

    const bool stretch_h = d->mStretch.testFlag(StretchHorizontal);
    const bool stretch_v = d->mStretch.testFlag(StretchVertical);
    const int sp = spacing();

    const int cols = d->cols();
    int itemWidth = 0;
    int widthRemain = 0;
    if (stretch_h && 0 < cols)
    {
        itemWidth = qMin((geometry.width() + sp) / cols - sp, d->mCellMaxSize.width());
        widthRemain = (geometry.width() + sp) % cols;
    }
    else
    {
        itemWidth = d->mCellSizeHint.width();
    }
    itemWidth = qBound(qMin(d->mPrefCellMinSize.width(), maxX), itemWidth, d->mPrefCellMaxSize.width());

    const int rows = d->rows();
    int itemHeight = 0;
    int heightRemain = 0;
    if (stretch_v && 0 < rows)
    {
        itemHeight = qMin((geometry.height() + sp) / rows - sp, d->mCellMaxSize.height());
        heightRemain = (geometry.height() + sp) % rows;
    }
    else
    {
        itemHeight = d->mCellSizeHint.height();
    }
    itemHeight = qBound(qMin(d->mPrefCellMinSize.height(), maxY), itemHeight, d->mPrefCellMaxSize.height());

#if 0
    qDebug() << "** GridLayout::setGeometry *******************************";
    qDebug() << "Geometry:" << geometry;
    qDebug() << "CellSize:" << d->mCellSizeHint;
    qDebug() << "Constraints:" << "min" << d->mPrefCellMinSize << "max" << d->mPrefCellMaxSize;
    qDebug() << "Count" << count();
    qDebug() << "Cols:" << d->cols() << "(" << d->mColumnCount << ")";
    qDebug() << "Rows:" << d->rows() << "(" << d->mRowCount << ")";
    qDebug() << "Stretch:" << "h:" << (d->mStretch.testFlag(StretchHorizontal)) << " v:" << (d->mStretch.testFlag(StretchVertical));
    qDebug() << "Item:" << "h:" << itemHeight << " w:" << itemWidth;
#endif

    int remain_height = heightRemain;
    int remain_width = widthRemain;
    if (d->mDirection == LeftToRight)
    {
        int height = itemHeight + (0 < remain_height-- ? 1 : 0);
        for (QLayoutItem *item : qAsConst(d->mItems))
        {
            if (!item->widget() || item->widget()->isHidden())
                continue;
            int width = itemWidth + (0 < remain_width-- ? 1 : 0);

            if (x + width > maxX)
            {
                x = geometry.left();
                y += height + sp;

                height = itemHeight + (0 < remain_height-- ? 1 : 0);
                remain_width = widthRemain;
            }

            const int left = visual_h_reversed ? geometry.left() + geometry.right() - x - width + 1 : x;
            d->setItemGeometry(item, QRect(left, y, width, height));
            x += width + sp;
        }
    }
    else
    {
        int width = itemWidth + (0 < remain_width-- ? 1 : 0);
        for (QLayoutItem *item : qAsConst(d->mItems))
        {
            if (!item->widget() || item->widget()->isHidden())
                continue;
            int height = itemHeight + (0 < remain_height-- ? 1 : 0);

            if (y + height > maxY)
            {
                y = geometry.top();
                x += width + sp;

                width = itemWidth + (0 < remain_width-- ? 1 : 0);
                remain_height = heightRemain;
            }
            const int left = visual_h_reversed ? geometry.left() + geometry.right() - x - width + 1 : x;
            d->setItemGeometry(item, QRect(left, y, width, height));
            y += height + sp;
        }
    }
    d->mAnimate = false;
}

/************************************************

 ************************************************/
QRect GridLayout::occupiedGeometry() const
{
    return d_func()->mOccupiedGeometry;
}
