/**
  * (c)LGPL2+
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
  * Copyright (C) 2007 John Tapsell <tapsell@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */
#include "qcategorizedsortfilterproxymodel.h"
#include "qcategorizedsortfilterproxymodel_p.h"

#include <climits>

#include <QItemSelection>
#include <QStringList>
#include <QSize>

//#include <kstringhandler.h>

int naturalCompare(const QString &_a, const QString &_b, Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive)
{
    // This method chops the input a and b into pieces of
    // digits and non-digits (a1.05 becomes a | 1 | . | 05)
    // and compares these pieces of a and b to each other
    // (first with first, second with second, ...).
    //
    // This is based on the natural sort order code code by Martin Pool
    // http://sourcefrog.net/projects/natsort/
    // Martin Pool agreed to license this under LGPL or GPL.

    // FIXME: Using toLower() to implement case insensitive comparison is
    // sub-optimal, but is needed because we compare strings with
    // localeAwareCompare(), which does not know about case sensitivity.
    // A task has been filled for this in Qt Task Tracker with ID 205990.
    // http://trolltech.com/developer/task-tracker/index_html?method=entry&id=205990
    QString a;
    QString b;
    if (caseSensitivity == Qt::CaseSensitive) {
        a = _a;
        b = _b;
    } else {
        a = _a.toLower();
        b = _b.toLower();
    }

    const QChar* currA = a.unicode(); // iterator over a
    const QChar* currB = b.unicode(); // iterator over b

    if (currA == currB) {
        return 0;
    }

    while (!currA->isNull() && !currB->isNull()) {
        const QChar* begSeqA = currA; // beginning of a new character sequence of a
        const QChar* begSeqB = currB;
        if (currA->unicode() == QChar::ObjectReplacementCharacter) {
            return 1;
        }

        if (currB->unicode() == QChar::ObjectReplacementCharacter) {
            return -1;
        }

        if (currA->unicode() == QChar::ReplacementCharacter) {
            return 1;
        }

        if (currB->unicode() == QChar::ReplacementCharacter) {
            return -1;
        }

        // find sequence of characters ending at the first non-character
        while (!currA->isNull() && !currA->isDigit() && !currA->isPunct() && !currA->isSpace()) {
            ++currA;
        }

        while (!currB->isNull() && !currB->isDigit() && !currB->isPunct() && !currB->isSpace()) {
            ++currB;
        }

        // compare these sequences
        const QStringRef& subA(a.midRef(begSeqA - a.unicode(), currA - begSeqA));
        const QStringRef& subB(b.midRef(begSeqB - b.unicode(), currB - begSeqB));
        const int cmp = QStringRef::localeAwareCompare(subA, subB);
        if (cmp != 0) {
            return cmp < 0 ? -1 : +1;
        }

        if (currA->isNull() || currB->isNull()) {
            break;
        }

        // find sequence of characters ending at the first non-character
        while ((currA->isPunct() || currA->isSpace()) && (currB->isPunct() || currB->isSpace())) {
            if (*currA != *currB) {
                return (*currA < *currB) ? -1 : +1;
            }
            ++currA;
            ++currB;
            if (currA->isNull() || currB->isNull()) {
                break;
            }
        }

        // now some digits follow...
        if ((*currA == QLatin1Char('0')) || (*currB == QLatin1Char('0'))) {
            // one digit-sequence starts with 0 -> assume we are in a fraction part
            // do left aligned comparison (numbers are considered left aligned)
            while (1) {
                if (!currA->isDigit() && !currB->isDigit()) {
                    break;
                } else if (!currA->isDigit()) {
                    return +1;
                } else if (!currB->isDigit()) {
                    return -1;
                } else if (*currA < *currB) {
                    return -1;
                } else if (*currA > *currB) {
                    return + 1;
                }
                ++currA;
                ++currB;
            }
        } else {
            // No digit-sequence starts with 0 -> assume we are looking at some integer
            // do right aligned comparison.
            //
            // The longest run of digits wins. That aside, the greatest
            // value wins, but we can't know that it will until we've scanned
            // both numbers to know that they have the same magnitude.

            bool isFirstRun = true;
            int weight = 0;
            while (1) {
                if (!currA->isDigit() && !currB->isDigit()) {
                    if (weight != 0) {
                        return weight;
                    }
                    break;
                } else if (!currA->isDigit()) {
                    if (isFirstRun) {
                        return *currA < *currB ? -1 : +1;
                    } else {
                        return -1;
                    }
                } else if (!currB->isDigit()) {
                    if (isFirstRun) {
                        return *currA < *currB ? -1 : +1;
                    } else {
                        return +1;
                    }
                } else if ((*currA < *currB) && (weight == 0)) {
                    weight = -1;
                } else if ((*currA > *currB) && (weight == 0)) {
                    weight = + 1;
                }
                ++currA;
                ++currB;
                isFirstRun = false;
            }
        }
    }

    if (currA->isNull() && currB->isNull()) {
        return 0;
    }

    return currA->isNull() ? -1 : + 1;
}

