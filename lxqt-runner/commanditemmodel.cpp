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

#include "commanditemmodel.h"

#include <LXQt/Globals>
#include <LXQt/Settings>
#include <XdgIcon>
#include <QFileInfo>
#include <QProcess>
#include <QDebug>
#include <limits.h>


/************************************************

 ************************************************/
CommandItemModel::CommandItemModel(bool useHistory, QObject *parent) :
    QSortFilterProxyModel(parent),
    mSourceModel(new CommandSourceItemModel(useHistory, this)),
    mOnlyHistory(false),
    mShowHistoryFirst(true)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setSourceModel(mSourceModel);
    sort(0);
}


/************************************************

 ************************************************/
CommandItemModel::~CommandItemModel()
{
}


/************************************************

 ************************************************/
bool CommandItemModel::isOutDated() const
{
    return mSourceModel->isOutDated();
}


/************************************************

 ************************************************/
const CommandProviderItem *CommandItemModel::command(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    QModelIndex ind = mapToSource(index);

    return mSourceModel->command(ind);
}


/************************************************

 ************************************************/
void CommandItemModel::clearHistory()
{
    mSourceModel->clearHistory();
}


/************************************************

 ************************************************/
bool CommandItemModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QRegExp re(filterRegExp());

    if (re.isEmpty() && !mOnlyHistory)
        return false;

    const CommandProviderItem *item = mSourceModel->command(sourceRow);

    if (!item)
        return false;

    bool accept = item->compare(re);
    if (accept)
    {
        //check if CustomCommand can be filtered out (equivalent app link is shown)
        const CustomCommandItem * cust_i = qobject_cast<const CustomCommandItem *>(item);
        if (nullptr != cust_i)
        {
            for (int i = mSourceModel->rowCount(sourceParent); 0 <= i; --i)
            {
                const AppLinkItem * app_i = qobject_cast<const AppLinkItem *>(mSourceModel->command(i));
                if (nullptr != app_i && cust_i->exec() == app_i->exec() && app_i->compare(re))
                    return false;
            }
        }
    }

    return accept;
}


/************************************************

 ************************************************/
bool CommandItemModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (left == mSourceModel->customCommandIndex())
        return true;

    if (right == mSourceModel->customCommandIndex())
        return false;

    if (mOnlyHistory)
        return left.row() < right.row();

    HistoryItem const * i_left = dynamic_cast<HistoryItem const *>(mSourceModel->command(left));
    HistoryItem const * i_right = dynamic_cast<HistoryItem const *>(mSourceModel->command(right));
    if (nullptr != i_left && nullptr == i_right)
        return mShowHistoryFirst;
    if (nullptr == i_left && nullptr != i_right)
        return !mShowHistoryFirst;
    if (nullptr != i_left && nullptr != i_right)
    {
        QRegExp re(filterRegExp());
        //Note: -1 should not be returned if the item passed the filter previously
        const int pos_left = re.indexIn(i_left->command());
        const int pos_right = re.indexIn(i_right->command());
        Q_ASSERT(-1 != pos_left && -1 != pos_right);
        return pos_left < pos_right
            || (pos_left == pos_right && QSortFilterProxyModel::lessThan(left, right));
    }

    return QSortFilterProxyModel::lessThan(left, right);
}


/************************************************

 ************************************************/
int CommandItemModel::itemType(const QModelIndex &index) const
{
    if (index.row() == mSourceModel->customCommandIndex().row())
        return 1;
    else if (index.row() < mSourceModel->externalProviderStartIndex().row())
        return 2;
    else
        return 3;
}


/************************************************

 ************************************************/
QModelIndex CommandItemModel::appropriateItem(const QString &pattern) const
{
    QModelIndex res;
    unsigned int rank = 0;

    int cnt = rowCount();

    for (int i=0; i<cnt; ++i)
    {
        QModelIndex ind = index(i,0);
        QModelIndex srcIndex = mapToSource(ind);
        if (srcIndex == mSourceModel->customCommandIndex())
            return ind;

        const CommandProviderItem *item = mSourceModel->command(srcIndex);
        if (!item)
            continue;

        unsigned int r = item->rank(pattern);
        if (r > rank)
        {
            res = ind;
            rank = r;
        }

        if (rank >= MAX_RANK)
            break;
    }

    if (!res.isValid())
        res = index(0, 0);

    return res;
}


/************************************************

 ************************************************/
void CommandItemModel::showHistoryFirst(bool first/* = true*/)
{
    mShowHistoryFirst = first;
    invalidate();
}

/************************************************

 ************************************************/
void CommandItemModel::rebuild()
{
    mSourceModel->rebuild();
}


/************************************************

 ************************************************/
