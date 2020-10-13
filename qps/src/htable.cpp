/*
 * htable.cpp
 * This file is part of qps -- Qt-based visual process status monitor
 *
 * Copyright 1997-1999 Mattias Engdegård
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */


//	very dirty and  complex code ( fasthyun@magicn.com)
//	Disaster !!

// TODO:
// * autoscroll speed proportional to distance from edge
// * interface to add/remove rows (for disclosure triangles)
// * interface to display pixmaps in cells (for disclosure triangles etc)
// * include sorting functionality here for more generality

#include "htable.h"
#include <QMouseEvent>
#include <QStyleOption>
#include <QPalette>
#include <QApplication>
#include <Qt>

FloatingHead::FloatingHead(QWidget *parent) : QWidget(parent) {}

void FloatingHead::setTitleAndSize(QString str, int w, int h)
{
    title = str;
    resize(w, h);
}
void FloatingHead::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    QStyleOptionHeader opt;
    opt.orientation = Qt::Horizontal;
    opt.textAlignment = Qt::AlignVCenter | Qt::AlignLeft;
    opt.fontMetrics = fontMetrics();
    p.setOpacity(0.7);
    opt.rect = rect();
    opt.text = title;
    opt.state = QStyle::State_Enabled | QStyle::State_Horizontal;
    opt.position = QStyleOptionHeader::OnlyOneSection;
    // CE_Header, CE_HeaderSection, CE_HeaderLabel
    style()->drawControl(QStyle::CE_Header, &opt, &p, this);
    return;
}

// TableHead: the horizontally scrollable table head
TableHead::TableHead(HeadedTable *parent)
    : QtTableView(parent)
    , htable(parent)
    , click_col(-1)
    , right_click_col(-1)
    , reversed_sort(false)
    , dragging(false)
    , drag_offset(0)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
    setTableFlags(Tbl_scrollLastHCell); // ?
    setNumRows(1);

    setMouseTracking(true); // for tooltip

    floatHead = new FloatingHead(parent); // tiny memory leak! don't care.
    floatHead->hide();
    setLayoutDirection(Qt::LeftToRight); // FIXME: A real fix for RTL is needed!
}

void TableHead::paintCell(QPainter *p, int /*row*/, int col)
{
    int w = htable->max_widths[col];

    QStyleOptionHeader opt;
    opt.orientation = Qt::Horizontal;
    opt.textAlignment = Qt::AlignVCenter | Qt::AlignLeft;
    opt.fontMetrics = fontMetrics();
    QRect rectR(0, 0, w, height());
    opt.rect = rectR;
    opt.text = htable->title(col);
    opt.state = QStyle::State_Enabled | QStyle::State_Horizontal;
    opt.section = col; // only KDE styles need this to draw separators
    opt.position = numCols() == 1 ? QStyleOptionHeader::OnlyOneSection
                                  : col == 0 ? QStyleOptionHeader::Beginning
                                             : col == numCols() - 1 ? QStyleOptionHeader::End
                                                                    : QStyleOptionHeader::Middle;
    if (col > -1 && htable->sortedCol() == col)
        opt.sortIndicator = reversed_sort ? QStyleOptionHeader::SortUp
                                          : QStyleOptionHeader::SortDown;
    else
        opt.sortIndicator = QStyleOptionHeader::None;
    style()->drawControl(QStyle::CE_Header, &opt, p, this);
}

void TableHead::eraseRight(QPainter* /*p*/, QRect& /*r*/)
{
    // Do nothing because this is an empty area
    // with the base background color.
}

// virtual !
int TableHead::cellWidth(int col) const { return htable->max_widths[col]; }

//
void TableHead::scrollSideways(int val) { setXOffset(val); }

void TableHead::mousePressEvent(QMouseEvent *e)
{
    int col = findCol(e->x());
    if (col == -1)
        return;

    if (e->button() == Qt::RightButton)
    {
        right_click_col = col;
        emit rightClicked(e->globalPos(), col);
    }
    else if (e->button() == Qt::LeftButton)
    {
        click_col = col;
        press = e->pos();
    }
}

