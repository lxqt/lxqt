/*
 * pstable.cpp
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

// ** toooo complex,so need new Design ... should be more simple.. (by
// fasthyun@magicn.com)

#include "proc.h"
#include "pstable.h"
#include "misc.h"

///#include <limits.h>
#include <QApplication>
#include <QClipboard>
#include <QToolTip>

// When a subtree is folded away, selections inside it disappear to prevent
// unexpected behaviour
static void clear_subtree_selections(Procinfo *p)
{
    for (int i = 0; i < p->table_children.size(); i++)
    {
        Procinfo *c = p->table_children[i];
        c->selected = false;
        clear_subtree_selections(c);
    }
}

// SLOT :
// connect(this, SIGNAL(titleClicked(int)), SLOT(setSortColumn(int)));
void Pstable::setSortColumn(int col)
{
    procview->setSortColumn(col, false);
    setSortedCol(col); // void HeadedTable::setSortedCol(int col)
    refresh();         // rebuild table
}

// AWKWARD
// sync Procview and HeadedTable set sorted column of table to procview->sortcol
// called by 1. Pstable::moveCol(int col, int place)
void Pstable::set_sortcol()
{
    for (int i = 0; i < procview->cats.size(); i++)
    {
        if (procview->cats[i] == procview->sortcat)
        {
            HeadedTable::setSortedCol(i); // HeadedTable
            return;
        }
    }
    setSortedCol(-1);
}

// inner
QString Pstable::title(int col)
{
    if (col >= procview->cats.size())
    {
        qDebug("Qps Bug!: title() over col !! %d\n", col);
        return "";
    }
    return procview->cats[col]->name;
}

QString Pstable::dragTitle(int col)
{
    if (col >= procview->cats.size()
        || procview->cats[col]->index == F_CMDLINE
        || (procview->cats[col]->index == F_PID && procview->treeview))
    {
        return ""; // not movable
    }
    return procview->cats[col]->name;
}

// TESTING
void Pstable::overpaintCell(QPainter *p, int row, int col, int xpos)
{
    if (col != 0)
        return;
    if (procview->cats[col]->id != F_CMD)
        return;

    int w;

    Procinfo *pi = procview->linear_procs[row];

    int n = pi->nthreads;
    if (n == 1)
        return;
#ifdef LINUX
    if (pi->pid != pi->tgid)
        return; // LINUX

#endif
    w = p->fontMetrics().width(text(row, col));

    QFont font = p->font();
    int size = font.pointSize(); // point size

    if (size <= 0)
        return; // saver!

    int h = body->cellHeight();
    h = p->fontMetrics().height(); // return pixel

    int msize = h * 3.0 / 8.0;
    // printf("DEBUG: height=%d, msize=%d\n",h,msize);

    if (h <= 11)
        return; // saver!

    if (msize < 6)
        msize = 6;

    // font.setPointSize(msize); // not pixel!
    font.setPixelSize(msize); // not pixel!
    p->setFont(font);
    p->drawText(xpos + w + 2, msize + msize / 3, QString::number(n));
    font.setPointSize(size);
    p->setFont(font);
}
//
QString Pstable::text(int row, int col)
{
    if (col >= procview->cats.size())
    {
        qDebug("Qps Bug!: text() over col !! %d\n", col);
        return "";
    }
    return procview->cats[col]->string(procview->linear_procs[row]);
}

int Pstable::totalRow() //???
{
    return procview->linear_procs.size();
}

int Pstable::colWidth(int col)
{
    // this is -1 for variable width fields, htable keeps track of it
    if (col >= procview->cats.size())
    {
        qDebug("Qps Bug!: colWidth() over col !! %d\n", col);
        return -1;
    }
    return procview->cats[col]->width();
}

int Pstable::alignment(int col)
{
    // qDebug("debug:alignment()\n");
    Category *cat = procview->cats[col];
    return cat->alignment();
}

// virtual of HeadedTable
void Pstable::setSelected(int row, bool flag)
{
    ///	qDebug("debug:Pstable::setSelected()\n");
    Procinfo *pi = procview->linear_procs[row];
    if (pi->selected != flag)
    {
        pi->selected = flag;
        //	body->repaintRow(row);  // TEMP trick..
    }
}

// virtual
bool Pstable::isSelected(int row)
{
    Procinfo *pi = procview->linear_procs[row];
    return pi->selected; // VALGRIND
}

int Pstable::rowDepth(int row)
{
    Procinfo *pi = procview->linear_procs[row];
    if (pi)
        return pi->level; // VALGRIND segfault

    qDebug("Qps bug: over row  %d!!!\n", row);
    return 0;
}

bool Pstable::lastChild(int row)
{
    Procinfo *pi = procview->linear_procs[row];
    if (pi)
        return pi->lastchild;
    qDebug("Qps bug: over row  %d!!!\n", row);
    return false;
}

// Segfault !!!!
// return  not PPID ! just virtual parent in tree
int Pstable::parentRow(int row)
{
    return procview->linear_procs[row]->parent_row;
    /* int ppid= procview->linear_procs[row]->ppid;
    Procinfo *pi= procview->getProcinfoByPID(ppid);
    if(pi) return ppid;
    else return -1; */
}

