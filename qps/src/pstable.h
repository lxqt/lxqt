/*
 * pstable.h
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

#ifndef PSTABLE_H
#define PSTABLE_H

#include "proc.h"
#include "htable.h"

class Pstable : public HeadedTable
{
    Q_OBJECT
  public:
    Pstable(QWidget *parent, Procview *pv);

    void set_sortcol();
    void moveCol(int col, int place) override;
    void refresh();

    // called by super
    bool hasSelection() { return false; };

    bool isSelected(int row) override;
    void setSelected(int row, bool sel) override;
    virtual int totalRow();
    void checkTableModel() override;
    void setReveseSort(bool reverse);

  public slots:
    // void selection_update(const Svec<int> *row);
    void setSortColumn(int col);
    void subtree_folded(int row);
    void showTip(QPoint p, int index);
    void setTreeMode(bool treemode);
    void mouseOnCell(int row, int col);
    void mouseOutOfCell();

  protected:
    // implementation of the interface to HeadedTable
    QString title(int col) override;
    QString dragTitle(int col) override;
    QString text(int row, int col) override;
    int colWidth(int col) override;
    int alignment(int col) override;
    QString tipText(int col) override;
    int rowDepth(int row) override;
    NodeState folded(int row) override;
    int parentRow(int row) override;
    bool lastChild(int row) override;
    char *total_selectedRow(int col) override;
    bool columnMovable(int col) override;

    void overpaintCell(QPainter *p, int row, int col, int xpos) override;
    //	virtual bool hasChildren(int row);

    void leaveEvent(QEvent *) override;

  private:
    Procview *procview;
};

#endif // PSTABLE_H
