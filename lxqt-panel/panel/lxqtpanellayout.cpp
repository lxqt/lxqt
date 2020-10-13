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


#include "lxqtpanellayout.h"
#include <QSize>
#include <QWidget>
#include <QEvent>
#include <QCursor>
#include <QApplication>
#include <QDebug>
#include <QPoint>
#include <QMouseEvent>
#include <QtAlgorithms>
#include <QPoint>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include "plugin.h"
#include "lxqtpanellimits.h"
#include "ilxqtpanelplugin.h"
#include "lxqtpanel.h"
#include "pluginmoveprocessor.h"
#include <QToolButton>
#include <QStyle>

#define ANIMATION_DURATION 250

class ItemMoveAnimation : public QVariantAnimation
{
public:
    ItemMoveAnimation(QLayoutItem *item) :
            mItem(item)
    {
        setEasingCurve(QEasingCurve::OutBack);
        setDuration(ANIMATION_DURATION);
    }

    void updateCurrentValue(const QVariant &current)
    {
        mItem->setGeometry(current.toRect());
    }

private:
    QLayoutItem* mItem;

};


struct LayoutItemInfo
{
    LayoutItemInfo(QLayoutItem *layoutItem=0);
    QLayoutItem *item;
    QRect geometry;
    bool separate;
    bool expandable;
};


LayoutItemInfo::LayoutItemInfo(QLayoutItem *layoutItem):
    item(layoutItem),
    separate(false),
    expandable(false)
{
    if (!item)
        return;

    Plugin *p = qobject_cast<Plugin*>(item->widget());
    if (p)
    {
        separate = p->isSeparate();
        expandable = p->isExpandable();
        return;
    }
}



/************************************************
  This is logical plugins grid, it's same for
  horizontal and vertical panel. Plugins keeps as:

   <---LineCount-->
   + ---+----+----+
   | P1 | P2 | P3 |
   +----+----+----+
   | P4 | P5 |    |
   +----+----+----+
         ...
   +----+----+----+
   | PN |    |    |
   +----+----+----+
 ************************************************/
class LayoutItemGrid
{
public:
    explicit LayoutItemGrid();
    ~LayoutItemGrid();

    void addItem(QLayoutItem *item);
    int count() const { return mItems.count(); }
    QLayoutItem *itemAt(int index) const { return mItems[index]; }
    QLayoutItem *takeAt(int index);


    const LayoutItemInfo &itemInfo(int row, int col) const;
    LayoutItemInfo &itemInfo(int row, int col);

    void update();

    int lineSize() const { return mLineSize; }
    void setLineSize(int value);

    int colCount() const { return mColCount; }
    void setColCount(int value);

    int usedColCount() const { return mUsedColCount; }

    int rowCount() const { return mRowCount; }

    void invalidate() { mValid = false; }
    bool isValid() const { return mValid; }

    QSize sizeHint() const { return mSizeHint; }

    bool horiz() const { return mHoriz; }
    void setHoriz(bool value);

    void clear();
    void rebuild();

    bool isExpandable() const { return mExpandable; }
    int expandableSize() const { return mExpandableSize; }

    void moveItem(int from, int to);

private:
    QVector<LayoutItemInfo> mInfoItems;
    int mColCount;
    int mUsedColCount;
    int mRowCount;
    bool mValid;
    int mExpandableSize;
    int mLineSize;

    QSize mSizeHint;
    QSize mMinSize;
    bool mHoriz;

    int mNextRow;
    int mNextCol;
    bool mExpandable;
    QList<QLayoutItem*> mItems;

    void doAddToGrid(QLayoutItem *item);
};


/************************************************

 ************************************************/
LayoutItemGrid::LayoutItemGrid()
{
    mLineSize = 0;
    mHoriz = true;
    clear();
}

LayoutItemGrid::~LayoutItemGrid()
{
    qDeleteAll(mItems);
}


