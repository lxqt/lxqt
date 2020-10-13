/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
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


#ifndef LXQTSENSORSPLUGIN_H
#define LXQTSENSORSPLUGIN_H

#include "../panel/ilxqtpanelplugin.h"
#include <QObject>


class LXQtSensors;

class LXQtSensorsPlugin: public QObject, public ILXQtPanelPlugin
{
    Q_OBJECT
public:
    explicit LXQtSensorsPlugin(const ILXQtPanelPluginStartupInfo &startupInfo);
    ~LXQtSensorsPlugin();

    virtual ILXQtPanelPlugin::Flags flags() const { return PreferRightAlignment | HaveConfigDialog; }
    virtual QWidget *widget();
    virtual QString themeId() const { return QStringLiteral("Sensors"); }

    bool isSeparate() const { return true; }
    QDialog *configureDialog();

    void realign();

protected:
    virtual void settingsChanged();

private:
    LXQtSensors *mWidget;
};


class LXQtSensorsPluginLibrary: public QObject, public ILXQtPanelPluginLibrary
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const
    {
        return new LXQtSensorsPlugin(startupInfo);
    }
};

#endif // LXQTSENSORSPLUGIN_H