void TableHead::mouseMoveEvent(QMouseEvent *e)
{
    int col = findCol(e->x());
    if (col < 0)
        return;

    if (e->buttons() == Qt::LeftButton
        && (htable->options & HTBL_REORDER_COLS))
    {
        if (!dragging && abs(press.x() - e->x()) > QApplication::startDragDistance())
        {
            QString title = htable->dragTitle(click_col);
            if (!title.isEmpty())
            {
                dragging = true;
                drag_offset = press.x() - htable->colOffset(click_col) + xOffset();
                floatHead->setTitleAndSize(title, cellWidth(click_col), height());
                floatHead->show();
            }
        }

        if (dragging)
        {
            floatHead->move(e->x() - drag_offset, 0);
            return;
        }
    }

    if (col != -1) // ToolTip
    {
        QString s = htable->tipText(col);

        if (!s.isEmpty())
            QToolTip::showText(e->globalPos(), s);
    }
}

void TableHead::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        int col = findCol(e->x());
        if (col < 0)
            col = (e->x() < 0) ? 0 : htable->ncols - 1; // good!

        if (!dragging) // just a click; no DND
        {
            if (col > -1 && col == click_col)
            {
                if (htable->sortedCol() == col) // sorting is reversed
                    reversed_sort = !reversed_sort;
                else // sorting will be done based on a new column
                    reversed_sort = false;
                emit htable->titleClicked(col);
            }
        }
        else if (click_col > -1) // dropping
        {
            int vcol = findColNoMinus(e->x());
            htable->moveCol(click_col, vcol); // -> Pstable::moveCol -> Procview::moveColumn
        }
        click_col = -1;
        dragging = false;
        if (floatHead->isVisible())
        {
            floatHead->hide();
            viewport()->update();
        }
    }
}

// TableBody: the table body, scrollable in all ways
TableBody::TableBody(HeadedTable *parent) : QtTableView(parent), htable(parent)
{

    // setTableFlags(Tbl_autoScrollBars | Tbl_smoothScrolling);
    setTableFlags(Tbl_snapToVGrid); // works
    setTableFlags(Tbl_hScrollBar | Tbl_vScrollBar);
    first_drag_row = prev_drag_row = -1;
    autoscrolling = false;
    gadget_click = false;
    setMouseTracking(true);
    setLayoutDirection(Qt::LeftToRight); // FIXME: A real fix for RTL is needed!
}

// virtual from QtTableView
void TableBody::scrollTrigger(int /*rx*/, int ry)
{
    if (ry == 0)
        return;
    int n = ry / cellHeight();
    /// printf("ry=%d n=%d\n",ry,n);
    if (ry > 0)
        tablecache.cutDown(n); // error?
    else
        tablecache.cutUp(-n);
    //	printf("ry=%d cellheight=%d n=%d\n",ry,cellHeight(),n);
}

// new, possible COMMON?
bool TableBody::isCellChanged(int row, int col)
{
    QString str;
    int xpos;
    int fold, dep;
    bool selected;
    bool sorted;
    int lc;
    int width;

    {
        str = htable->text(row, col);
        xpos = htable->colXPos(col);
        selected = htable->isSelected(row);
        sorted = (col > -1 && htable->sortedCol() == col);
        width = htable->max_widths[col];
    }

    // int left = leftCell(), top = topCell();
    CellAttribute *attr = tablecache.value(row - topCell(), col - leftCell());

    if (attr == nullptr) // never NULL
    {
        return true;
    }
    else
    {
        bool result = false;
        if (attr->text == str and attr->selected == selected and
            attr->sorted == sorted and attr->xpos == xpos and attr->w == width
            // and attr->size==tmp_size
            and
            (attr->size.height() >= tmp_size.height())
            //	and (attr->size.width()>=tmp_size.width())
            )
        {
            result = false;
        }
        else
        {
            //	attr->text=str;
            //	attr->selected=selected;
            //	attr->sorted=sorted;
            //	attr->xpos=xpos;
            //	attr->size=tmp_size;
            //	attr->w=width;

            //	if (row==29 and col==0)
            //	printf("(%d %d) xpos=%d %d sorted=%d %d sel=%d
            //%d str=%s
            //%s\n",attr->size.height(),tmp_size.height(),
            //			attr->xpos,xpos,attr->sorted,sorted,attr->selected,selected
            //			,qPrintable(attr->text),qPrintable(str));
            //	printf("true (%d
            //%d)\n",row-topCell(),col-leftCell());
            return true;
        }

        if (col == 0)
        {
            dep = htable->rowDepth(row); // folding_level
            fold = htable->folded(row);
            lc = htable->lastChild(row);

            if (attr->depth == dep /* Uninitial*/
                and attr->folded == fold and attr->lastchild == lc)
            {
                // false
            }
            else
            {
                //	attr->depth=dep;
                //	attr->folded=fold;
                result = true;
                //	printf("true (%d
                //%d)\n",row-topCell(),col-leftCell());
            }
        }
        return result;
    }
}

