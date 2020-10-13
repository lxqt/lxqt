/*
 * listmodel.cpp
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

#include "listmodel.h"
#include "watchcond.h"

extern QList<watchCond *> watchlist;

QModelIndex ListModel::index(int row, int column,
                             const QModelIndex & /*parent*/) const
{
    // printf("index %d %d\n",row,column);
    if (row >= 0 and column >= 0 and row < watchlist.size())
    {
        if (column < 2)
        {
            watchCond *item = watchlist[row];
            return createIndex(row, column, item);
        }
    }
    return QModelIndex();
}

void ListModel::update(int row)
{
    emit dataChanged(index(row, 0), index(row, 1));
}

void ListModel::update(const QModelIndex &idx) { emit dataChanged(idx, idx); }

// Pure Virtual
QModelIndex ListModel::parent(const QModelIndex & /*child*/) const
{
    return QModelIndex(); // no parent!
}
int ListModel::rowCount(const QModelIndex & /*parent*/) const
{
    return watchlist.size();
}
QVariant ListModel::headerData(int section, Qt::Orientation /*orientation*/,
                               int role) const
{
    //	printf("headerData\n");
    if (role == Qt::DisplayRole)
    {
        if (section == 0)
            return QString(tr( "Event Category" ) );
        if (section == 1)
            return QString( tr( "Enable" ) );
    }
    /*
    if (role == Qt::FontRole)
    {
            QFont f=QFont();
            f.setBold(false);
            return f;
    } */
    if (role == Qt::SizeHintRole)
    {
        //	return QSize(18,18);
    }
    return QVariant();
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
    //	printf("data\n");
    watchCond *item = static_cast<watchCond *>(index.internalPointer());
    if (index.column() == 0)
    {
        if (role == Qt::DisplayRole)
        {
            return QString(item->getstring());
        }
        if (role == Qt::DecorationRole)
        {
        }
        if (role == Qt::EditRole)
        {
        }
    }

    if (index.column() == 1)
    {
        if (role == Qt::CheckStateRole)
        {
            if (item->enable)
                return Qt::Checked;
            else
                return Qt::Unchecked;
        }
        if (role == Qt::TextAlignmentRole)
            return Qt::AlignRight;
        if (role == Qt::EditRole)
        {
        }
    }
    if (role == Qt::SizeHintRole)
    {
        //	return QSize(18,18);
    }
    return QVariant();
}