/************************************************

 ************************************************/
void LayoutItemGrid::clear()
{
    mRowCount = 0;
    mNextRow = 0;
    mNextCol = 0;
    mInfoItems.resize(0);
    mValid = false;
    mExpandable = false;
    mExpandableSize = 0;
    mUsedColCount = 0;
    mSizeHint = QSize(0,0);
    mMinSize = QSize(0,0);
}


/************************************************

 ************************************************/
void LayoutItemGrid::rebuild()
{
    clear();

    for(QLayoutItem *item : qAsConst(mItems))
    {
        doAddToGrid(item);
    }
}


/************************************************

 ************************************************/
void LayoutItemGrid::addItem(QLayoutItem *item)
{
    doAddToGrid(item);
    mItems.append(item);
}


/************************************************

 ************************************************/
void LayoutItemGrid::doAddToGrid(QLayoutItem *item)
{
    LayoutItemInfo info(item);

    if (info.separate && mNextCol > 0)
    {
        mNextCol = 0;
        mNextRow++;
    }

    int cnt = (mNextRow + 1 ) * mColCount;
    if (mInfoItems.count() <= cnt)
        mInfoItems.resize(cnt);

    int idx = mNextRow * mColCount + mNextCol;
    mInfoItems[idx] = info;
    mUsedColCount = qMax(mUsedColCount, mNextCol + 1);
    mExpandable = mExpandable || info.expandable;
    mRowCount = qMax(mRowCount, mNextRow+1);

    if (info.separate || mNextCol >= mColCount-1)
    {
        mNextRow++;
        mNextCol = 0;
    }
    else
    {
        mNextCol++;
    }

    invalidate();
}


/************************************************

 ************************************************/
QLayoutItem *LayoutItemGrid::takeAt(int index)
{
    QLayoutItem *item = mItems.takeAt(index);
    rebuild();
    return item;
}


/************************************************

 ************************************************/
void LayoutItemGrid::moveItem(int from, int to)
{
    mItems.move(from, to);
    rebuild();
}


/************************************************

 ************************************************/
const LayoutItemInfo &LayoutItemGrid::itemInfo(int row, int col) const
{
    return mInfoItems[row * mColCount + col];
}


/************************************************

 ************************************************/
LayoutItemInfo &LayoutItemGrid::itemInfo(int row, int col)
{
    return mInfoItems[row * mColCount + col];
}


/************************************************

 ************************************************/
void LayoutItemGrid::update()
{
    mExpandableSize = 0;
    mSizeHint = QSize(0,0);

    if (mHoriz)
    {
        mSizeHint.setHeight(mLineSize * mColCount);
        int x = 0;
        for (int r=0; r<mRowCount; ++r)
        {
            int y = 0;
            int rw = 0;
            for (int c=0; c<mColCount; ++c)
            {
                LayoutItemInfo &info = itemInfo(r, c);
                if (!info.item)
                    continue;

                QSize sz = info.item->sizeHint();
                info.geometry = QRect(QPoint(x,y), sz);
                y += sz.height();
                rw = qMax(rw, sz.width());
            }
            x += rw;

            if (itemInfo(r, 0).expandable)
                mExpandableSize += rw;

            mSizeHint.setWidth(x);
            mSizeHint.rheight() = qMax(mSizeHint.rheight(), y);
        }
    }
    else
    {
        mSizeHint.setWidth(mLineSize * mColCount);
        int y = 0;
        for (int r=0; r<mRowCount; ++r)
        {
            int x = 0;
            int rh = 0;
            for (int c=0; c<mColCount; ++c)
            {
                LayoutItemInfo &info = itemInfo(r, c);
                if (!info.item)
                    continue;

                QSize sz = info.item->sizeHint();
                info.geometry = QRect(QPoint(x,y), sz);
                x += sz.width();
                rh = qMax(rh, sz.height());
            }
            y += rh;

            if (itemInfo(r, 0).expandable)
                mExpandableSize += rh;

            mSizeHint.setHeight(y);
            mSizeHint.rwidth() = qMax(mSizeHint.rwidth(), x);
        }
    }

    mValid = true;
}