bool TableHead::isCellChanged(int row, int col)
{
    bool sorted = (col > -1 && htable->sortedCol() == col);

    if (!dragging && sorted && col == click_col)
        return true; // sorting is reversed

    bool result = false;

    QString str = htable->title(col);
    int xpos = htable->colXPos(col);
    CellAttribute *attr = tablecache.value(row - topCell(), col - leftCell());

    if (attr == nullptr) // Uninitialed?
    {
        result = true;
    }
    else if (attr->text != str || attr->sorted != sorted
             || attr->size != tmp_size || attr->xpos != xpos)
    {
        attr->text = str;
        attr->sorted = sorted;
        attr->xpos = xpos;
        attr->size = tmp_size;
        result = true;
    }

    return result;
}

// DRAFT CODE !!  BOTTLENECK !!!
// Description : draw a cell of table
// 		1. draw the background of a cell
// 		2. draw the text of a cell
//
// called by
// 		1.QtTableView::paintEvent()
void TableBody::paintCell(QPainter *p, int row, int col)
{
    CellAttribute *attr =
        tablecache.value(row - topCell(), col - leftCell()); // save cache

    bool sort;
    const int h = cellHeight();
    int w = htable->max_widths[col];
    attr->w = w;

    sort = (col > -1 && col == htable->sorted_col);
    attr->sorted = sort;


    // background
    if (htable->isSelected(row))
    {
        p->fillRect(0, 0, w, h, palette().brush(QPalette::Highlight));
        p->setPen(palette().color(QPalette::HighlightedText)); // text
        attr->selected = true;
    }
    else
    {
        attr->selected = false;
        if (sort)
        {
            p->fillRect(0, 0, w, h, palette().color(QPalette::AlternateBase));
        }
        // the background already has the base color, so there is no need to paint it here
        /*else
        {
            p->fillRect(0, 0, w, h, palette().color(QPalette::Base));
        }*/
        p->setPen(palette().color(QPalette::Text)); // text
    }

    // gridline (QPalette::Mid isn't good enough here)
    p->save();
    p->setOpacity(0.1);
    p->drawLine(0, h - 1, w -1, h - 1);
    p->restore();

    int align = Qt::AlignVCenter;
    int gap = h / 4;
    if (col == 0 && htable->treemode)
    {
        p->save();
        p->setOpacity(0.4); // only to draw branch lines

        QStyleOption opt;
        const int d = htable->rowDepth(row);
        attr->depth = d;
        int treestep = htable->treestep;

        for (int level = d, prow = row; level >= 0 && prow >= 0;
                level--, prow = htable->parentRow(prow))
        {
            if (level == d)
                continue;
            int x = gap + level * treestep;
            if (!htable->lastChild(prow))
            {
                p->drawLine(x + treestep / 2, 0, x + treestep / 2, h - 1); // vertical (first branch)
            }
        }

        const int x = gap + d * treestep + treestep / 2;
        p->drawLine(x + 1, h / 2, x + treestep - 1, h / 2); // horizontal

        HeadedTable::NodeState fs = htable->folded(row);
        attr->folded = fs;
        if (fs != HeadedTable::Leaf)
        {
            opt.state = opt.state | QStyle::State_Children;
            if (fs != HeadedTable::Closed)
            {
                opt.state = opt.state | QStyle::State_Open;
                p->drawLine(x + treestep, h / 2, x + treestep, h - 1); // vertical (after horizontal line)
            }
        }

        if (!htable->lastChild(row))
        {
            p->drawLine(x, 0, x, h - 1); // vertical (sub-branch)
        }
        else
        {
            p->drawLine(x, 0, x, h / 2); // vertical (end)
        }
        p->restore();

        attr->lastchild = htable->lastChild(row);

        QRect branchR(gap + d * treestep, 0, treestep, h);
        opt.rect = branchR;

        p->save(); // not really needed
        style()->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, p, this);
        p->restore();

        gap = x + treestep + 1;
        // FIXME: A real fix for RTL is needed!
        align |= (htable->layoutDirection() == Qt::RightToLeft ? Qt::AlignRight : Qt::AlignLeft);
    }
    else
    {
        // FIXME: A real fix for RTL is needed!
        if (htable->layoutDirection() == Qt::RightToLeft)
        { // reverse the alignment with RTL
            if (htable->alignment_col[col] == Qt::AlignRight)
              align |= Qt::AlignLeft;
            else
              align |= Qt::AlignRight;
        }
        else
            align |= htable->alignment_col[col];
        if (htable->alignment_col[col] == Qt::AlignRight) // using cache
        {
            w -= gap;
            gap = 0;
        }
    }

    p->drawText(gap, 0, w, h, align, htable->text(row, col));

    //htable->overpaintCell(p, row, col, gap); // senseless; Pstable::overpaintCell() should be removed

    // cache write!
    attr->text = htable->text(row, col);
    attr->xpos = htable->colXPos(col);
    attr->size = tmp_size;
}

