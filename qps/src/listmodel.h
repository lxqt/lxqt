/*
 * listmodel.h
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

#ifndef LISTMODEL_H
#define LISTMODEL_H

#include <QStandardItemModel>
// class ListModel : public QAbstractItemModel
class ListModel : public QAbstractTableModel
// class ListModel : public QStandardItemModel
{
    Q_OBJECT
  public:
    ListModel(/*QObject *parent = 0*/){};
    ~ListModel() override{};
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override; // pure
    QModelIndex parent(const QModelIndex &child) const override; // pure virtual
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex & /*parent*/) const override { return 2; };
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation o, int role) const override;
    //	QMap<int, QVariant> itemData ( const QModelIndex & index ) const
    //;
    void update()
    { // reset();
    }
    void update(const QModelIndex &idx);
    void update(int row);
    //  Qt::ItemFlags flags(const QModelIndex &index) const;
    //	void update(); //TEMP
};

#endif // LISTMODEL_H
