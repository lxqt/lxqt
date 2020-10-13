/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
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

#include "windownotifier.h"
#include <QWidget>
#include <QEvent>

void WindowNotifier::observeWindow(QWidget * w)
{
    //installing the same filter object multiple times doesn't harm
    w->installEventFilter(this);
}


bool WindowNotifier::eventFilter(QObject * watched, QEvent * event)
{
    QWidget * widget = qobject_cast<QWidget *>(watched); //we're observing only QWidgetw
    auto it = std::lower_bound(mShownWindows.begin(), mShownWindows.end(), widget);
    switch (event->type())
    {
        case QEvent::Close:
            watched->removeEventFilter(this);
#if __cplusplus >= 201703L
            [[fallthrough]];
#endif
            // fall through
        case QEvent::Hide:
            if (mShownWindows.end() != it)
                mShownWindows.erase(it);
            if (mShownWindows.isEmpty())
                emit lastHidden();
            break;
        case QEvent::Show:
            {
                const bool first_shown = mShownWindows.isEmpty();
                mShownWindows.insert(it, widget); //we keep the mShownWindows sorted
                if (first_shown)
                    emit firstShown();
            }
        default:
            break;
    }
    return false;
}