/************************************************

 ************************************************/
void LayoutItemGrid::setLineSize(int value)
{
    mLineSize = qMax(1, value);
    invalidate();
}


/************************************************

 ************************************************/
void LayoutItemGrid::setColCount(int value)
{
    mColCount = qMax(1, value);
    rebuild();
}

/************************************************

 ************************************************/
void LayoutItemGrid::setHoriz(bool value)
{
    mHoriz = value;
    invalidate();
}



/************************************************

 ************************************************/
LXQtPanelLayout::LXQtPanelLayout(QWidget *parent) :
    QLayout(parent),
    mLeftGrid(new LayoutItemGrid()),
    mRightGrid(new LayoutItemGrid()),
    mPosition(ILXQtPanel::PositionBottom),
    mAnimate(false)
{
    setContentsMargins(0, 0, 0, 0);
}


/************************************************

 ************************************************/
LXQtPanelLayout::~LXQtPanelLayout()
{
    delete mLeftGrid;
    delete mRightGrid;
}


/************************************************

 ************************************************/
void LXQtPanelLayout::addItem(QLayoutItem *item)
{
    LayoutItemGrid *grid = mRightGrid;

    Plugin *p = qobject_cast<Plugin*>(item->widget());
    if (p && p->alignment() == Plugin::AlignLeft)
        grid = mLeftGrid;

    grid->addItem(item);
}


/************************************************

 ************************************************/
void LXQtPanelLayout::globalIndexToLocal(int index, LayoutItemGrid **grid, int *gridIndex)
{
    if (index < mLeftGrid->count())
    {
        *grid = mLeftGrid;
        *gridIndex = index;
        return;
    }

    *grid = mRightGrid;
    *gridIndex = index - mLeftGrid->count();
}

/************************************************

 ************************************************/
void LXQtPanelLayout::globalIndexToLocal(int index, LayoutItemGrid **grid, int *gridIndex) const
{
    if (index < mLeftGrid->count())
    {
        *grid = mLeftGrid;
        *gridIndex = index;
        return;
    }

    *grid = mRightGrid;
    *gridIndex = index - mLeftGrid->count();
}


/************************************************

 ************************************************/
QLayoutItem *LXQtPanelLayout::itemAt(int index) const
{
    if (index < 0 || index >= count())
        return 0;

    LayoutItemGrid *grid=0;
    int idx=0;
    globalIndexToLocal(index, &grid, &idx);

    return grid->itemAt(idx);
}


/************************************************

 ************************************************/
QLayoutItem *LXQtPanelLayout::takeAt(int index)
{
    if (index < 0 || index >= count())
        return 0;

    LayoutItemGrid *grid=0;
    int idx=0;
    globalIndexToLocal(index, &grid, &idx);

    return grid->takeAt(idx);
}


/************************************************

 ************************************************/
int LXQtPanelLayout::count() const
{
    return mLeftGrid->count() + mRightGrid->count();
}


/************************************************

 ************************************************/
void LXQtPanelLayout::moveItem(int from, int to, bool withAnimation)
{
    if (from != to)
    {
        LayoutItemGrid *fromGrid=0;
        int fromIdx=0;
        globalIndexToLocal(from, &fromGrid, &fromIdx);

        LayoutItemGrid *toGrid=0;
        int toIdx=0;
        globalIndexToLocal(to, &toGrid, &toIdx);

        if (fromGrid == toGrid)
        {
            fromGrid->moveItem(fromIdx, toIdx);
        }
        else
        {
            QLayoutItem *item = fromGrid->takeAt(fromIdx);
            toGrid->addItem(item);
            //recalculate position because we removed from one and put to another grid
            LayoutItemGrid *toGridAux=0;
            globalIndexToLocal(to, &toGridAux, &toIdx);
            Q_ASSERT(toGrid == toGridAux); //grid must be the same (if not something is wrong with our logic)
            toGrid->moveItem(toGridAux->count()-1, toIdx);
        }
    }

    mAnimate = withAnimation;
    invalidate();
}


