/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
 * Authors:
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#ifndef GLOBAL_ACTION_CONFIG__DEFAULT_MODEL__INCLUDED
#define GLOBAL_ACTION_CONFIG__DEFAULT_MODEL__INCLUDED


#include <QAbstractTableModel>
#include <QMap>
#include <QColor>
#include <QFont>

#include "../daemon/meta_types.h"


class Actions;


template<class Key>
class QOrderedSet : public QMap<Key, Key>
{
public:
    typename QMap<Key, Key>::iterator insert(const Key &akey)
    {
        return QMap<Key, Key>::insert(akey, akey);
    }
};

class DefaultModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit DefaultModel(Actions *actions, const QColor &grayedOutColour, const QFont &highlightedFont, const QFont &italicFont, const QFont &highlightedItalicFont, QObject *parent = nullptr);
    ~DefaultModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    qulonglong id(const QModelIndex &index) const;

public slots:
    void daemonDisappeared();
    void daemonAppeared();

    void actionAdded(qulonglong id);
    void actionEnabled(qulonglong id, bool enabled);
    void actionModified(qulonglong id);
    void actionsSwapped(qulonglong id1, qulonglong id2);
    void actionRemoved(qulonglong id);

private:
    Actions *mActions;
    QMap<qulonglong, GeneralActionInfo> mContent;
    QMap<QString, QOrderedSet<qulonglong> > mShortcuts;

    QColor mGrayedOutColour;
    QFont mHighlightedFont;
    QFont mItalicFont;
    QFont mHighlightedItalicFont;

    QMap<QString, QString> mVerboseType;
};

#endif // GLOBAL_ACTION_CONFIG__DEFAULT_MODEL__INCLUDED