// DEL , Home key
void TableBody::jumpTop() { setYOffset(0); }

// DEL, End Key
void TableBody::jumpBottom() { setYOffset(maxYOffset()); }

/// !!!!
void TableBody::centerVertically(int row)
{
    int topcell = row - viewHeight() / (cellHeight() * 2);
    setTopCell(qMax(topcell, 0));
    update();
}

//??
void TableBody::showRange(int from, int to)
{
    int h = viewHeight() / cellHeight();
    if (to >= topCell() + h)
        setTopCell(qMax(0, qMin(from, to - h + 1)));
}

// virtual
// called by
//	1.
int TableBody::cellWidth(int col) const { return htable->max_widths[col]; }

// **** fix !!
void TableBody::updateRow(int /*row*/)
{
    // ??
    // for(int col = 0; col < htable->ncols; col++)
    // updateCell(row, col,false);	updateCell(row, col);
}

void TableBody::mousePressEvent(QMouseEvent *e)
{
    if (numRows() == 0)
        return; // *** prevent out of range

    // printf("mousePressEvent() 1\n");
    static int last_row = -1;

    int row = findRow(e->y());
    if (row == -1)
    {
        //	printf("mousePressEvent\n");
        htable->clearAllSelections();
        if (e->y() >= 0)
            row = numRows(); // if SHIFT+click outside ~
        first_drag_row = prev_drag_row = row;
        return;
    }

    if (!(htable->options & HTBL_ROW_SELECTION))
        return;

    if (e->button() == Qt::LeftButton)
    {
        // folding
        if (htable->treemode && htable->folding &&
            e->x() < htable->gadget_space +
                         htable->treestep * htable->rowDepth(row) &&
            htable->folded(row) != HeadedTable::Leaf)
        {
            emit htable->foldSubTree(row);
            gadget_click = true;
            last_row = row;
            //	clearCache(); // dont use cache
            return;
        }

        if (e->modifiers() & Qt::ShiftModifier)
        {
            if (row < last_row)
            {
                for (int i = row; i < last_row; i++)
                    htable->setSelected(i, true); // virtual
            }
            else
            {
                int lastRow = qMax(last_row, 0);
                for (int i = lastRow; i <= row; i++)
                    htable->setSelected(i, true); // virtual
            }
        }
        else if (e->modifiers() & Qt::ControlModifier)
        {
            htable->setSelected(row, !htable->isSelected(row));
        }
        else
            htable->selectOnlyOne(row);

        first_drag_row = prev_drag_row = row;

        emit htable->selectionChanged();
    }

    else if (e->button() == Qt::RightButton)
    {
        if (!htable->isSelected(row))
            htable->selectOnlyOne(row);
        emit htable->selectionChanged();
        // better? 	emit htable->rightClicked(e->globalPos());
    }
    last_row = row;
    //	htable->repaint_changed();
    repaintChanged(); // repaint
                      // view->update(); // fast but use more CPU
}

