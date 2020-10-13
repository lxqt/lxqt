/**********************************************************************
** $Id: qttableview.h,v 1.46 2011/08/27 00:13:41 fasthyun Exp $
**
** Definition of QtTableView class
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file contains a class moved out of the Qt GUI Toolkit API.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file COPYING included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QTTABLEVIEW_H
#define QTTABLEVIEW_H

#include <QScrollBar>
#include <QAbstractScrollArea>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>

class QScrollBar;
class QtTableView : public QAbstractScrollArea
{
    Q_OBJECT
  public:
    QWidget *view;
    int flag_view;
    int cellWidth() const;
    int cellHeight() const;
    void repaintRow(int row); // paintRow();
    void coverCornerSquare(bool);
    void clearCache() {}
    void repaintChanged();
    virtual bool isCellChanged(int r, int c) {
        Q_UNUSED(r);
        Q_UNUSED(c);
        return true;
    };
    virtual void eraseRight(QPainter *, QRect &r) {
        Q_UNUSED(r);
        return;
    }
    virtual void checkProfile(){};
    QSize tmp_size; // testing.
    bool test;
    QColor backColor;

  protected:
    QtTableView(QWidget *parent = nullptr, const char *name = nullptr);
    ~QtTableView() override;

    int numRows() const;
    int numCols() const;
    void setNumRows(int);
    void setNumCols(int);

    int topCell() const;
    int leftCell() const;
    void setTopCell(int row);
    void setLeftCell(int col);
    void setTopLeftCell(int row, int col);

    int xOffset() const;
    int yOffset() const;
    virtual void setXOffset(int);
    virtual void setYOffset(int);
    virtual void setOffset(int x, int y, bool updateScrBars = true, bool scroll = true);
    virtual void scrollTrigger(int x, int y) {
        Q_UNUSED(x);
        Q_UNUSED(y);
    }; // tmp

    virtual int cellWidth(int col) const;
    int cellHeight(int row) const;
    virtual void setCellWidth(int);
    virtual void setCellHeight(int);

    int totalWidth();
    int totalHeight();

    uint tableFlags() const;
    //    bool	testTableFlags( uint f ) const;
    virtual void setTableFlags(uint f);
    void clearTableFlags(uint f = ~0);

    void repaintCell(int row, int column, bool usecache = false);

    QRect cellUpdateRect() const;
    QRect viewRect() const;

    int lastRowVisible() const;
    int lastColVisible() const;

    bool rowIsVisible(int row) const;
    bool colIsVisible(int col) const;

  private slots:
    void horSbValue(int);
    void horSbSliding(int);
    void horSbSlidingDone();
    void verSbValue(int);
    void verSbSliding(int);
    void verSbSlidingDone();

  protected:
    virtual void paintCell(QPainter *, int row, int col) = 0;
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;

    int findRow(int yPos) const;
    int findCol(int xPos) const;
    int findColNoMinus(int xPos) const;

    bool rowYPos(int row, int *yPos) const;
    bool colXPos(int col, int *xPos) const;

    int maxXOffset();
    int maxYOffset();
    int maxColOffset();
    int maxRowOffset();

    int minViewX() const;
    int minViewY() const;
    int maxViewX() const;
    int maxViewY() const;
    int viewWidth() const;
    int viewHeight() const;

    void updateScrollBars();
    void updateOffsets();

    QRect cellUpdateR;

  private:
    int findRawRow(int yPos, int *cellMaxY, int *cellMinY = nullptr,
                   bool goOutsideView = false) const;
    int findRawCol(int xPos, int *cellMaxX, int *cellMinX = nullptr,
                   bool goOutsideView = false) const;
    int maxColsVisible() const;

    void updateScrollBars(uint);
    void showOrHideScrollBars();

    int nRows;
    int nCols;
    int xOffs, yOffs;
    int xCellOffs, yCellOffs;
    short xCellDelta, yCellDelta;
    short cellH, cellW; //

    uint eraseInPaint : 1;
    uint verSliding : 1;
    uint verSnappingOff : 1;
    uint horSliding : 1;
    uint horSnappingOff : 1;
    uint coveringCornerSquare : 1;
    uint sbDirty : 8;
    uint inSbUpdate : 1;

    uint tFlags;
};

const uint Tbl_vScrollBar = 0x00000001;
const uint Tbl_hScrollBar = 0x00000002;
const uint Tbl_autoVScrollBar = 0x00000004;
const uint Tbl_autoHScrollBar = 0x00000008;
const uint Tbl_autoScrollBars = 0x0000000C;

const uint Tbl_clipCellPainting = 0x00000100;
const uint Tbl_cutCellsV = 0x00000200;
const uint Tbl_cutCellsH = 0x00000400;
const uint Tbl_cutCells = 0x00000600;

const uint Tbl_scrollLastHCell = 0x00000800;
const uint Tbl_scrollLastVCell = 0x00001000;
const uint Tbl_scrollLastCell = 0x00001800;

const uint Tbl_smoothHScrolling = 0x00002000;
const uint Tbl_smoothVScrolling = 0x00004000;
const uint Tbl_smoothScrolling = 0x00006000;

const uint Tbl_snapToHGrid = 0x00008000;
const uint Tbl_snapToVGrid = 0x00010000;
const uint Tbl_snapToGrid = 0x00018000;

inline int QtTableView::numRows() const { return nRows; }

inline int QtTableView::numCols() const { return nCols; }

inline int QtTableView::topCell() const { return yCellOffs; }

inline int QtTableView::leftCell() const { return xCellOffs; }

inline int QtTableView::xOffset() const { return xOffs; }

inline int QtTableView::yOffset() const { return yOffs; }

inline int QtTableView::cellHeight() const { return cellH; }

inline int QtTableView::cellWidth() const { return cellW; }

inline uint QtTableView::tableFlags() const { return tFlags; }

#define testTableFlags(f) ((tFlags & f) != 0)
// inline bool QtTableView::testTableFlags( uint f ) const{ return (tFlags & f)
// != 0; }

inline QRect QtTableView::cellUpdateRect() const { return cellUpdateR; }

#endif // QTTABLEVIEW_H
