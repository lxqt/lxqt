/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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

#include "button.h"
#include <XdgIcon>

Button::Button(QWidget * parent) :
    QToolButton(parent)
{
    //Note: don't use the QStringLiteral here as it is causing a SEGFAULT in static finalization time
    //(the string is released upon our *.so removal, but the reference is still in held in libqtxdg...)
    setIcon(XdgIcon::fromTheme(QLatin1String("drive-removable-media")));
    setToolTip(tr("Removable media/devices manager"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoRaise(true);
}

Button::~Button()
{
}
