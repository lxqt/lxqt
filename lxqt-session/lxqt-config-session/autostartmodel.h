/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright (C) 2011  Alec Moskvin <alecm@gmx.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef AUTOSTARTMODEL_H
#define AUTOSTARTMODEL_H

#include <QAbstractItemModel>
#include <XdgAutoStart>

#include "autostartitem.h"

class AutoStartItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum ActiveButton
    {
        AddButton = 1,
        EditButton = 2,
        DeleteButton = 4
    };
    Q_DECLARE_FLAGS(ActiveButtons, ActiveButton)

    explicit AutoStartItemModel(QObject* parent = nullptr);
    ~AutoStartItemModel() override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    bool removeRow(int row, const QModelIndex& parent = QModelIndex());
    bool setEntry(const QModelIndex& index, XdgDesktopFile entry, bool overwrite=false);
    ActiveButtons activeButtons(const QModelIndex& selection) const;
    XdgDesktopFile desktopFile(const QModelIndex& index) const;

public slots:
    bool writeChanges();

public:
    QMap<QString, AutostartItem> items();

private:
    QMap<QString, AutostartItem> mItemMap;
    QPersistentModelIndex mGlobalIndex;
    QPersistentModelIndex mLXQtIndex;
    QStringList mGlobalItems;
    QStringList mLXQtItems;

    static QString indexToName(const QModelIndex& index);
    static bool showOnlyInLXQt(const XdgDesktopFile& file);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AutoStartItemModel::ActiveButtons)

#endif // AUTOSTARTMODEL_H
