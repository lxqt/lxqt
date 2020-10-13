/*
 * commandmodel.cpp
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

#include "commandmodel.h"
#include "command.h"

extern QList<Command *> commands;

CommandModel::CommandModel(QObject * /*parent*/) {}
CommandModel::~CommandModel() {}

QModelIndex CommandModel::index(int row, int column,
                                const QModelIndex & /*parent*/) const
{
    if (row >= 0 and column >= 0 and row < commands.size() and column < 1)
    {
        Command *cmd = commands[row];
        return createIndex(row, column, cmd);
    }
    else
        return QModelIndex();
}
QModelIndex CommandModel::parent(const QModelIndex & /*child*/) const
{
    return QModelIndex();
}
int CommandModel::rowCount(const QModelIndex & /*parent*/) const
{
    return commands.size();
}
// int CommandModel::columnCount(const QModelIndex &parent) const{return 1;};
QVariant CommandModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        Command *cmd = static_cast<Command *>(index.internalPointer());
        return cmd->name;
    }
    if (role == Qt::DecorationRole)
    {
    }
    return QVariant();
}
void CommandModel::update() {} // TEMP
