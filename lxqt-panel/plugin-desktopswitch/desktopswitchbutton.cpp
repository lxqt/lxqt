/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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


#include <QToolButton>
#include <QStyle>
#include <QVariant>
#include <lxqt-globalkeys.h>

#include "desktopswitchbutton.h"

DesktopSwitchButton::DesktopSwitchButton(QWidget * parent, int index, LabelType labelType,  const QString &title)
    : QToolButton(parent),
    mUrgencyHint(false)
{
    update(index, labelType, title);

    setCheckable(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void DesktopSwitchButton::update(int index, LabelType labelType, const QString &title)
{
    switch (labelType)
    {
        case LABEL_TYPE_NAME:
            setText(title);
            break;

        // A blank space was used in NONE Label Type as it uses less space
        // for each desktop button at the panel
        case LABEL_TYPE_NONE:
            setText(QStringLiteral(" "));
            break;

        default: // LABEL_TYPE_NUMBER
            setText(QString::number(index + 1));
    }

    if (!title.isEmpty())
    {
        setToolTip(title);
    }
}

void DesktopSwitchButton::setUrgencyHint(WId id, bool urgent)
{
    if (urgent)
        mUrgentWIds.insert(id);
    else
        mUrgentWIds.remove(id);

    if (mUrgencyHint != !mUrgentWIds.empty())
    {
        mUrgencyHint = !mUrgentWIds.empty();
        setProperty("urgent", mUrgencyHint);
        style()->unpolish(this);
        style()->polish(this);
        QToolButton::update();
    }
}
