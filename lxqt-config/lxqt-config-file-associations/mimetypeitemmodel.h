/*
* Copyright (c) Christian Surlykke
*
* This file is part of the LXQt project. <https://lxqt.org>
* It is distributed under the LGPL 2.1 or later license.
* Please refer to the LICENSE file for a copy of the license, and
* the AUTHORS file for copyright and authorship information.
*/

#ifndef MIMETYPEITEMMODEL_H
#define	MIMETYPEITEMMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <XdgMime>

// Used for MimetypeItemModel::data to return a QVariant wrapping an XdgMimeInfo*
#define MimeInfoRole 32

Q_DECLARE_METATYPE(XdgMimeInfo*)

/*!
 *
 */
class MimetypeItemModel : public QAbstractItemModel
{
public:
    MimetypeItemModel(QObject *parent = 0);
    virtual ~MimetypeItemModel();

    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const {return 1;}
};

class MimetypeFilterItemModel : public QSortFilterProxyModel
{
public:
    MimetypeFilterItemModel(QObject *parent = 0);

    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

    bool filterHelper(QModelIndex& source_index) const;
};


#endif	/* MIMETYPEITEMMODEL_H */

