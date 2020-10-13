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


#ifndef DESKTOPSWITCHBUTTON_H
#define DESKTOPSWITCHBUTTON_H

#include <QToolButton>
#include <QSet>

namespace GlobalKeyShortcut
{
class Action;
}

class DesktopSwitchButton : public QToolButton
{
    Q_OBJECT

public:
    enum LabelType { // Must match with combobox indexes
        LABEL_TYPE_NUMBER = 0,
        LABEL_TYPE_NAME = 1,
        LABEL_TYPE_NONE = 2
    };

    DesktopSwitchButton(QWidget * parent, int index, LabelType labelType, const QString &title=QString());
    void update(int index, LabelType labelType,  const QString &title);

    void setUrgencyHint(WId, bool);

private:

    // for urgency hint handling
    bool mUrgencyHint;
    QSet<WId> mUrgentWIds;
};

#endif