/************************************************

 ************************************************/
QSize LXQtPanelLayout::sizeHint() const
{
    if (!mLeftGrid->isValid())
        mLeftGrid->update();

    if (!mRightGrid->isValid())
        mRightGrid->update();

    QSize ls = mLeftGrid->sizeHint();
    QSize rs = mRightGrid->sizeHint();

    if (isHorizontal())
    {
        return QSize(ls.width() + rs.width(),
                     qMax(ls.height(), rs.height()));
    }
    else
    {
        return QSize(qMax(ls.width(), rs.width()),
                     ls.height() + rs.height());
    }
}


/************************************************

 ************************************************/
void LXQtPanelLayout::setGeometry(const QRect &geometry)
{
    if (!mLeftGrid->isValid())
        mLeftGrid->update();

    if (!mRightGrid->isValid())
        mRightGrid->update();

    QRect my_geometry{geometry};
    my_geometry -= contentsMargins();
    if (count())
    {
        if (isHorizontal())
            setGeometryHoriz(my_geometry);
        else
            setGeometryVert(my_geometry);
    }

    mAnimate = false;
    QLayout::setGeometry(my_geometry);
}


/************************************************

 ************************************************/
void LXQtPanelLayout::setItemGeometry(QLayoutItem *item, const QRect &geometry, bool withAnimation)
{
    Plugin *plugin = qobject_cast<Plugin*>(item->widget());
    if (withAnimation && plugin)
    {
        ItemMoveAnimation* animation = new ItemMoveAnimation(item);
        animation->setStartValue(item->geometry());
        animation->setEndValue(geometry);
        animation->start(animation->DeleteWhenStopped);
    }
    else
    {
        item->setGeometry(geometry);
    }
}


/************************************************

 ************************************************/
