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

#ifndef COMMANDITEMMODEL_H
#define COMMANDITEMMODEL_H

#include <providers.h>
#include <QSortFilterProxyModel>
#include <QAbstractListModel>
#include <QVariant>


class CommandSourceItemModel: public QAbstractListModel
{
    Q_OBJECT
public:
    explicit CommandSourceItemModel(bool useHistory, QObject *parent = 0);
    virtual ~CommandSourceItemModel();

    int rowCount(const QModelIndex &parent=QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const;

    bool isOutDated() const;
    const CommandProviderItem *command(const QModelIndex &index) const;
    const CommandProviderItem *command(int row) const;

    QString command() const { return mCustomCommandProvider->command(); }
    void setCommand(const QString &command);

    QPersistentModelIndex customCommandIndex() const { return mCustomCommandIndex; }
    QPersistentModelIndex externalProviderStartIndex() const { return mExternalProviderStartIndex; }

    /*! Flag if the history should be shown/stored
     */
    void setUseHistory(bool useHistory);
public slots:
    void rebuild();
    void clearHistory();

private:
    QList<CommandProvider*> mProviders;
    HistoryProvider *mHistoryProvider;
    CustomCommandProvider *mCustomCommandProvider;
    QPersistentModelIndex mCustomCommandIndex;
    QList<ExternalProvider*> mExternalProviders;
    QPersistentModelIndex mExternalProviderStartIndex;
};


class CommandItemModel: public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit CommandItemModel(bool useHistory, QObject *parent = 0);
    virtual ~CommandItemModel();

    bool isOutDated() const;
    const CommandProviderItem *command(const QModelIndex &index) const;

    QModelIndex  appropriateItem(const QString &pattern) const;

    bool isShowOnlyHistory() const { return mOnlyHistory; }
    void showOnlyHistory(bool onlyHistory) { mOnlyHistory = onlyHistory; }

    void showHistoryFirst(bool first = true);

    /*! Flag if the history should be shown/stored
     */
    inline void setUseHistory(bool useHistory) { mSourceModel->setUseHistory(useHistory); }

    QString command() const { return mSourceModel->command(); }
    void setCommand(const QString &command) { mSourceModel->setCommand(command); }

public slots:
    void rebuild();
    void clearHistory();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private:
    int itemType(const QModelIndex &index) const;
    CommandSourceItemModel *mSourceModel;
    bool mOnlyHistory;
    bool mShowHistoryFirst; //!< flag for history items to be shown first/last
};

#endif // COMMANDITEMMODEL_H