QCategorizedSortFilterProxyModel::QCategorizedSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new Private())

{
}

QCategorizedSortFilterProxyModel::~QCategorizedSortFilterProxyModel()
{
    delete d;
}

void QCategorizedSortFilterProxyModel::sort(int column, Qt::SortOrder order)
{
    d->sortColumn = column;
    d->sortOrder = order;

    QSortFilterProxyModel::sort(column, order);
}

bool QCategorizedSortFilterProxyModel::isCategorizedModel() const
{
    return d->categorizedModel;
}

void QCategorizedSortFilterProxyModel::setCategorizedModel(bool categorizedModel)
{
    if (categorizedModel == d->categorizedModel)
    {
        return;
    }

    d->categorizedModel = categorizedModel;

    invalidate();
}

int QCategorizedSortFilterProxyModel::sortColumn() const
{
    return d->sortColumn;
}

Qt::SortOrder QCategorizedSortFilterProxyModel::sortOrder() const
{
    return d->sortOrder;
}

void QCategorizedSortFilterProxyModel::setSortCategoriesByNaturalComparison(bool sortCategoriesByNaturalComparison)
{
    if (sortCategoriesByNaturalComparison == d->sortCategoriesByNaturalComparison)
    {
        return;
    }

    d->sortCategoriesByNaturalComparison = sortCategoriesByNaturalComparison;

    invalidate();
}

bool QCategorizedSortFilterProxyModel::sortCategoriesByNaturalComparison() const
{
    return d->sortCategoriesByNaturalComparison;
}

bool QCategorizedSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (d->categorizedModel)
    {
        int compare = compareCategories(left, right);

        if (compare > 0) // left is greater than right
        {
            return false;
        }
        else if (compare < 0) // left is less than right
        {
            return true;
        }
    }

    return subSortLessThan(left, right);
}

bool QCategorizedSortFilterProxyModel::subSortLessThan(const QModelIndex &left, const QModelIndex &right) const
{
    return QSortFilterProxyModel::lessThan(left, right);
}

int QCategorizedSortFilterProxyModel::compareCategories(const QModelIndex &left, const QModelIndex &right) const
{
    QVariant l = (left.model() ? left.model()->data(left, CategorySortRole) : QVariant());
    QVariant r = (right.model() ? right.model()->data(right, CategorySortRole) : QVariant());

    Q_ASSERT(l.isValid());
    Q_ASSERT(r.isValid());
    Q_ASSERT(l.type() == r.type());

    if (l.type() == QVariant::String)
    {
        QString lstr = l.toString();
        QString rstr = r.toString();

        if (d->sortCategoriesByNaturalComparison)
        {
            return naturalCompare(lstr, rstr);
        }
        else
        {
            if (lstr < rstr)
            {
                return -1;
            }

            if (lstr > rstr)
            {
                return 1;
            }

            return 0;
        }
    }

    qlonglong lint = l.toLongLong();
    qlonglong rint = r.toLongLong();

    if (lint < rint)
    {
        return -1;
    }

    if (lint > rint)
    {
        return 1;
    }

    return 0;
}