void LXQtPanelLayout::setGeometryHoriz(const QRect &geometry)
{
    const bool visual_h_reversed = parentWidget() && parentWidget()->isRightToLeft();
    // Calc expFactor for expandable plugins like TaskBar.
    double expFactor;
    {
        int expWidth = mLeftGrid->expandableSize() + mRightGrid->expandableSize();
        int nonExpWidth = mLeftGrid->sizeHint().width()  - mLeftGrid->expandableSize() +
                      mRightGrid->sizeHint().width() - mRightGrid->expandableSize();
        expFactor = expWidth ? ((1.0 * geometry.width() - nonExpWidth) / expWidth) : 1;
    }

    // Calc baselines for plugins like button.
    QVector<int> baseLines(qMax(mLeftGrid->colCount(), mRightGrid->colCount()));
    const int bh = geometry.height() / baseLines.count();
    const int base_center = bh >> 1;
    const int height_remain = 0 < bh ? geometry.height() % baseLines.size() : 0;
    {
        int base = geometry.top();
        for (auto i = baseLines.begin(), i_e = baseLines.end(); i_e != i; ++i, base += bh)
        {
            *i = base;
        }
    }

#if 0
    qDebug() << "** LXQtPanelLayout::setGeometryHoriz **************";
    qDebug() << "geometry: " << geometry;

    qDebug() << "Left grid";
    qDebug() << "  cols:" << mLeftGrid->colCount() << " rows:" << mLeftGrid->rowCount();
    qDebug() << "  usedCols" << mLeftGrid->usedColCount();

    qDebug() << "Right grid";
    qDebug() << "  cols:" << mRightGrid->colCount() << " rows:" << mRightGrid->rowCount();
    qDebug() << "  usedCols" << mRightGrid->usedColCount();
#endif


    // Left aligned plugins.
    int left=geometry.left();
    for (int r=0; r<mLeftGrid->rowCount(); ++r)
    {
        int rw = 0;
        int remain = height_remain;
        for (int c=0; c<mLeftGrid->usedColCount(); ++c)
        {
            const LayoutItemInfo &info = mLeftGrid->itemInfo(r, c);
            if (info.item)
            {
                QRect rect;
                if (info.separate)
                {
                    rect.setLeft(left);
                    rect.setTop(geometry.top());
                    rect.setHeight(geometry.height());

                    if (info.expandable)
                        rect.setWidth(info.geometry.width() * expFactor);
                    else
                        rect.setWidth(info.geometry.width());
                }
                else
                {
                    int height = bh + (0 < remain-- ? 1 : 0);
                    if (!info.item->expandingDirections().testFlag(Qt::Orientation::Vertical))
                        height = qMin(info.geometry.height(), height);
                    height = qMin(geometry.height(), height);
                    rect.setHeight(height);
                    rect.setWidth(qMin(info.geometry.width(), geometry.width()));
                    if (height < bh)
                        rect.moveCenter(QPoint(0, baseLines[c] + base_center));
                    else
                        rect.moveTop(baseLines[c]);
                    rect.moveLeft(left);
                }

                rw = qMax(rw, rect.width());
                if (visual_h_reversed)
                    rect.moveLeft(geometry.left() + geometry.right() - rect.x() - rect.width() + 1);
                setItemGeometry(info.item, rect, mAnimate);
            }
        }
        left += rw;
    }

    // Right aligned plugins.
    int right=geometry.right();
    for (int r=mRightGrid->rowCount()-1; r>=0; --r)
    {
        int rw = 0;
        int remain = height_remain;
        for (int c=0; c<mRightGrid->usedColCount(); ++c)
        {
            const LayoutItemInfo &info = mRightGrid->itemInfo(r, c);
            if (info.item)
            {
                QRect rect;
                if (info.separate)
                {
                    rect.setTop(geometry.top());
                    rect.setHeight(geometry.height());

                    if (info.expandable)
                        rect.setWidth(info.geometry.width() * expFactor);
                    else
                        rect.setWidth(info.geometry.width());

                    rect.moveRight(right);
                }
                else
                {
                    int height = bh + (0 < remain-- ? 1 : 0);
                    if (!info.item->expandingDirections().testFlag(Qt::Orientation::Vertical))
                        height = qMin(info.geometry.height(), height);
                    height = qMin(geometry.height(), height);
                    rect.setHeight(height);
                    rect.setWidth(qMin(info.geometry.width(), geometry.width()));
                    if (height < bh)
                        rect.moveCenter(QPoint(0, baseLines[c] + base_center));
                    else
                        rect.moveTop(baseLines[c]);
                    rect.moveRight(right);
                }

                rw = qMax(rw, rect.width());
                if (visual_h_reversed)
                    rect.moveLeft(geometry.left() + geometry.right() - rect.x() - rect.width() + 1);
                setItemGeometry(info.item, rect, mAnimate);
            }
        }
        right -= rw;
    }
}


/************************************************

 ************************************************/