// DRAFT tooltip
QString Pstable::tipText(int col)
{
    Category *cat = procview->cats[col];
    QString s(cat->help);
    // trick
    if (cat->index == F_STAT)
        s.append("\n(R =Running, S =Sleeping, T =sTopped, Z=Zombie)");
    if (cat->index == F_PLCY)
        s.append("\n(TS =Time Sharing)");
    // if(cat->index == F_RSS);
    // s.append("\nRSS = CODE + DATA + SHARE\n"
    //	   "RSS = TRS  + DRS  + SHARE\n");
    /// s.append("\n(RSS = TRS + DRS)");
    if (cat == procview->sortcat)
        s.append(procview->reversed ? "\n(sorted backwards)" : "\n(sorted)");

    //	printf("size=%d\n",s.capacity());
    return s;
}

void Pstable::showTip(QPoint p, int idx)
{
    QToolTip::showText(p, tipText(idx));
}

extern TFrame *infobox; // testing

// SLOT
void Pstable::mouseOutOfCell() { infobox->hide(); }

// TEST
void Pstable::mouseOnCell(int row, int col)
{
    // printf("cursor on (%d,%d)\n",row,col);

    Category *cat = procview->cats[col];
    QString s = "";

    return;

    // below TEST code!
    if (cat->index == F_USER)
    {
        Procinfo *pi = procview->linear_procs[row];
        s = pi->username;
    }
    else
    {
        infobox->hide();
        return;
    }

    if (infobox != nullptr)
    {
        // p.setY(p.y() + geometry().y());
        // p.setY(QCursor::pos().y() - geometry().y());
        // p.setY(QCursor::pos().y() - frameGeometry().y());
        // p.setX(QCursor::pos().x() - frameGeometry().x());
        // QPoint p=QCursor::pos();
        //	if(htable->numSelected()>=2)

        infobox->show();
        infobox->setPos();
        if (s > nullptr)
        {
            infobox->setText(s);
        }
        else
            infobox->hide();
    }

    // char *str=htable->total_selectedRow(col);
}

void Pstable::leaveEvent(QEvent *) {}

// DRAFT  who call?
// void TableBody::mouseMoveEvent(QMouseEvent *e)
char *Pstable::total_selectedRow(int col)
{
    static char buff[48];
    char mem_str[48];
    std::string name;
    int index;

    if (procview->cats.size() <= col or col < 0)
        return nullptr; // col == -1
    index = procview->cats[col]->index;

    switch (index)
    {
    case F_SIZE:
        name = "total SIZE: ";
        break;
    case F_RSS:
        name = "total RSS: ";
        break;
#ifdef LINUX
    case F_TRS:
        name = "total Text: ";
        break;
    case F_DRS:
        name = "total Data: ";
        break;
    case F_STACK:
        name = "total STACK: ";
        break;
#endif
    default:
        return nullptr;
    }

    int total = 0;
    int rows = procview->linear_procs.size();
    for (int i = 0; i < rows; i++)
        if (procview->linear_procs[i]->selected)
        {
            switch (index)
            {
            case F_SIZE:
                total += procview->linear_procs[i]->size;
                break;
            case F_RSS:
                total += procview->linear_procs[i]->resident;
                break;
#ifdef LINUX
            case F_TRS:
                total += procview->linear_procs[i]->trs;
                break;
            case F_DRS:
                total += procview->linear_procs[i]->drs;
                break;
            case F_STACK:
                total += procview->linear_procs[i]->stack;
                break;
#endif
            }
        }

    mem_string(total, mem_str); // misc.h
    strcpy(buff, name.c_str());
    strcat(buff, mem_str);
    // sqDebug(buff,"total:%s",total);
    return buff;
}

// 	col
//	colMoved(col, place);
Pstable::Pstable(QWidget *parent, Procview *pv) : HeadedTable(parent, 0)
{
    procview = pv;

    connect(this, SIGNAL(titleClicked(int)), SLOT(setSortColumn(int)));
    connect(this, SIGNAL(foldSubTree(int)), SLOT(subtree_folded(int)));
    connect(head, SIGNAL(toolTip(QPoint, int)), this,
            SLOT(showTip(QPoint, int)));
    connect(this, SIGNAL(flyOnCell(int, int)), SLOT(mouseOnCell(int, int)));
    connect(this, SIGNAL(outOfCell()), SLOT(mouseOutOfCell()));
}