void TableBody::mouseReleaseEvent(QMouseEvent *e)
{
    gadget_click = false;

    if (e->button() == Qt::LeftButton)
    {
        if (autoscrolling)
        {
            ////killTimers();		// no more autoscrolling
            first_drag_row = prev_drag_row = -1;
            autoscrolling = false;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        QPoint p(6, 3);
        p += e->globalPos();
        emit htable->rightClicked(p);
    }
}

void TableBody::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        int row = findRow(e->y());
        if (row != -1)
        {
            if (htable->options & HTBL_ROW_SELECTION &&
                !htable->isSelected(row))
                htable->selectOnlyOne(row);
            // htable->selectionNotify();
            emit htable->selectionChanged();
            //		htable->emit_double_click_signal(row);
            emit htable->doubleClicked(row);
        }
    }
}

// Bottle Neck ? no
void TableBody::leaveEvent(QEvent *) { emit htable->outOfCell(); }

void TableBody::mouseMoveEvent(QMouseEvent *e)
{
    if (numRows() == 0)
        return; // *** prevent out of range

    // DRAFT CODE !
    // Signal : find row, col(field name) emit flyOnEvent Signal
    int row = findRow(e->y());
    int col = findCol(e->x());

    if (row < 0 or col < 0)
    {
        emit htable->outOfCell(); // if...
        return;
    }

    emit htable->flyOnCell(row, col);

    if (e->buttons() == Qt::NoButton)
        return;
    if (e->buttons() == Qt::RightButton)
        return;

    if (e->buttons() & Qt::ControlModifier || gadget_click)
        return;

    if (row != prev_drag_row)
    {
        if (row == -1)
        {
            if (!autoscrolling)
            {
                // dragging outside table, cause scrolling
                scrolldir = (e->y() < 0) ? UP : DOWN;
                //	killTimers();
                //	startTimer(scroll_delay);
                autoscrolling = true;
            }
        }
        else
        {
            ////killTimers();
            autoscrolling = false;
            dragSelectTo(row);
        }
    }
    //	repaintRow(row);
    //	view->update();
    repaintChanged();
}

void TableBody::timerEvent(QTimerEvent *)
{
    // timer not proved !
    return;
    // printf("timer\n");
    if (!autoscrolling)
        return;
    ////killTimers();
    if (scrolldir == UP)
    {
        int top = topCell();
        setTopCell((top > 1) ? top - 1 : 0);
        dragSelectTo(topCell());
    }
    else
    {
        setTopCell(topCell() + 1);
        int bottom = lastRowVisible();
        dragSelectTo((bottom < numRows()) ? bottom : numRows() - 1);
    }
    startTimer(scroll_delay);
}

// change drag selection point from previous drag position to row
void TableBody::dragSelectTo(int row)
{
    int dir = (row > prev_drag_row) ? 1 : -1;
    if ((prev_drag_row - first_drag_row) * dir >= 0)
    {
        // moving away from start point
        for (int i = prev_drag_row + dir; i - dir != row; i += dir)
            htable->setSelected(i, true);
    }
    else
    {
        // moving towards start point
        for (int i = prev_drag_row; i != row; i += dir)
            htable->setSelected(i, false);
    }
    prev_drag_row = row;
    // htable->selectionNotify();
    emit htable->selectionChanged();
}