void LXQtPanelLayout::setGeometryVert(const QRect &geometry)
{
    const bool visual_h_reversed = parentWidget() && parentWidget()->isRightToLeft();
    // Calc expFactor for expandable plugins like TaskBar.
    double expFactor;
    {
        int expHeight = mLeftGrid->expandableSize() + mRightGrid->expandableSize();
        int nonExpHeight = mLeftGrid->sizeHint().height()  - mLeftGrid->expandableSize() +
                           mRightGrid->sizeHint().height() - mRightGrid->expandableSize();
        expFactor = expHeight ? ((1.0 * geometry.height() - nonExpHeight) / expHeight) : 1;
    }

    // Calc baselines for plugins like button.
    QVector<int> baseLines(qMax(mLeftGrid->colCount(), mRightGrid->colCount()));
    const int bw = geometry.width() / baseLines.count();
    const int base_center = bw >> 1;
    const int width_remain = 0 < bw ? geometry.width() % baseLines.size() : 0;
    {
        int base = geometry.left();
        for (auto i = baseLines.begin(), i_e = baseLines.end(); i_e != i; ++i, base += bw)
        {
            *i = base;
        }
    }

#if 0
    qDebug() << "** LXQtPanelLayout::setGeometryVert **************";
    qDebug() << "geometry: " << geometry;

    qDebug() << "Left grid";
    qDebug() << "  cols:" << mLeftGrid->colCount() << " rows:" << mLeftGrid->rowCount();
    qDebug() << "  usedCols" << mLeftGrid->usedColCount();

    qDebug() << "Right grid";
    qDebug() << "  cols:" << mRightGrid->colCount() << " rows:" << mRightGrid->rowCount();
    qDebug() << "  usedCols" << mRightGrid->usedColCount();
#endif

    // Top aligned plugins.
    int top=geometry.top();
    for (int r=0; r<mLeftGrid->rowCount(); ++r)
    {
        int rh = 0;
        int remain = width_remain;
        for (int c=0; c<mLeftGrid->usedColCount(); ++c)
        {
            const LayoutItemInfo &info = mLeftGrid->itemInfo(r, c);
            if (info.item)
            {
                QRect rect;
                if (info.separate)
                {
                    rect.moveTop(top);
                    rect.setLeft(geometry.left());
                    rect.setWidth(geometry.width());

                    if (info.expandable)
                        rect.setHeight(info.geometry.height() * expFactor);
                    else
                        rect.setHeight(info.geometry.height());
                }
                else
                {
                    rect.setHeight(qMin(info.geometry.height(), geometry.height()));
                    int width = bw + (0 < remain-- ? 1 : 0);
                    if (!info.item->expandingDirections().testFlag(Qt::Orientation::Horizontal))
                        width = qMin(info.geometry.width(), width);
                    width = qMin(geometry.width(), width);
                    rect.setWidth(width);
                    if (width < bw)
                        rect.moveCenter(QPoint(baseLines[c] + base_center, 0));
                    else
                        rect.moveLeft(baseLines[c]);
                    rect.moveTop(top);
                }

                rh = qMax(rh, rect.height());
                if (visual_h_reversed)
                    rect.moveLeft(geometry.left() + geometry.right() - rect.x() - rect.width() + 1);
                setItemGeometry(info.item, rect, mAnimate);
            }
        }
        top += rh;
    }


    // Bottom aligned plugins.
    int bottom=geometry.bottom();
    for (int r=mRightGrid->rowCount()-1; r>=0; --r)
    {
        int rh = 0;
        int remain = width_remain;
        for (int c=0; c<mRightGrid->usedColCount(); ++c)
        {
            const LayoutItemInfo &info = mRightGrid->itemInfo(r, c);
            if (info.item)
            {
                QRect rect;
                if (info.separate)
                {
                    rect.setLeft(geometry.left());
                    rect.setWidth(geometry.width());

                    if (info.expandable)
                        rect.setHeight(info.geometry.height() * expFactor);
                    else
                        rect.setHeight(info.geometry.height());
                    rect.moveBottom(bottom);
                }
                else
                {
                    rect.setHeight(qMin(info.geometry.height(), geometry.height()));
                    int width = bw + (0 < remain-- ? 1 : 0);
                    if (!info.item->expandingDirections().testFlag(Qt::Orientation::Horizontal))
                        width = qMin(info.geometry.width(), width);
                    width = qMin(geometry.width(), width);
                    rect.setWidth(width);
                    if (width < bw)
                        rect.moveCenter(QPoint(baseLines[c] + base_center, 0));
                    else
                        rect.moveLeft(baseLines[c]);
                    rect.moveBottom(bottom);
                }

                rh = qMax(rh, rect.height());
                if (visual_h_reversed)
                    rect.moveLeft(geometry.left() + geometry.right() - rect.x() - rect.width() + 1);
                setItemGeometry(info.item, rect, mAnimate);
            }
        }
        bottom -= rh;
    }
}


