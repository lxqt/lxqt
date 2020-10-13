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
#include <KWindowSystem/KWindowSystem>
#include <KWindowSystem/KWindowInfo>
#include <KWindowSystem/netwm_def.h>
#include "kbdkeeper.h"

//--------------------------------------------------------------------------------------------------

KbdKeeper::KbdKeeper(const KbdLayout & layout, KeeperType type):
    m_layout(layout),
    m_type(type)
{
    m_layout.readKbdInfo(m_info);
}

KbdKeeper::~KbdKeeper()
{}

bool KbdKeeper::setup()
{
    connect(&m_layout, SIGNAL(keyboardChanged()), SLOT(keyboardChanged()));
    connect(&m_layout, SIGNAL(layoutChanged(uint)), SLOT(layoutChanged(uint)));
    connect(&m_layout, SIGNAL(checkState()), SLOT(checkState()));

    return true;
}

void KbdKeeper::keyboardChanged()
{
    m_layout.readKbdInfo(m_info);
    emit changed();
}

void KbdKeeper::layoutChanged(uint group)
{
    m_info.setCurrentGroup(group);
    emit changed();
}

void KbdKeeper::checkState()
{}

void KbdKeeper::switchToNext()
{
    uint index = m_info.currentGroup();
    if (index < m_info.size() - 1)
        ++index;
    else
        index = 0;

    switchToGroup(index);
}

void KbdKeeper::switchToGroup(uint group)
{
    m_layout.lockGroup(group);
    emit changed();
}

//--------------------------------------------------------------------------------------------------

WinKbdKeeper::WinKbdKeeper(const KbdLayout & layout):
    KbdKeeper(layout, KeeperType::Window)
{}

WinKbdKeeper::~WinKbdKeeper()
{}

void WinKbdKeeper::layoutChanged(uint group)
{
    WId win = KWindowSystem::activeWindow();

    if (m_active == win){
        m_mapping[win] = group;
        m_info.setCurrentGroup(group);
    } else {
        if (!m_mapping.contains(win))
            m_mapping.insert(win, 0);
        m_layout.lockGroup(m_mapping[win]);
        m_active = win;
        m_info.setCurrentGroup(m_mapping[win]);
    }
    emit changed();
}

void WinKbdKeeper::checkState()
{
    WId win = KWindowSystem::activeWindow();

    if (!m_mapping.contains(win))
        m_mapping.insert(win, 0);
    m_layout.lockGroup(m_mapping[win]);
    m_active = win;
    m_info.setCurrentGroup(m_mapping[win]);
    emit changed();
}

void WinKbdKeeper::switchToGroup(uint group)
{
    WId win = KWindowSystem::activeWindow();
    m_mapping[win] = group;
    m_layout.lockGroup(group);
    m_info.setCurrentGroup(group);
    emit changed();
}


//--------------------------------------------------------------------------------------------------

AppKbdKeeper::AppKbdKeeper(const KbdLayout & layout):
    KbdKeeper(layout, KeeperType::Window)
{}

AppKbdKeeper::~AppKbdKeeper()
{}

void AppKbdKeeper::layoutChanged(uint group)
{
    KWindowInfo info = KWindowInfo(KWindowSystem::activeWindow(), NET::Properties(), NET::WM2WindowClass);
    QString app = QString::fromUtf8(info.windowClassName());

    if (m_active == app){
        m_mapping[app] = group;
        m_info.setCurrentGroup(group);
    } else {
        if (!m_mapping.contains(app))
            m_mapping.insert(app, 0);

        m_layout.lockGroup(m_mapping[app]);
        m_active = app;
        m_info.setCurrentGroup(m_mapping[app]);
    }
    emit changed();
}

void AppKbdKeeper::checkState()
{
    KWindowInfo info = KWindowInfo(KWindowSystem::activeWindow(), NET::Properties(), NET::WM2WindowClass);
    QString app = QString::fromUtf8(info.windowClassName());

    if (!m_mapping.contains(app))
        m_mapping.insert(app, 0);

    m_layout.lockGroup(m_mapping[app]);
    m_active = app;
    m_info.setCurrentGroup(m_mapping[app]);
    emit changed();
}


void AppKbdKeeper::switchToGroup(uint group)
{
    KWindowInfo info = KWindowInfo(KWindowSystem::activeWindow(), NET::Properties(), NET::WM2WindowClass);
    QString app = QString::fromUtf8(info.windowClassName());

    m_mapping[app] = group;
    m_layout.lockGroup(group);
    m_info.setCurrentGroup(group);
    emit changed();
}
