/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *   Dmitriy Zhukov <zjesclean@gmail.com>
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

#ifndef _KBDWATCHER_H_
#define _KBDWATCHER_H_

#include "kbdlayout.h"
#include "controls.h"
#include "kbdkeeper.h"

class KbdKeeper;

class KbdWatcher: public QObject
{
    Q_OBJECT
public:
    KbdWatcher();

    void setup();
    const KbdLayout & kbdLayout() const
    { return m_layout; }

    bool isLayoutEnabled() const
    { return m_layout.isEnabled(); }
public slots:
    void controlClicked(Controls cnt);
signals:
    void layoutChanged(const QString & sym, const QString & name, const QString & variant);
    void modifierStateChanged(Controls mod, bool active);

private:
    void createKeeper(KeeperType type);
private slots:
    void keeperChanged();

private:
    KbdLayout                 m_layout;
    QScopedPointer<KbdKeeper> m_keeper;
};

#endif