// HeadedTable: the actually useable class
HeadedTable::HeadedTable(QWidget *parent, int /*opts*/) : QWidget(parent)
{
    // meaningless?
    options = HTBL_ROW_CONTEXT_MENU | HTBL_ROW_SELECTION |
              HTBL_ROW_DOUBLE_CLICK | HTBL_HEADING_TOOLTIPS |
              HTBL_HEADING_CONTEXT_MENU | HTBL_HEADING_CLICK |
              HTBL_REORDER_COLS;

    head = new TableHead(this);
    body = new TableBody(this);
    head->setObjectName("head");
    body->setObjectName("body");

    sorted_col = -1;
    treemode = false;
    treestep = 0;
    lines = true;
    folding = true;
    gadget_space = 0;
    nrows = ncols = 0;

    head->setFrameShape(QFrame::NoFrame);
    body->setFrameShape(QFrame::NoFrame);

    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->setSpacing(0);

    vlayout->setContentsMargins(0, 0, 0, 0);

    vlayout->addWidget(head);
    vlayout->addWidget(body);
    setLayout(vlayout);

    // synchronize horizontal scrolling of head and body
    connect(body->horizontalScrollBar(), SIGNAL(valueChanged(int)), head,
            SLOT(scrollSideways(int)));

    // NOTE: It isn't enough to consider the font here because it may change at startup.
    // Instead, it will be taken into account when QEvent::FontChange() is sent.
    fontChange();
}

HeadedTable::~HeadedTable() {}

bool HeadedTable::event(QEvent *event)
{
    if (event->type() == QEvent::FontChange)
        fontChange();
    return QWidget::event(event);
}

// Find the correct heights of header and body cells with the current font
void HeadedTable::fontChange()
{
    // let the widget style calculate the heights

    QStyleOptionHeader opt;
    opt.textAlignment = Qt::AlignVCenter | Qt::AlignLeft;
    opt.orientation = Qt::Horizontal;
    opt.fontMetrics = fontMetrics();
    opt.text = "W"; // not really needed because all sane styles consider the font height
    opt.sortIndicator = QStyleOptionHeader::SortDown; // to have space for a sort indicator
    int H = style()->sizeFromContents(QStyle::CT_HeaderSection, &opt, QSize(), head).height();
    if (H % 2 != 0) H++;
    head->setCellHeight(H);
    head->setMaximumHeight(head->cellHeight());

    QStyleOptionViewItem ivOpt;
    ivOpt.fontMetrics = fontMetrics();
    int fontHeight = style()->sizeFromContents(QStyle::CT_ItemViewItem, &ivOpt, QSize(), this).height();
    if (fontHeight % 2 != 0) fontHeight++;

    body->setCellHeight(fontHeight);
    treestep = fontHeight;
    gadget_space = folding ? fontHeight + (fontHeight / 2) : 0;
}

// DRAFT  virtual !
void HeadedTable::moveCol(int /*col*/, int /*place*/)
{
    // go to void Procview::moveColumn(int col, int place)
}

// distance (in table coords) from left table edge of physical column
int HeadedTable::colOffset(int col)
{
    int x = 0;
    for (int c = 0; c < col; c++)
        x += max_widths[c];
    // x += body->cellWidth(c);
    return x;
}

// OPTIZ
inline int HeadedTable::colXPos(int col)
{
    int x = 0; // + coloffset
    for (int c = 0; c < col; c++)
        x += max_widths[c];
    return x;
}