CommandSourceItemModel::CommandSourceItemModel(bool useHistory, QObject *parent) :
    QAbstractListModel(parent),
    mHistoryProvider(nullptr)
{
    mCustomCommandProvider = new CustomCommandProvider;
    mProviders.append(mCustomCommandProvider);
    rebuild();
    mCustomCommandIndex = index(0, 0);

    if (useHistory)
    {
        mHistoryProvider = new HistoryProvider();
        mProviders.append(mHistoryProvider);
        mCustomCommandProvider->setHistoryProvider(mHistoryProvider);
    }

    mProviders.append(new AppLinkProvider());
#ifdef MATH_ENABLED
    mProviders.append(new MathProvider());
#endif
#ifdef VBOX_ENABLED
    mProviders.append(new VirtualBoxProvider());
#endif

    rebuild();
    mExternalProviderStartIndex = index(rowCount(), 0);

    LXQt::Settings settings(QSL("lxqt-runner"));
    int numExternalProviders = settings.beginReadArray(QSL("external providers"));
    for (int i = 0; i < numExternalProviders; i++)
    {
        settings.setArrayIndex(i);
        qDebug() << "Adding external provider:" << settings.value(QL1S("name")) << settings.value(QL1S("executable"));
        ExternalProvider* externalProvider = new ExternalProvider(settings.value(QL1S("name")).toString(),
                                                                  settings.value(QL1S("executable")).toString());
        mProviders.append(externalProvider);
        mExternalProviders.append(externalProvider);
    }
    settings.endArray();

    for(const CommandProvider* provider : qAsConst(mProviders))
    {
        connect(provider, SIGNAL(changed()), this, SIGNAL(layoutChanged()));
        connect(provider, SIGNAL(aboutToBeChanged()), this, SIGNAL(layoutAboutToBeChanged()));
    }

    rebuild();
}


/************************************************

 ************************************************/
CommandSourceItemModel::~CommandSourceItemModel()
{
    qDeleteAll(mProviders);
    mHistoryProvider = 0;
}


/************************************************

 ************************************************/
int CommandSourceItemModel::rowCount(const QModelIndex& /*parent*/) const
{
    int ret=0;
    for(const CommandProvider* provider : qAsConst(mProviders))
        ret += provider->count();

    return ret;
}


/************************************************

 ************************************************/
QVariant CommandSourceItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= rowCount())
        return QVariant();

    const CommandProviderItem *item = command(index);
    if (!item)
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
        return QString::fromLatin1("<b>%1</b><br>\n%2\n").arg(item->title(), item->comment());

    case Qt::DecorationRole:
        return item->icon();

    case Qt::ToolTipRole:
        return item->toolTip();

    }

    return QVariant();
}


/************************************************

 ************************************************/
bool CommandSourceItemModel::isOutDated() const
{
    QListIterator<CommandProvider*> i(mProviders);
    while (i.hasNext())
    {
        if (i.next()->isOutDated())
            return true;
    }

    return false;
}


/************************************************

 ************************************************/
void CommandSourceItemModel::rebuild()
{
    emit layoutAboutToBeChanged();
    // FIXME: is this implementation correct?
    QListIterator<CommandProvider*> i(mProviders);
    while (i.hasNext())
    {
        CommandProvider *p = i.next();
        if (p->isOutDated())
            p->rebuild();
    }
    emit layoutChanged();
}


/************************************************

 ************************************************/
void CommandSourceItemModel::clearHistory()
{
    if (mHistoryProvider)
    {
        beginResetModel();
        mHistoryProvider->clearHistory();
        endResetModel();
    } else
    {
        QScopedPointer<HistoryProvider> history_p{new HistoryProvider};
        history_p->clearHistory();
    }
}


/************************************************

 ************************************************/
const CommandProviderItem *CommandSourceItemModel::command(int row) const
{
    int n = row;
    QListIterator<CommandProvider*> i(mProviders);
    while (i.hasNext())
    {
        CommandProvider *p = i.next();
        if (n < p->count())
            return p->at(n);
        else
            n -=p->count();
    }

    return 0;
}


/************************************************

 ************************************************/
const CommandProviderItem *CommandSourceItemModel::command(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return command(index.row());
}


/***********************************************

 ***********************************************/
void CommandSourceItemModel::setCommand(const QString& command)
{
    mCustomCommandProvider->setCommand(command);
    for (ExternalProvider* const externalProvider : qAsConst(mExternalProviders))
    {
        externalProvider->setSearchTerm(command);
    }
}


/***********************************************

 ***********************************************/
void CommandSourceItemModel::setUseHistory(bool useHistory)
{
    const bool now_using_history = mHistoryProvider != nullptr;
    if (now_using_history == useHistory)
        return;
    beginResetModel();
    if (now_using_history)
    {
        mProviders.removeAll(mHistoryProvider);
        mCustomCommandProvider->setHistoryProvider(nullptr);
        delete mHistoryProvider;
        mHistoryProvider = nullptr;
    } else
    {
        mHistoryProvider = new HistoryProvider;
        mProviders.append(mHistoryProvider);
        mCustomCommandProvider->setHistoryProvider(mHistoryProvider);
        connect(mHistoryProvider, SIGNAL(changed()), this, SIGNAL(layoutChanged()));
        connect(mHistoryProvider, SIGNAL(aboutToBeChanged()), this, SIGNAL(layoutAboutToBeChanged()));
    }
    endResetModel();
}
