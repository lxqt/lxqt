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

#ifndef _KDBSTATE_H_
#define _KDBSTATE_H_

#include "../panel/ilxqtpanelplugin.h"
#include "settings.h"
#include "content.h"
#include "kbdwatcher.h"

class QLabel;

class KbdState : public QObject, public ILXQtPanelPlugin
{
    Q_OBJECT
public:
    KbdState(const ILXQtPanelPluginStartupInfo &startupInfo);
    virtual ~KbdState();

    virtual QString themeId() const
    { return QStringLiteral("KbIndicator"); }

    virtual ILXQtPanelPlugin::Flags flags() const
    { return PreferRightAlignment | HaveConfigDialog; }

    virtual bool isSeparate() const
    { return false; }

    virtual QWidget *widget()
    { return &m_content; }

    QDialog *configureDialog();
    virtual void realign();

    const Settings & prefs() const
    { return m_settings; }

    Settings & prefs()
    { return m_settings; }

protected slots:
    virtual void settingsChanged();

private:
    Settings    m_settings;
    KbdWatcher  m_watcher;
    Content     m_content;
};


#endif
