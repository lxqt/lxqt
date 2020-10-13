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

#include <QDebug>
#include "kbdwatcher.h"

KbdWatcher::KbdWatcher()
{
    connect(&m_layout, SIGNAL(modifierChanged(Controls,bool)), SIGNAL(modifierStateChanged(Controls,bool)));
    m_layout.init();
}

void KbdWatcher::setup()
{
    emit modifierStateChanged(Controls::Caps,   m_layout.isModifierLocked(Controls::Caps));
    emit modifierStateChanged(Controls::Num,    m_layout.isModifierLocked(Controls::Num));
    emit modifierStateChanged(Controls::Scroll, m_layout.isModifierLocked(Controls::Scroll));

    if (!m_keeper || m_keeper->type() != Settings::instance().keeperType()){
        createKeeper(Settings::instance().keeperType());
    } else {
        keeperChanged();
    }
}

void KbdWatcher::createKeeper(KeeperType type)
{
    switch(type)
    {
    case KeeperType::Global:
        m_keeper.reset(new KbdKeeper(m_layout));
        break;
    case KeeperType::Window:
        m_keeper.reset(new WinKbdKeeper(m_layout));
        break;
    case KeeperType::Application:
        m_keeper.reset(new AppKbdKeeper(m_layout));
        break;
    }

    connect(m_keeper.data(), SIGNAL(changed()), this, SLOT(keeperChanged()));

    m_keeper->setup();
    keeperChanged();
}

void KbdWatcher::keeperChanged()
{
    emit layoutChanged(m_keeper->sym(), m_keeper->name(), m_keeper->variant());
}

void KbdWatcher::controlClicked(Controls cnt)
{
    switch(cnt){
    case Controls::Layout:
        m_keeper->switchToNext();
        break;
    default:
        m_layout.lockModifier(cnt, !m_layout.isModifierLocked(cnt));
        break;
    }

}