void Pstable::setReveseSort(bool reverse)
{
    head->setReveseSort(reverse);
}

HeadedTable::NodeState Pstable::folded(int row)
{
    Procinfo *p = procview->linear_procs[row];
    return (p->table_children.size() > 0) ? (p->hidekids ? Closed : Open)
                                          : Leaf;
}

// slot: called when selection changes
//	called by
//		1.void HeadedTable::selectionNotify()
/*
void Pstable::selection_update(const Svec<int> *rows)
{
        //CLIPBOARD
        qDebug("debug:selection_update()\n");
        for(int i = 0; i < rows->size(); i++) {
                int row = (*rows)[i];
                procview->linear_procs[row]->selected = isSelected(row); //
WRONG !!!
        }

        qps->update_menu_selection_status();

        if(numSelected() > 0 && qps->pids_to_selection) {
                // set the X11 selection to "PID1 PID2 PID3 ..."
                QString s, num;
                int n = numRows();
                for(int i = 0; i < n; i++) {
                        if(isSelected(i)) {
                                num.setNum(procview->linear_procs[i]->pid);
                                s.append(num);
                                if(i < n - 1)
                                        s.append(" ");
                        }
                }

                // important: this mustn't be called non-interactively since Qt
uses
                // the selection time of the last mouse or keyboard event
////		QApplication::clipboard()->setText(s);
        }

} */

// call by
// 	1.void Qps::set_table_mode(bool treemode)
// 	slot: changes table mode
void Pstable::setTreeMode(bool treemode)
{
    HeadedTable::setTreeMode(treemode);
    procview->treeview = treemode;
    procview->fieldArrange();
    set_sortcol();
    refresh();
}

//
bool Pstable::columnMovable(int col)
{
    if (treemode)
    {
        if (col == 0)
            return false;
    }
    if (procview->cats[col]->index == F_CMDLINE)
        return false;

    return true;
}

// called by HeadedTable
// Description : FIELD movement by mouse drag to place From col
//	virtual HeadedTable::moveCol(col,place);
void Pstable::moveCol(int col, int place)
{
    procview->moveColumn(col, place);
    procview->fieldArrange();
    set_sortcol();
    refresh(); // width size changed ,...
    return;
}

// NEED Check !!
// Slot: called when a subtree is opened or closed
// row = row number of sheet
void Pstable::subtree_folded(int row)
{

    Procinfo *p = procview->linear_procs[row];
    p->hidekids = !p->hidekids;

    if (p->hidekids)
        clear_subtree_selections(p); // *** important

    refresh(); // tree rebuild only
    return;
    // ???
    Procinfo *nextp =
        (row < numRows() - 1) ? procview->linear_procs[row + 1] : 0;
    if (!p->hidekids)
    {
        // Show as much as possible of the opened subtree
        int r = row + 1;
        while (r < numRows() && procview->linear_procs[r] != nextp)
            r++;
        showRange(row, r - 1);
    }
    // This is a stopgap solution; it would be better to have htable
    // take care of the hiding of subtrees and repaint only the rows under
    // the line hidden
}

// DRAFT CODE:
// 1.procview->refresh: proc.refresh, rebuild
// 2.resetwidth
// 3.repaint

// called by
// 	1.void Qps::refresh()
void STATUSBAR_SETCOUNT(int n);

static unsigned int kgen = 123;
void Pstable::checkTableModel()
{
    //	int size=procview->linear_procs.size();
    //	if(size!=numRows())
    {
        //	printf("Warnning: this is the bug.. wrong! %d
        //%d\n",size,numRows());
    }
    if (procview->current_gen != kgen)
    {
        //		printf("Warnning: maybe this is the bug.. wrong!
        //%d
        //%d\n",kgen,procview->current_gen);
        //		HeadedTable::setTreeMode(procview->treeview);
        procview->rebuild();                       // for Table
        setNumRows(procview->linear_procs.size()); // 1.
        //	qDebug("Pstable setTreeMode =%d , procview.treeview
        //=%d\n",treemode,procview->treeview);
        setNumCols(procview->cats.size()); // 2. resetWidths()
        kgen = procview->current_gen;
    }
}

// paintEvent();

void Pstable::refresh()
{
    procview->rebuild();                       // for Table
    setNumRows(procview->linear_procs.size()); // 1.
    setNumCols(procview->cats.size());         // 2. resetWidths()  UNINITIAL
    kgen = procview->current_gen;
    repaint_changed();

    STATUSBAR_SETCOUNT(procview->num_process);
}
