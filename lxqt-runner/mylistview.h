/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2017 LXQt team
 * Authors:
 *   Palo Kisa <palo.kisa@gmail.com>
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

#if !defined(mylistview_h)
#define mylistview_h

#include <QListView>
#include <QDebug>

class MyListView : public QListView
{
public:
    using QListView::QListView;

    inline void setShownCount(int shownCount)
    {
        mShownCount = shownCount;
    }

protected:
    virtual QSize viewportSizeHint() const override
    {
        QAbstractItemModel * m = model();
        if (m == nullptr)
            return QSize{};

        QSize s{0, 0};
        for (int i = 0, i_e = qMin(mShownCount, model()->rowCount(QModelIndex{})); i != i_e; ++i)
        {
            const QSize s_i = sizeHintForIndex(m->index(i, 0, QModelIndex{}));
            s.rwidth() = qMax(s.width(), s_i.width());
            s.rheight() += s_i.height();
        }
        return s;
    }

private:
    int mShownCount = 4;
};

#endif // mylistview_h
