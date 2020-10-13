/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *  Balázs Béla <balazsbela[at]gmail.com>
 *  Paulo Lieuthier <paulolieuthier@gmail.com>
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
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef STATUSNOTIFIER_PLUGIN_H
#define STATUSNOTIFIER_PLUGIN_H

#include "../panel/ilxqtpanelplugin.h"
#include "statusnotifierwidget.h"
#include "statusnotifierconfiguration.h"

class StatusNotifier : public QObject, public ILXQtPanelPlugin
{
    Q_OBJECT
public:
    StatusNotifier(const ILXQtPanelPluginStartupInfo &startupInfo);

    bool isSeparate() const { return true; }
    void realign();
    QString themeId() const { return QStringLiteral("StatusNotifier"); }
    virtual Flags flags() const { return SingleInstance | HaveConfigDialog | NeedsHandle; }
    QWidget *widget() { return m_widget; }

    QDialog *configureDialog() override;

    void settingsChanged() { m_widget->settingsChanged(); }

private:
    StatusNotifierWidget *m_widget;
};

class StatusNotifierLibrary : public QObject, public ILXQtPanelPluginLibrary
{
    Q_OBJECT
//     Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const
    {
        return new StatusNotifier(startupInfo);
    }
};

#endif // STATUSNOTIFIER_PLUGIN_H
