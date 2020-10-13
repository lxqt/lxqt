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


#include "menustyle.h"
#include <QDebug>


/************************************************

 ************************************************/
MenuStyle::MenuStyle():
    QProxyStyle()
{
    mIconSize = DEFAULT_ICON_SIZE;
}


/************************************************

 ************************************************/
int MenuStyle::pixelMetric(PixelMetric metric, const QStyleOption * option, const QWidget * widget) const
{
    if (metric == QStyle::PM_SmallIconSize && mIconSize != DEFAULT_ICON_SIZE)
        return mIconSize;

    return QProxyStyle::pixelMetric(metric, option, widget);
}

/************************************************

 ************************************************/
int MenuStyle::styleHint(StyleHint hint, const QStyleOption * option, const QWidget* widget, QStyleHintReturn* returnData) const
{
    // By default, the popup menu will be closed when Alt key
    // is pressed. If SH_MenuBar_AltKeyNavigation style hint returns
    // false, this behavior can be supressed so let's do it.
    if(hint == QStyle::SH_MenuBar_AltKeyNavigation)
        return 0;
    return QProxyStyle::styleHint(hint, option, widget, returnData);
}