/************************************************

 ************************************************/
void LXQtPanelLayout::invalidate()
{
    mLeftGrid->invalidate();
    mRightGrid->invalidate();
    mMinPluginSize = QSize();
    QLayout::invalidate();
}


/************************************************

 ************************************************/
int LXQtPanelLayout::lineCount() const
{
    return mLeftGrid->colCount();
}


/************************************************

 ************************************************/
void LXQtPanelLayout::setLineCount(int value)
{
    mLeftGrid->setColCount(value);
    mRightGrid->setColCount(value);
    invalidate();
}


/************************************************

 ************************************************/
void LXQtPanelLayout::rebuild()
{
    mLeftGrid->rebuild();
    mRightGrid->rebuild();
}


/************************************************

 ************************************************/
int LXQtPanelLayout::lineSize() const
{
    return mLeftGrid->lineSize();
}


/************************************************

 ************************************************/
void LXQtPanelLayout::setLineSize(int value)
{
    mLeftGrid->setLineSize(value);
    mRightGrid->setLineSize(value);
    invalidate();
}


/************************************************

 ************************************************/
void LXQtPanelLayout::setPosition(ILXQtPanel::Position value)
{
    mPosition = value;
    mLeftGrid->setHoriz(isHorizontal());
    mRightGrid->setHoriz(isHorizontal());
}


/************************************************

 ************************************************/
bool LXQtPanelLayout::isHorizontal() const
{
    return mPosition == ILXQtPanel::PositionTop ||
            mPosition == ILXQtPanel::PositionBottom;
}


/************************************************

 ************************************************/
bool LXQtPanelLayout::itemIsSeparate(QLayoutItem *item)
{
    if (!item)
        return true;

    Plugin *p = qobject_cast<Plugin*>(item->widget());
    if (!p)
        return true;

    return p->isSeparate();
}


/************************************************

 ************************************************/
void LXQtPanelLayout::startMovePlugin()
{
    Plugin *plugin = qobject_cast<Plugin*>(sender());
    if (plugin)
    {
        // We have not memoryleaks there.
        // The processor will be automatically deleted when stopped.
        PluginMoveProcessor *moveProcessor = new PluginMoveProcessor(this, plugin);
        moveProcessor->start();
        connect(moveProcessor, SIGNAL(finished()), this, SLOT(finishMovePlugin()));
    }
}


/************************************************

 ************************************************/
void LXQtPanelLayout::finishMovePlugin()
{
    PluginMoveProcessor *moveProcessor = qobject_cast<PluginMoveProcessor*>(sender());
    if (moveProcessor)
    {
        Plugin *plugin = moveProcessor->plugin();
        int n = indexOf(plugin);
        plugin->setAlignment(n<mLeftGrid->count() ? Plugin::AlignLeft : Plugin::AlignRight);
        emit pluginMoved(plugin);
    }
}

/************************************************

 ************************************************/
void LXQtPanelLayout::moveUpPlugin(Plugin * plugin)
{
    const int i = indexOf(plugin);
    if (0 < i)
        moveItem(i, i - 1, true);
}

/************************************************

 ************************************************/
void LXQtPanelLayout::addPlugin(Plugin * plugin)
{
    connect(plugin, &Plugin::startMove, this, &LXQtPanelLayout::startMovePlugin);

    const int prev_count = count();
    addWidget(plugin);

    //check actual position
    const int pos = indexOf(plugin);
    if (prev_count > pos)
        moveItem(pos, prev_count, false);
}