// repaint columns from col0 to col1. If col1 is -1, repaint all
// the way to the right edge of the table.
// called by
//          1.void Qps::update_table(int col)
void HeadedTable::repaintColumns(int col0, int col1)
{
    QRect bvr = body->viewRect();
    QRect hvr = head->viewRect();
    int x0 = colOffset(col0) - body->xOffset();
    if (x0 > hvr.width())
        return;
    if (x0 < 0)
        x0 = 0;
    bvr.setLeft(x0);
    hvr.setLeft(x0);
    if (col1 >= 0)
    {
        int x1 = colOffset(col1) + max_widths[col1] - body->xOffset();
        if (x1 < hvr.width())
        {
            hvr.setRight(x1);
            bvr.setRight(x1);
        }
    }
    head->repaint(hvr);
    body->repaint(bvr);
}

// DEL -> virtual
// 	called by Pstable::setTreeMode(bool)
void HeadedTable::setTreeMode(bool tm)
{
    treemode = tm;
    head->clearCache();
    body->clearCache();
}

int HeadedTable::tableWidth() const
{
    QScrollBar *vs = body->verticalScrollBar();
    bool hidden = vs->minimum() == vs->maximum() // instead of !isVisible(), if update is paused
                  || vs->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, vs);
    return body->totalWidth() + (hidden ? 0 : vs->style()->pixelMetric(QStyle::PM_ScrollBarExtent));
}

// update table Head !!
void HeadedTable::setSortedCol(int col)
{
    sorted_col = col;
}

// should be virtual. why?
void HeadedTable::clearAllSelections()
{
    for (int row = 0; row < nrows; row++)
        setSelected(row, false);
    body->view->update();
}

void HeadedTable::selectOnlyOne(int row)
{
    for (int r = 0; r < nrows; r++)
        setSelected(r, r == row);
}

void HeadedTable::selectAll()
{
    for (int r = 0; r < nrows; r++)
        setSelected(r, true);
    emit selectionChanged();
}

// default implementation returns a null string (no tip d)
QString HeadedTable::tipText(int) { return ""; }
char *HeadedTable::total_selectedRow(int /*col*/) { return nullptr; }

// should be called before setNumCols()
void HeadedTable::setNumRows(int rows)
{
    nrows = rows;
    head->setNumRows(1);
    body->setNumRows(rows);

    // if update is paused or we don't want to wait for it;
    // to be used to spread the last column in updateColWidth()
    // (updateTable will be called in setNumCols)
    body->updateScrollBars();
}

void TableCache::setRow(int row)
{
    if (row < 0)
        return;

    int i, size = rows.size();
    if (row >= size)
    {
        //	rows.append(new TableRow);
    }

    if (row < nrow)
    {
        //		printf("row=%d,nrow=%d \n",row,nrow);
        if (size < nrow)
            nrow = size;
        for (i = row; i < nrow; i++)
        {
            for (int j = 0; j < 48; j++)
                rows[i]->cells[j].text = "";
        }
    }
    nrow = row;
}

void TableCache::setCol(int col)
{
    if (col < 0)
        return;
    if (col < ncol)
    {
        //	printf("clear cache\n");
        int i, size = rows.size();
        for (i = 0; i < size; i++)
            for (int j = col; j < ncol; j++)
                rows[i]->cells[j].text = "";
    }
    ncol = col;
}

// TEST
void TableHead::checkProfile()
{
    QRect viewR = viewRect();
    // if(viewR.y()!=0) printf(" qps: ooooohss....\n");
    int maxViewCol = findCol(viewR.width());
    if (maxViewCol < 0)
        maxViewCol = numCols();
    tablecache.setCol(maxViewCol);
    tablecache.setRow(1);
}

// TESt
void TableBody::checkProfile()
{
    htable->checkTableModel(); // important

    QRect viewR = viewRect();
    if (viewR.y() != 0)
        printf(" qps: ooooophss....\n");
    int maxViewRow = findRow(viewR.height());
    if (maxViewRow < 0)
        maxViewRow = numRows();
    int maxViewCol = findCol(viewR.width());
    if (maxViewCol < 0)
        maxViewCol = numCols();
    tablecache.setCol(maxViewCol);
    tablecache.setRow(maxViewRow);
}

