/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - The Lightweight Desktop Environment
 * https://lxqt.org
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

#include <QFileInfo>
#include <XdgDirs>
#include <XdgIcon>

#include "autostartmodel.h"
#include "autostartutils.h"

#include <LXQt/Globals>

AutoStartItemModel::AutoStartItemModel(QObject* parent) :
    QAbstractItemModel(parent),
    mItemMap(AutostartItem::createItemMap()),
    mGlobalIndex(QAbstractItemModel::createIndex(0, 0)),
    mLXQtIndex(QAbstractItemModel::createIndex(1, 0))
{
    QMap<QString,AutostartItem>::iterator iter;
    for (iter = mItemMap.begin(); iter != mItemMap.end(); ++iter)
    {
        if (!AutostartUtils::isLXQtModule(iter.value().file()))
        {
            if (showOnlyInLXQt(iter.value().file()))
                mLXQtItems.append(iter.key());
            else
                mGlobalItems.append(iter.key());
        }
    }
}

AutoStartItemModel::~AutoStartItemModel()
{
}

/*
 * All actual file operations are perfomed here when the window is closed.
 */
bool AutoStartItemModel::writeChanges()
{
    for(AutostartItem& item : mItemMap)
        item.commit();
    return true;
}

QMap<QString, AutostartItem> AutoStartItemModel::items()
{
    QMap<QString, AutostartItem> allItems;

    for(const QString &s : qAsConst(mLXQtItems))
        allItems[s] = mItemMap.value(s);

    for(const QString &s : qAsConst(mGlobalItems))
        allItems[s] = mItemMap.value(s);

    return allItems;
}

/*
 * Creates or replaces an autostart entry
 */
bool AutoStartItemModel::setEntry(const QModelIndex& index, XdgDesktopFile entry, bool overwrite)
{
    QModelIndex parent;
    if (index.parent().isValid())
        parent = index.parent();
    else if (index.isValid())
        parent = index;
    else
        parent = mGlobalIndex;

    QString fileName = QFileInfo(entry.fileName()).fileName();
    bool replacing = mItemMap.contains(fileName);
    if (!overwrite && replacing)
        return false;

    if (parent == mLXQtIndex)
        entry.setValue(QL1S("OnlyShowIn"), QL1S("LXQt;"));

    mItemMap[fileName].setFile(entry);

    if (replacing)
    {
        emit dataChanged(index, index);
        return true;
    }

    beginInsertRows(parent, 0, 0);
    if (parent == mGlobalIndex)
        mGlobalItems.append(fileName);
    else
        mLXQtItems.append(fileName);
    endInsertRows();

    return true;
}

bool AutoStartItemModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::CheckStateRole)
    {
        QString name;
        if (index.parent() == mGlobalIndex)
            name = mGlobalItems.value(index.row());
        else
            name = mLXQtItems.value(index.row());

        mItemMap[name].setEnabled(value == Qt::Checked);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

/*
 * When user clicks "Delete", removes the local item. If there is a "system" item,
 * it replaces it.
 */
bool AutoStartItemModel::removeRow(int row, const QModelIndex& parent)
{
    if (!parent.isValid())
        return false;
    QString item;

    if (parent == mGlobalIndex)
        item = mGlobalItems[row];
    else
        item = mLXQtItems[row];

    if (!mItemMap[item].removeLocal())
    {
        QModelIndex index = parent.child(row, 0);
        emit dataChanged(index, index);
        return false;
    }

    beginRemoveRows(parent, row, row);
    if (parent == mGlobalIndex)
        mGlobalItems.removeAt(row);
    else
        mLXQtItems.removeAt(row);
    endRemoveRows();

    if (mItemMap.value(item).isEmpty())
        mItemMap.remove(item);
    return true;
}

QModelIndex AutoStartItemModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!parent.isValid())
    {
        if (row == 0)
            return mGlobalIndex;
        else if (row == 1)
            return mLXQtIndex;
    }
    else if (parent == mGlobalIndex && row < mGlobalItems.size())
        return QAbstractItemModel::createIndex(row, column, (void*)&mGlobalItems[row]);
    else if (parent == mLXQtIndex && row < mLXQtItems.size())
        return QAbstractItemModel::createIndex(row, column, (void*)&mLXQtItems[row]);
    return QModelIndex();
}

QVariant AutoStartItemModel::data(const QModelIndex& index, int role) const
{
    if (!index.parent().isValid())
    {
        if (role == Qt::DisplayRole)
        {
            if (index.row() == 0)
                return QString(tr("Global Autostart"));
            else if (index.row() == 1)
                return QString(tr("LXQt Autostart"));
        }
        return QVariant();
    }

    QString name = indexToName(index);
    const AutostartItem& item = mItemMap.value(name);
    if (role == Qt::DisplayRole)
    {
        QString title = item.file().name();
        if (title.isEmpty())
            return name;
        return title;
    }
    else if (role == Qt::ToolTipRole)
    {
        QStringList tooltip;
        if (!item.isTransient())
            tooltip << tr("Location: %1").arg(item.file().fileName());
        if (item.overrides())
            tooltip << tr("Overrides: %1").arg(item.systemfile().fileName());
        return tooltip.join(QL1C('\n'));
    }
    else if (role == Qt::DecorationRole)
    {
        if (item.overrides())
            return XdgIcon::fromTheme(QSL("dialog-warning"));
        else if (!item.isLocal())
            return XdgIcon::fromTheme(QSL("computer"));
    }
    else if (role == Qt::CheckStateRole)
    {
        return item.isEnabled() ? Qt::Checked : Qt::Unchecked;
    }
    return QVariant();
}

Qt::ItemFlags AutoStartItemModel::flags(const QModelIndex& index) const
{
    if (index.parent().isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    if (index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return Qt::ItemFlags();
}

/*
 * Controls which buttons are enabled/disabled based on the selecton.
 */
AutoStartItemModel::ActiveButtons AutoStartItemModel::activeButtons(const QModelIndex& selection) const
{
    if (!selection.isValid() || !selection.parent().isValid())
        return AddButton;
    else if (!mItemMap.value(indexToName(selection)).isLocal())
        return AddButton | EditButton;
    else
        return AddButton | EditButton | DeleteButton;
}

QModelIndex AutoStartItemModel::parent(const QModelIndex& child) const
{
    QString name = indexToName(child);
    if (!name.isEmpty())
    {
        if (showOnlyInLXQt(mItemMap.value(name).file()))
           return mLXQtIndex;
        return mGlobalIndex;
    }
    return QModelIndex();
}

int AutoStartItemModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

int AutoStartItemModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid())
        return 2;
    if (parent == mGlobalIndex)
        return mGlobalItems.size();
    if (parent == mLXQtIndex)
        return mLXQtItems.size();
    return 0;
}

bool AutoStartItemModel::showOnlyInLXQt(const XdgDesktopFile& file)
{
    return file.value(QL1S("OnlyShowIn")) == QL1S("LXQt;");
}

/*
 * Maps the index to file name
 */
QString AutoStartItemModel::indexToName(const QModelIndex& index)
{
    QString* data = static_cast<QString*>(index.internalPointer());
    if (data)
        return *data;
    return QString();
}

XdgDesktopFile AutoStartItemModel::desktopFile(const QModelIndex& index) const
{
    return mItemMap.value(indexToName(index)).file();
}
