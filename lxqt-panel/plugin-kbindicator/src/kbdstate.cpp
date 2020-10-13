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

#include <QTimer>
#include <QEvent>

#include "kbdstate.h"
#include "kbdkeeper.h"
#include "kbdstateconfig.h"
#include <LXQt/lxqtsettings.h>

KbdState::KbdState(const ILXQtPanelPluginStartupInfo &startupInfo):
    QObject(),
    ILXQtPanelPlugin(startupInfo),
    m_content(m_watcher.isLayoutEnabled())
{
    Settings::instance().init(settings());

    connect(&m_content, &Content::controlClicked, &m_watcher, &KbdWatcher::controlClicked);
    connect(&m_watcher, &KbdWatcher::layoutChanged, &m_content, &Content::layoutChanged);
    connect(&m_watcher, &KbdWatcher::modifierStateChanged, &m_content, &Content::modifierStateChanged);

    settingsChanged();
}

KbdState::~KbdState()
{}

void KbdState::settingsChanged()
{
    m_content.setup();
    m_watcher.setup();
}

QDialog *KbdState::configureDialog()
{
    return new KbdStateConfig(&m_content);
}

void KbdState::realign()
{
    if (panel()->isHorizontal()){
        m_content.setMinimumSize(0, panel()->iconSize());
        m_content.showHorizontal();
    } else {
        m_content.setMinimumSize(panel()->iconSize(), 0);
        m_content.showVertical();
    }
}