// should be called after setNumRows()
void HeadedTable::setNumCols(int cols)
{
    ncols = cols;
    // resetWidths();
    for (int i = 0; i < cols; i++)
    {
        alignment_col[i] = alignment(i);
        updateColWidth(i);
    }
    head->setNumCols(ncols);
    body->setNumCols(ncols);

    // WARNING: The scrollbars, table viewport and offsets should be
    // updated here for the table and header widths to be correct.
    updateTable();
}

void HeadedTable::updateColWidth(int col)
{
    int w = 0;
    int sw = 0;
    int rows = numRows();
    bool treecol = (treemode && col == 0);

    for (int i = 0; i < rows; i++)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
        sw = fontMetrics().horizontalAdvance(text(i, col)) + 10;
#else
        sw = fontMetrics().width(text(i, col)) + 10;
#endif
        if (treecol)
            sw += treestep * rowDepth(i);
        if (sw > w)
            w = sw;
    }

    if (treecol)
    {
        w += gadget_space;
    }

    // get the header width by consulting the widget style
    QStyleOptionHeader opt;
    opt.orientation = Qt::Horizontal;
    opt.textAlignment = Qt::AlignVCenter | Qt::AlignLeft;
    opt.fontMetrics = fontMetrics();
    opt.text = title(col);
    opt.sortIndicator = QStyleOptionHeader::SortDown;
    int hw = style()->sizeFromContents(QStyle::CT_HeaderSection, &opt, QSize(), this).width();

    if (hw > w)
        w = hw;

    // to have an elegant table, spread the last column if its contents are left aligned
    if (col == ncols - 1 && alignment_col[col] == Qt::AlignLeft)
    {
        int prevWidths = 0;
        for (int i = 0 ; i < col; i++)
            prevWidths += max_widths[i];
        QScrollBar *vs = body->verticalScrollBar();
        bool hidden = vs->minimum() == vs->maximum() // instead of !isVisible(), if update is paused
                      || vs->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, vs);
        int scrollWidth = hidden ? 0 : vs->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
        w = qMax(w, width() - scrollWidth - prevWidths);
    }

    max_widths[col] = w;
}

void HeadedTable::updateTable()
{
    body->updateOffsets();
    head->updateOffsets();
    body->updateScrollBars();
    body->viewport()->update();
}

void HeadedTable::resetWidths()
{
    for (int i = 0; i < numCols() + 1; i++)
        max_widths[i] = 0;
}

// called by ?
void HeadedTable::repaintAll() // -> repaint()
{
    int count = 0, trow = 0;
    for (int row = 0; row < nrows; row++)
        if (isSelected(row))
        {
            count++;
            trow = row;
        }
    //	body->view->update();
    //	update(); // not work why?

    //	printf("Qps debug: repaintAll()\n");
    //	head->clearCache();
    //	body->clearCache();
    // update();
    body->view->update();
    head->view->update();
    if (count == 1)
        centerVertically(trow);
}

//  PROBLEM : update()  +  repaint() call = draw twice !!!
//	called by pstable::refresh()
//  DEL if (render_area smaller than full_arear ) then force draw
void HeadedTable::repaint_changed()
{
    //	printf("repaint_changed()\n");
    // body->setUpdatesEnabled(true);
    head->repaintChanged();
    body->repaintChanged();
}

void HeadedTable::hideEvent(QHideEvent * /*event*/) {}

void HeadedTable::paintEvent(QPaintEvent * /*e*/)
{
    // static int c = 0;
    //	printf("HeadedTable paintEvent %d\n",c++); //works ?
    //	QWidget::paintEvent(e);
    // body->clearCache();
    //	head->update();
}

void HeadedTable::showEvent(QShowEvent *)
{
    //	printf("HeadedTable showEvent\n"); // works
    //  before paintEvent call
    head->clearCache();
    body->clearCache();
}

void HeadedTable::resizeEvent(QResizeEvent *e)
{
    // DEBUG("resizeEvent() HeadedTable %d\n",head->cellHeight());
    ////	head->repaintRow(0);
    QWidget::resizeEvent(e);

    // update the width of the last column
    updateColWidth(numCols() - 1);
    updateTable();

    head->clearCache();
    body->clearCache();
}
